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
    uint32_t overall_time = millis();
    int8_t state = 1;
    uint16_t servo1 = random(1000,2000);
    uint16_t servo2 = random(1000,2000);
    uint16_t servo3 = random(1000,2000);
    uint16_t servo4 = random(1000,2000);
    uint16_t motor = random(1000,2000);
};

struct __attribute__((packed)) teensyStatus {
    uint8_t packet_type = StatusPk;
    uint32_t overall_time = millis();
    int8_t ac_state = -1;
    int8_t bno_state = -1;
    int8_t ism_state = -1;
    int8_t sd_state = -1;
};

struct __attribute__((packed)) espACStatus { //for future proofing atp but currently just holds the state of the AC
    uint8_t packet_type = StatusPk;
    int8_t esp_ac_state = -1;
};

struct __attribute__((packed)) espGCSStatus {
    uint8_t packet_type = StatusPk;
    int8_t esp_gcs_state = -1;
};


struct __attribute__((packed)) statusStruct {
    teensyStatus teensy_status;
    espACStatus esp_ac_status;
    espGCSStatus esp_gcs_status;
};

constexpr uint16_t AC_GCS_status_size = sizeof(espACStatus) + sizeof(espGCSStatus); // yeah we are defining/initializing this here, but i just use this everywhere, and i dont wanna make a dedicated cpp file
constexpr uint16_t teensy_AC_status_size = sizeof(teensyStatus) + sizeof(espACStatus);