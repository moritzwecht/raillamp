#include "mywebserver.h"
#include "leds.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <time.h>

WebServer server(80);

// Status Variablen
String currentStatus = "Bereit";
String currentError = "";

// Log buffer
static const int LOG_CAPACITY = 80;
static String logBuffer[LOG_CAPACITY];
static int logStart = 0;
static int logCount = 0;

// Extern aus leds.cpp
extern int currentBrightness;
extern int MAX_BRIGHTNESS;

// Extern aus main.cpp
extern unsigned long TIMEOUT;
extern int scheduleStartHour;
extern int scheduleStartMinute;
extern int scheduleEndHour;
extern int scheduleEndMinute;
extern bool scheduleEnabled;
extern bool isArmed;
extern unsigned long armedUntil;

void setTimeoutValue(unsigned long val);
void setSchedule(int startH, int startM, int endH, int endM);
void setScheduleEnabled(bool enabled);
void armFor(int hours);
void armUntilEndOfDay();
void disarm();
bool isCurrentlyArmed();
int getArmedRemainingMinutes();
bool isWithinSchedule();

static String formatTimeForLog() {
  time_t now = time(nullptr);
  if (now < 1609459200) {  // 2021-01-01, guard for unsynced time
    return String("--:--:--");
  }
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);
  char timeStr[9];
  sprintf(timeStr, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  return String(timeStr);
}

static String escapeJson(const String& input) {
  String out;
  out.reserve(input.length() + 8);
  for (size_t i = 0; i < input.length(); i++) {
    char c = input[i];
    if (c == '\"' || c == '\\') {
      out += '\\';
      out += c;
    } else if (c == '\n') {
      out += "\\n";
    } else if (c == '\r') {
      out += "\\r";
    } else if (c == '\t') {
      out += "\\t";
    } else {
      out += c;
    }
  }
  return out;
}

void logEvent(const String& message) {
  String line = formatTimeForLog() + " " + message;
  int idx;
  if (logCount < LOG_CAPACITY) {
    idx = (logStart + logCount) % LOG_CAPACITY;
    logCount++;
  } else {
    idx = logStart;
    logStart = (logStart + 1) % LOG_CAPACITY;
  }
  logBuffer[idx] = line;
}

void updateStatus(const String& status) {
  currentStatus = status;
  logEvent(status);
}

void updateError(const String& error) {
  currentError = error;
  logEvent("FEHLER: " + error);
}

// Content-Type basierend auf Dateiendung
String getContentType(String filename) {
  if (filename.endsWith(".html")) return "text/html";
  if (filename.endsWith(".css")) return "text/css";
  if (filename.endsWith(".js")) return "application/javascript";
  if (filename.endsWith(".ico")) return "image/x-icon";
  if (filename.endsWith(".png")) return "image/png";
  if (filename.endsWith(".jpg")) return "image/jpeg";
  if (filename.endsWith(".json")) return "application/json";
  return "text/plain";
}

