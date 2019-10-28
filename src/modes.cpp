#include <serial_control.h>
#include <Arduino.h>
#include <globals.h>
#include <power.h>
#include <modes.h>
#include <util.h>

#define MAX_DOTS 5
#define MAX_SPLIT_COLORS 10
#define MAX_PATTERN_LEN 20
#define MAX_FLASH_LEN 256

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

		void help() {
			Serial.println("No args");
		}
	}

	namespace Solid {
		const unsigned long update_time = 1000UL * 1UL;
		CRGB color = CRGB::Black;
		unsigned int intensity = 0;
		uint8_t scale;

		void do_iteration() {
			if (intensity == 0) {
				FastLED.showColor(color, scale);
			} else {
				FastLED.showColor(color, intensity);
			}
		}

		void handle_serial(const String serial_data[MAX_ARG_LEN]) {
			intensity = atoi(serial_data[2].c_str());
			color = CRGB(
				atoi(serial_data[3].c_str()),
				atoi(serial_data[4].c_str()),
				atoi(serial_data[5].c_str())
			);
			scale = Power::get_scale(color);

			iterate_fn = do_iteration;
			mode_update_time = update_time;
			cur_mode = Modes::LED_MODE_SOLID;
		}

		void help() {
			Serial.println("/ solid [intensity] [r] [g] [b] \\");
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

		void help() {
			Serial.println("/ dot [intensity] [bg_r] [bg_g] [bg_b] ...[[dot_size] [dot_speed(ms)] [dot_dir] [dot_pos(percentage)] [dot_r] [dot_g] [dot_b]] \\");
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

		void help() {
			Serial.println("/ split [intensity] [update_time(ms)] [dir] ...[[r] [g] [b]]] \\");
		}
	}

	namespace Pattern {
		CRGB pattern_colors[MAX_PATTERN_LEN * 2];
		unsigned int pattern_len = 0;
		unsigned int update_time = 0;
		long offset = 0;
		dir_t dir = DIR_FORWARDS;
		unsigned int intensity = 0;
		unsigned int block_size = 0;

		CRGB* get_offset_colors() {
			if (dir == DIR_FORWARDS) {
				// Go back in the colors
				return &pattern_colors[pattern_len - offset];
			} else {
				// Go forwards in the copied colors
				return &pattern_colors[offset];
			}
		}

		void do_iteration() {
			CRGB* offset_colors = get_offset_colors();
			for (unsigned int i = 0; i < NUM_LEDS; i += pattern_len * block_size) {
				for (unsigned int j = 0; j < pattern_len; j++) {
					int pattern_total_offset = i + (j * block_size);
					for (unsigned int k = 0; k < block_size; k++) {
						leds[pattern_total_offset + k] = offset_colors[j];
					}
				}
			}

			if (update_time != 0) {
				Util::apply_change(DIR_FORWARDS, &offset, pattern_len);
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
			block_size = atoi(serial_data[5].c_str());
			pattern_len = 0;

			for (int i = 6; i < MAX_ARG_LEN && serial_data[i].c_str()[0] != '\\'; i += 3) {
				pattern_colors[pattern_len++] = CRGB(
					atoi(serial_data[i].c_str()),
					atoi(serial_data[i + 1].c_str()),
					atoi(serial_data[i + 2].c_str())	
				);
			}
			for (unsigned int i = 0; i < pattern_len; i++) {
				pattern_colors[pattern_len + i] = CRGB(
					pattern_colors[i].r,
					pattern_colors[i].g,
					pattern_colors[i].b
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

		void help() {
			Serial.println("/ pattern [intensity] [update_time(ms)] [dir] [block_size] ...[[r] [g] [b]] \\");
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

				FastLED.showColor(color, Power::get_scale(color));
			}
		}

		void handle_serial(const String serial_data[MAX_ARG_LEN]) {
			FastLED.showColor(CRGB::Black);

			iterate_fn = do_iteration;
			mode_update_time = 0;
			cur_mode = Modes::LED_MODE_PRIMED;
		}

		void help() {
			Serial.println("/ prime \\");
		}
	}

	namespace Flash {
		typedef enum flash_mode {
			FLASH_MODE_JUMP,
			FLASH_MODE_FADE,
			FLASH_MODE_STROBE
		} flash_mode_t;

		unsigned long update_time = 1000UL * 1UL;
		CRGB flash_vec[MAX_FLASH_LEN];
		uint8_t scales[MAX_FLASH_LEN];
		unsigned int flash_vec_len;
		unsigned int color_index = 0;
		bool flash_index = 0;
		flash_mode_t flash_mode = FLASH_MODE_JUMP;
		unsigned int intensity = 0;

		unsigned int fade_index = 0;

		CRGB fade_color(CRGB current, CRGB next, int progress) {
			CRGB color;
			float p = progress / ((float)update_time - 1);
			float invert_p = 1 - p;
			color.r = (uint8_t) (invert_p * current.r) + (p * next.r) + 0.5;
			color.g = (uint8_t) (invert_p * current.g) + (p * next.g) + 0.5;
			color.b = (uint8_t) (invert_p * current.b) + (p * next.b) + 0.5;
			return color;
		}
		
		unsigned int last_loop_iter = millis();
		void iter_fade() {
			// Fade from color to next color
			unsigned int color_index_next = (color_index + 1) % flash_vec_len;
			CRGB color = fade_color(
				flash_vec[color_index],
				flash_vec[color_index_next],
				fade_index);
			unsigned int now = millis();
			fade_index += (now - last_loop_iter);
			last_loop_iter = now;
			if (fade_index >= update_time) {
				// Go to next color
				fade_index = 0;
				color_index = color_index_next;
			}

			if (intensity == 0) {
				FastLED.showColor(color, Power::get_scale(color));
			} else {
				FastLED.showColor(color, intensity);
			}
		}

		uint8_t white_scale = Power::get_scale(CRGB::White);
		void iter_strobe() {
			// Alternate color and black
			CRGB color;
			unsigned int overridable_intensity = intensity;
			if (flash_index == 1) {
				if (flash_vec_len == 0) {
					color = CRGB::White;
					overridable_intensity = white_scale;
				} else {
					color = flash_vec[color_index];
					overridable_intensity = overridable_intensity == 0 ?
						scales[color_index] : overridable_intensity;
					color_index = (color_index + 1) % flash_vec_len;
				}
			} else {
				color = CRGB::Black;
				overridable_intensity = 255;
			}
			flash_index = !flash_index;

			if (overridable_intensity == 0) {
				FastLED.showColor(color, Power::get_scale(color));
			} else {
				FastLED.showColor(color, overridable_intensity);
			}
		}

		void iter_jump() {
			// Jump to next color
			CRGB color = flash_vec[color_index];
			uint8_t scale = scales[color_index];
			color_index = (color_index + 1) % flash_vec_len;

			if (intensity == 0) {
				FastLED.showColor(color, scale);
			} else {
				FastLED.showColor(color, intensity);
			}
		}

		void handle_serial(const String serial_data[MAX_ARG_LEN]) {
			color_index = 0;
			flash_index = false;
			last_loop_iter = millis();
			intensity = atoi(serial_data[2].c_str());
			update_time = atol(serial_data[3].c_str());
			if (strcmp(serial_data[4].c_str(), "jump") == 0) {
				flash_mode = FLASH_MODE_JUMP;
			} else if (strcmp(serial_data[4].c_str(), "fade") == 0) {
				flash_mode = FLASH_MODE_FADE;
			} else if (strcmp(serial_data[4].c_str(), "strobe") == 0) {
				flash_mode = FLASH_MODE_STROBE;
			} else {
				Serial.println("Invalid mode");
				return;
			}

			flash_vec_len = 0;
			for (int i = 5; i < MAX_ARG_LEN && serial_data[i].c_str()[0] != '\\'; i += 3) {
				flash_vec[flash_vec_len] = CRGB(
					atoi(serial_data[i].c_str()),
					atoi(serial_data[i + 1].c_str()),
					atoi(serial_data[i + 2].c_str())
				);
				scales[flash_vec_len] = Power::get_scale(flash_vec[flash_vec_len]);
				flash_vec_len++;
			}

			if (flash_mode == FLASH_MODE_FADE && update_time == 0) {
				Serial.println("Update time 0 can't be used when fading");
				return;
			}

			switch (flash_mode) {
				case FLASH_MODE_FADE:
					mode_update_time = 0;
					fade_index = 0;
					iterate_fn = iter_fade;
					break;
				case FLASH_MODE_STROBE:
					mode_update_time = update_time / 2;
					iterate_fn = iter_strobe;
					break;
				case FLASH_MODE_JUMP:
					mode_update_time = update_time;
					iterate_fn = iter_jump;
					break;
			}
			cur_mode = Modes::LED_MODE_FLASH;
		}

		void help() {
			Serial.println("/ flash [intensity] [update_time(ms)] [mode(jump|fade|strobe)] ...[[r] [g] [b]] \\");
		}
	}
}