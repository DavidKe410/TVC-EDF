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

// Packed Data Struct
struct __attribute__((packed)) PackedDataStruct {
    uint8_t packet_type = 0;
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
    uint8_t packet_type = 1;
    uint32_t overall_time;
    int8_t state;
    uint16_t servo1;
    uint16_t servo2;
    uint16_t servo3;
    uint16_t servo4;
    uint16_t motor;
};

struct __attribute__((packed)) teensyStatus {
    uint32_t overall_time = millis();
    int8_t ac_state = -1;
    int8_t bno_state = -1;
    int8_t ism_state = -1;
    int8_t sd_state = -1;
};

struct __attribute__((packed)) espACStatus { //for future proofing atp but currently just holds the state of the AC
    int8_t esp_ac_state = -1;
};

struct __attribute__((packed)) espGCSStatus {
    int8_t esp_gcs_state = -1;
};


struct __attribute__((packed)) statusStruct {
    uint8_t packet_type = 2;
    teensyStatus teensy_status;
    espACStatus esp_ac_status;
    espGCSStatus esp_gcs_status;
};