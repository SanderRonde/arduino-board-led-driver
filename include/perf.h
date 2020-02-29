#include "globals.h"

namespace Perf {
	int section_define(const char* name);
	void section_begin(int section_id);
	void section_end(int section_id);
	void check_results();
}