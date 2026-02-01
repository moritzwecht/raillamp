#include "leds.h"
#include <Arduino.h>

#define LED_PIN 5
#define NUM_LEDS 20

int MAX_BRIGHTNESS = 30;
#define FADE_SPEED 5

CRGB leds[NUM_LEDS];
CRGB targetColor = CRGB(255, 140, 60);

bool lightsOn = false;
bool shouldFadeIn = false;
bool shouldFadeOut = false;
int currentBrightness = 0;

void setupLEDs() {
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(0);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
  Serial.println("LEDs initialisiert!");
}

void startFadeIn() {
  lightsOn = true;
  shouldFadeIn = true;
  shouldFadeOut = false;
  Serial.println("Fade-In startet...");
}

void startFadeOut() {
  shouldFadeOut = true;
  Serial.println("Fade-Out startet...");
}

bool isLightOn() {
  return lightsOn;
}

void setMaxBrightness(int brightness) {
  MAX_BRIGHTNESS = brightness;
  if (lightsOn) {
    FastLED.setBrightness(MAX_BRIGHTNESS);
    FastLED.show();
  }
}

void setColor(int r, int g, int b) {
  targetColor = CRGB(r, g, b);
  if (lightsOn) {
    fill_solid(leds, NUM_LEDS, targetColor);
    FastLED.show();
  }
}

void updateFade() {
  if (shouldFadeIn) {
    currentBrightness += FADE_SPEED;
    if (currentBrightness >= MAX_BRIGHTNESS) {
      currentBrightness = MAX_BRIGHTNESS;
      shouldFadeIn = false;
      Serial.println("Fade-In fertig");
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
      Serial.println("Fade-Out fertig");
    }
    FastLED.setBrightness(currentBrightness);
    fill_solid(leds, NUM_LEDS, targetColor);
    FastLED.show();
    delay(30);
  }
}