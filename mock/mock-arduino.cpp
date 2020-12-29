#include "../include/mock-arduino.h"

#include <math.h>
#include <sys/time.h>
#include <unistd.h>

#include "../include/mock-main.h"

#define SERIAL_BUF_SIZE 64

unsigned long _millis() {
    struct timeval tp;
    gettimeofday(&tp, NULL);
    unsigned long ms = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    return ms - time_subtract;
}

// Various system functions
unsigned long millis() { return _millis() - time_subtract; }

// Arduino functions
uint32_t analogRead(uint32_t pin) { return random(); }

void randomSeed(uint32_t dwSeed) { srand(dwSeed); }
long int random(int start, int end) { return (rand() * (end - start)) + start; }
float roundf(float x) { return round(x); }
void delay(unsigned long duration){};

