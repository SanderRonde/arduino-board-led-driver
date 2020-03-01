#include "../include/mock-arduino.h"
#include <math.h>
#include <sys/time.h>
#include <unistd.h>
#include "../include/mock-main.h"

#define SERIAL_BUF_SIZE 64

// Serial
int _read(int __fd, void* __buf, size_t __nbyte) {
    return read(__fd, __buf, __nbyte);
}
void MockSerial::begin(const uint32_t dwBaudRate) {
    if (verbose) {
        printf("[mock] serial started, using baud rate %ul\n", dwBaudRate);
    }
};
size_t MockSerial::println(const char* x) { return printf("%s\n", x); }
size_t MockSerial::println(int x) { return printf("%d\n", x); }
size_t MockSerial::print(const char* x) { return printf("%s", x); }
size_t MockSerial::print(int x) { return printf("%d", x); }
int MockSerial::available() {
    if (is_available) return true;

    is_available = inputAvailable();
    return is_available;
}
char MockSerial::read() {
    if (is_available) {
        _read(0, serial_buf, 1);
        char result = serial_buf[0];
        serial_buf[0] = '\0';
        is_available = false;
        return result;
    }
    return 0;
}

MockSerial Serial;

// Various system functions
unsigned long millis() {
    struct timeval tp;
    gettimeofday(&tp, NULL);
    unsigned long ms = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    return ms;
}

// Arduino functions
uint32_t analogRead(uint32_t pin) { return random(); }

void randomSeed(uint32_t dwSeed) { srand(dwSeed); }
long int random(int start, int end) { return (rand() * (end - start)) + start; }
float roundf(float x) { return round(x); }