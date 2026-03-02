#include "sensors.h"

//====== BNO085 ======
Adafruit_BNO08x  bno08x(-1);
sh2_SensorValue_t sensorValue;
uint32_t reportIntervalUs = 2000;
uint32_t bnoResetTimer = 0;
bool bnoReady = true;
//====== End BNO085 ======

// ======= ISM330DLC =======
SparkFun_ISM330DHCX ism330;
sfe_ism_data_t accelData;
// ======= End ISM330DLC =======

void setupBNO085(statusStruct &system_status) {
    while (!bno08x.begin_I2C()) { // This starts a wire that the ISM can use...AND overwrites the setClock speed to soemthing slower????wtf is wrong with this sensor
      Serial.println("Failed to find BNO08x chip");
      delay(500);
    }
    Serial.println("BNO08x I2C setup complete.");
    system_status.teensy_status.bno_state = 1;
}

void setupISM330(statusStruct &system_status) {
	while (!ism330.begin()){
		Serial.println("ISM did not begin. Please check the wiring...");
		delay(500);
	}
    ism330.deviceReset();
	// Wait for it to finish reseting
	while (!ism330.getDeviceReset()){delay(10);}
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
    system_status.teensy_status.ism_state = 1;
}

void readISM330(IMUData &data) {
	if( ism330.checkAccelStatus() ){
		ism330.getAccel(&accelData);
        data.accel_x = accelData.xData*0.00980665; // Convert mg to m/s²
        data.accel_y = accelData.yData*0.00980665;
        data.accel_z = accelData.zData*0.00980665;
        data.temp = ism330.getTemp()/256.0 + 25; // Convert to °C
        data.accel_time = millis();
        data.new_accel = true;
        //loopCount++;
    }else{
        data.new_accel = false; // Old data
    }
}

void readBNO085(IMUData &data) {
    if (bno08x.wasReset()) {
        Serial.println("BNO085 was reset.");
        setReports(SH2_ROTATION_VECTOR, reportIntervalUs);
        bnoResetTimer = millis(); // Don't want a blocking delay, so just keep things moving w/ old data
        bnoReady = false;
    }
    if (bnoReady && bno08x.getSensorEvent(&sensorValue)) {
        data.orien_cali_status = sensorValue.status;
        switch (sensorValue.sensorId) { // welp just have one report type rn, so kinda unnecessary
            case SH2_ROTATION_VECTOR:
                data.real = sensorValue.un.rotationVector.real;
                data.i = sensorValue.un.rotationVector.i;
                data.j = sensorValue.un.rotationVector.j;
                data.k = sensorValue.un.rotationVector.k;
                data.orien_time = millis();
                loopCount++; // for the main loop hz testing, can remove later
                break;
        }
    } else {
        if (millis() - bnoResetTimer > 150) {
            bnoReady = true;
        }
        data.orien_cali_status = -1; // for bad data or specifying that now the data is slightly old
    }
}

void setReports(sh2_SensorId_t reportType, uint32_t report_interval) {
    Serial.println("Setting desired reports");
    if (! bno08x.enableReport(reportType, report_interval)) {
      Serial.println("Could not enable rotation vector");
    }
}
