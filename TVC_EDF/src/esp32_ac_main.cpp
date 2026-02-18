#include <Arduino.h>
#include "WiFi.h"
#include <esp_now.h>
#include <HardwareSerial.h>
#include "SerialTransfer.h"
#include "data_structs.h"

// HardwareSerial HWSerial1(0);
// SerialTransfer serialTransfer;
PackedDataStruct rx_packed_data;
CommandStruct tx_command_data;

esp_now_peer_info_t peerInfo;

// Receiver (GCS) MAC Address
uint8_t broadcastAddress[] = {0x40, 0x4C, 0xCA, 0x3C, 0xF4, 0x6C};

#define CHANNEL 0
// void setup() {

//     Serial.begin(115200);
//     while (!Serial) delay(10); // will pause until serial console opens

//     HWSerial1.setRxBufferSize(2048); // just for more head room
//     HWSerial1.begin(921600, SERIAL_8N1, -1, -1); // RX, TX pins

//     serialTransfer.begin(HWSerial1, false, Serial, 50);

//     WiFi.mode(WIFI_MODE_STA);
//     Serial.println(WiFi.macAddress());

//     delay(500);
//     Serial.println("Done ESP setup");
// }
// int count = 0;
// void loop() {
//     unsigned long startMillis = millis();
//     while (count < 400) {
//         if (serialTransfer.available()) {
//             //uint8_t packetID = serialTransfer.currentPacketID();
//             uint16_t messageLen = serialTransfer.rxObj(rx_packed_data);
//             //Serial.println("Received packet with length: " + String(messageLen));
//             // Process the received data in rx_packed_data as needed
//             count++;
//             //Serial.println(count);
//         }
//     }
//     unsigned long endMillis = millis();
//     Serial.println("time taken: " + String(1000/((endMillis - startMillis)/400.0), 4));
//     count = 0;
//     Serial.println("buffering");
//     while(millis() - endMillis < 5000) {
//         if (serialTransfer.available()) {
//             serialTransfer.rxObj(rx_packed_data);
//         }
//     }
//     Serial.println("done buffering");
// }

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    Serial.print("\r\nLast Packet Send Status:\t");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

// Callback when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
    memcpy(&tx_command_data, incomingData, sizeof(tx_command_data));
    Serial.print("Bytes received from GCS: ");
    Serial.println(len);
}
 
void setup() {
    // Initialize Serial Monitor
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
}
 
void loop() {
// Send message via ESP-NOW
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &rx_packed_data, sizeof(rx_packed_data));

    if (result == ESP_OK) {
        Serial.println("Sent with success");
    }
    else {
        Serial.println("Error sending the data");
    }

    Serial.println(tx_command_data.overall_time);
    delay(500);
}