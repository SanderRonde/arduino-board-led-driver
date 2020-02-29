#include "modes.h"

namespace Util {
	void apply_change(Modes::dir_t dir, long *value, unsigned int step, long cap);
	void apply_change(Modes::dir_t dir, long *value, unsigned int step);
	void apply_change(Modes::dir_t dir, long *value);
}