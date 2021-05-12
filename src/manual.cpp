#include "../include/manual.h"

#include "../include/globals.h"

#define LOOP_DEFAULT_WAIT_TIME 100

const byte mock_data[] = {60, 0, 1, 0,   0, 0, 0,   0, 0, 0, 0, 0, 0, 0,
                          0,  0, 3, 132, 4, 3, 132, 2, 1, 0, 1, 0, 1, 62};
const int mock_data_len = sizeof(mock_data) / sizeof(byte);

namespace Manual {
    typedef struct led_step_meta {
        led_step_t step;
        int current_step_index;

        unsigned long last_jump;
        int current_offset;

        MOVING_STATUS step_direction;
        unsigned long last_alternate;
    } led_step_meta_t;

    led_effect_t led_effect = {.filled = false};

    namespace Parsing {
        const size_t max_len = (NUM_LEDS * 6) + 2;
        byte serial_data[max_len];

        namespace Reading {
            size_t read_offset;
            size_t max_offset = 0;

            size_t increment() {
                if (read_offset + 1 >= max_offset) {
                    Serial.println("Reading outside bounds, stopping");
                    return read_offset;
                }
                return ++read_offset;
            }

            short read_short() {
                byte high = serial_data[increment()];
                byte low = serial_data[increment()];
                return (high << 8) | low;
            }

            byte read_byte() { return serial_data[increment()]; }

            byte read_byte_no_increment() { return serial_data[read_offset]; }

            byte peek_byte() { return serial_data[read_offset + 1]; }

            CRGB read_color() {
                CRGB color;
                color.r = Reading::read_byte();
                color.g = Reading::read_byte();
                color.b = Reading::read_byte();
                return color;
            }
        }  // namespace Reading

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

        namespace Steps {
            namespace MoveData {
                move_data_t read_move_data() {
                    move_data_t move_data;
                    byte move_status = Reading::read_byte();
                    move_data.moving = (Manual::MOVING_STATUS)move_status;
                    move_data.jump_size = Reading::read_short();
                    move_data.jump_delay = Reading::read_short();
                    move_data.alternate = (bool)Reading::read_byte();
                    move_data.alternate_delay = Reading::read_short();
                    return move_data;
                }
            }  // namespace MoveData

            namespace Sequence {
                namespace Color {
                    CRGB* read_colors(short num_colors) {
                        CRGB* colors = (CRGB*)malloc(sizeof(CRGB) * num_colors);
                        for (int i = 0; i < num_colors; i++) {
                            colors[i] = Reading::read_color();
                        }
                        return colors;
                    }

                    led_single_color_t read_single_color() {
                        led_single_color_t single_color;
                        single_color.color = Reading::read_color();
                        return single_color;
                    }

                    led_color_sequence_t read_color_sequence() {
                        led_color_sequence_t color_sequence;
                        color_sequence.num_colors = Reading::read_short();
                        color_sequence.repetitions = Reading::read_short();
                        color_sequence.colors =
                            read_colors(color_sequence.num_colors);

                        return color_sequence;
                    }

                    led_random_color_t read_random_color() {
                        led_random_color_t random_color;
                        random_color.random_every_time = Reading::read_byte();
                        random_color.random_time = Reading::read_short();
                        random_color.size = Reading::read_short();
                        random_color.last_color_change = 0;
                        return random_color;
                    }

                    led_transparent_t read_transparent_color() {
                        led_transparent_t transparent;
                        transparent.size = Reading::read_short();
                        return transparent;
                    }
                }  // namespace Color

                led_sequence_t read_sequence() {
                    led_sequence_t sequence;
                    byte sequence_type = Reading::read_byte();
                    if (sequence_type == COLOR_TYPE_SINGLE) {
                        sequence.type = COLOR_TYPE_SINGLE;
                        sequence.single_color = Color::read_single_color();
                    } else if (sequence_type == COLOR_TYPE_SEQUENCE) {
                        sequence.type = COLOR_TYPE_SEQUENCE;
                        sequence.color_sequence = Color::read_color_sequence();
                    } else if (sequence_type == COLOR_TYPE_RANDOM) {
                        sequence.type = COLOR_TYPE_RANDOM;
                        sequence.random_color = Color::read_random_color();
                    } else if (sequence_type == COLOR_TYPE_TRANSPARENT) {
                        sequence.type = COLOR_TYPE_TRANSPARENT;
                        sequence.transparent = Color::read_transparent_color();
                    } else {
                        Serial.println("Found nested repeated sequence!");
                    }
                    return sequence;
                }

                led_sequence_t* read_sequences(short num_sequences) {
                    led_sequence_t* sequences = (led_sequence_t*)malloc(
                        sizeof(led_sequence_t) * num_sequences);

                    int sequence_index = 0;
                    for (int i = 0; sequence_index < num_sequences; i++) {
                        byte sequence_type = Reading::peek_byte();
                        if (sequence_type == COLOR_TYPE_REPEAT) {
                            sequence_type = Reading::read_byte();
                            short num_repetitions = Reading::read_short();
                            led_sequence_t repeated_sequence = read_sequence();
                            for (int j = 0; j < num_repetitions; j++) {
                                sequences[sequence_index++] = repeated_sequence;
                            }
                        } else {
                            sequences[sequence_index++] = read_sequence();
                        }
                    }
                    return sequences;
                }
            }  // namespace Sequence

