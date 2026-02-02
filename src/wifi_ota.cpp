#include "wifi_ota.h"
#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "log.h"

// WiFi Zugangsdaten
const char* ssid = "ArmbrustWG";
const char* password = "RuheBewahrenGuelleFahren";

void setupWiFi() {
  WiFi.mode(WIFI_STA);
  
  IPAddress ip(192, 168, 0, 216);
  IPAddress gateway(192, 168, 0, 1);
  IPAddress subnet(255, 255, 255, 0);
  IPAddress dns1(192, 168, 0, 1);
  IPAddress dns2(1, 1, 1, 1);
  
  if (!WiFi.config(ip, gateway, subnet, dns1, dns2)) {
    LOG_PRINTLN("Fehler bei statischer IP!");
  }
  
  WiFi.begin(ssid, password);
  LOG_PRINT("Verbinde mit WiFi");
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    LOG_PRINT(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    LOG_PRINTLN("");
    LOG_PRINT("WiFi verbunden! IP: ");
    LOG_PRINTLN(WiFi.localIP().toString());
  } else {
    LOG_PRINTLN("");
    LOG_PRINTLN("WiFi fehlgeschlagen!");
  }
}

void setupOTA() {
  if (WiFi.status() != WL_CONNECTED) return;
  
  ArduinoOTA.setHostname("nightlight");
  
  ArduinoOTA.onStart([]() {
    LOG_PRINTLN("OTA Update startet...");
  });
  
  ArduinoOTA.onEnd([]() {
    LOG_PRINTLN("\nOTA Update fertig!");
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    LOG_PRINTF("Progress: %u%%\r", (progress / (total / 100)));
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    LOG_PRINTF("OTA Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) LOG_PRINTLN("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) LOG_PRINTLN("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) LOG_PRINTLN("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) LOG_PRINTLN("Receive Failed");
    else if (error == OTA_END_ERROR) LOG_PRINTLN("End Failed");
  });
  
  ArduinoOTA.begin();
  LOG_PRINTLN("OTA bereit!");
}

void handleOTA() {
  ArduinoOTA.handle();
}
