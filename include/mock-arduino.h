#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define boolean bool
#define Min(a, b) (((a) < (b)) ? (a) : (b))
#define min(a, b) Min(a, b)
#define Max(a, b) (((a) > (b)) ? (a) : (b))
#define max(a, b) Max(a, b)

typedef uint8_t byte;

uint32_t analogRead(uint32_t ulPin);

extern void randomSeed(uint32_t dwSeed);
unsigned long _millis();
unsigned long millis();

long int random(int min, int max);
void delay(unsigned long duration);

class String {
    
};