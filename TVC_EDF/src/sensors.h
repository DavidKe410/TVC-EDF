#pragma once
#include "config.h"
#include "data_structs.h"
#include "SparkFun_ISM330DHCX.h"
#include <Adafruit_BNO08x.h>

//====== BNO085 ======
extern Adafruit_BNO08x  bno08x;
extern sh2_SensorValue_t sensorValue;
extern uint32_t reportIntervalUs;
//====== End BNO085 ======

// ======= ISM330DLC =======
extern SparkFun_ISM330DHCX ism330;
extern sfe_ism_data_t accelData;
// ======= End ISM330DLC =======

void setupBNO085();
void setReports(sh2_SensorId_t reportType, uint32_t report_interval);
void readBNO085(IMUData &data);
void setupISM330();
void readISM330(IMUData &data);