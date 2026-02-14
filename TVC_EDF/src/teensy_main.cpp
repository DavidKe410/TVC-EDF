#include <Arduino.h>
#include <Wire.h>
#include "teensy/config.h"
#include "teensy/data_structs.h"
#include "teensy/data_hub.h"
#include "teensy/sensors.h"

//============================================ Configuration ============================================
//========================================== End Configuration ==========================================

// Function Declarations (Prototypes)

void setup() {

    Serial.begin(115200);
    //while (!Serial) delay(10); // will pause until serial console opens
    delay(500);

    Serial7.begin(115200); // Should be able to increase as needed
    Serial7.addMemoryForWrite(tx_buffer, sizeof(tx_buffer));
    //serialTransfer.begin(Serial7);

    setupSD();

    setupBNO085(); // This starts a wire that the ISM can use...AND overwrites the setClock speed to soemthing slower????wtf is wrong with this sensor

    Wire.setClock(400000);

    setupISM330();

    delay(3000);
}

void loop() {
    unsigned long startMillis = millis();
    while (loopCount < 400) {
        all_data.overall_time = millis();
        //delayMicroseconds(160);
        readBNO085(all_data.imu);
        readISM330(all_data.imu); //ism after bno085 for more consistent sampling where both are valid in one cycle
        packData(all_data, packed_data);
        logData(packed_data);
        //sendData(packed_data); // alr rate limited
        Serial.println("hey im teensy");
        Serial7.println("Hello there");

        // while ((millis() - startMillis) < (loopCount * CYCLE_TIME_MS)) {
        //     // wait until next cycle
        //     delayMicroseconds(100);
        // }
        delay(2000);
    }
    unsigned long endMillis = millis();
    Serial.println("time taken: " + String(1000/((endMillis - startMillis)/400.0), 4));
    cleanupSD(); // Cleanup SD card
    loopCount = 0;
    delay(3000);
    setupSD(); // Setup SD card again for next logging session
}

//============================================ Functions ============================================
//========================================== End Functions ==========================================