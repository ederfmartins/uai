#include <stdlib.h>
#include <limits.h>

#include "vector.h"

static void _ensure_capacity(ptr_vector_t vector) {
	uint32 old_capacity = vector->capacity;
	if (vector->size >= old_capacity) {
		vector->capacity = (old_capacity * 3)/2 + 1;
		vector->elements = (void**)realloc(vector->elements,vector->capacity*sizeof(void*));
		if (vector->elements == NULL) msgerror(E_NOMEM);
	}
}

ptr_vector_t vector_create(uint32 capacity, _COMPARE *compare, _HASH_CODE *hash_code, _DESTROY *destroy, _CLONE *clone) {
	ptr_vector_t vector = NULL;

	vector = (ptr_vector_t)malloc(sizeof(vector_t));
	if (vector == NULL) msgerror(E_NOMEM);

	vector->elements = (void**)malloc(capacity*sizeof(void*));
	if (vector->elements == NULL) msgerror(E_NOMEM);

	vector->capacity = capacity;
	vector->size = 0;
	vector->hash = 0;
	vector->compare = compare;
	vector->hash_code = hash_code;
	vector->destroy = destroy;
	vector->clone = clone;

	return vector;
}

ptr_vector_t vector_sub_vector(ptr_vector_t vector, uint32 min, uint32 max) {
	ptr_vector_t new_vector = NULL;
	uint32 i, index;

	if (max == 0 || min > max || min > vector->size-1 || max > vector->size) {
		msgerror(E_OPTARG, "vector_sub_vector: limites invalidos");
	} 

	new_vector = vector_create(max-min, vector->compare, vector->hash_code, NULL, vector->clone);
	
	for (i=min, index=0; i<max; i++, index++) {
		new_vector->elements[index] = vector->elements[i];
		new_vector->size++;
	}

	return new_vector;
}

ptr_vector_t vector_clone(ptr_vector_t vector) {
	uint32 i;
	ptr_vector_t new_vector = vector_create(vector->size, vector->compare, vector->hash_code, (vector->clone != NULL)?vector->destroy:NULL, vector->clone);

	for (i=0; i<vector->size; i++) {
		if (vector->clone != NULL) {
			vector_add(new_vector, vector->clone(vector->elements[i]));
		} else {
			vector_add(new_vector, vector->elements[i]);
		}
	}

	return new_vector;
}

bool vector_contains(ptr_vector_t vector, void *elem) {
	return vector_index_of(vector, elem) != -1;
}

bool vector_is_empty(ptr_vector_t vector) {
	return vector->size == 0;
}

bool vector_remove(ptr_vector_t vector, void *elem) {
	uint32 i;
	int32 index = vector_index_of(vector, elem);

	if (index < 0) return FALSE;

	if (vector->destroy) vector->destroy(vector->elements[index]);
	vector->elements[index] = NULL;

	for (i=index; i<vector->size-1; i++)
		vector->elements[i] = vector->elements[i+1];
	vector->elements[vector->size-1] = NULL;
	vector->size--;
	vector->hash = 0;

	return TRUE;
}

uint32 vector_hash_code(ptr_vector_t vector) {
	if (vector->hash == 0) {
		uint32 i, hash = 0;
		for (i=0; i<vector->size; i++) {
				hash += vector->hash_code(vector->elements[i]);
		}
		vector->hash = hash%UINT_MAX;
	}
	return vector->hash;
}

int32 vector_index_of(ptr_vector_t vector, void *elem) {
	uint32 i;
	for (i=0; i<vector->size; i++)
		if (vector->compare(&elem, &vector->elements[i]) == 0) 
			return i;
	return -1;
}

int32 vector_last_index_of(ptr_vector_t vector, void *elem) {
	int32 i;
	for (i=vector->size-1; i>=0; i--)
		if (vector->compare(&elem, &vector->elements[i]) == 0) 
			return i;
	return -1;
}

int32 vector_compare(const void *value1, const void *value2) {
	ptr_vector_t *v1 = (ptr_vector_t*)value1;
	ptr_vector_t *v2 = (ptr_vector_t*)value2;
	uint32 i, size = 0;

	size = MIN((*v1)->size,(*v2)->size);
	for (i=0; i<size; i++) {
		int32 ret = (*v1)->compare(&(*v1)->elements[i], &(*v2)->elements[i]);
		if (ret != 0) return ret;
	}
	return ((*v1)->size - (*v2)->size);
}

void* vector_remove_index(ptr_vector_t vector, uint32 index) {
	void *removed_elem = NULL;
	uint32 i;

	if (index >= vector->size) msgerror(E_OPTARG, "vector_remove_index: posicao invalida");

	removed_elem = vector->elements[index];
	for (i=index; i<vector->size-1; i++)
		vector->elements[i] = vector->elements[i+1];
	vector->elements[vector->size-1] = NULL;
	vector->size--;
	vector->hash = 0;
	return removed_elem;
}

