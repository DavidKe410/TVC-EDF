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
    uint8_t packet_type = 2;
    uint32_t overall_time = millis();
    int8_t ac_state = -1;
    int8_t bno_state = -1;
    int8_t ism_state = -1;
    int8_t sd_state = -1;
};

struct __attribute__((packed)) espACStatus { //for future proofing atp but currently just holds the state of the AC
    uint8_t packet_type = 2;
    int8_t esp_ac_state = -1;
};

struct __attribute__((packed)) espGCSStatus {
    uint8_t packet_type = 2;
    int8_t esp_gcs_state = -1;
};


struct __attribute__((packed)) statusStruct {
    teensyStatus teensy_status;
    espACStatus esp_ac_status;
    espGCSStatus esp_gcs_status;
};

uint16_t AC_GCS_status_size = sizeof(system_status.esp_ac_status) + sizeof(system_status.esp_gcs_status); // yeah we are defining/initializing this here, but i just use this everywhere, and i dont wanna make a dedicated cpp file
uint16_t teensy_AC_status_size = sizeof(system_status.teensy_status) + sizeof(system_status.esp_ac_status);