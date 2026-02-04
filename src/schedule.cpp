#include "schedule.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <time.h>
#include "log.h"
#include "leds.h"
#include "pir.h"

static const char* kScheduleUrl = "https://railroadlantern-web.vercel.app/api/schedule";
static const char* kTwilightUrl = "https://railroadlantern-web.vercel.app/api/twilight";

static const unsigned long kScheduleCheckIntervalMs = 30000;
static const unsigned long kScheduleFetchRetryMs = 30000;
static const unsigned long kHttpTimeoutMs = 10000;

struct ScheduleConfig {
  String startTime;
  String endTime;
  String startType;
  String endType;
  bool enabled = false;
};

struct TwilightTimes {
  String civilDawn;
  String civilDusk;
};

static ScheduleConfig scheduleConfig;
static String effectiveStart;
static String effectiveEnd;
static bool scheduleLoaded = false;
static int lastFetchYday = -1;
static unsigned long lastFetchAttemptMs = 0;
static unsigned long lastEvalMs = 0;
static ScheduleState cachedState = ScheduleState::Unknown;

static bool timeIsValid() {
  time_t now = time(nullptr);
  return now > 1700000000;
}

static bool getLocalTimeNow(struct tm& timeInfo) {
  if (!timeIsValid()) return false;
  time_t now = time(nullptr);
  localtime_r(&now, &timeInfo);
  return true;
}

static int timeToMinutes(const String& time) {
  if (time.length() < 5) return -1;
  int hours = time.substring(0, 2).toInt();
  int minutes = time.substring(3, 5).toInt();
  return hours * 60 + minutes;
}

static bool shouldBeOn(const String& current, const String& start, const String& end) {
  int currentMin = timeToMinutes(current);
  int startMin = timeToMinutes(start);
  int endMin = timeToMinutes(end);
  if (currentMin < 0 || startMin < 0 || endMin < 0) return false;

  if (startMin > endMin) {
    return (currentMin >= startMin || currentMin < endMin);
  }
  return (currentMin >= startMin && currentMin < endMin);
}

static void logScheduleEvent(const char* event, const String& details) {
  if (details.length() == 0) {
    logEvent(event, isLightOn(), getCurrentBrightness(), getMotionState(), nullptr);
  } else {
    logEvent(event, isLightOn(), getCurrentBrightness(), getMotionState(), details);
  }
}

static bool parseSchedule(const String& body, ScheduleConfig& config) {
  DynamicJsonDocument doc(512);
  DeserializationError err = deserializeJson(doc, body);
  if (err) return false;

  // API returns nested object: {"schedule": {...}}
  JsonVariant schedule = doc["schedule"];
  if (!schedule.is<JsonObject>()) return false;

  JsonVariant startTime = schedule["start_time"];
  JsonVariant endTime = schedule["end_time"];
  JsonVariant startType = schedule["start_type"];
  JsonVariant endType = schedule["end_type"];
  JsonVariant enabled = schedule["enabled"];

  if (!startType.is<const char*>() || !endType.is<const char*>()) return false;
  if (!enabled.is<bool>()) return false;

  config.startType = String(startType.as<const char*>());
  config.endType = String(endType.as<const char*>());
  config.enabled = enabled.as<bool>();

  if (startTime.is<const char*>()) {
    config.startTime = String(startTime.as<const char*>());
  } else {
    config.startTime = "";
  }

  if (endTime.is<const char*>()) {
    config.endTime = String(endTime.as<const char*>());
  } else {
    config.endTime = "";
  }

  return true;
}

static bool parseTwilight(const String& body, TwilightTimes& twilight) {
  DynamicJsonDocument doc(384);
  DeserializationError err = deserializeJson(doc, body);
  if (err) return false;

  JsonVariant civilDawn = doc["civil_dawn"];
  JsonVariant civilDusk = doc["civil_dusk"];
  if (!civilDawn.is<const char*>() || !civilDusk.is<const char*>()) return false;

  twilight.civilDawn = String(civilDawn.as<const char*>());
  twilight.civilDusk = String(civilDusk.as<const char*>());
  return true;
}

