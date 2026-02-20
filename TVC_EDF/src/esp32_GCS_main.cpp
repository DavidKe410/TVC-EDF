#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include "data_structs.h"

// Receiver (onboard ESP) MAC Address
uint8_t broadcastAddress[] = {0x40, 0x4C, 0xCA, 0x3C, 0xFD, 0x5C};

PackedDataStruct rx_packed_data;
CommandStruct tx_command_data;
statusStruct system_status;

esp_now_peer_info_t peerInfo;

#define CHANNEL 0
// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    //Serial.print("\r\nLast Packet Send Status:\t");
    //Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

// Callback when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
    memcpy(&rx_packed_data, incomingData, sizeof(rx_packed_data)); // len == sizeof(rx_packeted_data)
    //Serial.print("Bytes received: ");
    //Serial.println(len);
    Serial.println(rx_packed_data.accel_z);
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
    tx_command_data.overall_time = millis();
    tx_command_data.state = 1;
    tx_command_data.servo1 = random(1000,2000);
    tx_command_data.servo2 = random(1000,2000);
    tx_command_data.servo3 = random(1000,2000);
    tx_command_data.servo4 = random(1000,2000);
    tx_command_data.motor = random(1000,2000);

    // Send message via ESP-NOW
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &tx_command_data, sizeof(tx_command_data));

    if (result == ESP_OK) {
        Serial.println("Sent with success");
    }
    else {
        Serial.println("Error sending the data");
    }
    delay(500);
}