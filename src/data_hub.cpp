#include "data_hub.h"

// Struct to consolidate all data
AllData all_data; 
// Packed Data Struct
PackedDataStruct packed_data;
// Command Struct from GCS
CommandStruct rx_command;
// Status Struct for both ac and gcs
statusStruct system_status;

void receiveData(){ //This is for the onboard teensy
    if (serialTransfer.available()){
        uint8_t packetID = serialTransfer.currentPacketID();
        switch (packetID){
            case 1:
                serialTransfer.rxObj(rx_command);
                // If we have multiple packet types, we can use the packetID to determine how to parse the data
                break;
            case 2:
                serialTransfer.rxObj(system_status.esp_ac_status);
                serialTransfer.rxObj(system_status.esp_gcs_status, sizeof(system_status.esp_ac_status)); // If rxObj removes it from the buffer, could this sizeof just be 0
                break;
        }
    }
}

void packData(AllData &data, PackedDataStruct &packed) {
    // Brute force copy data into packed struct :/
    packed.overall_time = data.overall_time;
    packed.state = data.state;
    packed.accel_time = data.imu.accel_time;
    packed.accel_x = data.imu.accel_x;
    packed.accel_y = data.imu.accel_y;
    packed.accel_z = data.imu.accel_z;
    packed.orien_time = data.imu.orien_time;
    packed.real = data.imu.real;
    packed.i = data.imu.i;
    packed.j = data.imu.j;
    packed.k = data.imu.k;
    packed.temp = data.imu.temp;
    packed.new_accel = data.imu.new_accel;
    packed.orien_cali_status = data.imu.orien_cali_status;
}

void sendData(PackedDataStruct &packed_data, statusStruct &system_status) {
    if (system_status.esp_ac_status.esp_ac_state == -1){ return; }

    // Rate limit telemetry to TELE_RATE
    if (packed_data.overall_time - last_tele_ms >= TELE_RATE) { // Using packed_data.overall_time as millis()
        // Only send if there is enough room for the packet (100 bytes + overhead)
        if ((size_t)Serial7.availableForWrite() >= (sizeof(packed_data) + 20)) {
            serialTransfer.txObj(packed_data);
            serialTransfer.sendData(sizeof(packed_data), 0);
        } else {
            Serial.println("Dropped a telemetry packet to ESP32");
        }

        // Prevent drift timers by resetting if we're too far behind, otherwise just increment by the rate.
        if (packed_data.overall_time - last_tele_ms > 5 * TELE_RATE) {
            last_tele_ms = packed_data.overall_time; // reset if behind
        }else{
            last_tele_ms += TELE_RATE;
        }
    }

    if (packed_data.overall_time - last_status_ms >= STATUS_RATE) {
        if ((size_t)Serial7.availableForWrite() >= (sizeof(system_status.teensy_status) + 20)) {
            serialTransfer.txObj(system_status.teensy_status);
            serialTransfer.sendData(sizeof(system_status.teensy_status), 2); // status packet id = 2
        } else {
            Serial.println("Dropped a system status packet");
        }

        if (packed_data.overall_time - last_status_ms > 5 * STATUS_RATE) {
            last_status_ms = packed_data.overall_time; // reset if behind
        }else{
            last_status_ms += STATUS_RATE;
        }
    }

}

void logData(PackedDataStruct &packed_data) {
    // Amount of data in ringBuf.
    size_t n = log_rb.bytesUsed();
    if ((n + file.curPosition()) > (LOG_FILE_SIZE - 20)) {
        Serial.println("File full - quitting.");
        return;
    }
    if (n > maxUsed) {maxUsed = n;}

    if (n >= 512 && !file.isBusy()) {
        // Not busy only allows one sector before possible busy wait.
        // Write one sector from RingBuf to file.
        if (512 != log_rb.writeOut(512)) {
            Serial.println("writeOut failed");
            return;
        }
    }

    log_rb.write((uint8_t*)&packed_data, sizeof(packed_data));

    if (log_rb.getWriteError()) {
        // Error caused by too few free bytes in RingBuf.
        Serial.println("WriteError");
        return;
      }
    // Flush RB data into file object. Force SD to potentially work more inefficiently but at least gets data over
    if (n > (LOG_BUF_CAPACITY * 0.75)) {
        log_rb.sync(); 
    }
}

void cleanupSD(){
    log_rb.sync();
    file.truncate();
    Serial.print("fileSize: ");
    Serial.println((uint32_t)file.fileSize());
    Serial.print("maxBytesUsed: ");
    Serial.println(maxUsed);
    file.close();
}

void setupSD(statusStruct &system_status){
    // Initialize the SD.
    if (!sd.begin(SD_CONFIG)) {
      sd.initErrorHalt(&Serial);
    }

    int fileIteration = 0;
    boolean fileCreated = false;
    while(!fileCreated && fileIteration < 1000){
        String tempName = "FLIGHT" + String(fileIteration) + ".bin";
        int str_len = tempName.length() + 1;
        char LOG_FILENAME[str_len];
        tempName.toCharArray(LOG_FILENAME, str_len);
        // Try to create a new file, fail if it already exists
        if (file.open(LOG_FILENAME, O_RDWR | O_CREAT | O_EXCL)) {
            fileCreated = true;
        } else {
            fileIteration++;
        }
    }

    if(!fileCreated){
        Serial.println("No available filename - file not open/created.");
    }

    // File must be pre-allocated to avoid huge
    // delays searching for free clusters.
    if (!file.preAllocate(LOG_FILE_SIZE)) {
      Serial.println("preAllocate failed\n");
      file.close();
      return;
    }
    // initialize the RingBuf.
    log_rb.begin(&file);

    Serial.println("SD setup complete");
    system_status.teensy_status.sd_state = 1;
}