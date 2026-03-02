#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include "data_structs.h"
#include "config.h"

// Receiver (onboard ESP) MAC Address
uint8_t broadcastAddress[] = {0x40, 0x4C, 0xCA, 0x3C, 0xFD, 0x5C};

PackedDataStruct rx_packed_data;
CommandStruct tx_command_data;
statusStruct system_status;

esp_now_peer_info_t peerInfo;
esp_err_t result = ESP_OK;

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    //Serial.print("\r\nLast Packet Send Status:\t");
    result = ESP_OK; // clearing buffer by 1 i geuss, so potentially be able to send again
    //Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

int countweirdrate = 0;
int count = 0;
uint32_t previous_time = millis();

// Callback when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
    uint8_t packetID = incomingData[0]; // first byte is packet type
    if (packetID == 0 && len == sizeof(PackedDataStruct)) {
        memcpy(&rx_packed_data, incomingData, len);
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
    } else if (packetID == 1 && len == sizeof(CommandStruct)) { // alright there shouldn't be any reason we get a command, delete later
        memcpy(&tx_command_data, incomingData, len);
    } else if (packetID == 2 && len == teensy_AC_status_size) {
        memcpy(&system_status.teensy_status, incomingData, len);
    } else {
        Serial.print("Received packet with unknown format. Packet ID: ");
        Serial.print(packetID);
        Serial.print(", Length: ");
        Serial.print(len);
        Serial.print(", incomingdata sizse: ");
        Serial.println(sizeof(incomingData));
        return;
    }
}

void setup() {
    // Init Serial Monitor
    Serial.begin(115200);
    
    // Set device as a Wi-Fi Station
    WiFi.mode(WIFI_STA);
    // Init ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }
    esp_wifi_config_espnow_rate(WIFI_IF_STA, ESPNOW_RATE); // lower to increase range in the future
    // Once ESPNow is successfully Init, we will register for Send CB to
    // get the status of Transmitted packet
    esp_now_register_send_cb(OnDataSent);
    
    // Register peer
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = CHANNEL;  
    peerInfo.encrypt = false;
    
    // Add peer        
    if (esp_now_add_peer(&peerInfo) != ESP_OK){
        Serial.println("Failed to add peer");
        return;
    }

    // Register for a callback function that will be called when data is received
    esp_now_register_recv_cb(OnDataRecv);
    Serial.println("GCS ESP32 setup complete");
    system_status.esp_gcs_status.esp_gcs_state = 1;
}

void loop() {
    // Set values to send
    uint32_t current_time = millis();
    if (current_time - last_cmd_ms >= COMMAND_RATE) {
        if (result == ESP_OK) {
            result = esp_now_send(broadcastAddress, (uint8_t *) &tx_command_data, sizeof(tx_command_data));
        } else {
            Serial.println("ESP32 GCS not ready for ESP_NOW Command Send");
        }

        if (current_time - last_cmd_ms > 5 * COMMAND_RATE) {
            last_cmd_ms = current_time; // reset if behind
        } else {
            last_cmd_ms += COMMAND_RATE;
        }
    }
    //Serial.println("current_time: " + String(current_time) + ", last_status_ms: " + String(last_status_ms));
    if (current_time - last_status_ms >= STATUS_RATE) {
        if (result == ESP_OK) {
            result = esp_now_send(broadcastAddress, (uint8_t *) &system_status.esp_gcs_status, sizeof(system_status.esp_gcs_status));
        } else {
            Serial.println("ESP32 GCS not ready for ESP_NOW Status Send");
        }

        if (current_time - last_status_ms > 5 * STATUS_RATE) {
            last_status_ms = current_time; // reset if behind
        } else {
            last_status_ms += STATUS_RATE;
        }
        Serial.println(F("GCS--- Teensy Status ---"));
        Serial.print(F("Time (ms):   ")); Serial.println(system_status.teensy_status.overall_time);
        Serial.print(F("AC State:    ")); Serial.println(system_status.teensy_status.ac_state);
        Serial.print(F("BNO State:   ")); Serial.println(system_status.teensy_status.bno_state);
        Serial.print(F("ISM State:   ")); Serial.println(system_status.teensy_status.ism_state);
        Serial.print(F("SD State:    ")); Serial.println(system_status.teensy_status.sd_state);
        Serial.print(F("ESP AC State:  ")); Serial.println(system_status.esp_ac_status.esp_ac_state);
        Serial.print(F("ESP GCS State: ")); Serial.println(system_status.esp_gcs_status.esp_gcs_state);
        Serial.print(F("Werid tele rate:  ")); Serial.println(countweirdrate);
    }

}