#include <Arduino.h>
#include <globals.h>

#define ALL_WHITE_AMPS 12600UL
#define MAX_SCALE_FACTOR 127UL
#define PRETTY_SCALE_FACTOR 90UL
#define MAX_AMPS_UINT (ALL_WHITE_AMPS * MAX_SCALE_FACTOR)
#define PRETTY_AMPS_UINT (ALL_WHITE_AMPS * PRETTY_SCALE_FACTOR)

#define RED_POWER_DRAW 16
#define GREEN_POWER_DRAW 11
#define BLUE_POWER_DRAW 15

namespace Power {
	uint8_t get_scale() {
		unsigned long total_draw = 0;
		for (int i = 0; i < NUM_LEDS; i++) {
			total_draw += leds[i].r * RED_POWER_DRAW;
			total_draw += leds[i].g * GREEN_POWER_DRAW;
			total_draw += leds[i].b * BLUE_POWER_DRAW;
		}
		double draw_percent = ((double) PRETTY_AMPS_UINT) / (double)total_draw;
		return draw_percent * 255UL;
	}
}