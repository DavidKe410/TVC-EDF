#include <Arduino.h>
#include <Adafruit_BNO08x.h>
#include <Wire.h>
#include "SparkFun_ISM330DHCX.h"
#include "RingBuf.h"
#include "SdFat.h"

//============================================ Configuration ============================================

// ======== General ========
const short CYCLE_TIME_MS = 4; // 350 Hz
short loopCount = 0;
// ====== End General ======

//====== BNO085 ======
Adafruit_BNO08x  bno08x(-1);
sh2_SensorValue_t sensorValue;
long reportIntervalUs = 2000;
//====== End BNO085 ======

// ======= ISM330DLC =======
SparkFun_ISM330DHCX ism330;
sfe_ism_data_t accelData;
// ======= End ISM330DLC =======

//======= Data Logger ========
#define SD_CONFIG SdioConfig(FIFO_SDIO)

const short log_line_length = 100; // Estimated number of bytes per line 
    
// Size for line length at cycle time for 10 minutes.
const size_t LOG_FILE_SIZE = log_line_length * (1000/CYCLE_TIME_MS) * 10 * 60;

// Space to hold around 2s of data.
const size_t LOG_BUF_CAPACITY  = log_line_length * (1000/CYCLE_TIME_MS) * 2;

// Max RingBuf used bytes. Useful to understand RingBuf overrun.
size_t maxUsed = 0;

SdFs sd;
FsFile file;

// RingBuf for File type FsFile.
RingBuf<FsFile, LOG_BUF_CAPACITY> log_rb;
//===== End Data Logger ========

//====== Telemetry =======
#define TELE_SERIAL Serial1

const short tele_line_length = 100; // Estimated number of bytes per line 

// Space to hold around 2s of data.
const size_t TELE_BUF_CAPACITY  = tele_line_length * (1000/CYCLE_TIME_MS) * 2;

// RingBuf for radio hardware serial telemetry output
RingBuf<HardwareSerial, TELE_BUF_CAPACITY> tele_rb;
//====== End Telemetry =======



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
    short temp;
    bool new_accel;
    short orien_cali_status;
};

// Struct to consolidate all data
struct AllData {
    IMUData imu;
    short state = 0;
} all_data; // Global variable to hold all data
//============================================= End Data Structs ============================================

// Function Declarations (Prototypes)
void setupSD();
void cleanupSD();
void logData(AllData &data);
void setReports(sh2_SensorId_t reportType, long report_interval);
void readBNO085(IMUData &data);
void setupISM330();
void readISM330(IMUData &data);
void sendData(AllData &data);

void setup() {

    Serial.begin(115200);
    while (!Serial) delay(10); // will pause until serial console opens

    Serial1.begin(115200);

    tele_rb.begin(&TELE_SERIAL);

    setupSD();

    while (!bno08x.begin_I2C()) { // This starts a wire that the ISM can use...AND overwrites the setClock speed to soemthing slower????wtf is wrong with this sensor
      Serial.println("Failed to find BNO08x chip");
      delay(500);
    }

    Wire.setClock(400000);

    setupISM330();

    delay(3000);
}

int count = 0;
int validAccel = 0;
int validOrien = 0;
void loop() {
    unsigned long startMillis = millis();
    while (count < 1000) {
        //delayMicroseconds(160);
        readBNO085(all_data.imu);
        readISM330(all_data.imu); //ism after bno085 for more consistent sampling where both are valid in one cycle
        logData(all_data);
        // while ((millis() - startMillis) < (count * CYCLE_TIME_MS)) {
        //     // wait until next cycle
        //     delayMicroseconds(100);
        // }
        //delay(2000);
    }
    unsigned long endMillis = millis();
    Serial.println("time taken: " + String(1000/((endMillis - startMillis)/1000.0), 4));
    Serial.println("Valid Accel: " + String(validAccel) + ". Valid Orien: " + String(validOrien));
    cleanupSD(); // Cleanup SD card
    count = 0;
    delay(3000);
    setupSD(); // Setup SD card again for next logging session
    validAccel = 0;
    validOrien = 0;
}

