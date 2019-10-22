#include <serial_control.h>
#include <Arduino.h>
#include <globals.h>
#include <power.h>
#include <modes.h>
#include <util.h>

#define MAX_DOTS 5
#define MAX_SPLIT_COLORS 10
#define MAX_PATTERN_LEN 20

namespace Modes {
	Modes::led_mode_t cur_mode = Modes::LED_MODE_OFF;

	void (*iterate_fn)(void) = NULL;
	unsigned long mode_update_time = 1000UL * 1UL;

	namespace Off {
		const unsigned long update_time = 1000UL * 1UL;

		void do_iteration() {
			FastLED.showColor(CRGB::Black);
		}

		void handle_serial(const String serial_data[MAX_ARG_LEN]) {
			iterate_fn = do_iteration;
			mode_update_time = update_time;
			cur_mode = Modes::LED_MODE_OFF;
		}
	}

	namespace Solid {
		const unsigned long update_time = 1000UL * 1UL;
		CRGB color = CRGB::Black;
		unsigned int intensity = 0;

		void do_iteration() {
			for (int i = 0; i < NUM_LEDS; i++) {
				leds[i] = color;
			}

			if (intensity == 0) {
				FastLED.show(Power::get_scale());
			} else {
				FastLED.show(intensity);
			}
		}

		void handle_serial(const String serial_data[MAX_ARG_LEN]) {
			intensity = atoi(serial_data[2].c_str());
			color = CRGB(
				atoi(serial_data[3].c_str()),
				atoi(serial_data[4].c_str()),
				atoi(serial_data[5].c_str())
			);

			iterate_fn = do_iteration;
			mode_update_time = update_time;
			cur_mode = Modes::LED_MODE_SOLID;
		}
	}

	namespace Dot {
		typedef struct dot {
			unsigned int dot_size;
			unsigned int dot_speed;
			unsigned int dot_pos;
			unsigned long last_move;
			dir_t dir;
			CRGB dot_color;
		} dot_t;

		const unsigned long update_time = 10;
		CRGB bg_color = CRGB::Black;
		dot_t dots[MAX_DOTS] = {{
			.dot_size = 0,
			.dot_speed = 0,
			.dot_pos = 0
		}};
		unsigned int intensity = 0;

		void do_iteration() {
			// Draw the background
			for (int i = 0; i < NUM_LEDS; i++) {
				leds[i] = bg_color;
			}

			// Draw the dots
			for (int i = 0; i < MAX_DOTS; i++) {
				dot_t* dot = &dots[i];
				if (dot->dot_size == 0) break;

				if (millis() - dot->last_move >= dot->dot_speed) {
					// Move it
					if (dot->dir == DIR_FORWARDS) {
						dot->dot_pos = (dot->dot_pos + 1) % NUM_LEDS;
					} else {
						dot->dot_pos = ((dot->dot_pos - 1) + NUM_LEDS) % NUM_LEDS;
					}
					dot->last_move = millis();
				}

				for (unsigned int j = dot->dot_pos; j < dot->dot_pos + dot->dot_size; j++) {
					leds[j % NUM_LEDS] = dot->dot_color;
				}
			}

			if (intensity == 0) {
				FastLED.show(Power::get_scale());
			} else {
				FastLED.show(intensity);
			}
		}

		void handle_serial(const String serial_data[MAX_ARG_LEN]) {
			intensity = atoi(serial_data[2].c_str());

			// Set background color
			bg_color = CRGB(
				atoi(serial_data[3].c_str()),
				atoi(serial_data[4].c_str()),
				atoi(serial_data[5].c_str())
			);

			// Parse the dots
			int last_dot = 0;
			for (int i = 6; i < MAX_ARG_LEN && serial_data[i].c_str()[0] != '\\'; i += 7) {
				last_dot = ((i - 6) % 7);
				dot_t* dot = &dots[last_dot];
				// First get the size
				dot->dot_size = atoi(serial_data[i].c_str());
				// Then the speed
				dot->dot_speed = atoi(serial_data[i + 1].c_str());
				// Then the direction
				dot->dir = atoi(serial_data[i + 2].c_str()) == 0 ? DIR_BACKWARDS : DIR_FORWARDS;
				// Then the position as a percentage
				int pos_percent = atoi(serial_data[i + 3].c_str());
				dot->dot_pos = (unsigned int)(((float)pos_percent * NUM_LEDS) / 100);
				// Then the color
				dot->dot_color = CRGB(
					atoi(serial_data[i + 4].c_str()),
					atoi(serial_data[i + 5].c_str()),
					atoi(serial_data[i + 6].c_str())
				);
			}
			// Unset all other dots
			for (int i = last_dot + 1; i < MAX_DOTS; i++) {
				dots[i].dot_size = 0;
			}

			iterate_fn = do_iteration;
			mode_update_time = update_time;
			cur_mode = Modes::LED_MODE_OFF;
		}
	}

