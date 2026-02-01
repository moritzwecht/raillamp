#pragma once
#include <FastLED.h>

void setupLEDs();
void startFadeIn();
void startFadeOut();
void updateFade();
bool isLightOn();
void setMaxBrightness(int brightness);
void setColor(int r, int g, int b);