#include "pir.h"
#include <Arduino.h>
#include "log.h"

#define PIR_PIN_1 13
#define PIR_PIN_2 14

static bool readMotionRaw() {
  int motion1 = digitalRead(PIR_PIN_1);
  int motion2 = digitalRead(PIR_PIN_2);
  return (motion1 == HIGH || motion2 == HIGH);
}

void setupPIR() {
  pinMode(PIR_PIN_1, INPUT);
  pinMode(PIR_PIN_2, INPUT);
  LOG_PRINTLN("PIR Sensoren initialisiert!");
}

bool isMotionDetected() {
  bool motion = readMotionRaw();
  if (motion) {
    int motion1 = digitalRead(PIR_PIN_1);
    LOG_PRINTLN("Bewegung erkannt! (Sensor " + String(motion1 == HIGH ? 1 : 2) + ")");
  }
  return motion;
}

bool getMotionState() {
  return readMotionRaw();
}
