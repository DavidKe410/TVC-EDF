#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include "data_structs.h"
#include "config.h"

// Receiver (onboard ESP) MAC Address
uint8_t broadcastAddress[] = {0x40, 0x4C, 0xCA, 0x3C, 0xFD, 0x5C};

PackedDataStruct g_packed_data;
CommandStruct g_command;
StatusStruct g_system_status;

esp_now_peer_info_t peerInfo;
esp_err_t result = ESP_OK;

uint32_t last_laptop_status = 0;

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    result = ESP_OK; // clearing buffer by 1 i geuss, so potentially be able to send again
}

// Callback when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
    uint8_t packetID = incomingData[0]; // first byte is packet type
    if (packetID == TelemetryPk && len == sizeof(PackedDataStruct)) {
        memcpy(&g_packed_data, incomingData, len);
        if ((size_t)Serial.availableForWrite() >= (sizeof(PackedDataStruct) + availWriteMargin)) {
            serialTransfer.txObj(g_packed_data);
            serialTransfer.sendData(sizeof(PackedDataStruct), TelemetryPk);
        } else {
            Serial.println("Dropped a telemetry packet, GCS to Laptop");
        }
    } else if (packetID == StatusPk && len == teensy_AC_status_size) {
        memcpy(&g_system_status.teensy_status, incomingData, len);
        last_status_rx = millis();
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
    Serial.begin(serialT_Baud);
    
    serialTransfer.begin(Serial, true, Serial, serialT_timeout);

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
    g_system_status.esp_gcs_status.esp_gcs_state = 1;
}

void loop() {
    if (serialTransfer.available()) { // getting held up by stale packet here
        switch (serialTransfer.currentPacketID()){
            case CommandPk:
                serialTransfer.rxObj(g_command);
                if (result == ESP_OK) {
                    result = esp_now_send(broadcastAddress, (uint8_t *) &g_command, sizeof(CommandStruct));
                }
                break;
            case StatusPk:
                serialTransfer.rxObj(g_system_status.laptop_status);
                last_laptop_status = millis();
                break;
        }
        // Serial.print("\r\nLast Packet Send Status:\t");
        // Serial.println(result == ESP_OK ? "Success" : "Fail");
        //count++;
    }

    if ((millis()-last_status_rx) >= heartbeat_timeout) {
        g_system_status.teensy_status.ac_state = -2; // mark as disconnected
        g_system_status.esp_ac_status.esp_ac_state = -2; // mark as disconnected
    }

    if ((millis()-last_laptop_status) >= heartbeat_timeout) {
        g_system_status.laptop_status.laptop_state = -2;
    }

    // Set values to send
    uint32_t current_time = millis();
    
    if (current_time - last_status_ms >= STATUS_RATE) {
        if (result == ESP_OK) {
            result = esp_now_send(broadcastAddress, (uint8_t *) &g_system_status.esp_gcs_status, (sizeof(espGCSStatus)+sizeof(laptopStatus)));
        }

        if (current_time - last_status_ms > 5 * STATUS_RATE) {
            last_status_ms = current_time; // reset if behind
        } else {
            last_status_ms += STATUS_RATE;
        }
    }
}