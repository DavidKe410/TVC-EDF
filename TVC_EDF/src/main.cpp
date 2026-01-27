#include <Arduino.h>
#include <Adafruit_BNO08x.h>
#include "RingBuf.h"
#include "SdFat.h"

//============================================ Configuration ============================================

// ======== General ========
const short CYCLE_TIME_MS = 10; // 100 Hz
short loopCount = 0;
// ====== End General ======

//====== BNO085 ======
#define BNO08X_RESET -1
Adafruit_BNO08x  bno08x(BNO08X_RESET);
sh2_SensorValue_t sensorValue;
long reportIntervalUs = 2000;
//====== End BNO085 ======

//======= Data Logger ========
#define SD_CONFIG SdioConfig(FIFO_SDIO)

// Size to log 10 byte lines at 25 kHz for more than ten minutes.
const size_t LOG_FILE_SIZE = 10 * 25000 * 600;  // 150,000,000 bytes.

// Space to hold around 2s of data for 80 byte lines at 100 sps.
const size_t RING_BUF_CAPACITY  = 8000 * 2;

// Max RingBuf used bytes. Useful to understand RingBuf overrun.
size_t maxUsed = 0;

SdFs sd;
FsFile file;

// RingBuf for File type FsFile.
RingBuf<FsFile, RING_BUF_CAPACITY> rb;
//===== End Data Logger ========

//========================================== End Configuration ==========================================

//============================================ Data Structs ============================================

// Struct to store BNO085 + perhaps some ISM330 IMU data
struct IMUData {
    float accel_x;
    float accel_y;
    float accel_z;
    float real;
    float i;
    float j;
    float k;
    short cali_status;
};

// Struct to consolidate all data
struct AllData {
    //DuplexData duplex;
    IMUData imu;
    short state = 0;
} all_data; // Global variable to hold all data
//============================================= End Data Structs ============================================

// Function Declarations (Prototypes)
void setupSD();
void cleanupSD();
void logData(AllData &data);
//void readReceiver(DuplexData &data);
void setReports(sh2_SensorId_t reportType, long report_interval);
void readBNO085(IMUData &data);

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10); // will pause until serial console opens

    setupSD();

    while (!bno08x.begin_I2C()) {
      Serial.println("Failed to find BNO08x chip");
      delay(500);
    }
    delay(3000);
}
int count = 0;

void loop() {
    unsigned long startMillis = millis();
    //readReceiver(all_data.duplex);

    while (count < 500) {
        readBNO085(all_data.imu);
        //logData(all_data);
        //delay(2000);
    }
    unsigned long endMillis = millis();
    Serial.println("time taken: " + String(1000/((endMillis - startMillis)/500.0), 4));
    count = 0;
    //cleanupSD(); // Cleanup SD card
    delay(1000);
    //setupSD(); // Setup SD card again for next logging session
}

//============================================ Functions ============================================

void logData(AllData &data) {
    // Amount of data in ringBuf.
    size_t n = rb.bytesUsed();
    if ((n + file.curPosition()) > (LOG_FILE_SIZE - 20)) {
        Serial.println("File full - quitting.");
        return;
    }
    if (n > maxUsed) {
        maxUsed = n;
    }
    if (n >= 512 && !file.isBusy()) {
        // Not busy only allows one sector before possible busy wait.
        // Write one sector from RingBuf to file.
        if (512 != rb.writeOut(512)) {
        Serial.println("writeOut failed");
        return;
        }
    }
    rb.print(millis());
    rb.write(',');
    rb.print(data.state);
    rb.write(',');
    rb.print(data.imu.accel_x, 4);
    rb.write(',');
    rb.print(data.imu.accel_y, 4);
    rb.write(',');
    rb.print(data.imu.accel_z, 4);
    rb.write(',');
    rb.print(data.imu.real, 5);
    rb.write(',');
    rb.print(data.imu.i, 5);
    rb.write(',');
    rb.print(data.imu.j, 5);
    rb.write(',');
    rb.print(data.imu.k, 5);
    rb.write(',');
    rb.println(data.imu.cali_status);
    if (rb.getWriteError()) {
        // Error caused by too few free bytes in RingBuf.
        Serial.println("WriteError");
        return;
      }
    // Write any RingBuf data to file.
    rb.sync();
}

void cleanupSD(){
    file.truncate();
    Serial.print("fileSize: ");
    Serial.println((uint32_t)file.fileSize());
    Serial.print("maxBytesUsed: ");
    Serial.println(maxUsed);
    file.close();
}

void setupSD(){
    // Initialize the SD.
    if (!sd.begin(SD_CONFIG)) {
      sd.initErrorHalt(&Serial);
    }

    int fileIteration = 0;
    boolean fileCreated = false;
    while(!fileCreated && fileIteration < 1000){
        String tempName = "FLIGHT" + String(fileIteration) + ".csv";
        int str_len = tempName.length() + 1;
        char LOG_FILENAME[str_len];
        tempName.toCharArray(LOG_FILENAME, str_len);
        // Try to create a new file, fail if it already exists
        if (file.open(LOG_FILENAME, O_RDWR | O_CREAT | O_EXCL)) {
            fileCreated = true;
        } else {
            fileIteration++;
        }
    }

    if(!fileCreated){
        Serial.println("No available filename - file not open/created.");
    }

    // File must be pre-allocated to avoid huge
    // delays searching for free clusters.
    if (!file.preAllocate(LOG_FILE_SIZE)) {
      Serial.println("preAllocate failed\n");
      file.close();
      return;
    }
    // initialize the RingBuf.
    rb.begin(&file);
}

void readBNO085(IMUData &data) {
    if (bno08x.wasReset()) {
        Serial.println("Sensor was reset ");
        setReports(SH2_ROTATION_VECTOR, 1000);
        delay(300);
    }
    if (bno08x.getSensorEvent(&sensorValue)) {
        data.cali_status = sensorValue.status;
        switch (sensorValue.sensorId) {
            case SH2_ROTATION_VECTOR:
                data.real = sensorValue.un.rotationVector.real;
                data.i = sensorValue.un.rotationVector.i;
                data.j = sensorValue.un.rotationVector.j;
                data.k = sensorValue.un.rotationVector.k;
                count++;
                break;
        }
    } else {
        data.cali_status = -1; // for bad data or specifying that now the data is slightly old
    }
}

void setReports(sh2_SensorId_t reportType, long report_interval) {
    Serial.println("Setting desired reports");
    if (! bno08x.enableReport(reportType, report_interval)) {
      Serial.println("Could not enable rotation vector");
    }
}
//============================================ End Functions ============================================