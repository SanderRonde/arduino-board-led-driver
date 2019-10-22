#pragma once

#include <FastLED.h>
#include <string.h>
#include <Arduino.h>

#define LED_PIN 7
#define LEDS_PER_M 60
#define METERS_PER_STRIP 5
#define STRIPS 3
#define NUM_LEDS ((LEDS_PER_M * METERS_PER_STRIP) * STRIPS)

extern CRGB leds[NUM_LEDS];