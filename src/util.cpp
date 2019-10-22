#include <modes.h>

namespace Util {
	void apply_change(Modes::dir_t dir, long *value) {
		if (dir == Modes::DIR::DIR_FORWARDS) {
			if (*value >= __LONG_MAX__ - 100) {
				*value = 0;
			} else {
				*value = *value + 1;
			}
		} else {
			if (*value <= -(__LONG_MAX__ - 100)) {
				*value = 0;
			} else {
				*value = *value - 1;
			}
		}
	}
}