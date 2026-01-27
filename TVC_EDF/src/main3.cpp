// #include <Arduino.h>
// #include <Adafruit_BNO08x.h>

// // //============================================ Configuration ============================================

// // //====== BNO085 ======
// // #define BNO08X_CS 10
// // #define BNO08X_INT 9
// // #define BNO08X_RESET 6
// // Adafruit_BNO08x  bno08x(BNO08X_RESET);
// // sh2_SensorValue_t sensorValue;
// // long reportIntervalUs = 1000;
// // //====== End BNO085 ======

// // void setReports(void) {
// //   Serial.println("Setting desired reports");
// //   if (! bno08x.enableReport(SH2_ROTATION_VECTOR, reportIntervalUs)) {
// //     Serial.println("Could not enable game vector");
// //   }
// // }

// // void setup(void) {
// //   Serial.begin(115200);
// //   while (!Serial) delay(10);     // will pause Zero, Leonardo, etc until serial console opens

// //   Serial.println("Adafruit BNO08x test!");

// //   if (!bno08x.begin_SPI(BNO08X_CS, BNO08X_INT)) {
// //     Serial.println("Failed to find BNO08x chip");
// //     while (1) { delay(10); }
// //   }
// //   Serial.println("BNO08x Found!");

// //   setReports();

// //   Serial.println("Reading events");
// //   delay(100);
// // }

// // bool started = false;
// // int count = 0;
// // unsigned long startMillis = 0;

// // void loop() {
// //   if (bno08x.wasReset()) {
// //     Serial.print("sensor was reset ");
// //     setReports();
// //   }

// //   unsigned long t0 = micros();
// //   for (int i = 0; i < 500; i++) {
// //     if(!bno08x.getSensorEvent(&sensorValue)){
// //       Serial.println("No data available");
// //     };  // just read, no Serial
// //   }
// //   unsigned long t1 = micros();
// //   float rate = 500.0 / ((t1 - t0) / 1000000.0);
// //   Serial.println("Measured rate (Hz): " + String(rate));

// //   // if (!started) {
// //   //   Serial.println("Starting timing test...");
// //   //   started = true;
// //   //   startMillis = millis();
// //   // }

// //   // if (count < 500) {
// //   //   bno08x.getSensorEvent(&sensorValue);
// //   //   count++;
// //   // } else {
// //   //   count = 0;
// //   //   unsigned long endMillis = millis();
// //   //   Serial.println("time taken: " + String(1000/((endMillis - startMillis)/500.0), 4));
// //   //   startMillis = millis();
// //   // }
// // }

// #define BNO08X_RESET -1

// Adafruit_BNO08x  bno08x(BNO08X_RESET);
// sh2_SensorValue_t sensorValue;


// // Here is where you define the sensor outputs you want to receive
// void setReports(void) {
//   Serial.println("Setting desired reports");
//   if (! bno08x.enableReport(SH2_ARVR_STABILIZED_RV, 1000)) {
//     Serial.println("Could not enable rotation vector");
//   }
// }


// void setup(void) {
//   Serial.begin(115200);
//   while (!Serial) delay(10);     // will pause Zero, Leonardo, etc until serial console opens

//   Serial.println("Adafruit BNO08x test!");
//   Wire.begin();
//   Wire.setClock(400000); 
//   // Try to initialize!
//   if (!bno08x.begin_I2C()) {
//     Serial.println("Failed to find BNO08x chip");
//     while (1) { delay(10); }
//   }

//   Serial.println("BNO08x Found!");

//   for (int n = 0; n < bno08x.prodIds.numEntries; n++) {
//     Serial.print("Part ");
//     Serial.print(bno08x.prodIds.entry[n].swPartNumber);
//     Serial.print(": Version :");
//     Serial.print(bno08x.prodIds.entry[n].swVersionMajor);
//     Serial.print(".");
//     Serial.print(bno08x.prodIds.entry[n].swVersionMinor);
//     Serial.print(".");
//     Serial.print(bno08x.prodIds.entry[n].swVersionPatch);
//     Serial.print(" Build ");
//     Serial.println(bno08x.prodIds.entry[n].swBuildNumber);
//   }

//   setReports();

//   Serial.println("Reading events");
//   delay(100);
// }




// void loop() {
//   delay(10);

//   if (bno08x.wasReset()) {
//     Serial.print("sensor was reset ");
//     setReports();
//   }
//   int count = 0;
//   unsigned long t0 = micros();
//   while (count<500){
//     if(!bno08x.getSensorEvent(&sensorValue)){
//       //Serial.println("No data available");
//     }else{
//       //Serial.println("Got data");
//       count++;
//     }
//   }
//   switch (sensorValue.sensorId) {
//     case SH2_LINEAR_ACCELERATION:
//         Serial.println(sensorValue.un.linearAcceleration.x);
//         Serial.println(sensorValue.un.linearAcceleration.y);
//         Serial.println(sensorValue.un.linearAcceleration.z);
//         //Serial.println("Accel data read");
//         break;
//   }
//   unsigned long t1 = micros();
//   float rate = 500.0 / ((t1 - t0) / 1000000.0);
//   Serial.println("Measured rate (Hz): " + String(rate));
// }