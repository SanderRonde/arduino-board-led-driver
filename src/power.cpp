#include <Arduino.h>
#include <globals.h>

#define ALL_WHITE_AMPS 12600UL
#define MAX_SCALE_FACTOR 127UL
#define PRETTY_SCALE_FACTOR 90UL
#define MAX_AMPS_UINT (ALL_WHITE_AMPS * MAX_SCALE_FACTOR)
#define PRETTY_AMPS_UINT (ALL_WHITE_AMPS * PRETTY_SCALE_FACTOR)
#define SPLIT_FACTOR 0.45

#define RED_POWER_DRAW 16
#define GREEN_POWER_DRAW 11
#define BLUE_POWER_DRAW 15

#define POWER_SPLIT

namespace Power {
	uint8_t get_scale(CRGB color) {
		#ifdef POWER_SPLIT
		unsigned long unscaled_draw = 0;

		unscaled_draw += color.r * 200 * RED_POWER_DRAW;
		unscaled_draw += color.g * 200 * GREEN_POWER_DRAW;
		unscaled_draw += color.b * 200 * BLUE_POWER_DRAW;

		unsigned long total_draw = unscaled_draw / SPLIT_FACTOR;
		#else
		unsigned long total_draw = 0;
		
		total_draw += color.r * NUM_LEDS * RED_POWER_DRAW;
		total_draw += color.g * NUM_LEDS * GREEN_POWER_DRAW;
		total_draw += color.b * NUM_LEDS * BLUE_POWER_DRAW;

		#endif
		double draw_percent = ((double) PRETTY_AMPS_UINT) / (double)total_draw;
		return (draw_percent / 2) * 255UL;
	}

	uint8_t get_scale() {
		#ifdef POWER_SPLIT
		unsigned long total_draw_1 = 0;
		unsigned long total_draw_2 = 0;

		for (int i = 0; i < 200; i++) {
			total_draw_1 += leds[i].r * RED_POWER_DRAW;
			total_draw_1 += leds[i].g * GREEN_POWER_DRAW;
			total_draw_1 += leds[i].b * BLUE_POWER_DRAW;
		}
		for (int i = 200; i < 300; i++) {
			total_draw_2 += leds[i].r * RED_POWER_DRAW;
			total_draw_2 += leds[i].g * GREEN_POWER_DRAW;
			total_draw_2 += leds[i].b * BLUE_POWER_DRAW;
		}
		unsigned long total_draw = (max(total_draw_1, total_draw_2) / SPLIT_FACTOR);
		#else
		unsigned long total_draw = 0;

		for (int i = 0; i < NUM_LEDS; i++) {
			total_draw += leds[i].r * RED_POWER_DRAW;
			total_draw += leds[i].g * GREEN_POWER_DRAW;
			total_draw += leds[i].b * BLUE_POWER_DRAW;
		}
		#endif
		double draw_percent = ((double) PRETTY_AMPS_UINT) / (double)total_draw;
		return (draw_percent / 2) * 255UL;
	}
}