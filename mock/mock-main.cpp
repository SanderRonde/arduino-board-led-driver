#include "../include/mock-main.h"
#include <unistd.h>
#include "../include/mock-arduino.h"

#define VERBOSE_ARG "--verbose"
#define VERBOSE_SHORT_ARG "-v"
#define TIME_SHORT_ARG "-t"

bool verbose = false;
unsigned long time_subtract = 0;

bool inputAvailable() {
    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
    return (FD_ISSET(0, &fds));
}

void read_io(int argc, char* argv[]) {
    for (int i = 0; i < argc; i++) {
        if (strncmp(argv[i], VERBOSE_ARG, strlen(VERBOSE_ARG)) == 0 ||
            strncmp(argv[i], VERBOSE_SHORT_ARG, strlen(VERBOSE_SHORT_ARG)) ==
                0) {
            verbose = true;
        } else if (strncmp(argv[i], TIME_SHORT_ARG, strlen(TIME_SHORT_ARG)) == 0) {
            time_subtract = millis() - strtoul(argv[i + 1], NULL, 10);
        }
    }
}

int main(int argc, char* argv[]) {
    inputAvailable();

    read_io(argc, argv);

    if (verbose) {
        if (time_subtract > 0) {
            printf("[mock] using starting time ~%lu\n", millis());
        }
        printf("[mock] using verbose. booting...\n");
    }
    unsigned long setup_start = millis();
    setup();
    unsigned long setup_duration = millis() - setup_start;
    if (verbose) {
        printf("[mock] setup done in %lu ms, running main loop\n",
               setup_duration);
    }
    while (true) {
        loop();
    }

	return 0;
}