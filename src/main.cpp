#include <Arduino.h>
#include "wifi_ota.h"
#include "leds.h"
#include "pir.h"
#include "mywebserver.h"

unsigned long lastMotionTime = 0;
unsigned long TIMEOUT = 10000;

void setTimeoutValue(unsigned long val) {
  TIMEOUT = val;
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nNachtlicht startet...");

  setupWiFi();
  setupOTA();
  setupPIR();
  setupLEDs();
  setupWebserver();

  Serial.println("Setup fertig!");
}

void loop() {
  handleOTA();
  handleWebserver();

  if (isMotionDetected()) {
    lastMotionTime = millis();

    if (!isLightOn()) {
      startFadeIn();
    }
  }

  updateFade();

  if (isLightOn() && (millis() - lastMotionTime > TIMEOUT)) {
    startFadeOut();
  }

  delay(50);
}
