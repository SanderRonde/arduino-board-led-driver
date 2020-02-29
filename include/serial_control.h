#pragma once

#include <string.h>
#include <Arduino.h>

#define ARG_BLOCK_LEN 512
#define MAX_ARG_BLOCKS 32
#define HOLD_TIME 3000

namespace SerialControl {
	extern char* char_blocks[MAX_ARG_BLOCKS];
	extern boolean new_data;

	void handle_serial();

	void clear_char_buffers();
	void signal_read();

	void recv_with_end_marker();
}