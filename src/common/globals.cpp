#include "globals.h"

// Initialize shared variables

PackedDataStruct g_packed_data;
CommandStruct g_command;
StatusStruct g_system_status;

uint16_t loopCount        = 0;
uint32_t last_status_ms   = 0;
uint32_t last_cmd_ms      = 0;
uint32_t last_status_rx   = 0;

SerialTransfer serialTransfer;

uint16_t AC_GCS_status_size = sizeof(espACStatus) + sizeof(espGCSStatus) + sizeof(laptopStatus);
uint16_t teensy_AC_status_size = sizeof(teensyStatus) + sizeof(espACStatus);

#ifdef ESP32
    esp_now_peer_info_t peerInfo;
    esp_err_t result = ESP_OK;
    uint32_t last_status_rx2  = 0;
#endif