#include <Arduino.h>
#include <Adafruit_BNO08x.h>
//#include <Servo.h>
#include "RingBuf.h"
#include "SdFat.h"

//============================================ Configuration ============================================

// ======== General ========
const short CYCLE_TIME_MS = 10; // 100 Hz
short loopCount = 0;
// ====== End General ======

//====== BNO085 ======
#define BNO08X_CS 10
#define BNO08X_INT 9
#define BNO08X_RESET 6
Adafruit_BNO08x  bno08x(BNO08X_RESET);
sh2_SensorValue_t sensorValue;
sh2_SensorId_t orien_reportType = SH2_ROTATION_VECTOR;
sh2_SensorId_t accel_reportType = SH2_LINEAR_ACCELERATION;
long reportIntervalUs = 5000;
//====== End BNO085 ======

//======== Servos and ESC ========
// Servo esc;
// Servo leftAileron;
// Servo rightAileron;
// Servo stabilator;
// Servo rudder;
//======== End Servos and ESC ========

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

// Struct to store Duplex receiver data
// struct DuplexData {
//     int throttle;
//     int leftAileron;
//     int rightAileron;
//     int stabilator;
//     int rudder;
//     int manualOverride;
//     bool defaultValues;
// };

// Struct to store BNO085 IMU data
struct IMUData {
    float accel_x;
    float accel_y;
    float accel_z;
    float real;
    float i;
    float j;
    float k;
    uint8_t cali_status;
    bool valid_accel;
    bool valid_orien;
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
    // Wire.begin();
    // Wire.setClock(400000);  // Set I2C clock speed to 400kHz, remove if unreliability occurs
    delay(100);

    setupSD();

    if (!bno08x.begin_SPI(BNO08X_CS, BNO08X_INT)) {
        Serial.println("Failed to find BNO08x chip");
        while (1) { delay(10); }
    }
    setReports(orien_reportType, reportIntervalUs);
    setReports(accel_reportType, reportIntervalUs);

    // esc.attach(2);
    // leftAileron.attach(33);
    // rightAileron.attach(3);
    // stabilator.attach(36);
    // rudder.attach(37);
    // esc.writeMicroseconds(1000); // Set ESC to minimum throttle
    // leftAileron.writeMicroseconds(1500);
    // rightAileron.writeMicroseconds(1500);
    // stabilator.writeMicroseconds(1500);
    // rudder.writeMicroseconds(1500);

    delay(3000);
}

void loop() {
    unsigned long startMillis = millis();
    //readReceiver(all_data.duplex);
    readBNO085(all_data.imu);
    //Serial.println("Accel (m/s^2): X=" + String(all_data.imu.accel_x, 4) + 
                  //  " Y=" + String(all_data.imu.accel_y, 4) + 
                  //  " Z=" + String(all_data.imu.accel_z, 4));
    Serial.println(sensorValue.status);
    // esc.writeMicroseconds(all_data.duplex.throttle);
    // leftAileron.writeMicroseconds(all_data.duplex.leftAileron);
    // rightAileron.writeMicroseconds(all_data.duplex.rightAileron);
    // stabilator.writeMicroseconds(all_data.duplex.stabilator);
    // rudder.writeMicroseconds(all_data.duplex.rudder);
    logData(all_data); // Log data before transition

    if (loopCount++ > 1000000){ // After 1 second, transition to next state
        cleanupSD(); // Cleanup SD card
        while(true) { // Wait indefinitely after landing
            delay(1000);
        }
    }
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
    rb.print(data.imu.valid_accel);
    rb.write(',');
    rb.print(data.imu.real, 5);
    rb.write(',');
    rb.print(data.imu.i, 5);
    rb.write(',');
    rb.print(data.imu.j, 5);
    rb.write(',');
    rb.print(data.imu.k, 5);
    rb.write(',');
    rb.println(data.imu.valid_orien);
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
        Serial.print("sensor was reset ");
        setReports(orien_reportType, reportIntervalUs);
        setReports(accel_reportType, reportIntervalUs);
    }
    if (bno08x.getSensorEvent(&sensorValue)) {
        switch (sensorValue.sensorId) {
            case SH2_LINEAR_ACCELERATION:
                data.accel_x = sensorValue.un.linearAcceleration.x;
                data.accel_y = sensorValue.un.linearAcceleration.y;
                data.accel_z = sensorValue.un.linearAcceleration.z;
                data.valid_accel = true;
                break;
            case SH2_ROTATION_VECTOR:
                data.real = sensorValue.un.rotationVector.real;
                data.i = sensorValue.un.rotationVector.i;
                data.j = sensorValue.un.rotationVector.j;
                data.k = sensorValue.un.rotationVector.k;
                data.valid_orien = true;
                break;
            default:
                // Poor system, instead of bool, use a short or smth to specify: good data=1,bad=0,old=2, cause this is just old data
                data.valid_accel = false;
                data.valid_orien = false;
                break;
        }
    } else {
        data.valid_accel = false;
        data.valid_orien = false;
        Serial.println("Failed to read from BNO085.");
    }
}

void setReports(sh2_SensorId_t reportType, long report_interval) {
    Serial.println("Setting desired reports");
    if (! bno08x.enableReport(reportType, report_interval)) {
      Serial.println("Could not enable stabilized remote vector or acceleration report");
    }
}

//============================================ End Functions ============================================