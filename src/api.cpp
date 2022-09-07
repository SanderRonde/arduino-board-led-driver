#include "api.h"

#include "ESP8266WebServer.h"
#include "globals.h"
#include "lib-net.h"
#include "lib-telnet.h"
#include "stepper.h"

#define SERVER_PORT 80

namespace API {
    ESP8266WebServer server(SERVER_PORT);

    void on_404() { server.send(404, "text/plain", "Not Found"); }

    void on_leds() {
        String result = "";
        result += "{\"status\": \"success\", \"leds\": ";
        result += NUM_LEDS;
        result += "}";
        server.send(200, "application/json", result.c_str());
    }

    void on_steps() {
        if (!server.hasArg("data")) {
            server.send(401, "text/plain", "Bad request");
            return;
        }
        bool success = Stepper::set(server.arg("data"));
        String result = "{\"status\": \"";
        if (success) {
            result += "success";
        } else {
            result += "error";
        }
        result += "\"}";

        server.send(200, "application/json", result.c_str());
    }

    void on_off() {
        Stepper::off();
        server.send(200, "application/json", "{\"status\": \"success\"}");
    }

    void on_on() {
        Stepper::on();
        server.send(200, "application/json", "{\"status\": \"success\"}");
    }

    void on_is_on() {
        bool is_on = Stepper::is_on();
        String result = "";
        result += "{\"status\": \"success\", \"enabled\": ";
        result += is_on ? "true" : "false";
        result += "}";
        server.send(200, "application/json", result.c_str());
    }

    void setup() {
        server.begin(SERVER_PORT);
        server.on("/effects/steps", HTTP_POST, on_steps);
        server.on("/leds", HTTP_GET, on_leds);
        server.on("/off", HTTP_POST, on_off);
        server.on("/on", HTTP_POST, on_on);
        server.on("/is_on", HTTP_GET, on_is_on);
        server.onNotFound(on_404);
        LOGF("Server listening on port %d\n", SERVER_PORT);
    }

    void loop() { server.handleClient(); }
}  // namespace API