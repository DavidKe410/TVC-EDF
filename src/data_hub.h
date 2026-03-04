#pragma once
#include "config.h"
#include "data_structs.h"


// Struct to consolidate all data
extern AllData g_all_data;
// Packed Data Struct
extern PackedDataStruct g_packed_data;
// Command Struct from GCS
extern CommandStruct g_command;
// Status Struct for both ac and gcs
extern StatusStruct g_system_status;

void setupSD(StatusStruct &system_status);
void cleanupSD();
void logData(PackedDataStruct &packed_data);
void packData(AllData &all_data, PackedDataStruct &packed_data);
void sendData(PackedDataStruct &packed_data, StatusStruct &system_status);
void receiveData(CommandStruct &rx_command, StatusStruct &system_status);
void processCMD(CommandStruct &rx_command, StatusStruct &system_status);