#include "stepper.h"

#include "ArduinoJson.h"
#include "FastLED.h"
#include "globals.h"

namespace Stepper {
    CRGB leds[NUM_LEDS] = {CRGB::Black};
    bool disabled = false;

    enum MOVING_STATUS {
        MOVING_STATUS_OFF,
        MOVING_STATUS_FORWARDS,
        MOVING_STATUS_BACKWARDS
    };

    enum COLOR_TYPE {
        COLOR_TYPE_SINGLE,
        COLOR_TYPE_SEQUENCE,
        COLOR_TYPE_RANDOM,
        COLOR_TYPE_TRANSPARENT,
        COLOR_TYPE_REPEAT
    };

    typedef struct led_single_color {
        CRGB color;
    } led_single_color_t;

    typedef struct led_transparent {
        short size;
    } led_transparent_t;

    typedef struct led_color_sequence {
        short num_colors;
        CRGB* colors;
        short repetitions;
    } led_color_sequence_t;

    typedef struct led_random_color {
        short size;
        bool random_every_time;
        short random_time;

        bool initialized = false;
        CRGB chosen_color;
        unsigned long last_color_change;
    } led_random_color_t;

    typedef struct led_sequence {
        COLOR_TYPE type;
        led_single_color_t single_color;
        led_color_sequence_t color_sequence;
        led_random_color_t random_color;
        led_transparent_t transparent;
    } led_sequence_t;

    typedef struct move_data {
        MOVING_STATUS moving;
        short jump_size;
        short jump_delay;
        bool alternate;
        short alternate_delay;
    } move_data_t;

    typedef struct led_step {
        short delay_until_next;
        move_data_t move_data;
        CRGB background;
        short num_sequences;
        led_sequence_t* sequences;
    } led_step_t;

    typedef struct led_effect {
        bool filled;
        short num_steps;
        led_step_t* steps;
    } led_effect_t;

    typedef struct led_step_data {
        led_step_t step;

        unsigned long last_jump;
        int current_offset;

        MOVING_STATUS step_direction;
        unsigned long last_alternate;
    } led_step_data_t;

    typedef struct led_step_meta {
        int current_step_index;
        unsigned long last_switch;

        led_step_data_t data;
    } led_step_meta_t;

    led_effect_t led_effect = {.filled = false};

    namespace Drawing {
        namespace Steps {
            namespace Current {
                led_step_meta_t step_meta;
                led_step_data_t* step_data = NULL;

                led_step_data_t* get_current_step() {
                    if (led_effect.num_steps == 1) {
                        return &step_data[0];
                    }

                    unsigned long time_diff =
                        step_meta.last_switch == 0
                            ? 0
                            : millis() - step_meta.last_switch;
                    led_step_t current_step = step_meta.data.step;
                    if (time_diff >=
                        (unsigned long)current_step.delay_until_next) {
                        // Bump index
                        step_meta.current_step_index =
                            (step_meta.current_step_index + 1) %
                            led_effect.num_steps;

                        // Load new step
                        step_meta.data =
                            step_data[step_meta.current_step_index];
                        step_meta.last_switch = millis();
                    }
                    return &step_data[step_meta.current_step_index];
                }

                void init() {
                    if (step_data) {
                        free(step_data);
                    }

                    step_data = (led_step_data_t*)malloc(
                        sizeof(led_step_data_t) * led_effect.num_steps);
                    for (int i = 0; i < led_effect.num_steps; i++) {
                        led_step_data_t data;
                        data.step = led_effect.steps[i];
                        data.current_offset = 0;
                        data.last_alternate = millis();
                        data.step_direction = data.step.move_data.moving;
                        data.last_alternate = millis();
                        step_data[i] = data;
                    }

                    step_meta.last_switch = millis();
                    step_meta.current_step_index = 0;
                    step_meta.data = step_data[0];
                }
            }  // namespace Current

            namespace Leds {
                CRGB layout_leds[NUM_LEDS];

