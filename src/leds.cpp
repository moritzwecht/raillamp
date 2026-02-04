#include "leds.h"
#include <Arduino.h>
#include "log.h"
#include "pir.h"

#define LED_PIN 5
#define NUM_LEDS 20

// static const int MAX_BRIGHTNESS = 255;
static const int MAX_BRIGHTNESS = 150;
#define FADE_SPEED 5

CRGB leds[NUM_LEDS];
// CRGB targetColor = CRGB(255, 255, 255);
CRGB targetColor = CRGB(255, 140, 60); // Warmweiß
// CRGB targetColor = CRGB(255, 0, 0); // Rot

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
  logEvent("leds_init", lightsOn, currentBrightness, getMotionState(), nullptr);
}

void startFadeIn() {
  if (shouldFadeIn) return;
  lightsOn = true;
  shouldFadeIn = true;
  shouldFadeOut = false;
  LOG_PRINTLN("Fade-In startet...");
}

void startFadeOut() {
  if (shouldFadeOut || !lightsOn) return;
  shouldFadeOut = true;
  LOG_PRINTLN("Fade-Out startet...");
}

bool isLightOn() {
  return lightsOn;
}

int getCurrentBrightness() {
  return currentBrightness;
}

bool isFadeActive() {
  return shouldFadeIn || shouldFadeOut;
}

void updateFade() {
  if (shouldFadeIn) {
    currentBrightness += FADE_SPEED;
    if (currentBrightness >= MAX_BRIGHTNESS) {
      currentBrightness = MAX_BRIGHTNESS;
      shouldFadeIn = false;
      LOG_PRINTLN("Fade-In fertig");
      logEvent("light_on", lightsOn, currentBrightness, getMotionState(), "fade_in_complete");
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
      logEvent("light_off", lightsOn, currentBrightness, getMotionState(), "fade_out_complete");
    }
    FastLED.setBrightness(currentBrightness);
    fill_solid(leds, NUM_LEDS, targetColor);
    FastLED.show();
    delay(30);
  }
}
