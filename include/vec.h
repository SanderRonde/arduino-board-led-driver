#pragma once

#include <stdlib.h>

#define VEC_PART_LEN 16

namespace Vec {
	template<typename vec_type>
	struct vec_part_t {
		vec_part_t<vec_type>* next;
		u_int16_t length : 5;
		vec_type items[VEC_PART_LEN];
	};

	template<typename vec_type>
	struct vec_t {
		long length;
		vec_part_t<vec_type>* part;
	};

	template<typename vec_type>
	vec_t<vec_type>* vec_create();

	template<typename vec_type>
	void vec_push(vec_t<vec_type>* vec, vec_type value);

	template<typename vec_type>
	vec_type vec_get(vec_t<vec_type>* vec, int index);

	template<typename vec_type>
	void vec_foreach(vec_t<vec_type>* vec, void (*fn)(vec_type value, int index));

	template<typename vec_type>
	void vec_del(vec_t<vec_type>* vec);

	template<typename vec_type>
	void vec_free(vec_t<vec_type*>* vec);

	template<typename vec_type>
	Vec::vec_t<vec_type> vec_refresh(Vec::vec_t<vec_type>* vec, bool free);
}