static bool httpGetJson(const char* url, String& response) {
  if (WiFi.status() != WL_CONNECTED) return false;

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  if (!http.begin(client, url)) {
    http.end();
    return false;
  }
  http.setTimeout(kHttpTimeoutMs);

  int status = http.GET();
  if (status >= 200 && status < 300) {
    response = http.getString();
    http.end();
    return true;
  }

  http.end();
  return false;
}

static bool fetchScheduleInternal() {
  String scheduleBody;
  if (!httpGetJson(kScheduleUrl, scheduleBody)) {
    logScheduleEvent("schedule_error", "schedule_http");
    return false;
  }

  ScheduleConfig config;
  if (!parseSchedule(scheduleBody, config)) {
    logScheduleEvent("schedule_error", "schedule_parse");
    return false;
  }

  String start = config.startTime;
  String end = config.endTime;
  TwilightTimes twilight;

  if (config.startType == "civil_dusk" || config.endType == "civil_dawn") {
    String twilightBody;
    if (!httpGetJson(kTwilightUrl, twilightBody)) {
      logScheduleEvent("schedule_error", "twilight_http");
      return false;
    }
    if (!parseTwilight(twilightBody, twilight)) {
      logScheduleEvent("schedule_error", "twilight_parse");
      return false;
    }
    if (config.startType == "civil_dusk") {
      start = twilight.civilDusk;
    }
    if (config.endType == "civil_dawn") {
      end = twilight.civilDawn;
    }
  }

  if (start.length() < 5 || end.length() < 5) {
    logScheduleEvent("schedule_error", "time_missing");
    return false;
  }

  scheduleConfig = config;
  effectiveStart = start;
  effectiveEnd = end;
  scheduleLoaded = true;

  struct tm timeInfo;
  if (getLocalTimeNow(timeInfo)) {
    lastFetchYday = timeInfo.tm_yday;
  }

  String details = "Start: " + effectiveStart + " (" + scheduleConfig.startType + "), End: " +
                   effectiveEnd + " (" + scheduleConfig.endType + ")";
  logScheduleEvent("schedule_loaded", details);
  return true;
}

void setupSchedule() {
  configTzTime("CET-1CEST,M3.5.0/2,M10.5.0/3", "pool.ntp.org", "time.nist.gov", "time.google.com");
  lastFetchAttemptMs = 0;
  lastEvalMs = 0;
  cachedState = ScheduleState::Unknown;
}

void handleSchedule() {
  bool timeValid = timeIsValid();
  if (!timeValid) {
    cachedState = ScheduleState::Unknown;
  }

  unsigned long nowMs = millis();
  if (!scheduleLoaded && (nowMs - lastFetchAttemptMs > kScheduleFetchRetryMs)) {
    lastFetchAttemptMs = nowMs;
    fetchScheduleInternal();
  }

  struct tm timeInfo;
  if (timeValid && getLocalTimeNow(timeInfo)) {
    if (scheduleLoaded && timeInfo.tm_yday != lastFetchYday && timeInfo.tm_hour >= 3 &&
        (nowMs - lastFetchAttemptMs > kScheduleFetchRetryMs)) {
      lastFetchAttemptMs = nowMs;
      fetchScheduleInternal();
    }
  }

  if (nowMs - lastEvalMs < kScheduleCheckIntervalMs) return;
  lastEvalMs = nowMs;

  if (!scheduleLoaded) {
    cachedState = ScheduleState::Blocked;
    return;
  }

  if (!scheduleConfig.enabled) {
    cachedState = ScheduleState::Blocked;
    return;
  }

  if (!timeValid || !getLocalTimeNow(timeInfo)) {
    cachedState = ScheduleState::Blocked;
    return;
  }

  char currentTime[6];
  snprintf(currentTime, sizeof(currentTime), "%02d:%02d", timeInfo.tm_hour, timeInfo.tm_min);
  bool active = shouldBeOn(String(currentTime), effectiveStart, effectiveEnd);
  cachedState = active ? ScheduleState::Allowed : ScheduleState::Blocked;
}

ScheduleState getScheduleState() {
  return cachedState;
}
