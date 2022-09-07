#pragma once

#include "WString.h"

namespace Stepper {
    void loop();

		void setup();

		bool set(String data);

		void off();

		void on();

		bool is_on();
}