// Datei von LittleFS servieren
bool handleFileRead(String path) {
  if (path.endsWith("/")) path += "index.html";

  String contentType = getContentType(path);

  if (LittleFS.exists(path)) {
    File file = LittleFS.open(path, "r");
    server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

// Route Handlers
void handleRoot() {
  if (!handleFileRead("/index.html")) {
    server.send(404, "text/plain", "Datei nicht gefunden");
  }
}

void handleStatus() {
  String currentTime = "--:--";
  time_t now = time(nullptr);
  if (now >= 1609459200) {
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    char timeStr[6];
    sprintf(timeStr, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
    currentTime = String(timeStr);
  }

  String json = "{";
  json += "\"lightsOn\":" + String(isLightOn() ? "true" : "false") + ",";
  json += "\"brightness\":" + String(currentBrightness) + ",";
  json += "\"maxBrightness\":" + String(MAX_BRIGHTNESS) + ",";
  json += "\"timeout\":" + String(TIMEOUT / 1000) + ",";
  json += "\"pir1\":" + String(digitalRead(13)) + ",";
  json += "\"pir2\":" + String(digitalRead(14)) + ",";
  json += "\"currentTime\":\"" + currentTime + "\",";
  json += "\"scheduleEnabled\":" + String(scheduleEnabled ? "true" : "false") + ",";
  json += "\"scheduleStartHour\":" + String(scheduleStartHour) + ",";
  json += "\"scheduleStartMinute\":" + String(scheduleStartMinute) + ",";
  json += "\"scheduleEndHour\":" + String(scheduleEndHour) + ",";
  json += "\"scheduleEndMinute\":" + String(scheduleEndMinute) + ",";
  json += "\"isArmed\":" + String(isCurrentlyArmed() ? "true" : "false") + ",";
  json += "\"armedRemaining\":" + String(getArmedRemainingMinutes()) + ",";
  json += "\"withinSchedule\":" + String(isWithinSchedule() ? "true" : "false") + ",";
  json += "\"status\":\"" + currentStatus + "\",";
  json += "\"error\":\"" + currentError + "\"";
  json += "}";

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

void handleLog() {
  String json = "{";
  json += "\"count\":" + String(logCount) + ",";
  json += "\"entries\":[";
  for (int i = 0; i < logCount; i++) {
    int idx = (logStart + i) % LOG_CAPACITY;
    json += "\"" + escapeJson(logBuffer[idx]) + "\"";
    if (i < logCount - 1) json += ",";
  }
  json += "]}";

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

void handleLogClear() {
  logStart = 0;
  logCount = 0;
  server.send(200, "text/plain", "OK");
}

// Handler für alle Routen
void handleNotFound() {
  String uri = server.uri();

  // Versuche zuerst, die Datei von LittleFS zu servieren
  if (handleFileRead(uri)) {
    return;
  }

  // /set/brightness/<value>
  if (uri.startsWith("/set/brightness/")) {
    String valStr = uri.substring(16);
    int val = valStr.toInt();
    if (val >= 0 && val <= 255) {
      setMaxBrightness(val);
      updateStatus("Helligkeit auf " + String(val) + " gesetzt");
      server.send(200, "text/plain", "OK");
    } else {
      server.send(400, "text/plain", "Ungültiger Wert (0-255)");
    }
    return;
  }

  // /set/timeout/<value>
  if (uri.startsWith("/set/timeout/")) {
    String valStr = uri.substring(13);
    int val = valStr.toInt();
    if (val >= 1 && val <= 300) {
      setTimeoutValue(val * 1000);
      updateStatus("Timeout auf " + String(val) + "s gesetzt");
      server.send(200, "text/plain", "OK");
    } else {
      server.send(400, "text/plain", "Ungültiger Wert (1-300)");
    }
    return;
  }

  // /set/schedule/<startH>/<startM>/<endH>/<endM>
  if (uri.startsWith("/set/schedule/")) {
    String params = uri.substring(14);
    int slashes[3];
    int idx = 0;
    for (int i = 0; i < params.length() && idx < 3; i++) {
      if (params[i] == '/') {
        slashes[idx++] = i;
      }
    }

    if (idx == 3) {
      int startH = params.substring(0, slashes[0]).toInt();
      int startM = params.substring(slashes[0] + 1, slashes[1]).toInt();
      int endH = params.substring(slashes[1] + 1, slashes[2]).toInt();
      int endM = params.substring(slashes[2] + 1).toInt();

      if (startH >= 0 && startH <= 23 && startM >= 0 && startM <= 59 &&
          endH >= 0 && endH <= 23 && endM >= 0 && endM <= 59) {
        setSchedule(startH, startM, endH, endM);
        char buf[50];
        sprintf(buf, "Zeitplan: %02d:%02d - %02d:%02d", startH, startM, endH, endM);
        updateStatus(String(buf));
        server.send(200, "text/plain", "OK");
      } else {
        updateError("Ungültige Zeitwerte");
        server.send(400, "text/plain", "Ungültige Zeitwerte");
      }
    } else {
      updateError("Formatfehler Zeitplan");
      server.send(400, "text/plain", "Format: /set/schedule/HH/MM/HH/MM");
    }
    return;
  }

  // /set/schedule/enabled/<0|1>
  if (uri.startsWith("/set/schedule/enabled/")) {
    String valStr = uri.substring(22);
    bool enabled = (valStr == "1" || valStr == "true");
    setScheduleEnabled(enabled);
    updateStatus(enabled ? "Zeitplan aktiviert" : "Zeitplan deaktiviert");
    server.send(200, "text/plain", "OK");
    return;
  }

  // /arm/<hours> oder /arm/day
  if (uri.startsWith("/arm/")) {
    String param = uri.substring(5);
    if (param == "day") {
      armUntilEndOfDay();
      updateStatus("Scharf bis Tagesende");
    } else {
      int hours = param.toInt();
      if (hours >= 1 && hours <= 12) {
        armFor(hours);
        updateStatus("Scharf für " + String(hours) + "h");
      } else {
        updateError("Ungültige Stunden (1-12)");
        server.send(400, "text/plain", "Ungültige Stunden (1-12)");
        return;
      }
    }
    server.send(200, "text/plain", "OK");
    return;
  }

  // /disarm
  if (uri == "/disarm") {
    disarm();
    updateStatus("Scharf-Schaltung deaktiviert");
    server.send(200, "text/plain", "OK");
    return;
  }

  // Keine bekannte Route
  server.send(404, "text/plain", "Nicht gefunden: " + uri);
}

void setupWebserver() {
  // LittleFS initialisieren
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS Mount fehlgeschlagen!");
    return;
  }
  Serial.println("LittleFS bereit");

  // Dateien im Filesystem auflisten
  File root = LittleFS.open("/");
  File file = root.openNextFile();
  Serial.println("Dateien im Filesystem:");
  while (file) {
    Serial.printf("  %s (%d bytes)\n", file.name(), file.size());
    file = root.openNextFile();
  }

  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/log", handleLog);
  server.on("/log/clear", handleLogClear);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("Webserver gestartet auf Port 80!");
  Serial.println("Öffne: http://192.168.0.216");
}

void handleWebserver() {
  server.handleClient();
}
