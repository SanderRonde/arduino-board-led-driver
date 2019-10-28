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
	void vec_push_part(Vec::vec_part_t<vec_type>* vec, vec_type value) {
		if (vec->length >= VEC_PART_LEN) {
			if (vec->next == NULL) {
				vec->next = vec_part_create<vec_type>();
			}
			vec_push_part(vec->next);
		} else {
			vec->items[vec->length++] = value;
		}
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
	void vec_part_foreach(Vec::vec_part_t<vec_type>* part, void (*fn)(vec_type value, int index)) {
		for (int i = 0; i < part->length; i++) {
			fn(part->items[i], i);
		}
		if (part->next) {
			vec_part_foreach(part->next, fn);
		}
	}

	template<typename vec_type>
	void vec_del_part(Vec::vec_part_t<vec_type>* part) {
		if (part->next) {
			vec_del_part(part->next);
		}
		free(part);
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

	template <class vec_type>
	Vector<vec_type>::Vector() {
		length = 0;
		part = vec_part_create<vec_type>();
	}

	template <class vec_type>
	void Vector<vec_type>::push(vec_type value) {
		vec_push_part(part, value);
		length++;
	}

	template <class vec_type>
	vec_type Vector<vec_type>::get(int index) {
		return vec_part_get(part, index);
	}

	template <class vec_type>
	void Vector<vec_type>::foreach(void (*fn)(vec_type value, int index)) {
		vec_part_foreach(part, fn);
	}

	template <class vec_type>
	void Vector<vec_type>::refresh() {
		vec_del_part<vec_type>(part);
		part = vec_part_create<vec_type>();
		length = 0;
	}

	template <class vec_type>
	Vector<vec_type>::~Vector() {
		vec_del_part(part);
	}
}