#include "../include/modes.h"

namespace Util {
	void apply_change(Modes::dir_t dir, long *value, unsigned int step, long cap) {
		if (dir == Modes::DIR::DIR_FORWARDS) {
			if (*value >= cap) {
				*value = 0;
			} else {
				*value = *value + step;
			}
		} else {
			if (*value <= -cap) {
				*value = 0;
			} else {
				*value = *value - step;
			}
		}
	}

	void apply_change(Modes::dir_t dir, long *value, unsigned int step) {
		apply_change(dir, value, step, __LONG_MAX__ - 100);
	}

	void apply_change(Modes::dir_t dir, long *value) {
		apply_change(dir, value, 1, __LONG_MAX__ - 100);
	}
}