#pragma once
#include "config.h"
#include "data_structs.h"


// Struct to consolidate all data
extern AllData all_data;
// Packed Data Struct
extern PackedDataStruct packed_data;
// Command Struct from GCS
extern CommandStruct rx_command;
// Status Struct for both ac and gcs
extern statusStruct system_status;

void setupSD(statusStruct &system_status);
void cleanupSD();
void logData(PackedDataStruct &packed_data);
void packData(AllData &all_data, PackedDataStruct &packed_data);
void sendData(PackedDataStruct &packed_data, statusStruct &system_status);
void receiveData();