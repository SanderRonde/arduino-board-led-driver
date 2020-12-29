#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SERIAL_BUF_SIZE 64

class MockSerial {
   private:
    char serial_buf[SERIAL_BUF_SIZE] = {0};
    bool is_available = false;

    fd_set fds;

   public:
    void begin(const uint32_t dwBaudRate);
    size_t println(const char* x);
    size_t println(int x);
    size_t print(const char* x);
    size_t print(int x);
    size_t write(const char* chars);
    size_t readBytesUntil(char stop_char, uint8_t* data, size_t max_len);

    int available();
    char read();

    operator bool() { return true; };
};
extern MockSerial Serial;