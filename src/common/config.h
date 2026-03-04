#pragma once
#include <stdint.h>
#include <stddef.h>

//=========================================================
// SYSTEM CONFIGURATION
// Shared across ALL microcontrollers (Teensy & ESP32s)
//=========================================================
namespace config {

    // Timing
        constexpr uint8_t  CYCLE_TIME_MS        = 4;   // 250 Hz
        constexpr uint16_t STATUS_INTERVAL_MS   = 500; // 2 Hz
        constexpr uint16_t HEARTBEAT_TIMEOUT_MS = STATUS_INTERVAL_MS * 3;
        constexpr uint8_t  CMD_INTERVAL_MS      = 10;  // 100 Hz
        constexpr uint16_t TELE_INTERVAL_MS     = 10;  // 100 Hz

    // Serial Communications
        constexpr uint32_t SERIAL_T_BAUD  = 2000000;
        constexpr uint32_t SERIAL_T_TIMEOUT_MS = 5;

        constexpr uint8_t  LINE_LENGTH  = 50; 
        constexpr size_t   TX_CAPACITY  = LINE_LENGTH * (1000 / TELE_INTERVAL_MS) * 2;
        constexpr size_t   RX_CAPACITY  = 1024; // both esp and teensy use, maybe diff

        constexpr uint8_t  AVAIL_WRITE_MARGIN = 15;
}

//=========================================================
// HARDWARE-SPECIFIC CONFIGURATION
//=========================================================

#ifdef ESP32
    #include <esp_wifi_types.h>
    namespace config {
        constexpr uint8_t CHANNEL = 0;
        constexpr wifi_phy_rate_t ESP_PHY_RATE = WIFI_PHY_RATE_6M;
    }
#endif

#ifdef CORE_TEENSY
    namespace config {
        // Data log
            constexpr uint8_t  EST_LINE_LENGTH   = 50; // Estimated line length for calculating how large file needs to be
            constexpr size_t   FILE_SIZE     = LINE_LENGTH * (1000 / CYCLE_TIME_MS) * 10 * 60;
            constexpr size_t   LOG_BUF_CAPACITY  = LINE_LENGTH * (1000 / CYCLE_TIME_MS) * 2;
    }
#endif