#include "../include/globals.h"
#include "../include/modes.h"
#include "../include/serial_control.h"

void setup() {
    Serial.begin(115200);
#ifndef MOCK
    FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS, 0);
#else
    FastLED.addLeds(leds, NUM_LEDS, 0);
#endif
    while (!Serial) {
    }

    randomSeed(analogRead(0));
}

CRGB leds[NUM_LEDS] = {CRGB::Black};
unsigned long last_run = millis() - 1;
void loop() {
    // Fetch serial data
    SerialControl::recv_with_end_marker();

    // Assert iterate FN exists
    if (Modes::iterate_fn == NULL) {
        Modes::iterate_fn = Modes::Off::do_iteration;
        Modes::mode_update_time = Modes::Off::update_time;
    }

    // If new serial data, handle that serial data
    if (SerialControl::new_data && Modes::serial_override) {
        Modes::serial_override();
    }
    if (SerialControl::new_data) {
        SerialControl::handle_serial();
    }

    // If new data, iterate
    if (Modes::force_update) {
        // Do iterate function
        Modes::iterate_fn();

        // Reset new_data
        Modes::force_update = false;
    } else if (Modes::mode_update_time == 0) {
        // Do iterate function
        Modes::iterate_fn();
    } else if (millis() - last_run >= Modes::mode_update_time) {
        // Set last run
        last_run = millis();

        // Do iterate function
        Modes::iterate_fn();
    }
}