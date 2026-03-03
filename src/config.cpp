#include "config.h"

uint16_t loopCount = 0;

#ifdef CORE_TEENSY
    //======= Data Logger ========
    // Max RingBuf used bytes. Useful to understand RingBuf overrun.
    size_t maxUsed = 0;

    SdFs sd;
    FsFile file;

    RingBuf<FsFile, LOG_BUF_CAPACITY> log_rb;
    //===== End Data Logger ========

    //====== Telemetry =======
    uint32_t last_tele_ms = 0;

    uint8_t tx_buffer[TX_BUF_CAPACITY];

    uint8_t rx_buffer[RX_BUF_CAPACITY];
#endif
SerialTransfer serialTransfer;
//====== End Telemetry =======

uint32_t last_status_ms = 0;

uint32_t last_cmd_ms = 0;

uint32_t last_status_rx = 0;

uint8_t availWriteMargin = 15;