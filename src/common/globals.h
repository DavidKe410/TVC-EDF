#pragma once
#include <stdint.h>
#include "config.h"
#include "data_structs.h"
#include "SerialTransfer.h"

// Shared Global Variables

extern PackedDataStruct g_packed_data; // Would fit better within data_hub, but esps also use
extern CommandStruct g_command;
extern StatusStruct g_system_status;

extern uint16_t loopCount;
extern uint32_t last_status_ms;
extern uint32_t last_cmd_ms;
extern uint32_t last_status_rx;

// Shared Objects
extern SerialTransfer serialTransfer;

extern uint16_t AC_GCS_status_size;
extern uint16_t teensy_AC_status_size;

#ifdef ESP32 // Ones common to both
    #include <esp_now.h>
    extern esp_now_peer_info_t peerInfo;
    extern esp_err_t result;
    extern uint32_t last_status_rx2; //yeah creative name :/
#endif