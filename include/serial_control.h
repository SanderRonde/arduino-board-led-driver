#pragma once

#include "globals.h"

#define ARG_BLOCK_LEN 512
#define MAX_ARG_BLOCKS 64
#define HOLD_TIME 3000

#define MAX_WORD_LEN 64
#define MAX_WORDS 128

#define WORD_EQ(words, index, expected) \
    strcmp(words->text[index], expected) == 0

typedef struct words {
    char text[MAX_WORD_LEN][MAX_WORDS];
    int num_words;

    inline char* operator[](uint8_t x) __attribute__((always_inline)) {
        return text[x];
    }

    inline const char* operator[](uint8_t x) const
        __attribute__((always_inline)) {
        return text[x];
    }
} words_t;

namespace SerialControl {
    extern char* char_blocks[MAX_ARG_BLOCKS];
    extern boolean new_data;
    extern bool no_draw;

    void handle_serial();

    void clear_char_buffers();
    void signal_read();

    void read_serial();
}  // namespace SerialControl