void* vector_set(ptr_vector_t vector, uint32 index, void *elem) {
	void *old_elem = NULL;
	if (index >= vector->size) msgerror(E_OPTARG, "vector_set: posicao invalida");

	old_elem = vector->elements[index];
	vector->elements[index] = elem;

	return old_elem;
}

void* vector_get(ptr_vector_t vector, uint32 index) {
	if (index >= vector->size) msgerror(E_OPTARG, "vector_get: posicao invalida");
	return vector->elements[index];
}

void vector_clear(ptr_vector_t vector) {
	uint32 i;
	for (i=0; i<vector->size; i++) {
		if (vector->destroy && vector->elements[i] != NULL) 
			vector->destroy(vector->elements[i]);
		vector->elements[i] = NULL;
	}
	vector->size = 0;
	vector->hash = 0;
}

void vector_destroy(ptr_vector_t vector) {
	vector_clear(vector);
	free(vector->elements);
	free(vector);
}

void vector_add(ptr_vector_t vector, void *elem) {
	_ensure_capacity(vector);
	vector->elements[vector->size++] = elem;
	vector->hash = 0;
}

void vector_add_all(ptr_vector_t v1, ptr_vector_t v2) {
	uint32 i;
	for (i=0; i<v2->size; i++) {
		vector_add(v1, v2->elements[i]);
	}
}

void vector_remove_all(ptr_vector_t v1, ptr_vector_t v2) {
	uint32 i;
	for (i=0; i<v2->size; i++) {
		vector_remove(v1, v2->elements[i]);
	}
}

void vector_add_index(ptr_vector_t vector, uint32 index, void *elem) {
	if (index >= vector->size) msgerror(E_OPTARG, "vector_add_index: posicao invalida");

	if (vector->elements[index] == NULL) {
		vector->elements[index] = elem;
	} else {
		int32 i;
		_ensure_capacity(vector);
		for (i=vector->size; i>index; i--)
			vector->elements[i] = vector->elements[i-1];
		vector->elements[index] = elem;
	}
	vector->size++;
	vector->hash = 0;
}

void vector_trim_to_size(ptr_vector_t vector) {
	if (vector->size == vector->capacity || vector->size == 0) return;
	vector->elements = (void**)realloc(vector->elements,vector->size*sizeof(void*));
	if (vector->elements == NULL) msgerror(E_NOMEM);
	vector->capacity = vector->size;
}

void vector_replace_all(ptr_vector_t v, ptr_vector_t v1, ptr_vector_t v2) {
	uint32 index1 = 0, index2 = 0, i;
	int32 ret = 0;

	while (index1 < v1->size && index2 < v2->size) { 
		while ((ret = v->compare(&v1->elements[index1], &v2->elements[index2])) != 0) {
			if (ret > 0) {
				if (vector_index_of(v, v2->elements[index2]) == -1) vector_add(v, v2->elements[index2]);
				index2++;
				if (index2 >= v2->size) {
					for (i=index1; i<v1->size; i++) {
						vector_remove(v, v1->elements[i]);
					}
					break;
				}
			} else {
				vector_remove(v, v1->elements[index1]);
				index1++;
				if (index1 >= v1->size) {
					for (i=index2; i<v2->size; i++)
						if (vector_index_of(v, v2->elements[i]) == -1) vector_add(v, v2->elements[i]);
					break;
				}
			}
		}
		if (index1 < v1->size && index2 < v2->size) {
			index1++;
			index2++;
		}
	}

	if (index1 < v1->size) {
		for (i=index1; i<v1->size; i++) {
			vector_remove(v, v1->elements[i]);
		}
	}
	if (index2 < v2->size) {
		for (i=index2; i<v2->size; i++)
			if (vector_index_of(v, v2->elements[i]) == -1) vector_add(v, v2->elements[i]);
	}
}

void vector_replace_range(ptr_vector_t v1, uint32 min, uint32 max, ptr_vector_t v2) {
	uint32 diff = v2->size - (max-min);
	uint32 size = v1->size + diff;
	uint32 index;
	int32 i;

	if (min > v1->size) msgerror(E_OPTARG, "vector_replace_range: valor minimo invalido");
	if (max > v1->size) msgerror(E_OPTARG, "vector_replace_range: valor maximo invalido");

	while (v1->capacity < size) _ensure_capacity(v1);

	for (i=size-1; i>=max; i--) {
		if (v1->destroy != NULL) v1->destroy(v1->elements[i]);
		v1->elements[i] = v1->elements[i-diff];
	}

	for (i=0, index=min; i<v2->size; i++, index++) {
		v1->elements[index] = v2->elements[i];
	}

	v1->size = size;
	v1->hash = 0;
}

