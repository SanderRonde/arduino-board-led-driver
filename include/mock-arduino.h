#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#define boolean bool
#define Min(a, b)           (((a) < (b)) ?  (a) : (b))
#define min(a, b)   Min(a, b)
#define SERIAL_BUF_SIZE 64

void setup();
void loop();

uint32_t analogRead(uint32_t ulPin);

class MockSerial {
	private:
		char serial_buf[SERIAL_BUF_SIZE];

		bool is_available = false;
	public:
		void begin(const uint32_t dwBaudRate);
		size_t println(const char* x);
		size_t println(int x);
		size_t print(const char* x);
		size_t print(int x);

		int available();
		char read();
		
		operator bool() { return true; };
};
extern MockSerial Serial;

extern void randomSeed( uint32_t dwSeed );
uint32_t millis();

long int random(int min, int max);