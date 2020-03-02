#include "../include/modes.h"
#include "../include/globals.h"
#include "../include/power.h"
#include "../include/serial_control.h"
#include "../include/util.h"

#define MAX_DOTS 20
#define MAX_SPLIT_COLORS 10
#define MAX_PATTERN_LEN 20
#define MAX_FLASH_LEN 256
#define BEAT_MAX_DURATION 200
#define BEAT_DURATION_SCALE_SIZE 400
#define MAX_CONFIDENCE 70
#define MIN_CONFIDENCE 35

#define BEAT_CHUNK_BEATS 10000
#define BEAT_CHUNK_SIZE BEAT_CHUNK_BEATS * 3

// #define DEBUG_BEATS 1

namespace Modes {
    Modes::led_mode_t cur_mode = Modes::LED_MODE_OFF;

    void (*iterate_fn)(void) = NULL;
    void (*serial_override)(void) = NULL;
    unsigned long mode_update_time = 1000UL * 1UL;

    bool force_update = true;

    namespace Off {
        const unsigned long update_time = 1000UL * 1UL;

        void do_iteration() { FastLED.showColor(CRGB::Black); }

        void handle_serial(words_t* words) {
            serial_override = NULL;
            iterate_fn = do_iteration;
            mode_update_time = update_time;
            cur_mode = Modes::LED_MODE_OFF;
        }

        void help() { Serial.println("No args"); }
    }  // namespace Off

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

        void handle_serial(words_t* words) {
            serial_override = NULL;
            intensity = atoi(words->text[2]);
            color = CRGB(atoi(words->text[3]), atoi(words->text[4]),
                         atoi(words->text[5]));
            scale = Power::get_scale(color);

            iterate_fn = do_iteration;
            mode_update_time = update_time;
            cur_mode = Modes::LED_MODE_SOLID;
        }

