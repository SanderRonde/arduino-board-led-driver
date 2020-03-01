#include "../include/mock-main.h"
#include <unistd.h>
#include "../include/mock-arduino.h"

#define VERBOSE_ARG "--verbose"
#define VERBOSE_SHORT_ARG "-v"

bool verbose = false;

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

int main(int argc, char* argv[]) {
    inputAvailable();

    for (int i = 0; i < argc; i++) {
        if (strncmp(argv[i], VERBOSE_ARG, strlen(VERBOSE_ARG)) == 0 ||
            strncmp(argv[i], VERBOSE_SHORT_ARG, strlen(VERBOSE_SHORT_ARG)) ==
                0) {
            verbose = true;
        }
    }

    if (verbose) {
        printf("[mock] using verbose. Booting...\n");
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
}