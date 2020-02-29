#include "../include/mock-arduino.h"
#include <sys/time.h>
#include <unistd.h>
#include <math.h>

#define SERIAL_BUF_SIZE 64

// Serial
int _read(int __fd, void *__buf, size_t __nbyte) {
	return read(__fd, __buf, __nbyte);
}
void MockSerial::begin(const uint32_t dwBaudRate) {
	printf("Serial began using baud rate %ul\n", dwBaudRate);
};
size_t MockSerial::println(const char* x) {
	printf("%s\n", x);
}
size_t MockSerial::println(int x) {
	printf("%d\n", x);
}
size_t MockSerial::print(const char* x) {
	printf("%s", x);
}
size_t MockSerial::print(int x) {
	printf("%d", x);
}
int MockSerial::available() {
	if (is_available) return true;

	bool result = _read(0, serial_buf, 1) > 0;
	is_available = result;
}
char MockSerial::read() {
	if (is_available) {
		char result = serial_buf[0];
		serial_buf[0] = '\0';
		is_available = false;
		return result;
	}
	return 0;
}

MockSerial Serial;


// Various system functions
uint32_t millis() {
	struct timeval tp;
	gettimeofday(&tp, NULL);
	uint32_t ms = tp.tv_sec * 1000 + tp.tv_usec / 1000;
}

// Arduino functions
uint32_t analogRead(uint32_t pin) {
	return random();
}

void randomSeed(uint32_t dwSeed) { 
	srand(dwSeed);
}
long int random(int start, int end) {
	return (rand() * (end - start)) + start;
}
float roundf(float x) {
	return round(x);
}