#include "pir.h"
#include <Arduino.h>

#define PIR_PIN_1 13
#define PIR_PIN_2 14

void setupPIR() {
  pinMode(PIR_PIN_1, INPUT);
  pinMode(PIR_PIN_2, INPUT);
  Serial.println("PIR Sensoren initialisiert!");
}

bool isMotionDetected() {
  int motion1 = digitalRead(PIR_PIN_1);
  int motion2 = digitalRead(PIR_PIN_2);
  
  if (motion1 == HIGH || motion2 == HIGH) {
    Serial.println("Bewegung erkannt! (Sensor " + 
                   String(motion1 == HIGH ? 1 : 2) + ")");
    return true;
  }
  return false;
}