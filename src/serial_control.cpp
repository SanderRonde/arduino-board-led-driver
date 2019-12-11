#include <serial_control.h>
#include <Arduino.h>
#include <globals.h>
#include <modes.h>

namespace SerialControl {
	char received_chars[MAX_ARG_LEN];
	boolean new_data = false;

	int parse_serial(const String str, String data[MAX_ARG_LEN]) {
		int data_index = 0;
		String current_word = "";

		const char* c_str = str.c_str();
		for (unsigned int i = 0; i < str.length(); i++) {
			if (c_str[i] == ' ') {
				if (data_index > MAX_ARG_LEN) return MAX_ARG_LEN;
				data[data_index++] = current_word;
				current_word = "";
			} else {
				current_word += c_str[i];
			}
		}
		data[data_index++] = current_word;
		data[data_index++] = "";
		return data_index - 1;
	}

	bool checksum_serial(String serial_data[MAX_ARG_LEN], int length) {
		return serial_data[0][0] == '/' && serial_data[length - 1][0] == '\\';
	}

	void get_help(String serial_data[MAX_ARG_LEN]) {
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

	void handle_serial(const String str) {
		String serial_data[MAX_ARG_LEN];
		int length = parse_serial(str, serial_data);

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
	}

	void recv_with_end_marker() {
		static byte ndx = 0;
		char endMarker = '\n';
		char rc;
	
		while (Serial.available() > 0 && new_data == false) {
			rc = Serial.read();

			if (rc != endMarker) {
				received_chars[ndx] = rc;
				ndx++;
				if (ndx >= MAX_ARG_LEN) {
					ndx = MAX_ARG_LEN - 1;
				}
			}
			else {
				received_chars[ndx] = '\0'; // terminate the string
				ndx = 0;
				new_data = true;
			}
		}
	}
}