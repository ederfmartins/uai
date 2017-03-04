#ifndef _HASH_H_

#define _HASH_H_

#include "util.h"
#include "vector.h"

typedef struct hash_entry {
	void *key;
	void *value;
	struct hash_entry *next;
} hash_entry_t, *ptr_hash_entry_t;

typedef struct {
	ptr_hash_entry_t first_entry;
} hash_sync_t, *ptr_hash_sync_t;

typedef struct {
	uint32 size;
	uint32 capacity;
	uint32 hash;
	float load_factor;

	ptr_hash_sync_t entries;

	_HASH_CODE *hash_code;		// Hash function for the hash_table keys
	_COMPARE *compare;		    // Equals function for the hash_table keys
	_DESTROY *destroy_key;
	_DESTROY *destroy_value;
	_CLONE *clone_key;
	_CLONE *clone_value;
} hash_t, *ptr_hash_t;

ptr_hash_t hash_create(uint32, float, _COMPARE*, _HASH_CODE*, _DESTROY*, _DESTROY*, _CLONE*, _CLONE*);
ptr_hash_t hash_clone(ptr_hash_t);

bool hash_contains_key(ptr_hash_t, void*);

uint32 hash_hash_code(ptr_hash_t);

int32 hash_compare(const void*, const void*);

void hash_clear(ptr_hash_t);
void hash_destroy(ptr_hash_t);
void hash_put(ptr_hash_t, void*, void*);
void hash_set_hash_code(ptr_hash_t, _HASH_CODE*);

void* hash_remove(ptr_hash_t, void*);
void* hash_get(ptr_hash_t, void*);

ptr_vector_t hash_get_keys(ptr_hash_t);
ptr_vector_t hash_get_values(ptr_hash_t);
ptr_vector_t hash_get_values_by_key(ptr_hash_t, void*);

#endif

