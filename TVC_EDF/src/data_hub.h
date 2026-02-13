#pragma once
#include "config.h"
#include "data_structs.h"


// Struct to consolidate all data
extern AllData all_data; 
// Packed Data Struct
extern PackedStruct packed_data;

void setupSD();
void cleanupSD();
void logData(PackedStruct &packed_data);
void packData(AllData &all_data, PackedStruct &packed_data);
void sendData(PackedStruct &packed_data);