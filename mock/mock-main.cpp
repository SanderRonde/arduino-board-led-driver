#include "../include/mock-main.h"
#include <string.h>
#include <stdio.h>

#define VERBOSE_ARG "--verbose"
#define VERBOSE_SHORT_ARG "-v"

bool verbose = false;

int main(int argc, char* argv[]) {
	for (int i = 0; i < argc; i++) {
		if (strncmp(argv[i], VERBOSE_ARG, strlen(VERBOSE_ARG)) == 0 ||
			strncmp(argv[i], VERBOSE_SHORT_ARG, strlen(VERBOSE_SHORT_ARG)) == 0) {
			verbose = true;
		}
	}

	if (verbose) {
		printf("[mock] using verbose. Booting...\n");
	}
	setup();
	if (verbose) {
		printf("[mock] setup done, running main loop\n");
	}
	while (true) {
		loop();
	}
}