#include <serial_control.h>
#include <Arduino.h>
#include <globals.h>
#include <modes.h>

namespace SerialControl {
	char* char_blocks[MAX_ARG_BLOCKS];
	boolean new_data = false;

	int parse_serial(String data[ARG_BLOCK_LEN]) {
		int data_index = 0;
		String current_word = "";

		for (unsigned int i = 0; i < MAX_ARG_BLOCKS; i++) {
			if (char_blocks[i] == NULL) break;
			for (unsigned int j = 0; j < strnlen(char_blocks[i], ARG_BLOCK_LEN); j++) {
				if (char_blocks[i][j] == ' ') {
					data[data_index++] = current_word;
					current_word = "";
				} else {
					current_word += char_blocks[i][j];
				}
			}
		}
		data[data_index++] = current_word;
		data[data_index++] = "";
		return data_index - 1;
	}

	bool checksum_serial(String serial_data[ARG_BLOCK_LEN], int length) {
		return serial_data[0][0] == '/' && serial_data[length - 1][0] == '\\';
	}

	void get_help(String serial_data[ARG_BLOCK_LEN]) {
		if (serial_data[2] == "off") {
			Modes::Off::help();
		} else if (serial_data[2] == "solid") {
			Modes::Solid::help();
		} else if (serial_data[2] == "dot") {
			Modes::Dot::help();
		} else if (serial_data[2] == "split") {
			Modes::Split::help();
		} else if (serial_data[2] == "pattern") {
			Modes::Pattern::help();
		} else if (serial_data[2] == "prime" ){
			Modes::Prime::help();
		} else if (serial_data[2] == "flash" ){
			Modes::Flash::help();
		} else if (serial_data[2] == "rainbow"){
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
		String serial_data[ARG_BLOCK_LEN];
		int length = parse_serial(serial_data);

		if (!checksum_serial(serial_data, length)) {
			return;
		}

		if (serial_data[1] == "off") {
			Modes::Off::handle_serial(serial_data);
		} else if (serial_data[1] == "solid") {
			Modes::Solid::handle_serial(serial_data);
		} else if (serial_data[1] == "dot") {
			Modes::Dot::handle_serial(serial_data);
		} else if (serial_data[1] == "split") {
			Modes::Split::handle_serial(serial_data);
		} else if (serial_data[1] == "pattern") {
			Modes::Pattern::handle_serial(serial_data);
		} else if (serial_data[1] == "prime" ){
			Modes::Prime::handle_serial(serial_data);
		} else if (serial_data[1] == "flash" ){
			Modes::Flash::handle_serial(serial_data);
		} else if (serial_data[1] == "rainbow") {
			Modes::Rainbow::handle_serial(serial_data);
		} else if (serial_data[1] == "random") {
			Modes::Random::handle_serial(serial_data);
		} else if (serial_data[1] == "beats") {
			Modes::Beats::handle_serial(serial_data);
		} else if (serial_data[1] == "leds") {
			Serial.println(NUM_LEDS);
		} else if (serial_data[1] == "help") {
			get_help(serial_data);
		}
		Serial.println("ack");
		Serial.println("ack");
		Serial.println("ack");
		Serial.println("ack");
		Serial.println("ack");

		signal_read();
	}

	void recv_with_end_marker() {
		if (!Serial.available()) return;

		char rc;
		char endMarker = '\n';
		int char_index = 0;
		int block_index = 0;

		unsigned long start_time = millis();
		while (millis() - start_time <= HOLD_TIME || Serial.available()) {
			if (Serial.available() == 0) continue;

			rc = Serial.read();

			if (char_blocks[block_index] == NULL) {
				char_blocks[block_index] = (char*) malloc(sizeof(char) * ARG_BLOCK_LEN);
			}

			if (rc != endMarker) {
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
				char_blocks[block_index][char_index] = '\0'; // terminate the string
				new_data = true;
				
				return;
			}
		}
	}
}

// / beats 255 0 0 0 0 0 0 100 0 \