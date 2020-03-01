#include "../include/globals.h"

#define RED_POWER_DRAW 16
#define GREEN_POWER_DRAW 11
#define BLUE_POWER_DRAW 15

#define TO_CHECK_LEDS LEDS_PER_STRIP
#define ALL_WHITE_AMPS \
    ((RED_POWER_DRAW + GREEN_POWER_DRAW + BLUE_POWER_DRAW) * TO_CHECK_LEDS)
#define MAX_SCALE_FACTOR 180UL
#define PRETTY_SCALE_FACTOR 180UL
#define MAX_AMPS_UINT (ALL_WHITE_AMPS * MAX_SCALE_FACTOR)
#define PRETTY_AMPS_UINT (ALL_WHITE_AMPS * PRETTY_SCALE_FACTOR)

namespace Power {
    uint8_t get_scale(CRGB color) {
        unsigned long total_draw = 0;

        total_draw += color.r * TO_CHECK_LEDS * RED_POWER_DRAW;
        total_draw += color.g * TO_CHECK_LEDS * GREEN_POWER_DRAW;
        total_draw += color.b * TO_CHECK_LEDS * BLUE_POWER_DRAW;

        double draw_percent = ((double)PRETTY_AMPS_UINT) / (double)total_draw;
        return draw_percent * 255UL;
    }

    uint8_t get_scale() {
        unsigned long total_draw = 0;

        for (int i = 0; i < TO_CHECK_LEDS; i++) {
            total_draw += leds[i].r * RED_POWER_DRAW;
            total_draw += leds[i].g * GREEN_POWER_DRAW;
            total_draw += leds[i].b * BLUE_POWER_DRAW;
        }
        double draw_percent = ((double)PRETTY_AMPS_UINT) / (double)total_draw;
        return draw_percent * 255UL;
    }
}  // namespace Power