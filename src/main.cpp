#include "../include/globals.h"
#include "../include/manual.h"

void setup() {
    Serial.begin(115200);
#ifndef MOCK
    FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS, 0);
#else
    FastLED.addLeds(leds, NUM_LEDS, 0);
#endif
    while (!Serial) {
    }

    Manual::setup()
    randomSeed(analogRead(0));
}

CRGB leds[NUM_LEDS] = {CRGB::Black};
unsigned long last_run = millis() - 1;
void loop() { Manual::loop(); }