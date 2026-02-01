#include "wifi_ota.h"
#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

// WiFi Zugangsdaten
const char* ssid = "ArmbrustWG";
const char* password = "RuheBewahrenGuelleFahren";

void setupWiFi() {
  WiFi.mode(WIFI_STA);
  
  IPAddress ip(192, 168, 0, 216);
  IPAddress gateway(192, 168, 0, 1);
  IPAddress subnet(255, 255, 255, 0);
  
  if (!WiFi.config(ip, gateway, subnet)) {
    Serial.println("Fehler bei statischer IP!");
  }
  
  WiFi.begin(ssid, password);
  Serial.print("Verbinde mit WiFi");
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("WiFi verbunden! IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("WiFi fehlgeschlagen!");
  }
}

void setupOTA() {
  if (WiFi.status() != WL_CONNECTED) return;
  
  ArduinoOTA.setHostname("nightlight");
  
  ArduinoOTA.onStart([]() {
    Serial.println("OTA Update startet...");
  });
  
  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA Update fertig!");
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  
  ArduinoOTA.begin();
  Serial.println("OTA bereit!");
}

void handleOTA() {
  ArduinoOTA.handle();
}