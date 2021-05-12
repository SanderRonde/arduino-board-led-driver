#pragma once

#include "../include/globals.h"

namespace Manual {
    void setup();
    void loop();

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
}  // namespace Manual