#pragma once
#include <Arduino.h>

void setupWebserver();
void handleWebserver();
void updateStatus(const String& status);
void updateError(const String& error);