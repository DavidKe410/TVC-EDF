#include "data_hub.h"

// For testing
int countweirdrate = 0;
int count = 0;
uint32_t previous_time = millis();
// -------------

AllData g_all_data;

std::vector<uint16_t> g_command_ids = {};

size_t maxUsed = 0;
SdFs sd;
FsFile file;
RingBuf<FsFile, config::LOG_BUF_CAPACITY> log_rb;

uint32_t last_tele_ms = 0;
uint8_t tx_buffer[config::TX_CAPACITY];
uint8_t rx_buffer[config::RX_CAPACITY];

void processCMD(CommandStruct &command, StatusStruct &system_status, std::vector<uint16_t> &command_ids) { // have a list of processed commands so we don't process multiple times?
    switch (command.type_cmd){
        case 1: // manual command value for now
            //set servos to this etc.
            break;
        case 2: // random integer rn, will need to standardize and perhaps enum this
            if (std::find(command_ids.begin(), command_ids.end(), command.cmd_ID) == command_ids.end()){ // all of this kinda like a placeholder, simple ack
                command_ids.push_back(command.cmd_ID);
                // more processing yay
            }
            break;
    }
    command.type_cmd = 0; //idk after we process it, revert it to 0 so we don't keep processing it when new commands aren't coming in which means command is stuck in the last one
}

void receiveData(CommandStruct &command, StatusStruct &system_status){ //This is for the onboard teensy
    if (serialTransfer.available()){
        switch (serialTransfer.currentPacketID()){
            case CommandPk:
                serialTransfer.rxObj(command);
                
                count++;
                if (count == 400) {
                    uint32_t current_time = millis();
                    float rate = 1000/((current_time - previous_time)/400.0);
                    if (rate < 95 || rate > 105) {
                        countweirdrate++;
                    }
                    Serial.println("Received 400 packets, average rate: " + String(rate, 4) + " packets/s");
                    previous_time = current_time;
                    count = 0;
                }

                break;
            case StatusPk:
                serialTransfer.rxObj(system_status.esp_ac_status);
                serialTransfer.rxObj(system_status.esp_gcs_status, sizeof(espACStatus)); // If rxObj removes it from the buffer, could this sizeof just be 0
                serialTransfer.rxObj(system_status.laptop_status, sizeof(espACStatus) + sizeof(espGCSStatus));
                last_status_rx = millis();
                break;
        }
    }
    if ((millis()-last_status_rx) >= config::HEARTBEAT_TIMEOUT_MS) {
        system_status.esp_ac_status.esp_ac_state = -2; // mark as disconnected
        system_status.esp_gcs_status.esp_gcs_state = -2; // mark as disconnected
        system_status.laptop_status.laptop_state = -2;
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

void sendData(PackedDataStruct &packed_data, StatusStruct &system_status, std::vector<uint16_t> &command_ids) {
    // Rate limit telemetry to TELE_RATE
    if ((packed_data.overall_time - last_tele_ms >= config::TELE_INTERVAL_MS) && (system_status.esp_ac_status.esp_ac_state > -1)) { // Using packed_data.overall_time as millis()
        if ((size_t)Serial7.availableForWrite() >= (sizeof(packed_data) + config::AVAIL_WRITE_MARGIN)) {
            serialTransfer.txObj(packed_data);
            serialTransfer.sendData(sizeof(packed_data), TelemetryPk);
        } else {
            Serial.println("Dropped a telemetry packet to ESP32");
        }

        // Prevent drift timers by resetting if we're too far behind, otherwise just increment by the rate.
        if (packed_data.overall_time - last_tele_ms > 5 * config::TELE_INTERVAL_MS) {
            last_tele_ms = packed_data.overall_time; // reset if behind
        }else{
            last_tele_ms += config::TELE_INTERVAL_MS;
        }
    }

    if (packed_data.overall_time - last_status_ms >= config::STATUS_INTERVAL_MS) {
        if ((size_t)Serial7.availableForWrite() >= (sizeof(system_status.teensy_status) + config::AVAIL_WRITE_MARGIN)) {
            system_status.teensy_status.overall_time = packed_data.overall_time;
            if (size(command_ids) != 0){
                system_status.teensy_status.cmd_ack_ID = command_ids[0];
                command_ids.erase(command_ids.begin());
            }else{
                system_status.teensy_status.cmd_ack_ID = 0; //so we don't keep acknowledging the last received message if we disconnect and the GCS tries to resend it
            }
            serialTransfer.txObj(system_status.teensy_status);
            serialTransfer.sendData(sizeof(system_status.teensy_status), StatusPk); // status packet id = 2
        } else {
            Serial.println("Dropped a system status packet");
        }

        if (packed_data.overall_time - last_status_ms > 5 * config::STATUS_INTERVAL_MS) {
            last_status_ms = packed_data.overall_time; // reset if behind
        }else{
            last_status_ms += config::STATUS_INTERVAL_MS;
        }
        Serial.println(F("--- Teensy Status ---"));
        Serial.print(F("Time (ms):   ")); Serial.println(system_status.teensy_status.overall_time);
        Serial.print(F("AC State:    ")); Serial.println(system_status.teensy_status.ac_state);
        Serial.print(F("BNO State:   ")); Serial.println(system_status.teensy_status.bno_state);
        Serial.print(F("ISM State:   ")); Serial.println(system_status.teensy_status.ism_state);
        Serial.print(F("SD State:    ")); Serial.println(system_status.teensy_status.sd_state);
        Serial.print(F("ESP AC State:  ")); Serial.println(system_status.esp_ac_status.esp_ac_state);
        Serial.print(F("ESP GCS State: ")); Serial.println(system_status.esp_gcs_status.esp_gcs_state);
        Serial.print(F("Laptop State: ")); Serial.println(system_status.laptop_status.laptop_state);
        Serial.print(F("Cmd ID:     ")); Serial.println(system_status.teensy_status.cmd_ack_ID);
        Serial.print(F("Werid cmd rate:  ")); Serial.println(countweirdrate);
        Serial.println(F("---------------------"));
    }

}

void logData(PackedDataStruct &packed_data) {
    // Amount of data in ringBuf.
    size_t n = log_rb.bytesUsed();
    if ((n + file.curPosition()) > (config::FILE_SIZE - 20)) {
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
    if (n > (config::LOG_BUF_CAPACITY * 0.75)) {
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

void setupSD(StatusStruct &system_status){
    // Initialize the SD.
    if (!sd.begin(SdioConfig(FIFO_SDIO))) {
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
    if (!file.preAllocate(config::FILE_SIZE)) {
      Serial.println("preAllocate failed\n");
      file.close();
      return;
    }
    // initialize the RingBuf.
    log_rb.begin(&file);

    Serial.println("SD setup complete");
    system_status.teensy_status.sd_state = 1;
}