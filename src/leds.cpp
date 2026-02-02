#include "leds.h"
#include <Arduino.h>
#include "log.h"

#define LED_PIN 5
#define NUM_LEDS 20

static const int MAX_BRIGHTNESS = 255;
#define FADE_SPEED 5

CRGB leds[NUM_LEDS];
CRGB targetColor = CRGB(255, 255, 255);

bool lightsOn = false;
bool shouldFadeIn = false;
bool shouldFadeOut = false;
int currentBrightness = 0;

void setupLEDs() {
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(0);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
  LOG_PRINTLN("LEDs initialisiert!");
}

void startFadeIn() {
  lightsOn = true;
  shouldFadeIn = true;
  shouldFadeOut = false;
  LOG_PRINTLN("Fade-In startet...");
}

void startFadeOut() {
  shouldFadeOut = true;
  LOG_PRINTLN("Fade-Out startet...");
}

bool isLightOn() {
  return lightsOn;
}

int getCurrentBrightness() {
  return currentBrightness;
}

void updateFade() {
  if (shouldFadeIn) {
    currentBrightness += FADE_SPEED;
    if (currentBrightness >= MAX_BRIGHTNESS) {
      currentBrightness = MAX_BRIGHTNESS;
      shouldFadeIn = false;
      LOG_PRINTLN("Fade-In fertig");
    }
    FastLED.setBrightness(currentBrightness);
    fill_solid(leds, NUM_LEDS, targetColor);
    FastLED.show();
    delay(30);
  }

  if (shouldFadeOut) {
    currentBrightness -= FADE_SPEED;
    if (currentBrightness <= 0) {
      currentBrightness = 0;
      shouldFadeOut = false;
      lightsOn = false;
      LOG_PRINTLN("Fade-Out fertig");
    }
    FastLED.setBrightness(currentBrightness);
    fill_solid(leds, NUM_LEDS, targetColor);
    FastLED.show();
    delay(30);
  }
}
