#include <Arduino.h>
#include <HardwareSerial.h>

HardwareSerial MySerial0(0);


String receivedMessage;
void setup() {

    Serial.begin(115200);
    while (!Serial) delay(10); // will pause until serial console opens


    MySerial0.begin(115200, SERIAL_8N1, -1, -1); // RX, TX pins

    Serial.println("dones esp setup");
    delay(500);
}

void loop() {
    Serial.println("hey im esp323");
    while (MySerial0.available() > 0) {
        char receivedChar = MySerial0.read();
        if (receivedChar == '\n') {
            Serial.println(receivedMessage);  // Print the received message in the Serial monitor
            receivedMessage = "";  // Reset the received message
        } else {
            receivedMessage += receivedChar;  // Append characters to the received message
        }
    }
    delay(1000);
}