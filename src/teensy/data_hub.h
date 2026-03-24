#pragma once
#include "common/config.h"
#include "common/globals.h"
#include "common/data_structs.h"
#include "SdFat.h"
#include "RingBuf.h"
#include <vector>
#include <algorithm>

// Struct to consolidate all data
extern AllData g_all_data; 

// For commands
extern std::vector<uint16_t> g_command_ids;

// SD Card Objects
extern size_t maxUsed;
extern SdFs sd;
extern FsFile file;
extern RingBuf<FsFile, config::LOG_BUF_CAPACITY> log_rb;

// Buffers
extern uint32_t last_tele_ms;
extern uint8_t tx_buffer[config::TX_CAPACITY];
extern uint8_t rx_buffer[config::RX_CAPACITY];

void setupSD(StatusStruct &system_status);
void cleanupSD();
void logData(PackedDataStruct &packed_data);
void packData(AllData &all_data, PackedDataStruct &packed_data);
void sendData(PackedDataStruct &packed_data, StatusStruct &system_status, std::vector<uint16_t> &command_ids);
void receiveData(CommandStruct &rx_command, StatusStruct &system_status);
void processCMD(CommandStruct &rx_command, StatusStruct &system_status, std::vector<uint16_t> &command_ids);