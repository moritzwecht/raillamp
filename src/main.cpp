#include <Arduino.h>
#include "wifi_ota.h"
#include "leds.h"
#include "pir.h"
#include "webserver.h"

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
// ```

// ---

// ## **Aufruf im Browser:**
// ```
// http://192.168.0.200






// ######################################################
// ######################################################
// Old Code
// ######################################################
// ######################################################


// #include <Arduino.h>
// #include <FastLED.h>
// #include <WiFi.h>
// #include <ESPmDNS.h>
// #include <WiFiUdp.h>
// #include <ArduinoOTA.h>

// // WiFi Zugangsdaten
// const char* ssid = "ArmbrustWG";
// const char* password = "RuheBewahrenGuelleFahren";

// // Pin Definitionen
// #define PIR_PIN_1 13
// #define PIR_PIN_2 14
// #define LED_PIN 5

// // LED Einstellungen
// #define NUM_LEDS 20
// #define MAX_BRIGHTNESS 255
// #define FADE_SPEED 5

// CRGB leds[NUM_LEDS];

// // Variablen
// bool lightsOn = false;
// bool shouldFadeIn = false;
// bool shouldFadeOut = false;
// int currentBrightness = 0;
// unsigned long lastMotionTime = 0;
// const unsigned long TIMEOUT = 10000;


// // Farbe
// // CRGB targetColor = CRGB(255, 140, 60);
// CRGB targetColor = CRGB(255, 255, 255);

// void setupWiFi() {
//   WiFi.mode(WIFI_STA);
  
//   // Feste IP-Adresse setzen
//   IPAddress ip(192, 168, 0, 216);
//   IPAddress gateway(192, 168, 0, 1);
//   IPAddress subnet(255, 255, 255, 0);
  
//   if (!WiFi.config(ip, gateway, subnet)) {
//     Serial.println("Fehler bei statischer IP!");
//   }
  
//   WiFi.begin(ssid, password);
//   Serial.print("Verbinde mit WiFi");
  
//   int attempts = 0;
//   while (WiFi.status() != WL_CONNECTED && attempts < 20) {
//     delay(500);
//     Serial.print(".");
//     attempts++;
//   }
  
//   if (WiFi.status() == WL_CONNECTED) {
//     Serial.println();
//     Serial.print("WiFi verbunden! IP: ");
//     Serial.println(WiFi.localIP());
//   } else {
//     Serial.println();
//     Serial.println("WiFi fehlgeschlagen!");
//   }
// }

// void setupOTA() {
//   if (WiFi.status() != WL_CONNECTED) return;
  
//   ArduinoOTA.setHostname("nightlight");
  
//   ArduinoOTA.onStart([]() {
//     Serial.println("OTA Update startet...");
//     fill_solid(leds, NUM_LEDS, CRGB::Black);
//     FastLED.show();
//   });
  
//   ArduinoOTA.onEnd([]() {
//     Serial.println("\nOTA Update fertig!");
//   });
  
//   ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
//     Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
//   });
  
//   ArduinoOTA.onError([](ota_error_t error) {
//     Serial.printf("OTA Error[%u]: ", error);
//     if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
//     else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
//     else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
//     else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
//     else if (error == OTA_END_ERROR) Serial.println("End Failed");
//   });
  
//   ArduinoOTA.begin();
//   Serial.println("OTA bereit!");
// }

// void fadeIn() {
//   if (currentBrightness < MAX_BRIGHTNESS) {
//     currentBrightness += FADE_SPEED;
//     if (currentBrightness > MAX_BRIGHTNESS) {
//       currentBrightness = MAX_BRIGHTNESS;
//       shouldFadeIn = false;
//       Serial.println("Fade-In fertig");
//     }
//     FastLED.setBrightness(currentBrightness);
//     fill_solid(leds, NUM_LEDS, targetColor);
//     FastLED.show();
//   } else {
//     shouldFadeIn = false;
//   }
// }

// void fadeOut() {
//   if (currentBrightness > 0) {
//     currentBrightness -= FADE_SPEED;
//     if (currentBrightness < 0) {
//       currentBrightness = 0;
//       shouldFadeOut = false;
//       lightsOn = false;
//       Serial.println("Fade-Out fertig");
//     }
//     FastLED.setBrightness(currentBrightness);
//     fill_solid(leds, NUM_LEDS, targetColor);
//     FastLED.show();
//   } else {
//     shouldFadeOut = false;
//     lightsOn = false;
//   }
// }

// void setup() {
//   Serial.begin(115200);
//   Serial.println("\nNachtlicht mit 2x PIR + OTA startet...");
  
//   // WiFi & OTA
//   setupWiFi();
//   setupOTA();
  
//   // PIR Sensoren
//   pinMode(PIR_PIN_1, INPUT);
//   pinMode(PIR_PIN_2, INPUT);
  
//   // LED Strip
//   FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
//   FastLED.setBrightness(0);
//   fill_solid(leds, NUM_LEDS, CRGB::Black);
//   FastLED.show();
  
//   Serial.println("Setup fertig!");
// }

// void loop() {
//   // OTA Handler
//   ArduinoOTA.handle();
  
//   // Beide PIR Sensoren auslesen
//   int motion1 = digitalRead(PIR_PIN_1);
//   int motion2 = digitalRead(PIR_PIN_2);
  
//   // Bewegung von einem der beiden Sensoren
//   if (motion1 == HIGH || motion2 == HIGH) {
//     lastMotionTime = millis();
    
//     if (!lightsOn && !shouldFadeIn) {
//       Serial.println("Bewegung erkannt! (Sensor " + 
//                      String(motion1 == HIGH ? 1 : 2) + ")");
//       lightsOn = true;
//       shouldFadeIn = true;
//       shouldFadeOut = false;
//     }
//   }
  
//   // Fade-In
//   if (shouldFadeIn) {
//     fadeIn();
//     delay(30);
//   }
  
//   // Fade-Out
//   if (shouldFadeOut) {
//     fadeOut();
//     delay(30);
//   }
  
//   // Timeout prüfen
//   if (lightsOn && !shouldFadeIn && !shouldFadeOut && 
//       (millis() - lastMotionTime > TIMEOUT)) {
//     Serial.println("Timeout! Fade-Out startet...");
//     shouldFadeOut = true;
//   }
  
//   delay(50);
// }