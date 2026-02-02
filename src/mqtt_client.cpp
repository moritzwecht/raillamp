#include "mqtt_client.h"
#include "leds.h"
#include "pir.h"
#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#ifndef MQTT_HOST
#define MQTT_HOST "localhost"
#endif

#ifndef MQTT_PORT
#define MQTT_PORT 8883
#endif

#ifndef MQTT_USER
#define MQTT_USER ""
#endif

#ifndef MQTT_PASS
#define MQTT_PASS ""
#endif

#ifndef MQTT_STATUS_TOPIC
#define MQTT_STATUS_TOPIC "raillamp/status"
#endif

static WiFiClientSecure secureClient;
static PubSubClient mqtt(secureClient);

static unsigned long lastConnectAttemptMs = 0;
static unsigned long lastPublishMs = 0;

static bool lastLightsOn = false;
static int lastBrightness = -1;
static bool lastMotion = false;

static void addClientConfig() {
  secureClient.setInsecure();
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setKeepAlive(30);
  mqtt.setSocketTimeout(5);
}

static bool connectMQTT() {
  String clientId = "raillamp-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  if (strlen(MQTT_USER) > 0) {
    return mqtt.connect(clientId.c_str(), MQTT_USER, MQTT_PASS);
  }
  return mqtt.connect(clientId.c_str());
}

void setupMQTT() {
  addClientConfig();
}

void handleMQTT() {
  if (mqtt.connected()) {
    mqtt.loop();
    return;
  }

  unsigned long now = millis();
  if (now - lastConnectAttemptMs < 5000) return;
  lastConnectAttemptMs = now;

  if (connectMQTT()) {
    publishStatus(true, isLightOn(), getCurrentBrightness(), getMotionState());
  }
}

void publishStatus(bool force, bool lightsOn, int brightness, bool motion) {
  if (!mqtt.connected()) return;

  unsigned long now = millis();
  bool changed = (lightsOn != lastLightsOn) || (brightness != lastBrightness) || (motion != lastMotion);
  if (!force && !changed && (now - lastPublishMs < 5000)) return;

  String json = "{";
  json += "\"lightsOn\":" + String(lightsOn ? "true" : "false") + ",";
  json += "\"brightness\":" + String(brightness) + ",";
  json += "\"motion\":" + String(motion ? "true" : "false");
  json += "}";

  mqtt.publish(MQTT_STATUS_TOPIC, json.c_str(), true);

  lastLightsOn = lightsOn;
  lastBrightness = brightness;
  lastMotion = motion;
  lastPublishMs = now;
}
