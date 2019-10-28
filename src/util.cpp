#include <modes.h>

namespace Util {
	void apply_change(Modes::dir_t dir, long *value, long cap) {
		if (dir == Modes::DIR::DIR_FORWARDS) {
			if (*value >= cap) {
				*value = 0;
			} else {
				*value = *value + 1;
			}
		} else {
			if (*value <= -cap) {
				*value = 0;
			} else {
				*value = *value - 1;
			}
		}
	}

	void apply_change(Modes::dir_t dir, long *value) {
		apply_change(dir, value, __LONG_MAX__ - 100);
	}
}