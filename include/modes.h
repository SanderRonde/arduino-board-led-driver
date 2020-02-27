#pragma once
#include <serial_control.h>

namespace Modes {
	typedef enum LED_MODE {
		LED_MODE_DOT,
		LED_MODE_SOLID,
		LED_MODE_SPLIT,
		LED_MODE_PATTERN,
		LED_MODE_PRIMED,
		LED_MODE_FLASH,
		LED_MODE_RAINBOW,
		LED_MODE_RANDOM,
		LED_MODE_BEATS,
		LED_MODE_OFF
	} led_mode_t;

	typedef enum DIR {
		DIR_BACKWARDS = 0,
		DIR_FORWARDS = 1
	} dir_t;

	extern Modes::led_mode_t cur_mode;

	extern void (*iterate_fn)(void);
	extern void (*serial_override)(void);
	extern unsigned long mode_update_time;
	
	extern bool force_update;

	namespace Off {
		extern const unsigned long update_time;

		void do_iteration();

		void handle_serial(const String serial_data[ARG_BLOCK_LEN]);

		void help();
	}

	namespace Solid {
		void do_iteration();

		void handle_serial(const String serial_data[ARG_BLOCK_LEN]);

		void help();
	}

	namespace Dot {
		void do_iteration();

		void handle_serial(const String serial_data[ARG_BLOCK_LEN]);

		void help();
	}

	namespace Split {
		void do_iteration();

		void handle_serial(const String serial_data[ARG_BLOCK_LEN]);
		
		void help();
	}

	namespace Pattern {
		void do_iteration();

		void handle_serial(const String serial_data[ARG_BLOCK_LEN]);

		void help();
	}

	namespace Prime {
		void do_iteration();

		void handle_serial(const String serial_data[ARG_BLOCK_LEN]);
		
		void help();
	}

	namespace Flash {
		void handle_serial(const String serial_data[ARG_BLOCK_LEN]);
		
		void help();
	}

	namespace Rainbow {
		void handle_serial(const String serial_data[ARG_BLOCK_LEN]);
		
		void help();
	}

	namespace Random {
		void handle_serial(const String serial_data[ARG_BLOCK_LEN]);
		
		void help();
	}
	
	namespace Beats {
		void handle_serial(const String serial_data[ARG_BLOCK_LEN]);
		
		void help();
	}
}