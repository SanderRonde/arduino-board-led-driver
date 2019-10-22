#pragma once

#include <string.h>
#include <Arduino.h>

#define MAX_ARG_LEN 256

namespace SerialControl {
	extern char received_chars[MAX_ARG_LEN];
	extern boolean new_data;

	void handle_serial(const String str);

	void recv_with_end_marker();
}