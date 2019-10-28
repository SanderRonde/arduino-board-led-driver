#pragma once

#include <stdlib.h>

#define VEC_PART_LEN 16

namespace Vec {
	template<typename vec_type>
	struct vec_part_t {
		vec_part_t<vec_type>* next;
		unsigned int length : 5;
		vec_type items[VEC_PART_LEN];
	};

	template <class vec_type>
	class Vector {
		public:
			long length;
			Vec::vec_part_t<vec_type>* part;

			void push(vec_type value);
			vec_type get(int index);
			void foreach(void (*fn)(vec_type value, int index));
			void refresh();

			Vector();
			~Vector();
	};
}