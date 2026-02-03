#include <Arduino.h>
#include "wifi_ota.h"
#include "leds.h"
#include "pir.h"
#include "mqtt_client.h"
#include "log.h"
#include <WiFi.h>
#include <esp_system.h>

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

  const char* resetReason = "unknown";
  esp_reset_reason_t reason = esp_reset_reason();
  switch (reason) {
    case ESP_RST_POWERON: resetReason = "power_on"; break;
    case ESP_RST_SW: resetReason = "software_reset"; break;
    case ESP_RST_PANIC: resetReason = "panic"; break;
    case ESP_RST_INT_WDT: resetReason = "interrupt_wdt"; break;
    case ESP_RST_TASK_WDT: resetReason = "task_wdt"; break;
    case ESP_RST_WDT: resetReason = "other_wdt"; break;
    case ESP_RST_BROWNOUT: resetReason = "brownout"; break;
    case ESP_RST_DEEPSLEEP: resetReason = "deep_sleep"; break;
    case ESP_RST_SDIO: resetReason = "sdio"; break;
    default: resetReason = "unknown"; break;
  }
  logEvent("reset_reason", isLightOn(), getCurrentBrightness(), getMotionState(), resetReason);

  LOG_PRINTLN("Setup fertig!");
  logEvent("boot", isLightOn(), getCurrentBrightness(), getMotionState(), "setup_complete");
}

void loop() {
  handleOTA();
  handleMQTT();
  handleLog();

  static bool lastWiFiConnected = false;
  bool wifiConnected = (WiFi.status() == WL_CONNECTED);
  if (wifiConnected != lastWiFiConnected) {
    if (wifiConnected) {
      logEvent("wifi_reconnect", isLightOn(), getCurrentBrightness(), getMotionState(),
               WiFi.localIP().toString());
    } else {
      logEvent("wifi_disconnect", isLightOn(), getCurrentBrightness(), getMotionState(), nullptr);
    }
    lastWiFiConnected = wifiConnected;
  }

  static bool lastMotionState = false;
  bool motionDetected = isMotionDetected();

  if (motionDetected != lastMotionState) {
    logEvent(motionDetected ? "motion_on" : "motion_off",
             isLightOn(),
             getCurrentBrightness(),
             motionDetected,
             nullptr);
    lastMotionState = motionDetected;
  }

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