                CRGB* layout(led_step_data_t* current_step) {
                    // Draw the background
                    for (int i = 0; i < NUM_LEDS; i++) {
                        layout_leds[i] = current_step->step.background;
                    }

                    // Draw the rest
                    int led_index = 0;
                    for (int i = 0; i < current_step->step.num_sequences; i++) {
                        led_sequence_t sequence =
                            current_step->step.sequences[i];
                        if (sequence.type == COLOR_TYPE_SINGLE) {
                            layout_leds[led_index++] =
                                sequence.single_color.color;
                        } else if (sequence.type == COLOR_TYPE_RANDOM) {
                            led_random_color_t random_color_config =
                                sequence.random_color;
                            if (random_color_config.random_every_time) {
                                if (millis() -
                                        random_color_config.last_color_change >=
                                    (unsigned long)
                                        random_color_config.random_time) {
                                    CHSV color = CHSV(random(0, 255), 255, 255);
                                    random_color_config.chosen_color = color;
                                }
                            } else if (!random_color_config.initialized) {
                                // Random once and not chosen yet,
                                // choose one now
                                CHSV color = CHSV(random(0, 255), 255, 255);
                                random_color_config.initialized = true;
                                random_color_config.chosen_color = color;
                            }
                            for (int j = 0; j < random_color_config.size; j++) {
                                layout_leds[led_index++] =
                                    random_color_config.chosen_color;
                            }
                        } else if (sequence.type == COLOR_TYPE_SEQUENCE) {
                            led_color_sequence_t color_sequence =
                                sequence.color_sequence;
                            for (int j = 0; j < color_sequence.repetitions;
                                 j++) {
                                for (int k = 0; k < color_sequence.num_colors;
                                     k++) {
                                    layout_leds[led_index++] =
                                        color_sequence.colors[k];
                                }
                            }
                        } else if (sequence.type == COLOR_TYPE_TRANSPARENT) {
                            led_transparent_t transparent =
                                sequence.transparent;
                            led_index += transparent.size;
                        }
                    }

                    return layout_leds;
                }
            }  // namespace Leds

            namespace Offset {
                CRGB offset_leds[NUM_LEDS];

                int get_wrapped_index(int index) {
                    if (index < 0) {
                        return NUM_LEDS + index;
                    }
                    return index % NUM_LEDS;
                }

                CRGB* do_offset(CRGB* layout, led_step_data_t* current_step) {
                    move_data_t move_data = current_step->step.move_data;
                    if (move_data.moving == MOVING_STATUS_OFF) {
                        return layout;
                    }

                    // Check if we should alternate
                    unsigned long now = millis();
                    if (move_data.alternate) {
                        // Check if we should change alternate direction
                        if (now - current_step->last_alternate >=
                            (unsigned long)move_data.alternate_delay) {
                            // Yep, change direction
                            if (current_step->step_direction ==
                                MOVING_STATUS_FORWARDS) {
                                current_step->step_direction =
                                    MOVING_STATUS_BACKWARDS;
                            } else {
                                current_step->step_direction =
                                    MOVING_STATUS_FORWARDS;
                            }
                            current_step->last_alternate = now;
                        }
                    }

                    // Check if we should increase the offset
                    if (now - current_step->last_jump >=
                        (unsigned long)move_data.jump_delay) {
                        // Yep
                        if (current_step->step_direction ==
                            MOVING_STATUS_FORWARDS) {
                            current_step->current_offset =
                                (current_step->current_offset +
                                 move_data.jump_size) %
                                NUM_LEDS;
                        } else {
                            current_step->current_offset =
                                (current_step->current_offset -
                                 move_data.jump_size) %
                                NUM_LEDS;
                        }
                        current_step->last_jump = now;
                    }

                    // Apply that change
                    if (current_step->current_offset == 0) {
                        return layout;
                    }

                    int offset = current_step->current_offset;
                    for (int i = 0; i < NUM_LEDS; i++) {
                        offset_leds[get_wrapped_index(i + offset)] = layout[i];
                    }

                    return offset_leds;
                }
            }  // namespace Offset

            namespace Draw {
                void draw(CRGB* offset_leds) {
                    for (int i = 0; i < NUM_LEDS; i++) {
                        leds[i] = offset_leds[i];
                    }
                    FastLED.show();
                }
            }  // namespace Draw

            void init() { Current::init(); }
        }  // namespace Steps

        void init() { Steps::init(); }

        void draw_blank() {
            for (int i = 0; i < NUM_LEDS; i++) {
                leds[i] = CRGB::Black;
            }
            FastLED.show();
        }

        void draw() {
            if (!led_effect.filled || disabled) {
                draw_blank();
                return;
            }

            // Check the current step and draw that
            led_step_data_t* current_step = Steps::Current::get_current_step();
            CRGB* layout_leds = Steps::Leds::layout(current_step);
            CRGB* offset_leds =
                Steps::Offset::do_offset(layout_leds, current_step);
            Steps::Draw::draw(offset_leds);
        }
    }  // namespace Drawing

    namespace Parser {
        CRGB parse_color(JsonArray doc) { return CRGB(doc[0], doc[1], doc[2]); }

        move_data_t read_move_data(JsonObject doc) {
            move_data_t move_data;
            byte move_status = doc["move_status"];
            move_data.moving = (MOVING_STATUS)move_status;
            move_data.jump_size = doc["jump_size"];
            move_data.jump_delay = doc["jump_delay"];
            move_data.alternate = (bool)doc["alternate"];
            move_data.alternate_delay = doc["alternate_delay"];
            return move_data;
        }

