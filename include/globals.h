#pragma once

#ifndef MOCK
#include <Arduino.h>
#include <FastLED.h>
#else
#include "mock-arduino.h"
#include "mock-fastled.h"
#include "mock-main.h"
#include "mock-serial.h"
#endif

#define LED_PIN 7
#define LEDS_PER_M 60
#define METERS_PER_STRIP 5
#define LEDS_PER_STRIP (LEDS_PER_M * METERS_PER_STRIP)
#define STRIPS 3
#define NUM_LEDS (LEDS_PER_STRIP * STRIPS)

extern CRGB leds[NUM_LEDS];