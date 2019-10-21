#pragma once
#include <serial_control.h>

namespace Modes {
	typedef enum LED_MODE {
		LED_MODE_DOT,
		LED_MODE_SOLID,
		LED_MODE_SPLIT,
		LED_MODE_PATTERN,
		LED_MODE_PRIMED,
		LED_MODE_OFF
	} led_mode_t;

	extern Modes::led_mode_t cur_mode;

	extern void (*iterate_fn)(void);
	extern unsigned long mode_update_time;

	namespace Off {
		extern const unsigned long update_time;

		void do_iteration();

		void handle_serial(const String serial_data[MAX_ARG_LEN]);
	}

	namespace Solid {
		void do_iteration();

		void handle_serial(const String serial_data[MAX_ARG_LEN]);
	}

	namespace Dot {
		void do_iteration();

		void handle_serial(const String serial_data[MAX_ARG_LEN]);
	}

	namespace Split {
		void do_iteration();

		void handle_serial(const String serial_data[MAX_ARG_LEN]);
	}

	namespace Pattern {
		void do_iteration();

		void handle_serial(const String serial_data[MAX_ARG_LEN]);
	}

	namespace Prime {
		void do_iteration();

		void handle_serial(const String serial_data[MAX_ARG_LEN]);
	}
}