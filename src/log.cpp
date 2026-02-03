#include "log.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "leds.h"

static WiFiServer telnetServer(23);
static WiFiClient telnetClient;
static bool serverStarted = false;

static bool sendQueuedEvent();
static bool canSendNow();

#ifndef LOG_QUEUE_SIZE
#define LOG_QUEUE_SIZE 30
#endif

#ifndef LOG_SEND_INTERVAL_MS
#define LOG_SEND_INTERVAL_MS 5000
#endif

#ifndef LOG_HTTP_TIMEOUT_MS
#define LOG_HTTP_TIMEOUT_MS 1000
#endif

struct LogEventItem {
  char event[32];
  bool lightsOn;
  int brightness;
  bool motion;
  bool hasMessage;
  char message[96];
};

static LogEventItem logQueue[LOG_QUEUE_SIZE];
static size_t logQueueHead = 0;
static size_t logQueueTail = 0;
static size_t logQueueCount = 0;
static unsigned long lastSendAttemptMs = 0;

#ifndef LOGS_ENDPOINT
#define LOGS_ENDPOINT ""
#endif

#ifndef LOGS_API_KEY
#define LOGS_API_KEY ""
#endif

void setupLog() {
  serverStarted = false;
}

void handleLog() {
  if (!serverStarted && WiFi.status() == WL_CONNECTED) {
    telnetServer.begin();
    telnetServer.setNoDelay(true);
    serverStarted = true;
  }
  if (!serverStarted) {
    return;
  }
  if (!telnetClient || !telnetClient.connected()) {
    if (telnetServer.hasClient()) {
      if (telnetClient) {
        telnetClient.stop();
      }
      telnetClient = telnetServer.available();
    }
  }

  if (telnetClient && telnetClient.connected()) {
    while (telnetClient.available()) {
      telnetClient.read();
    }
  }

  if (logQueueCount > 0 && canSendNow()) {
    // Try to send one queued event per loop.
    sendQueuedEvent();
  }
}

static void writeToOutputs(const char* msg) {
  Serial.print(msg);
  if (telnetClient && telnetClient.connected()) {
    telnetClient.print(msg);
  }
}

void logPrint(const char* msg) {
  writeToOutputs(msg);
}

void logPrint(const String& msg) {
  writeToOutputs(msg.c_str());
}

void logPrintln(const char* msg) {
  Serial.println(msg);
  if (telnetClient && telnetClient.connected()) {
    telnetClient.println(msg);
  }
}

void logPrintln(const String& msg) {
  logPrintln(msg.c_str());
}

void logPrintf(const char* fmt, ...) {
  char buffer[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);
  writeToOutputs(buffer);
}

static bool logsConfigured() {
  return strlen(LOGS_ENDPOINT) > 0 && strlen(LOGS_API_KEY) > 0;
}

static bool canSendNow() {
  if (!logsConfigured()) return false;
  if (WiFi.status() != WL_CONNECTED) return false;
  if (isFadeActive()) return false;
  unsigned long now = millis();
  if (now - lastSendAttemptMs < LOG_SEND_INTERVAL_MS) return false;
  lastSendAttemptMs = now;
  return true;
}

static void appendJsonEscaped(String& out, const char* input) {
  if (!input) return;
  for (const char* p = input; *p; ++p) {
    char c = *p;
    if (c == '\\' || c == '"') {
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
}

static bool enqueueEvent(const char* event, bool lightsOn, int brightness, bool motion, const char* message) {
  if (logQueueCount >= LOG_QUEUE_SIZE) {
    // Drop oldest to make room.
    logQueueHead = (logQueueHead + 1) % LOG_QUEUE_SIZE;
    logQueueCount--;
  }

  LogEventItem& item = logQueue[logQueueTail];
  item.event[0] = '\0';
  item.message[0] = '\0';
  item.hasMessage = false;

  if (event) {
    strlcpy(item.event, event, sizeof(item.event));
  }
  item.lightsOn = lightsOn;
  item.brightness = brightness;
  item.motion = motion;

  if (message && message[0] != '\0') {
    strlcpy(item.message, message, sizeof(item.message));
    item.hasMessage = true;
  }

  logQueueTail = (logQueueTail + 1) % LOG_QUEUE_SIZE;
  logQueueCount++;
  return true;
}

static bool sendQueuedEvent() {
  if (!logsConfigured()) return false;
  if (WiFi.status() != WL_CONNECTED) return false;
  if (logQueueCount == 0) return false;
  if (isFadeActive()) return false;

  LogEventItem& item = logQueue[logQueueHead];
  if (item.event[0] == '\0') return false;

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  if (!http.begin(client, LOGS_ENDPOINT)) {
    http.end();
    return false;
  }
  http.setTimeout(LOG_HTTP_TIMEOUT_MS);
  http.addHeader("Content-Type", "application/json");
  String authHeader = String("Bearer ") + LOGS_API_KEY;
  http.addHeader("Authorization", authHeader);

  String payload = "{";
  payload += "\"event\":\"";
  appendJsonEscaped(payload, item.event);
  payload += "\",";
  payload += "\"lights_on\":";
  payload += (item.lightsOn ? "true" : "false");
  payload += ",";
  payload += "\"brightness\":";
  payload += String(item.brightness);
  payload += ",";
  payload += "\"motion\":";
  payload += (item.motion ? "true" : "false");
  if (item.hasMessage) {
    payload += ",\"message\":\"";
    appendJsonEscaped(payload, item.message);
    payload += "\"";
  }
  payload += "}";

  int status = http.POST(payload);
  http.end();
  if (status >= 200 && status < 300) {
    logQueueHead = (logQueueHead + 1) % LOG_QUEUE_SIZE;
    logQueueCount--;
    return true;
  }
  return false;
}

void logEvent(const char* event, bool lightsOn, int brightness, bool motion, const char* message) {
  if (!logsConfigured()) return;
  if (!event || event[0] == '\0') return;

  enqueueEvent(event, lightsOn, brightness, motion, message);
}

void logEvent(const char* event, bool lightsOn, int brightness, bool motion, const String& message) {
  logEvent(event, lightsOn, brightness, motion, message.c_str());
}
