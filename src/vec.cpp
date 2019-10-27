#include <stdlib.h>
#include <vec.h>

namespace Vec {
	template<typename vec_type>
	Vec::vec_part_t<vec_type>* vec_part_create() {
		Vec::vec_part_t<vec_type>* vec_part = (Vec::vec_part_t<vec_type>*) 
			malloc(sizeof(Vec::vec_part_t<vec_type>));
		vec_part->length = 0;
		vec_part->next = NULL;
		return vec_part;
	}

	template<typename vec_type>
	Vec::vec_t<vec_type>* vec_create() {
		Vec::vec_t<vec_type>* vec = (Vec::vec_t<vec_type>*) malloc(sizeof(Vec::vec_t<vec_type>));
		vec->length = 0;
		vec->part = vec_part_create();
		return vec;
	}

	template<typename vec_type>
	void vec_push_part(Vec::vec_part_t<vec_type>* vec, vec_type value) {
		if (vec->length >= VEC_PART_LEN) {
			if (vec->next == NULL) {
				vec->next = vec_part_create();
			}
			vec_push_part(vec->next);
		} else {
			vec->items[vec->length++] = value;
		}
	}

	template<typename vec_type>
	void vec_push(Vec::vec_t<vec_type>* vec, vec_type value) {
		vec_push_part(vec->part, value);
		vec->length++;
	}

	template<typename vec_type>
	vec_type vec_part_get(Vec::vec_part_t<vec_type>* part, int index) {
		if (index >= VEC_PART_LEN) {
			assert(part->next);
			return vec_part_get(part->next, index - VEC_PART_LEN);
		}
		return part->items[index];
	}

	template<typename vec_type>
	vec_type vec_get(Vec::vec_t<vec_type>* vec, int index) {
		return vec_part_get(vec->part, index);
	}

	template<typename vec_type>
	void vec_part_foreach(Vec::vec_part_t<vec_type>* part, void (*fn)(vec_type value, int index)) {
		for (int i = 0; i < part->length; i++) {
			fn(part->items[i], i);
		}
		if (part->next) {
			vec_part_foreach(part->next, fn);
		}
	}

	template<typename vec_type>
	void vec_foreach(Vec::vec_t<vec_type>* vec, void (*fn)(vec_type value, int index)) {
		vec_part_foreach(vec->part, fn);
	}

	template<typename vec_type>
	void vec_del_part(Vec::vec_part_t<vec_type>* part) {
		if (part->next) {
			vec_del_part(part->next);
		}
		free(part);
	}

	template<typename vec_type>
	void vec_del(Vec::vec_t<vec_type>* vec) {
		vec_del_part(vec->part);
		free(vec);
	}

	template<typename vec_type>
	void vec_free_part(Vec::vec_part_t<vec_type*>* part) {
		if (part->next) {
			vec_del_part(part->next);
		}
		for (int i = 0; i < part->length; i++) {
			free(part->items[i]);
		}
		free(part);
	}

	template<typename vec_type>
	void vec_free(Vec::vec_t<vec_type*>* vec) {
		vec_del_part(vec->part);
		free(vec);
	}

	template<typename vec_type>
	Vec::vec_t<vec_type> vec_refresh(Vec::vec_t<vec_type>* vec, bool free) {
		if (free) {
			vec_free(vec);
		} else {
			vec_del(vec);
		}
		return vec_create();
	}
}