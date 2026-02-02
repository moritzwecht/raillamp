#include <Arduino.h>
#include "wifi_ota.h"
#include "leds.h"
#include "pir.h"
#include "mqtt_client.h"
#include "log.h"

// Motion timeout (ms)
unsigned long lastMotionTime = 0;
static const unsigned long kMotionTimeoutMs = 30000;

void setup() {
  Serial.begin(115200);
  LOG_PRINTLN("\nNachtlicht startet...");

  setupWiFi();
  setupLog();
  setupOTA();
  setupMQTT();
  setupPIR();
  setupLEDs();

  LOG_PRINTLN("Setup fertig!");
}

void loop() {
  handleOTA();
  handleMQTT();
  handleLog();

  static bool lastMotionState = false;
  bool motionDetected = isMotionDetected();

  lastMotionState = motionDetected;

  if (motionDetected) {
    lastMotionTime = millis();

    if (!isLightOn()) {
      startFadeIn();
    }
  }

  updateFade();

  if (isLightOn() && (millis() - lastMotionTime > kMotionTimeoutMs)) {
    startFadeOut();
  }

  publishStatus(false, isLightOn(), getCurrentBrightness(), motionDetected);

  delay(50);
}
