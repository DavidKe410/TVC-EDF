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

uint32_t last_cmd_ms = millis();
uint8_t COMMAND_RATE = 10;

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    //Serial.print("\r\nLast Packet Send Status:\t");
    //Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

// Callback when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
    uint8_t packetID = incomingData[0]; // first byte is packet type
    if (packetID == 0 && len == sizeof(PackedDataStruct)) {
        memcpy(&rx_packed_data, incomingData, len);
    } 
    else if (packetID == 1 && len == sizeof(CommandStruct)) {
        memcpy(&tx_command_data, incomingData, len);
    }else if (packetID == 2 && len == sizeof(statusStruct)) {
        memcpy(&system_status, incomingData, len);
    } else {
        Serial.print("Received packet with unknown format. Packet ID: ");
        Serial.print(packetID);
        Serial.print(", Length: ");
        Serial.println(len);
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
    system_status.esp_gcs_state = 1;
}

void loop() {
    // Set values to send
    tx_command_data = {1, millis(), 1, random(1000,2000), random(1000,2000), random(1000,2000), random(1000,2000)};
    uint32_t current_time = millis();
    if (current_time - last_cmd_ms >= COMMAND_RATE) {
        if (result == ESP_OK) {
            result = esp_now_send(broadcastAddress, (uint8_t *) &tx_command_data, sizeof(tx_command_data));
        } else {
            Serial.println("ESP32 GCS not ready for ESP_NOW Command Send");
        }

        if (current_time - last_cmd_ms > 5 * COMMAND_RATE) {
            last_cmd_ms = current_time; // reset if behind
        }else{
            last_cmd_ms += COMMAND_RATE;
        }
    }

    if (current_time - last_status_ms >= STATUS_RATE) {
        if (result == ESP_OK) {
            result = esp_now_send(broadcastAddress, (uint8_t *) &system_status, sizeof(system_status));
        } else {
            Serial.println("ESP32 GCS not ready for ESP_NOW Status Send");
        }

        if (current_time - last_status_ms > 5 * STATUS_RATE) {
            last_status_ms = current_time; // reset if behind
        }else{
            last_status_ms += STATUS_RATE;
        }
    }
}