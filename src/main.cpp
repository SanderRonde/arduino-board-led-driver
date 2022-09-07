#include "api.h"
#include "globals.h"
#include "lib-includes.h"
#include "lib-main.h"
#include "lib-ota.h"
#include "lib-telnet.h"
#include "secrets.h"
#include "stepper.h"
#include "string.h"

void setup() {
    Serial.begin(115200);
    Serial.println("Booting");
    Telnet::setup(String("board-ceiling-leds").c_str(), WIFI_SSID, WIFI_PW);
    Stepper::setup();
    API::setup();
    Main::connect_done();
}

void loop() {
    API::loop();
    Stepper::loop();
}