            led_step_t read_step() {
                led_step_t step;
                step.delay_until_next = Reading::read_short();
                step.move_data = MoveData::read_move_data();
                step.background = Reading::read_color();
                step.num_sequences = Reading::read_short();

                step.sequences = Sequence::read_sequences(step.num_sequences);
                return step;
            }

            led_step_t* read_steps(short num_steps) {
                led_step_t* steps =
                    (led_step_t*)malloc(sizeof(led_step_t) * num_steps);
                for (int i = 0; i < num_steps; i++) {
                    steps[i] = read_step();
                }
                return steps;
            }
        }  // namespace Steps

        int parse_serial_data(int data_len) {
            Freeing::free_led_effect();

            Reading::read_offset = 0;
            Reading::max_offset = data_len;
            // First char should be <
            if ((char)Reading::read_byte_no_increment() != '<') {
                Serial.println("Failed to read starting character");
                return 1;
            }

            short num_steps = Reading::read_short();
            led_effect.num_steps = num_steps;
            led_effect.steps = Steps::read_steps(num_steps);
            led_effect.filled = true;

            // Last char should be >
            byte read_char = Reading::read_byte();
            if ((char)read_char != '>') {
                char text[500];
                sprintf(text,
                        "Failed to read end character, instead read: %d at "
                        "offset %u of max %d\n",
                        read_char, Parsing::Reading::read_offset, data_len);
                Serial.println(text);
                return 1;
            }
            Serial.println("# Success!");
            return 0;
        }

        void turn_off() { Freeing::free_led_effect(); }

        int read_serial() {
#ifndef MOCK
            if (!Serial.available()) {
                return 0;
            }
            memset(serial_data, 0, sizeof(serial_data));
            size_t read_bytes =
                Serial.readBytesUntil('>', serial_data, max_len);
            serial_data[read_bytes] = '>';
            return read_bytes + 1;
#else
            for (int i = 0; i < mock_data_len; i++) {
                serial_data[i] = mock_data[i];
            }
            return mock_data_len;
#endif
        }
    }  // namespace Parsing

    namespace Drawing {
        namespace Steps {
            namespace Current {
                unsigned long last_switch = 0;

                led_step_meta_t step_meta;

                void reset_step() {
                    step_meta.last_jump = millis();
                    step_meta.current_offset = 0;
                    step_meta.step_direction = step_meta.step.move_data.moving;
                    step_meta.last_alternate = millis();
                }

                led_step_meta_t* get_current_step() {
                    if (led_effect.num_steps == 1) {
                        return &step_meta;
                    }

                    unsigned long time_diff =
                        last_switch == 0 ? 0 : millis() - last_switch;
                    led_step_t current_step = step_meta.step;
                    if (time_diff >=
                        (unsigned long)current_step.delay_until_next) {
                        step_meta.current_step_index =
                            step_meta.current_step_index + led_effect.num_steps;
                        step_meta.step =
                            led_effect.steps[step_meta.current_step_index];
                        reset_step();
                        last_switch = millis();
                    }
                    return &step_meta;
                }

                void init() {
                    last_switch = 0;
                    step_meta.step = led_effect.steps[0];
                    step_meta.current_step_index = 0;
                    reset_step();
                }
            }  // namespace Current

            namespace Leds {
                CRGB layout_leds[NUM_LEDS];

                CRGB* layout(led_step_meta_t* current_step) {
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

                CRGB* do_offset(CRGB* layout, led_step_meta_t* current_step) {
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
            if (!led_effect.filled) {
                return;
            }

            // Check the current step and draw that
            led_step_meta_t* current_step = Steps::Current::get_current_step();
            CRGB* layout_leds = Steps::Leds::layout(current_step);
            CRGB* offset_leds =
                Steps::Offset::do_offset(layout_leds, current_step);
            Steps::Draw::draw(offset_leds);
        }
    }  // namespace Drawing

    bool data_read = false;
    void mock_loop() {
        if (!data_read) {
            int data_len = Parsing::read_serial();
            if (data_len != 0) {
                Parsing::parse_serial_data(data_len);
                Drawing::init();
            }
            data_read = true;
        }
    }

    void setup() {
        Drawing::draw_blank();
    }

    char serial_text[512];
    void loop() {
#ifdef MOCK
        mock_loop();
#else
        if (Serial.available()) {
            size_t read_bytes = Serial.readBytesUntil('\n', serial_text, 512);
            if (!strncmp(serial_text, "leds", read_bytes)) {
                Serial.println(NUM_LEDS);
            } else if (!strncmp(serial_text, "off", read_bytes)) {
                Parsing::turn_off();
                Drawing::draw_blank();
            } else if (strncmp(serial_text, "manual",
                               min(read_bytes, strlen("manual"))) == 0) {
                serial_text[read_bytes] = '\0';

                Serial.println("ready");

                delay(100);
                int data_len = Parsing::read_serial();
                if (data_len != 0) {
                    if (Parsing::parse_serial_data(data_len) == 0) {
                        Serial.println("Read succesfully");
                    }
                    Drawing::init();
                }
            }
        }
#endif

        Drawing::draw();
    }  // namespace Manual
}  // namespace Manual