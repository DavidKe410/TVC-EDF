#include <Arduino.h>
#include "WiFi.h"
#include <esp_now.h>
#include <HardwareSerial.h>
#include "data_structs.h"
#include "config.h"

HardwareSerial HWSerial1(0);
PackedDataStruct rx_packed_data;
CommandStruct tx_command_data;
statusStruct system_status;

esp_now_peer_info_t peerInfo;

// Receiver (GCS) MAC Address
uint8_t broadcastAddress[] = {0x40, 0x4C, 0xCA, 0x3C, 0xF4, 0x6C};

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    //IDK if we do anything on a send :/, have the 
}

// Callback when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
    memcpy(&tx_command_data, incomingData, len);
    Serial.print("Bytes received from GCS: ");
    Serial.println(len);
}


void setup() {

    Serial.begin(115200);
    //while (!Serial) delay(10); // will pause until serial console opens

    // Set device as a Wi-Fi Station
    WiFi.mode(WIFI_STA);
    // Init ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }

    HWSerial1.setRxBufferSize(2048); // just for more head room
    HWSerial1.begin(921600, SERIAL_8N1, -1, -1); // RX, TX pins
    serialTransfer.begin(HWSerial1, true, Serial, 5); // Since telemetry hz is 100, 10 ms for each packet, travel time only a.5-1.5ms, might as well go to new packet

    // Once ESPNow is successfully Init, we will register for Send CB to get the status of Transmitted packet
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

    delay(500);
    Serial.println("ESP setup complete");
    system_status.esp_ac_state = 1;
}

int count = 0;

void loop() {
    unsigned long startMillis = millis();
    while (count < 400) {
        if (serialTransfer.available()) {
            uint8_t packetID = serialTransfer.currentPacketID();
            esp_err_t result;
            switch (packetID){
                case 0:
                    serialTransfer.rxObj(rx_packed_data);
                    result = esp_now_send(broadcastAddress, (uint8_t *) &rx_packed_data, sizeof(rx_packed_data));
                    break;
                case 1:
                    serialTransfer.rxObj(system_status);
                    result = esp_now_send(broadcastAddress, (uint8_t *) &system_status, sizeof(system_status));
                    break;
            }

            // Serial.print("\r\nLast Packet Send Status:\t");
            // Serial.println(result == ESP_OK ? "Success" : "Fail");
            count++;
            Serial.println(count);
        }
        if (system_status.overall_time - last_status_ms >= STATUS_RATE) {
            if ((size_t)HWSerial1.availableForWrite() >= (sizeof(system_status) + 20)) {
                serialTransfer.txObj(system_status);
                serialTransfer.sendData(sizeof(system_status), 1);
            } else {
                Serial.println("Dropped a system status packet");
            }

            if (system_status.overall_time - last_status_ms > 5 * STATUS_RATE) {
                last_status_ms = system_status.overall_time; // reset if behind
            }else{
                last_status_ms += STATUS_RATE;
            }
        }
    }
    unsigned long endMillis = millis();
    Serial.println("time taken: " + String(1000/((endMillis - startMillis)/400.0), 4));
    count = 0;
    Serial.println("buffering");
    while(millis() - endMillis < 5000) {
        if (serialTransfer.available()) {
            serialTransfer.rxObj(rx_packed_data);
        }
    }
    Serial.println("done buffering");
    //Serial.println(tx_command_data.overall_time);
}