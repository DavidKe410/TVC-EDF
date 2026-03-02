#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "data_structs.h"
#include "data_hub.h"
#include "sensors.h"

void setup() {

    Serial.begin(115200);
    //while (!Serial) delay(10); // will pause until serial console opens
    delay(300);

    Serial7.addMemoryForRead(rx_buffer, sizeof(rx_buffer)); // just for more head room
    Serial7.addMemoryForWrite(tx_buffer, sizeof(tx_buffer));
    Serial7.begin(serialT_Baud); // Should be able to increase as needed
    serialTransfer.begin(Serial7, true, Serial, serialT_timeout); //default 50, may decrease if rx commands at higher rate

    //setupSD(system_status);

    setupBNO085(g_system_status); // This starts a wire that the ISM can use...AND overwrites the setClock speed to soemthing slower????wtf is wrong with this sensor

    Wire.setClock(400000);

    setupISM330(g_system_status);

    delay(300);
    g_system_status.teensy_status.ac_state = 1;
    serialTransfer.reset(); // just to clear out any garbage data that might be in the buffer from before setup
}

void loop() {
    unsigned long startMillis = millis();
    while (loopCount < 400) {
        g_all_data.overall_time = millis();
        readBNO085(g_all_data.imu);
        readISM330(g_all_data.imu); //ism after bno085 for more consistent sampling where both are valid in one cycle
        packData(g_all_data, g_packed_data);
        //logData(packed_data);
        sendData(g_packed_data, g_system_status); // alr rate limited, based on all_data.overall_time - though may just want to just use millis()
        receiveData(g_rx_command, g_system_status);
        g_system_status.teensy_status.ac_state = 2;
        // while ((millis() - startMillis) < (loopCount * CYCLE_TIME_MS)) {
        //     // wait until next cycle
        //     delayMicroseconds(100);
        // }
        //delay(100);
    }
    unsigned long endMillis = millis();
    Serial.println("time taken: " + String(1000/((endMillis - startMillis)/400.0), 4));
    //cleanupSD(); // Cleanup SD card
    loopCount = 0;
    //delay(5000);
    //setupSD(); // Setup SD card again for next logging session
}

//============================================ Functions ============================================
//========================================== End Functions ==========================================