        void help() { Serial.println("/ solid [intensity] [r] [g] [b] \\"); }
    }  // namespace Solid

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
        dot_t dots[MAX_DOTS] = {{.dot_size = 0, .dot_speed = 0, .dot_pos = 0}};
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
                        dot->dot_pos =
                            ((dot->dot_pos - 1) + NUM_LEDS) % NUM_LEDS;
                    }
                    dot->last_move = millis();
                }

                for (unsigned int j = dot->dot_pos;
                     j < dot->dot_pos + dot->dot_size; j++) {
                    leds[j % NUM_LEDS] = dot->dot_color;
                }
            }

            if (intensity == 0) {
                FastLED.show(Power::get_scale());
            } else {
                FastLED.show(intensity);
            }
        }

        void handle_serial(words_t* words) {
            serial_override = NULL;
            intensity = atoi(words->text[2]);

            // Set background color
            bg_color = CRGB(atoi(words->text[3]), atoi(words->text[4]),
                            atoi(words->text[5]));

            // Parse the dots
            int dot_index = 0;
            for (int i = 6; i < ARG_BLOCK_LEN && words->text[i][0] != '\\';
                 i += 7) {
                dot_t* dot = &dots[dot_index];
                // First get the size
                dot->dot_size = atoi(words->text[i]);
                // Then the speed
                dot->dot_speed = atoi(words->text[i + 1]);
                // Then the direction
                dot->dir = atoi(words->text[i + 2]) == 0 ? DIR_BACKWARDS
                                                         : DIR_FORWARDS;
                // Then the position as a percentage
                int pos_percent = atoi(words->text[i + 3]);
                dot->dot_pos =
                    (unsigned int)(((float)pos_percent * NUM_LEDS) / 100);
                // Then the color
                dot->dot_color =
                    CRGB(atoi(words->text[i + 4]), atoi(words->text[i + 5]),
                         atoi(words->text[i + 6]));
                dot_index++;
            }
            // Unset all other dots
            for (int i = dot_index; i < MAX_DOTS; i++) {
                dots[i].dot_size = 0;
            }

            iterate_fn = do_iteration;
            mode_update_time = update_time;
            cur_mode = Modes::LED_MODE_OFF;
        }

        void help() {
            Serial.println(
                "/ dot [intensity] [bg_r] [bg_g] [bg_b] ...[[dot_size] "
                "[dot_speed(ms)] [dot_dir] [dot_pos(percentage)] [dot_r] "
                "[dot_g] [dot_b]] \\");
        }
    }  // namespace Dot

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
            for (int i = 0; i < split_colors_len;
                 i++, current_split_start += leds_per_split) {
                for (int j = 0; j < leds_per_split && j < NUM_LEDS; j++) {
                    leds[(current_split_start + j + offset + NUM_LEDS) %
                         NUM_LEDS] = split_colors[i];
                }
            }

            // If there are any left, set them as well
            if (current_split_start + split_colors_len < NUM_LEDS) {
                for (int i = current_split_start + split_colors_len;
                     i < NUM_LEDS; i++) {
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

        void handle_serial(words_t* words) {
            serial_override = NULL;
            intensity = atoi(words->text[2]);
            update_time = atoi(words->text[3]);
            dir = atoi(words->text[4]) == 0 ? DIR_BACKWARDS : DIR_FORWARDS;
            split_colors_len = 0;

            for (int i = 5; i < ARG_BLOCK_LEN && words->text[i][0] != '\\';
                 i += 3) {
                split_colors[split_colors_len++] =
                    CRGB(atoi(words->text[i]), atoi(words->text[i + 1]),
                         atoi(words->text[i + 2]));
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
            Serial.println(
                "/ split [intensity] [update_time(ms)] [dir] ...[[r] [g] [b]]] "
                "\\");
        }
    }  // namespace Split

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
            for (unsigned int i = 0; i < NUM_LEDS;
                 i += pattern_len * block_size) {
                for (unsigned int j = 0; j < pattern_len; j++) {
                    int pattern_total_offset = i + (j * block_size);
                    for (unsigned int k = 0; k < block_size; k++) {
                        leds[pattern_total_offset + k] = offset_colors[j];
                    }
                }
            }

            if (update_time != 0) {
                Util::apply_change(DIR_FORWARDS, &offset, 1, pattern_len);
            }

            if (intensity == 0) {
                FastLED.show(Power::get_scale());
            } else {
                FastLED.show(intensity);
            }
        }

        void handle_serial(words_t* words) {
            serial_override = NULL;
            intensity = atoi(words->text[2]);
            update_time = atoi(words->text[3]);
            dir = atoi(words->text[4]) == 0 ? DIR_BACKWARDS : DIR_FORWARDS;
            block_size = atoi(words->text[5]);
            pattern_len = 0;

            for (int i = 6; i < ARG_BLOCK_LEN && words->text[i][0] != '\\';
                 i += 3) {
                pattern_colors[pattern_len++] =
                    CRGB(atoi(words->text[i]), atoi(words->text[i + 1]),
                         atoi(words->text[i + 2]));
            }
            for (unsigned int i = 0; i < pattern_len; i++) {
                pattern_colors[pattern_len + i] =
                    CRGB(pattern_colors[i].r, pattern_colors[i].g,
                         pattern_colors[i].b);
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
            Serial.println(
                "/ pattern [intensity] [update_time(ms)] [dir] [block_size] "
                "...[[r] [g] [b]] \\");
        }
    }  // namespace Pattern

    namespace Prime {
        void override_serial() {
            // It will likely only concern the first block
            char* str = SerialControl::char_blocks[0];

            // 3 * 2 hex chars
            if (strnlen(str, ARG_BLOCK_LEN) != 6) {
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

            SerialControl::clear_char_buffers();
        }

        void do_iteration() {}

        void handle_serial(words_t* words) {
            serial_override = override_serial;
            FastLED.showColor(CRGB::Black);

            iterate_fn = do_iteration;
            mode_update_time = 0;
            cur_mode = Modes::LED_MODE_PRIMED;
        }

        void help() { Serial.println("/ prime \\"); }
    }  // namespace Prime

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
        int flash_index = 0;
        flash_mode_t flash_mode = FLASH_MODE_JUMP;
        unsigned int intensity = 0;

        unsigned int fade_index = 0;
        int amount_per_strobe = 0;

        CRGB fade_color(CRGB current, CRGB next, int progress) {
            CRGB color;
            float p = progress / ((float)update_time - 1);
            float invert_p = 1 - p;
            color.r = (uint8_t)(invert_p * current.r) + (p * next.r);
            color.g = (uint8_t)(invert_p * current.g) + (p * next.g);
            color.b = (uint8_t)(invert_p * current.b) + (p * next.b);
            return color;
        }

        unsigned int last_loop_iter = millis();
        void iter_fade() {
            // Fade from color to next color
            unsigned int color_index_next = (color_index + 1) % flash_vec_len;
            CRGB color = fade_color(flash_vec[color_index],
                                    flash_vec[color_index_next], fade_index);
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
            if (flash_index == 0) {
                if (flash_vec_len == 0) {
                    color = CRGB::White;
                    overridable_intensity = white_scale;
                } else {
                    color = flash_vec[color_index];
                    overridable_intensity = overridable_intensity == 0
                                                ? scales[color_index]
                                                : overridable_intensity;
                    color_index = (color_index + 1) % flash_vec_len;
                }
            } else {
                color = CRGB::Black;
                overridable_intensity = 255;
            }
            flash_index++;
            if (flash_index >= amount_per_strobe) {
                flash_index = 0;
            }

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

        void handle_serial(words_t* words) {
            serial_override = NULL;
            color_index = 0;
            flash_index = false;
            last_loop_iter = millis();
            intensity = atoi(words->text[2]);
            update_time = atol(words->text[3]);
            amount_per_strobe = atoi(words->text[4]);
            if (WORD_EQ(words, 5, "jump")) {
                flash_mode = FLASH_MODE_JUMP;
            } else if (WORD_EQ(words, 5, "fade")) {
                flash_mode = FLASH_MODE_FADE;
            } else if (WORD_EQ(words, 5, "strobe")) {
                flash_mode = FLASH_MODE_STROBE;
            } else {
                Serial.println("Invalid mode");
                return;
            }

            flash_vec_len = 0;
            for (int i = 6; i < ARG_BLOCK_LEN && words->text[i][0] != '\\';
                 i += 3) {
                flash_vec[flash_vec_len] =
                    CRGB(atoi(words->text[i]), atoi(words->text[i + 1]),
                         atoi(words->text[i + 2]));
                scales[flash_vec_len] =
                    Power::get_scale(flash_vec[flash_vec_len]);
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
                    mode_update_time = update_time > 1 ? update_time / 2 : 1;
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
            Serial.println(
                "/ flash [intensity] [update_time(ms)] [amount_per_strobe] "
                "[mode(jump|fade|strobe)] ...[[r] [g] [b]] \\");
        }
    }  // namespace Flash

    namespace Rainbow {
        unsigned int update_time = 0;
        long offset = 0;
        unsigned int last_loop_iter = millis();
        unsigned int step = 1;

        const float delta = (float)255 / (float)NUM_LEDS;

        void paint_rainbow() {
            float total = 0;
            long led_offset = offset;

            for (int i = 0; i < NUM_LEDS;
                 i++, total += delta, led_offset += 1) {
                if (led_offset >= NUM_LEDS) {
                    led_offset = led_offset % NUM_LEDS;
                }

                leds[led_offset].setHSV(min(255, round(total)), 255, 255);
            }

            FastLED.show(255);
        }

        void do_iteration() {
            if (update_time == 0) return;

            paint_rainbow();
            Util::apply_change(Modes::DIR_FORWARDS, &offset, step);
        }

        void handle_serial(words_t* words) {
            serial_override = NULL;
            update_time = atoi(words->text[2]);
            if (update_time == 0) {
                mode_update_time = 1000;
            } else {
                mode_update_time = update_time;
            }
            step = atoi(words->text[3]);
            if (step == 0) {
                step = 1;
            }

            iterate_fn = do_iteration;
            cur_mode = Modes::LED_MODE_RAINBOW;
            offset = 0;

            paint_rainbow();
        }

        void help() { Serial.println("/ rainbow [update_time(ms)] [step] \\"); }
    }  // namespace Rainbow

    namespace Random {
        unsigned int update_time = 0;
        unsigned int block_size = 1;

        void do_iteration() {
            for (unsigned int i = 0; i < NUM_LEDS;) {
                CHSV color = CHSV(random(0, 255), 255, 255);

                for (unsigned int j = 0; j < block_size && i < NUM_LEDS;
                     j++, i++) {
                    leds[i] = color;
                }
            }
            FastLED.show();
        }

        void handle_serial(words_t* words) {
            serial_override = NULL;
            update_time = atoi(words->text[2]);
            mode_update_time = update_time;
            block_size = atoi(words->text[3]);
            if (block_size == 0) {
                block_size = 1;
            }

            iterate_fn = do_iteration;
            cur_mode = Modes::LED_MODE_RANDOM;
        }

        void help() {
            Serial.println("/ rainbow [update_time(ms)] [block_size] \\");
        }

    }  // namespace Random

    namespace Beats {
        typedef struct beat_struct {
            unsigned long start;
            unsigned long end;
            int confidence;
        } beat_struct_t;

        // Pre-provided configs
        CRGB background_color = CRGB::Black;
        CRGB foreground_color = CRGB::Black;
        CRGB progress_color = CRGB::Black;
        bool progress_disabled = false;
        bool random_mode = false;
        unsigned int block_size = 0;

        // "runtime" configs
        bool is_playing = false;
        unsigned long duration = 0;
        long long start_time_diff = 0;
        int total_beats = 0;
        int beat_write_index = 0;
        beat_struct_t* beats = NULL;

        // Runtime configs
        long long last_playing_time = 0;
        int beat_run_index = 0;
        int last_random_beat_index = -1;

        int get_current_beat(long long playing_time, beat_struct_t* beat, int* index) {
            if (is_playing) {
                int upperbound = min(beat_write_index, total_beats);
                while (beat_run_index < upperbound &&
                       playing_time > beats[beat_run_index].end) {
                    beat_run_index++;
                }

                if (beat_run_index < upperbound &&
                    beats[beat_run_index].confidence > MIN_CONFIDENCE) {
                    beat_struct_t current_beat = beats[beat_run_index];
                    long long play_diff = playing_time - current_beat.start;
                    if (play_diff >= 0) {
                        if (index != NULL) {
                            *index = beat_run_index;
                        }
                        if (beat != NULL) {
                            *beat = current_beat;
                        }
                        return 0;
                    } else {
                        // Not inside of the beat
                    }
                }
            }
            return 1;
        }

        void do_flash_iteration() {
            // Draw background
            for (int i = 0; i < NUM_LEDS; i++) {
                leds[i] = background_color;
            }

            // Say at time 3 i'm sent that start_time_diff = 5
            // then it started at -2
            // that means at time 4, it has been playing for 6 (2 + 4)
            // so to calculate playing_time, do millis() + X
            // where X is the time that it started before millis() existed
            // so if initially, millis() < start_time_diff, set
            // X to start_time_diff - millis()
            // if millis() >= start_time_diff then
            //
            // Say at time 5 i'm sent that start_time_diff = 3
            // then it started at 2
            // that means that at time 6 it has been playing for 4
            // which is equal to millis() + X
            // where X is start_time_diff - millis()
            long long playing_time = (long long)millis() + start_time_diff;

            // Draw progress bar
            if (!progress_disabled && start_time_diff != 0) {
                if (is_playing) {
                    last_playing_time = playing_time;
                }

                float percentage_played;
                if (last_playing_time && duration) {
                    percentage_played =
                        (float)last_playing_time / (float)duration;
                } else {
                    percentage_played = 0;
                }

                int leds_amount = (int)((float)NUM_LEDS * percentage_played);
                for (int i = 0; i < min(leds_amount, NUM_LEDS); i++) {
                    leds[i] = progress_color;
                }
            }

            beat_struct_t current_beat;
            if (get_current_beat(playing_time, &current_beat, NULL) == 0) {
                // Inside of the beat
                long long play_diff = playing_time - current_beat.start;
                float confidence_scale =
                    (float)min(MAX_CONFIDENCE, current_beat.confidence) /
                    (float)MAX_CONFIDENCE;
                unsigned long beat_duration =
                    current_beat.end - current_beat.start;
                float fade_scale =
                    1.0 - ((float)play_diff / (float)beat_duration);

                float brightness = max(confidence_scale * fade_scale, 0);
                float inverse_brightness = (1 - brightness) / 4;
                CRGB foreground_copy = CRGB(foreground_color);
                foreground_copy.nscale8(max(brightness, 0.4) * 256);
#ifdef DEBUG_BEATS
                printf(
                    "Beat, brightness: %.6f, inverse: %.6f. Duration: "
                    "%lu. Diff: %lld, index: %d. Confidence: %d\n",
                    brightness, inverse_brightness, beat_duration, play_diff,
                    beat_run_index, current_beat.confidence);
#endif

                for (int i = 0; i < NUM_LEDS; i++) {
                    leds[i].nscale8(inverse_brightness * 256.0);
                    leds[i] += foreground_copy;
                }
            }

            FastLED.show();
        }

        void do_random_iteration() {
            long long playing_time = (long long)millis() + start_time_diff;

            int index = 0;
            if (get_current_beat(playing_time, NULL, &index) == 0 && index != last_random_beat_index) {
                for (unsigned int i = 0; i < NUM_LEDS;) {
                    CHSV color = CHSV(random(0, 255), 255, 255);

                    for (unsigned int j = 0; j < block_size && i < NUM_LEDS;
                         j++, i++) {
                        leds[i] = color;
                    }
                }

                last_random_beat_index = index;

                FastLED.show();
            }
        }

        void do_iteration() {
            if (random_mode) {
                do_random_iteration();
            } else {
                do_flash_iteration();
            }
        }

        const char START_MARKER = '/';
        const char END_MARKER = '\n';

        typedef enum SERIAL_STATE {
            INITIAL,
            B_RECEIVED,
            PLAYSTATE,
            PLAYSTART,
            INPUT_DURATION,
            BEATS_START,
            BEATS_READING
        } serial_state_t;

        typedef enum BEAT_STRUCT_INDEX {
            START,
            DURATION,
            CONFIDENCE
        } beat_struct_index_t;

        void increment_char_index(int* block_index, int* char_index, int* err) {
            *char_index = *char_index + 1;
            if (*char_index >= ARG_BLOCK_LEN) {
                *char_index = 0;
                *block_index = *block_index + 1;
                if (*block_index >= MAX_ARG_BLOCKS) {
                    printf("ERR: Max blocks exceeded\n");
                    *block_index = 0;
                    *err = 1;
                }
                if (SerialControl::char_blocks[*block_index] == NULL) {
                    printf("ERR: Reading from uninitialized block at %d, %d\n",
                           *block_index, *char_index);
                    *block_index = 0;
                    *err = 1;
                }
            }
        }

        unsigned long read_num(int* block_index, int* char_index, int* err) {
            char num_buf[ARG_BLOCK_LEN] = {0};
            int index = 0;

            if (*block_index >= MAX_ARG_BLOCKS) return 0;

            char c = SerialControl::char_blocks[*block_index][*char_index];
            if (c == ',' || c == ' ') return 0;

            increment_char_index(block_index, char_index, err);

            while (c != ',' && c != ' ') {
                num_buf[index++] = c;

                c = SerialControl::char_blocks[*block_index][*char_index];

                increment_char_index(block_index, char_index, err);
            }

            num_buf[index] = '\0';
            return strtoul(num_buf, NULL, 10);
        }

        void on_serial() {
            int block_index = 0;
            // Ignore the first 2 chars
            int char_index = 2;

            char first_char =
                SerialControl::char_blocks[block_index][char_index++];
            if (first_char != 'b') {
                return;
            }

            char input_mode =
                SerialControl::char_blocks[block_index][char_index++];
            if (input_mode == 'p') {
                // Playmode
                char play_mode =
                    SerialControl::char_blocks[block_index][char_index++];
                if (play_mode == '1') {
                    is_playing = true;
                } else {
                    is_playing = false;
                }
            } else if (input_mode == 's') {
                // Start
                int err = 0;
                unsigned long num = read_num(&block_index, &char_index, &err);
                if (err) return SerialControl::signal_read();
                ;
                start_time_diff = (long long)num - (long long)millis();
                beat_run_index = 0;
            } else if (input_mode == 'd') {
                // Duration
                int err = 0;
                unsigned long num = read_num(&block_index, &char_index, &err);
                if (err) return SerialControl::signal_read();
                ;
                duration = num;
                last_playing_time = 0;
                beat_run_index = 0;
            } else if (input_mode == 'b') {
                int err = 0;
                if (SerialControl::char_blocks[block_index][char_index] ==
                    '+') {
                    char_index++;

                    beat_write_index =
                        (int)read_num(&block_index, &char_index, &err);
                } else {
                    // Free previous one
                    if (beats != NULL) {
                        free(beats);
                    }

                    // Allocate array
                    total_beats =
                        (int)read_num(&block_index, &char_index, &err);
                    if (err) return SerialControl::signal_read();
                    ;
                    beats = (beat_struct_t*)malloc(sizeof(beat_struct_t) *
                                                   total_beats);
                    beat_write_index = 0;
                }
                beat_run_index = 0;

                int beat_end =
                    min(beat_write_index + BEAT_CHUNK_BEATS, total_beats);
                for (; beat_write_index < beat_end; beat_write_index++) {
                    beats[beat_write_index].start =
                        read_num(&block_index, &char_index, &err);
                    if (err) return SerialControl::signal_read();
                    ;

                    unsigned long duration =
                        read_num(&block_index, &char_index, &err);
                    if (err) return SerialControl::signal_read();
                    ;
                    beats[beat_write_index].end =
                        beats[beat_write_index].start + duration;

                    int confidence =
                        (int)read_num(&block_index, &char_index, &err);
                    if (err) return SerialControl::signal_read();
                    ;
                    beats[beat_write_index].confidence = confidence;
                }

                printf("Read %d beats at this point. That is %ld seconds\n",
                       beat_write_index,
                       beats[beat_write_index - 1].start / 1000);
            } else {
                // No match, return
                return;
            }

            Serial.println("ack");

            SerialControl::signal_read();
        }

        void handle_serial(words_t* words) {
            FastLED.showColor(CRGB::Black);
            if (atoi(words->text[2]) == 1) {
                random_mode = true;
                block_size = atoi(words->text[3]);
                foreground_color = CRGB::Black;
                background_color = CRGB::Black;
                progress_color = CRGB::Black;
            } else {
                random_mode = false;
                block_size = 0;
                foreground_color =
                    CRGB(atoi(words->text[4]), atoi(words->text[5]),
                         atoi(words->text[6]));

                background_color =
                    CRGB(atoi(words->text[7]), atoi(words->text[8]),
                         atoi(words->text[9]));

                progress_color =
                    CRGB(atoi(words->text[10]), atoi(words->text[11]),
                         atoi(words->text[12]));
                if (progress_color.r == 0 && progress_color.g == 0 &&
                    progress_color.b == 0) {
                    progress_disabled = true;
                } else {
                    progress_disabled = false;
                }
            }

            iterate_fn = do_iteration;
            mode_update_time = 0;
            cur_mode = Modes::LED_MODE_BEATS;
            serial_override = on_serial;
        }

        void help() {
            Serial.println(
                "/ beats [random] [block_size] [r] [g] [b] [bg_r] [bg_g] "
                "[bg_b] [prog_r] "
                "[prog_g] "
                "[prog_b] \\");
        }
    }  // namespace Beats
}  // namespace Modes