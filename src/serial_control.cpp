#include "../include/serial_control.h"
#include "../include/globals.h"
#include "../include/modes.h"

#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>

namespace SerialControl {
    char* char_blocks[MAX_ARG_BLOCKS];
    bool new_data = false;
    bool no_draw = false;

    words_t* parse_serial() {
        words_t* words = (words_t*)malloc(sizeof(words_t));
        words->num_words = 0;

        int char_index = 0;

        for (unsigned int i = 0; i < MAX_ARG_BLOCKS; i++) {
            if (char_blocks[i] == NULL) break;
            for (unsigned int j = 0; j < strnlen(char_blocks[i], ARG_BLOCK_LEN);
                 j++) {
                if (char_blocks[i][j] == ' ') {
                    words->text[words->num_words][char_index] = '\0';
                    words->num_words++;
                    char_index = 0;

                    if (words->num_words >= MAX_WORDS) {
                        Serial.println("ERR: Input exceeds max words");
                        return words;
                    }
                } else {
                    words->text[words->num_words][char_index] =
                        char_blocks[i][j];
                    char_index++;

                    if (char_index >= MAX_WORD_LEN) {
                        Serial.println("ERR: Input exceeds max word length");
                        return words;
                    }
                }
            }
        }

        words->text[words->num_words][char_index] = '\0';
        words->num_words++;

        return words;
    }

    bool checksum_serial(words_t* words) {
        return words->text[0][0] == '/' &&
               words->text[words->num_words - 1][0] == '\\';
    }

    void get_help(words_t* words) {
        if (WORD_EQ(words, 2, "off")) {
            Modes::Off::help();
        } else if (WORD_EQ(words, 2, "solid")) {
            Modes::Solid::help();
        } else if (WORD_EQ(words, 2, "dot")) {
            Modes::Dot::help();
        } else if (WORD_EQ(words, 2, "split")) {
            Modes::Split::help();
        } else if (WORD_EQ(words, 2, "pattern")) {
            Modes::Pattern::help();
        } else if (WORD_EQ(words, 2, "prime")) {
            Modes::Prime::help();
        } else if (WORD_EQ(words, 2, "flash")) {
            Modes::Flash::help();
        } else if (WORD_EQ(words, 2, "rainbow")) {
            Modes::Rainbow::help();
        } else {
            Serial.println("Unkown help type, type: \"/ help [cmd] \\\"");
        }
    }

    void clear_char_buffers() {
        for (int i = 0; i < MAX_ARG_BLOCKS; i++) {
            if (char_blocks[i] != NULL) {
                free(char_blocks[i]);
                char_blocks[i] = NULL;
            }
        }
    }

    void signal_read() {
        new_data = false;
        clear_char_buffers();
        Modes::force_update = true;
    }

    void handle_serial() {
        words_t* words = parse_serial();

        if (!checksum_serial(words)) {
            free(words);
            signal_read();
            return;
        }

        if (WORD_EQ(words, 1, "off")) {
            Modes::Off::handle_serial(words);
        } else if (WORD_EQ(words, 1, "solid")) {
            Modes::Solid::handle_serial(words);
        } else if (WORD_EQ(words, 1, "dot")) {
            Modes::Dot::handle_serial(words);
        } else if (WORD_EQ(words, 1, "split")) {
            Modes::Split::handle_serial(words);
        } else if (WORD_EQ(words, 1, "pattern")) {
            Modes::Pattern::handle_serial(words);
        } else if (WORD_EQ(words, 1, "prime")) {
            Modes::Prime::handle_serial(words);
        } else if (WORD_EQ(words, 1, "flash")) {
            Modes::Flash::handle_serial(words);
        } else if (WORD_EQ(words, 1, "rainbow")) {
            Modes::Rainbow::handle_serial(words);
        } else if (WORD_EQ(words, 1, "random")) {
            Modes::Random::handle_serial(words);
        } else if (WORD_EQ(words, 1, "beats")) {
            Modes::Beats::handle_serial(words);
        } else if (WORD_EQ(words, 1, "leds")) {
            Serial.println(NUM_LEDS);
        } else if (WORD_EQ(words, 1, "help")) {
            get_help(words);
        }
        Serial.println("ack");

        signal_read();
        free(words);
    }

    void read_explicit() {
        // Send marker indicating we're ready to receive and then just infinitely wait
        Serial.println("|1");
    }

    void read_serial() {
        if (!Serial.available()) return;

        char rc;
        char end_marker = '\n';
        int char_index = 0;
        int block_index = 0;

        unsigned long start_time = millis();
        while (millis() - start_time <= HOLD_TIME || Serial.available()) {
            if (Serial.available() == 0) continue;

            start_time = millis();

            rc = Serial.read();

            if (rc == '>') {
                // Send marker indicating we're ready to receive and then just infinitely wait
                Serial.println("|1");

                no_draw = true;
                
                while (!Serial.available()) {}
                rc = Serial.read();
                if (rc == '\n') {
                    continue;
                }
            }
            if (rc == '<') {
                no_draw = false;
                while (!Serial.available()) {}
                rc = Serial.read();
                if (rc == '\n') {
                    continue;
                }
            }

            if (char_blocks[block_index] == NULL) {
                char_blocks[block_index] =
                    (char*)malloc(sizeof(char) * ARG_BLOCK_LEN);
                memset(char_blocks[block_index], 0, sizeof(char) * ARG_BLOCK_LEN);
            }

            if (rc != end_marker) {
                char_blocks[block_index][char_index] = rc;
                char_index++;
                if (char_index >= ARG_BLOCK_LEN) {
                    block_index++;
                    char_index = 0;
                    if (block_index >= MAX_ARG_BLOCKS) {
                        Serial.println("ERR: Serial overflow");
                        return;
                    }
                }
            } else {
                if (block_index == 0 && char_index == 0) return;

                char_blocks[block_index][char_index] =
                    '\0';  // terminate the string
                new_data = true;
                printf("read text:\n");
                for (int i = 0; i <= block_index; i++) {
                    char buf[ARG_BLOCK_LEN + 1];
                    strncpy(buf, char_blocks[i], sizeof(char) * ARG_BLOCK_LEN);
                    buf[ARG_BLOCK_LEN] = '\0';
                    printf("%s", buf);
                }
                printf("\n");

                return;
            }
        }
    }
}  // namespace SerialControl