#pragma once

#include "SerialTransfer.h"

constexpr uint8_t CYCLE_TIME_MS = 4; // 350 Hz
extern uint16_t loopCount;

extern SerialTransfer serialTransfer;

#define CHANNEL 0

constexpr uint8_t STATUS_RATE = 50;

extern uint32_t last_status_ms;

// #ifdef ARDUINO_ARCH_ESP32
//     // This code only exists for ESP32, ESP32-S2, ESP32-C3, etc.
// #endif
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

    constexpr uint8_t TELE_RATE = 10;

    constexpr uint8_t TELE_LINE_LENGTH = 50; // Estimated number of bytes per line

    // Space to hold around 2s of data.
    constexpr size_t TX_BUF_CAPACITY = TELE_LINE_LENGTH * (1000 / TELE_RATE) * 2;
    extern uint8_t tx_buffer[TX_BUF_CAPACITY];
    //====== End Telemetry/Commands =======
#endif