	namespace Split {
		CRGB split_colors[MAX_SPLIT_COLORS];
		int split_colors_len = 0;
		int update_time = 0;
		int leds_per_split = 0;
		long offset = 0;
		dir_t dir = DIR_FORWARDS;
		unsigned int intensity = 0;

		void do_iteration() {
			int current_split_start = 0;
			for (int i = 0; i < split_colors_len; i++, current_split_start += leds_per_split) {
				for (int j = 0; j < leds_per_split && j < NUM_LEDS; j++) {
					leds[(current_split_start + j + offset + NUM_LEDS) % NUM_LEDS] = split_colors[i];
				}
			}

			// If there are any left, set them as well
			if (current_split_start + split_colors_len < NUM_LEDS) {
				for (int i = current_split_start + split_colors_len; i < NUM_LEDS; i++) {
					leds[(i + offset + NUM_LEDS) % NUM_LEDS] = split_colors[0];
				}
			}

			if (update_time != 0) {
				Util::apply_change(dir, &offset);
			}

			if (intensity == 0) {
				FastLED.show(Power::get_scale());
			} else {
				FastLED.show(intensity);
			}
		}

		void handle_serial(const String serial_data[MAX_ARG_LEN]) {
			intensity = atoi(serial_data[2].c_str());
			update_time = atoi(serial_data[3].c_str());
			dir = atoi(serial_data[4].c_str()) == 0 ? DIR_BACKWARDS : DIR_FORWARDS;
			split_colors_len = 0;

			for (int i = 5; i < MAX_ARG_LEN && serial_data[i].c_str()[0] != '\\'; i += 3) {
				split_colors[split_colors_len++] = CRGB(
					atoi(serial_data[i].c_str()),
					atoi(serial_data[i + 1].c_str()),
					atoi(serial_data[i + 2].c_str())	
				);
			}
			
			leds_per_split = NUM_LEDS / split_colors_len;

			if (split_colors_len % 2 != 0) {
				Serial.println("Number of split colors needs to be even");
			}

			iterate_fn = do_iteration;
			if (update_time != 0) {
				mode_update_time = update_time;
			} else {
				mode_update_time = 1000;
			}
			cur_mode = Modes::LED_MODE_SPLIT;
		}
	}

	namespace Pattern {
		CRGB pattern_colors[MAX_PATTERN_LEN];
		int pattern_len = 0;
		int update_time = 0;
		long offset = 0;
		dir_t dir = DIR_FORWARDS;

		void do_iteration() {
			for (int i = 0; i < NUM_LEDS; i += pattern_len) {
				for (int j = 0; j < pattern_len; j++) {
					leds[(j + i + offset + NUM_LEDS) % NUM_LEDS] = pattern_colors[j];
				}
			}

			if (update_time != 0) {
				Util::apply_change(dir, &offset);
			}

			FastLED.show();
		}

		void handle_serial(const String serial_data[MAX_ARG_LEN]) {
			update_time = atoi(serial_data[2].c_str());
			dir = atoi(serial_data[3].c_str()) == 0 ? DIR_BACKWARDS : DIR_FORWARDS;
			pattern_len = 0;

			for (int i = 4; i < MAX_ARG_LEN && serial_data[i].c_str()[0] != '\\'; i += 3) {
				pattern_colors[pattern_len++] = CRGB(
					atoi(serial_data[i].c_str()),
					atoi(serial_data[i + 1].c_str()),
					atoi(serial_data[i + 2].c_str())	
				);
			}
			
			iterate_fn = do_iteration;
			if (update_time != 0) {
				mode_update_time = update_time;
			} else {
				mode_update_time = 1000;
			}
			cur_mode = Modes::LED_MODE_PATTERN;
		}
	}

	namespace Prime {
		void do_iteration() {
			if (SerialControl::new_data) {
				char* str = SerialControl::received_chars;

				// 3 * 2 hex chars
				if (strlen(str) != 6) {
					return;
				}

				CRGB color;
				char* cur_part = str + 4;
				color.b = strtol(cur_part, NULL, 16);
				cur_part -= 2;
				cur_part[2] = '\0';
				color.g = strtol(cur_part, NULL, 16);
				cur_part -= 2;
				cur_part[2] = '\0';
				color.r = strtol(cur_part, NULL, 16);

				for (int i = 0; i < NUM_LEDS; i++) {
					leds[i] = color;
				}

				FastLED.show(Power::get_scale());
			}
		}

		void handle_serial(const String serial_data[MAX_ARG_LEN]) {
			FastLED.showColor(CRGB::Black);

			iterate_fn = do_iteration;
			mode_update_time = 0;
			cur_mode = Modes::LED_MODE_PRIMED;
		}
	}
}