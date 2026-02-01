#include <Arduino.h>
#include <time.h>
#include "wifi_ota.h"
#include "leds.h"
#include "pir.h"
#include "mywebserver.h"

// Motion timeout
unsigned long lastMotionTime = 0;
unsigned long TIMEOUT = 10000;

// Zeitsteuerung (Stunde und Minute)
int scheduleStartHour = 22;
int scheduleStartMinute = 0;
int scheduleEndHour = 6;
int scheduleEndMinute = 0;
bool scheduleEnabled = true;

// Scharf-Schaltung (armed)
bool isArmed = false;
unsigned long armedUntil = 0;  // 0 = nicht aktiv, sonst Unix-Timestamp

// NTP Konfiguration
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;      // UTC+1 (MEZ)
const int daylightOffset_sec = 3600;  // Sommerzeit +1h

void setTimeoutValue(unsigned long val) {
  TIMEOUT = val;
}

void setSchedule(int startH, int startM, int endH, int endM) {
  scheduleStartHour = startH;
  scheduleStartMinute = startM;
  scheduleEndHour = endH;
  scheduleEndMinute = endM;
}

void setScheduleEnabled(bool enabled) {
  scheduleEnabled = enabled;
}

void armFor(int hours) {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    time_t now;
    time(&now);
    armedUntil = now + (hours * 3600);
    isArmed = true;
    Serial.printf("Scharf geschaltet für %d Stunden (bis %ld)\n", hours, armedUntil);
  }
}

void armUntilEndOfDay() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    time_t now;
    time(&now);
    // Berechne Sekunden bis Mitternacht
    int secondsUntilMidnight = (23 - timeinfo.tm_hour) * 3600 +
                               (59 - timeinfo.tm_min) * 60 +
                               (60 - timeinfo.tm_sec);
    armedUntil = now + secondsUntilMidnight;
    isArmed = true;
    Serial.printf("Scharf geschaltet bis Tagesende (%d Sekunden)\n", secondsUntilMidnight);
  }
}

void disarm() {
  isArmed = false;
  armedUntil = 0;
  Serial.println("Scharf-Schaltung deaktiviert");
}

bool isCurrentlyArmed() {
  if (!isArmed) return false;

  time_t now;
  time(&now);

  if (now >= armedUntil) {
    isArmed = false;
    armedUntil = 0;
    Serial.println("Scharf-Schaltung abgelaufen");
    return false;
  }
  return true;
}

int getArmedRemainingMinutes() {
  if (!isArmed) return 0;

  time_t now;
  time(&now);

  if (now >= armedUntil) return 0;
  return (armedUntil - now) / 60;
}

bool isWithinSchedule() {
  if (!scheduleEnabled) return true;  // Wenn deaktiviert, immer erlaubt

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Fehler beim Abrufen der Zeit");
    return true;  // Im Fehlerfall erlauben
  }

  int currentMinutes = timeinfo.tm_hour * 60 + timeinfo.tm_min;
  int startMinutes = scheduleStartHour * 60 + scheduleStartMinute;
  int endMinutes = scheduleEndHour * 60 + scheduleEndMinute;

  // Über Mitternacht hinweg (z.B. 22:00 - 06:00)
  if (startMinutes > endMinutes) {
    return (currentMinutes >= startMinutes || currentMinutes < endMinutes);
  }
  // Normaler Bereich (z.B. 18:00 - 23:00)
  return (currentMinutes >= startMinutes && currentMinutes < endMinutes);
}

bool shouldReactToMotion() {
  // Wenn manuell scharf geschaltet, reagieren
  if (isCurrentlyArmed()) return true;

  // Sonst nach Zeitplan
  return isWithinSchedule();
}

void setupNTP() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("NTP-Zeit wird synchronisiert...");

  // Warte auf erste Synchronisation
  struct tm timeinfo;
  int attempts = 0;
  while (!getLocalTime(&timeinfo) && attempts < 10) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (attempts < 10) {
    Serial.println();
    Serial.printf("Zeit synchronisiert: %02d:%02d:%02d\n",
                  timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  } else {
    Serial.println("\nZeit-Synchronisation fehlgeschlagen!");
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nNachtlicht startet...");

  setupWiFi();
  setupNTP();
  setupOTA();
  setupPIR();
  setupLEDs();
  setupWebserver();

  Serial.println("Setup fertig!");
}

void loop() {
  handleOTA();
  handleWebserver();

  if (isMotionDetected() && shouldReactToMotion()) {
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
