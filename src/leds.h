#pragma once
#include <FastLED.h>

void setupLEDs();
void startFadeIn();
void startFadeOut();
void updateFade();
bool isLightOn();
int getCurrentBrightness();
bool isFadeActive();
