#include <Arduino.h>
#include "WiFi.h"
#include <esp_now.h>
#include <HardwareSerial.h>
#include "data_structs.h"
#include "config.h"

HardwareSerial HWSerial1(0);
PackedDataStruct g_packed_data;
CommandStruct g_command;
StatusStruct g_system_status;

esp_now_peer_info_t peerInfo;
esp_err_t result = ESP_OK;

// Receiver (GCS) MAC Address
uint8_t broadcastAddress[] = {0x40, 0x4C, 0xCA, 0x3C, 0xF4, 0x6C};

uint32_t last_GCS_status = 0;

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    // this is prob overengineered :/ with all the result checks for esp_now_send, but if we send seomthing, clearing space in buffer, its prob good to send again
    result = ESP_OK;
}

// Callback when data is received, this is specifically from GCS as its ESP-NOW
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
    uint8_t packetID = incomingData[0]; // first byte is packet type
    if (packetID == CommandPk && len == sizeof(CommandStruct)) {
        memcpy(&g_command, incomingData, len); // Potentially a way of directly using incomingData w/o memcpy, but this works for now
        if (g_system_status.teensy_status.ac_state > -1) {
            if ((size_t)HWSerial1.availableForWrite() >= (len + availWriteMargin)) {
                serialTransfer.txObj(g_command);
                serialTransfer.sendData(len, CommandPk);
            } else {
                Serial.println("Dropped a cmd packet, AC to Teensy");
            }
        }
    }else if (packetID == StatusPk && len == (sizeof(espGCSStatus) + sizeof(laptopStatus))) {
        memcpy(&g_system_status.esp_gcs_status, incomingData, len);
        last_GCS_status = millis();
    } else {
        Serial.print("Received packet with unknown format. Packet ID: ");
        Serial.print(packetID);
        Serial.print(", Length: ");
        Serial.println(len);
        return;
    }
}


void setup() {

    Serial.begin(115200);
    // while (!Serial) delay(10); // will pause until serial console opens

    // Set device as a Wi-Fi Station
    WiFi.mode(WIFI_STA);
    // Init ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }
    esp_wifi_config_espnow_rate(WIFI_IF_STA, ESPNOW_RATE); // lower to increase range in the future

    HWSerial1.setRxBufferSize(1024); // just for more head room
    HWSerial1.begin(serialT_Baud, SERIAL_8N1, -1, -1); // RX, TX pins
    serialTransfer.begin(HWSerial1, true, Serial, serialT_timeout); // Since telemetry hz is 100, 10 ms for each packet, travel time only a.5-1.5ms, might as well go to new packet
    
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

    delay(50);
    serialTransfer.reset(); // just to clear out any garbage data that might be in the buffer from before setup
    Serial.println("ESP setup complete");
    g_system_status.esp_ac_status.esp_ac_state = 1;
}

int countweirdrate = 0;
int count = 0;
uint32_t previous_time = millis();

void loop() {
    if (serialTransfer.available()) {
        switch (serialTransfer.currentPacketID()){
            case TelemetryPk:
                serialTransfer.rxObj(g_packed_data);
                if (result == ESP_OK) {
                    result = esp_now_send(broadcastAddress, (uint8_t *) &g_packed_data, sizeof(g_packed_data));
                }
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
                serialTransfer.rxObj(g_system_status.teensy_status);
                last_status_rx = millis();
                break;
        }
        // Serial.print("\r\nLast Packet Send Status:\t");
        // Serial.println(result == ESP_OK ? "Success" : "Fail");
        //count++;
    }
    
    if ((millis()-last_status_rx) >= heartbeat_timeout) {
        g_system_status.teensy_status.ac_state = -2; // mark as disconnected
    }

    if ((millis()-last_GCS_status) >= heartbeat_timeout) {
        g_system_status.esp_gcs_status.esp_gcs_state = -2; // mark as disconnected
        g_system_status.laptop_status.laptop_state = -2;
    }

    uint32_t current_status_time = millis();
    if (current_status_time - last_status_ms >= STATUS_RATE) { // Sending GCS and AC status over to Teensy through HWSerial + SerialTransfer
        if ((size_t)HWSerial1.availableForWrite() >= (AC_GCS_status_size + availWriteMargin)) {
            serialTransfer.txObj(g_system_status.esp_ac_status);
            serialTransfer.txObj(g_system_status.esp_gcs_status, sizeof(g_system_status.esp_ac_status)); // place gcs status right after ac status in the buffer, thats the index not the size of msg
            serialTransfer.txObj(g_system_status.laptop_status, sizeof(g_system_status.esp_ac_status) + sizeof(g_system_status.esp_gcs_status));
            serialTransfer.sendData(AC_GCS_status_size, StatusPk);
        } else {
            Serial.println("Dropped a system status packet, AC to Teensy");
        }
        if (result == ESP_OK) {
            esp_now_send(broadcastAddress, (uint8_t *) &g_system_status, teensy_AC_status_size); // Just need to send Teensy and AC since GCS has its own truth
        }
        if (current_status_time - last_status_ms > 5 * STATUS_RATE) {
            last_status_ms = current_status_time; // reset if behind
        }else{
            last_status_ms += STATUS_RATE;
        }
        Serial.println(F("AC--- Teensy Status ---"));
        Serial.print(F("Time (ms):   ")); Serial.println(g_system_status.teensy_status.overall_time);
        Serial.print(F("AC State:    ")); Serial.println(g_system_status.teensy_status.ac_state);
        Serial.print(F("BNO State:   ")); Serial.println(g_system_status.teensy_status.bno_state);
        Serial.print(F("ISM State:   ")); Serial.println(g_system_status.teensy_status.ism_state);
        Serial.print(F("SD State:    ")); Serial.println(g_system_status.teensy_status.sd_state);
        Serial.print(F("ESP AC State:  ")); Serial.println(g_system_status.esp_ac_status.esp_ac_state);
        Serial.print(F("ESP GCS State: ")); Serial.println(g_system_status.esp_gcs_status.esp_gcs_state);
        Serial.print(F("Laptop State: ")); Serial.println(g_system_status.laptop_status.laptop_state);
        Serial.print(F("Werid tele rate:  ")); Serial.println(countweirdrate);
        Serial.println(F("---------------------"));
    }
}