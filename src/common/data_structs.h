#pragma once
#include <Arduino.h>

// Struct to store BNO085 + perhaps some ISM330 IMU data
struct IMUData {
    uint32_t accel_time;
    float accel_x;
    float accel_y;
    float accel_z;
    uint32_t orien_time;
    float real;
    float i;
    float j;
    float k;
    int8_t temp;
    uint8_t new_accel;
    int8_t orien_cali_status;
};

// Struct to consolidate all data
struct AllData {
    uint32_t overall_time;
    IMUData imu;
    int8_t state = 0;
}; // Global variable to hold all data



enum PacketType : uint8_t {
    TelemetryPk = 0,
    CommandPk = 1,
    StatusPk = 2
};



// Packed Data Struct
struct __attribute__((packed)) PackedDataStruct {
    uint8_t packet_type = TelemetryPk;
    uint32_t overall_time;
    int8_t state;
    uint32_t accel_time;
    float accel_x;
    float accel_y;
    float accel_z;
    uint32_t orien_time;
    float real;
    float i;
    float j;
    float k;
    int8_t temp;
    uint8_t new_accel;
    int8_t orien_cali_status;
};

struct __attribute__((packed)) CommandStruct {
    uint8_t packet_type = CommandPk;
    uint8_t type_cmd = 0; // maybe redundant with the manual control flag, could specify for an ack back
    int8_t state = 0; // 0 idle, 1 auto, 2 manual
    uint32_t overall_time = millis();
    uint16_t servo1 = random(1000,2000);
    uint16_t servo2 = random(1000,2000);
    uint16_t servo3 = random(1000,2000);
    uint16_t servo4 = random(1000,2000);
    uint16_t motor = random(1000,2000);
    uint16_t cmd_ID = 0; // may have teensy send back with particular ack num matching GCS
};

struct __attribute__((packed)) teensyStatus {
    uint8_t packet_type = StatusPk;
    uint32_t overall_time = millis();
    int8_t ac_state = -1;
    int8_t bno_state = -1;
    int8_t ism_state = -1;
    int8_t sd_state = -1;
    uint16_t cmd_ack_ID = 0;
};

struct __attribute__((packed)) espACStatus { //for future proofing atp but currently just holds the state of the AC
    uint8_t packet_type = StatusPk;
    int8_t esp_ac_state = -1;
};

struct __attribute__((packed)) espGCSStatus {
    uint8_t packet_type = StatusPk;
    int8_t esp_gcs_state = -1;
};

struct __attribute__((packed)) laptopStatus {
    uint8_t packet_type = StatusPk;
    int8_t laptop_state = -1;
};

struct __attribute__((packed)) StatusStruct {
    teensyStatus teensy_status;
    espACStatus esp_ac_status;
    espGCSStatus esp_gcs_status;
    laptopStatus laptop_status;
};