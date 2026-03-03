#pragma once

#include "SerialTransfer.h"

constexpr uint8_t CYCLE_TIME_MS = 4; // 350 Hz
extern uint16_t loopCount;

extern SerialTransfer serialTransfer;

constexpr uint16_t STATUS_RATE = 500; // idk why we need this to be constexpr

extern uint32_t last_status_ms;

extern uint32_t last_status_rx;

constexpr uint16_t heartbeat_timeout = STATUS_RATE * 3;

extern uint32_t last_cmd_ms;

constexpr uint8_t COMMAND_RATE = 10;

constexpr uint32_t serialT_Baud = 2000000;

constexpr uint32_t serialT_timeout = 5; // ms

extern uint8_t availWriteMargin;

#ifdef ESP32
    #include <esp_wifi_types.h>
    #define CHANNEL 0
    const wifi_phy_rate_t ESPNOW_RATE = WIFI_PHY_RATE_6M;
#endif

#ifdef CORE_TEENSY
    // This code only exists for Teensy boards

    #include "SdFat.h"
    #include "RingBuf.h"

    //======= Data Logger ========
    #define SD_CONFIG SdioConfig(FIFO_SDIO)

    // Estimated number of bytes per line
    constexpr uint8_t LOG_LINE_LENGTH = 50;

    // Size for line length at cycle time for 10 minutes.
    constexpr size_t LOG_FILE_SIZE = LOG_LINE_LENGTH * (1000/CYCLE_TIME_MS) * 10 * 60;

    // Space to hold around 2s of data.
    constexpr size_t LOG_BUF_CAPACITY = LOG_LINE_LENGTH * (1000/CYCLE_TIME_MS) * 2;

    // Max RingBuf used bytes. Useful to understand RingBuf overrun.
    extern size_t maxUsed;
    extern SdFs sd;
    extern FsFile file;

    // RingBuf for File type FsFile.
    extern RingBuf<FsFile, LOG_BUF_CAPACITY> log_rb;
    //===== End Data Logger ========

    //====== Telemetry/Commands =======
    extern uint32_t last_tele_ms;

    constexpr uint16_t TELE_RATE = 10;

    constexpr uint8_t TELE_LINE_LENGTH = 50; // Estimated number of bytes per line

    // Space to hold around 2s of data.
    constexpr size_t TX_BUF_CAPACITY = TELE_LINE_LENGTH * (1000 / TELE_RATE) * 2;
    extern uint8_t tx_buffer[TX_BUF_CAPACITY];

    constexpr size_t RX_BUF_CAPACITY = 1024; //TELE_LINE_LENGTH * (1000 / TELE_RATE) * 2;
    extern uint8_t rx_buffer[RX_BUF_CAPACITY];

    //====== End Telemetry/Commands =======

#endif