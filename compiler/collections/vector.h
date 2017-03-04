#ifndef _VECTOR_H_

#define _VECTOR_H_

#include "util.h"

typedef struct vector {
	uint32 hash;
	uint32 capacity;
	uint32 size;
	void **elements;

	_COMPARE *compare;
	_HASH_CODE *hash_code; 
	_DESTROY *destroy;
	_CLONE *clone;
} vector_t, *ptr_vector_t;

ptr_vector_t vector_create(uint32, _COMPARE*, _HASH_CODE*, _DESTROY*, _CLONE*);
ptr_vector_t vector_sub_vector(ptr_vector_t, uint32, uint32);
ptr_vector_t vector_clone(ptr_vector_t);

bool vector_contains(ptr_vector_t, void*);
bool vector_is_empty(ptr_vector_t);
bool vector_remove(ptr_vector_t, void*);

uint32 vector_hash_code(ptr_vector_t);

int32 vector_compare(const void*, const void*);
int32 vector_index_of(ptr_vector_t, void*);
int32 vector_last_index_of(ptr_vector_t, void*);

void* vector_set(ptr_vector_t, uint32, void*);
void* vector_get(ptr_vector_t, uint32);
void* vector_remove_index(ptr_vector_t, uint32);

void vector_add(ptr_vector_t, void*);
void vector_add_all(ptr_vector_t, ptr_vector_t);
void vector_add_index(ptr_vector_t, uint32, void*);
void vector_remove_all(ptr_vector_t, ptr_vector_t);
void vector_trim_to_size(ptr_vector_t);
void vector_replace_range(ptr_vector_t, uint32, uint32, ptr_vector_t);
void vector_replace_all(ptr_vector_t, ptr_vector_t, ptr_vector_t);
void vector_clear(ptr_vector_t);
void vector_destroy(ptr_vector_t);

#endif
