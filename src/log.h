#pragma once
#include <Arduino.h>

void setupLog();
void handleLog();

void logPrint(const char* msg);
void logPrint(const String& msg);
void logPrintln(const char* msg);
void logPrintln(const String& msg);
void logPrintf(const char* fmt, ...);

#define LOG_PRINT(x) logPrint(x)
#define LOG_PRINTLN(x) logPrintln(x)
#define LOG_PRINTF(...) logPrintf(__VA_ARGS__)