        led_sequence_t read_sequence(JsonObject doc) {
            led_sequence_t sequence;
            byte sequence_type = doc["type"];
            if (sequence_type == COLOR_TYPE_SINGLE) {
                sequence.type = COLOR_TYPE_SINGLE;
                led_single_color single_color;
                single_color.color = parse_color(doc["color"]);
                sequence.single_color = single_color;
            } else if (sequence_type == COLOR_TYPE_SEQUENCE) {
                sequence.type = COLOR_TYPE_SEQUENCE;
                led_color_sequence_t color_sequence;
                color_sequence.num_colors = doc["num_colors"];
                color_sequence.repetitions = doc["repetitions"];
                color_sequence.colors =
                    (CRGB*)malloc(sizeof(CRGB) * color_sequence.num_colors);
                for (int i = 0; i < color_sequence.num_colors; i++) {
                    color_sequence.colors[i] = parse_color(doc["colors"][i]);
                }
                sequence.color_sequence = color_sequence;
            } else if (sequence_type == COLOR_TYPE_RANDOM) {
                sequence.type = COLOR_TYPE_RANDOM;
                led_random_color_t random_color;
                random_color.random_every_time =
                    doc["color"]["random_every_time"];
                random_color.random_time = doc["color"]["random_time"];
                random_color.size = doc["color"]["size"];
                random_color.last_color_change = 0;
                sequence.random_color = random_color;
            } else if (sequence_type == COLOR_TYPE_TRANSPARENT) {
                sequence.type = COLOR_TYPE_TRANSPARENT;
                led_transparent_t transparent_color;
                transparent_color.size = doc["color"]["size"];
                sequence.transparent = transparent_color;
            } else {
                Serial.println("Found nested repeated sequence!");
            }
            return sequence;
        }

        led_sequence_t* read_sequences(JsonObject doc, short num_sequences) {
            led_sequence_t* sequences =
                (led_sequence_t*)malloc(sizeof(led_sequence_t) * num_sequences);

            int sequence_index = 0;
            for (int i = 0; sequence_index < num_sequences; i++) {
                byte sequence_type = doc["sequences"][i]["type"];
                if (sequence_type == COLOR_TYPE_REPEAT) {
                    short num_repetitions = doc["sequences"][i]["repetitions"];
                    led_sequence_t repeated_sequence =
                        read_sequence(doc["sequences"][i]["sequence"]);
                    for (int j = 0; j < num_repetitions; j++) {
                        sequences[sequence_index++] = repeated_sequence;
                    }
                } else {
                    sequences[sequence_index++] =
                        read_sequence(doc["sequences"][i]);
                }
            }
            return sequences;
        }

        led_step_t read_step(JsonObject doc) {
            led_step_t step;
            step.delay_until_next = doc["delay_until_next"];
            step.move_data = read_move_data(doc["move_data"]);
            step.background = parse_color(doc["background"]);
            step.num_sequences = doc["num_sequences"];

            step.sequences = read_sequences(doc, step.num_sequences);

            return step;
        }

        led_step_t* parse_steps(DynamicJsonDocument doc, int num_steps) {
            led_step_t* steps =
                (led_step_t*)malloc(sizeof(led_step_t) * num_steps);
            for (int i = 0; i < num_steps; i++) {
                steps[i] = read_step(doc["steps"][i]);
            }
            return steps;
        }

        bool parse(String data) {
            // Wild guess amount
            DynamicJsonDocument doc(8192);
            DeserializationError error = deserializeJson(doc, data.c_str());
            if (error.code() != error.Ok) {
                return false;
            }

            led_effect.num_steps = doc["num_steps"];
            led_effect.steps = parse_steps(doc, led_effect.num_steps);
            led_effect.filled = true;
            Drawing::init();
            disabled = false;

            return true;
        }
    }  // namespace Parser

    namespace Freeing {
        void free_led_effect() {
            if (!led_effect.filled) return;
            for (int i = 0; i < led_effect.num_steps; i++) {
                led_step_t step = led_effect.steps[i];
                for (int j = 0; j < step.num_sequences; j++) {
                    led_sequence_t sequence = step.sequences[j];
                    if (sequence.type == COLOR_TYPE_SEQUENCE) {
                        free(sequence.color_sequence.colors);
                        sequence.color_sequence.colors = NULL;
                    }
                }
                free(step.sequences);
                step.sequences = NULL;
            }
            free(led_effect.steps);
            led_effect.steps = NULL;
            led_effect.filled = false;
        }
    }  // namespace Freeing

    void setup() {
#ifndef MOCK
        FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS, 0);
#else
        FastLED.addLeds(leds, NUM_LEDS, 0);
#endif
        FastLED.setDither(0);
        randomSeed(analogRead(0));
    }

    bool set(String data) { return Parser::parse(data); }

    void loop() { Drawing::draw(); }

    void off() { disabled = true; }

    void on() { disabled = true; }

    bool is_on() { return !disabled; }
}  // namespace Stepper