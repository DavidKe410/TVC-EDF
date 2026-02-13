#include "data_hub.h"

// Struct to consolidate all data
AllData all_data; 
// Packed Data Struct
PackedStruct packed_data;

void packData(AllData &data, PackedStruct &packed) {
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

void sendData(PackedStruct &packed_data) {
    // Rate limit telemetry to TELE_RATE
    if (all_data.overall_time - last_tele_MS >= TELE_RATE) {
        // Only send if there is enough room for the packet (100 bytes + overhead)
        if ((size_t)Serial1.availableForWrite() >= (sizeof(packed_data) + 20)) {
            serialTransfer.sendDatum(packed_data);
        } else {
            Serial.println("Dropped a packet for ESP32 telemetry");
        }

        // Prevent drift timers by resetting if we're too far behind, otherwise just increment by the rate.
        if (all_data.overall_time - last_tele_MS > 5 * TELE_RATE) {
            last_tele_MS = all_data.overall_time; // reset if behind
        }else{
            last_tele_MS += TELE_RATE;
        }
    }

}

void logData(PackedStruct &packed_data) {
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

void setupSD(){
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
}