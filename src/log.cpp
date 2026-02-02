#include "log.h"
#include <WiFi.h>

static WiFiServer telnetServer(23);
static WiFiClient telnetClient;
static bool serverStarted = false;

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
    return;
  }

  while (telnetClient.available()) {
    telnetClient.read();
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