//============================================ Functions ============================================
void setupISM330() {
	while (!ism330.begin()){
		Serial.println("ISM did not begin. Please check the wiring...");
		delay(500);
	}
    ism330.deviceReset();
	// Wait for it to finish reseting
	while (!ism330.getDeviceReset()){delay(1);}
    delay(100);
 	ism330.setDeviceConfig();
	ism330.setBlockDataUpdate();   

	ism330.setAccelDataRate(ISM_XL_ODR_833Hz);
	ism330.setAccelFullScale(ISM_4g);
	ism330.setGyroDataRate(ISM_GY_ODR_OFF); // Not planning to use gyro data atm

	// Turn on the accelerometer's filter and apply settings.
	ism330.setAccelFilterLP2();
	ism330.setAccelSlopeFilter(ISM_LP_ODR_DIV_400);
    Serial.println("ISM330DHCX setup complete.");
}

void readISM330(IMUData &data) {
	if( ism330.checkAccelStatus() ){
		ism330.getAccel(&accelData);
        data.accel_x = accelData.xData*0.00980665; // Convert mg to m/s²
        data.accel_y = accelData.yData*0.00980665;
        data.accel_z = accelData.zData*0.00980665;
        data.temp = ism330.getTemp()/256.0 + 25; // Convert to °C
        data.new_accel = true;
        validAccel++;
        //count++;
    }else{
        data.new_accel = false; // Old data
    }
}

void readBNO085(IMUData &data) {
    if (bno08x.wasReset()) {
        Serial.println("BNO085 was reset.");
        setReports(SH2_ROTATION_VECTOR, 2000);
        delay(300);
    }
    if (bno08x.getSensorEvent(&sensorValue)) {
        data.orien_cali_status = sensorValue.status;
        switch (sensorValue.sensorId) {
            case SH2_ROTATION_VECTOR:
                data.real = sensorValue.un.rotationVector.real;
                data.i = sensorValue.un.rotationVector.i;
                data.j = sensorValue.un.rotationVector.j;
                data.k = sensorValue.un.rotationVector.k;
                validOrien++;
                count++;
                break;
        }
    } else {
        data.orien_cali_status = -1; // for bad data or specifying that now the data is slightly old
    }
}

void setReports(sh2_SensorId_t reportType, long report_interval) {
    Serial.println("Setting desired reports");
    if (! bno08x.enableReport(reportType, report_interval)) {
      Serial.println("Could not enable rotation vector");
    }
}

void sendData(AllData &data) {
    tele_rb.write((uint8_t*)(&data), sizeof(AllData));
    if (tele_rb.getWriteError()) {
        // Error caused by too few free bytes in RingBuf. Producing faster than UART can drain. So it stops accepting and RB fills.
        Serial.println("WriteError");
        return;
    }
    // Moves RB data into UART TX FIFO
    tele_rb.sync();
}

void logData(AllData &data) {
    // Amount of data in ringBuf.
    size_t n = log_rb.bytesUsed();
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
        if (512 != log_rb.writeOut(512)) {
        Serial.println("writeOut failed");
        return;
        }
    }
    log_rb.print(millis());
    log_rb.write(',');
    log_rb.print(data.state);
    log_rb.write(',');
    log_rb.print(data.imu.accel_x, 4);
    log_rb.write(',');
    log_rb.print(data.imu.accel_y, 4);
    log_rb.write(',');
    log_rb.print(data.imu.accel_z, 4);
    log_rb.write(',');
    log_rb.print(data.imu.real, 5);
    log_rb.write(',');
    log_rb.print(data.imu.i, 5);
    log_rb.write(',');
    log_rb.print(data.imu.j, 5);
    log_rb.write(',');
    log_rb.print(data.imu.k, 5);
    log_rb.write(',');
    log_rb.print(data.imu.temp);
    log_rb.write(',');
    log_rb.print(data.imu.new_accel);
    log_rb.write(',');
    log_rb.println(data.imu.orien_cali_status);
    if (log_rb.getWriteError()) {
        // Error caused by too few free bytes in RingBuf.
        Serial.println("WriteError");
        return;
      }
    // Flush RB data into file object.
    log_rb.sync(); // Can Rate limit this, but prob not very necessary
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
    log_rb.begin(&file);
}
//============================================ End Functions ============================================