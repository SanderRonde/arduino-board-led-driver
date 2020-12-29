#include "../include/mock-serial.h"

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
size_t MockSerial::write(const char* x) { return printf("%s", x); };
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
size_t MockSerial::readBytesUntil(char stop_char, uint8_t* data,
                                  size_t max_len) {
    if (is_available) {
        size_t index = 0;

        char result;
        do {
            _read(0, serial_buf, 1);
            result = serial_buf[0];
            data[index++] = result;
        } while (result != stop_char && index <= max_len);

        return index - 1;
    }
    return 0;
}

MockSerial Serial;
