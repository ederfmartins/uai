#include <stdlib.h>
#include <limits.h>

#include "hash.h"

static void _rehash(ptr_hash_t hash, bool must_grow) {
	uint32 i, old_capacity = hash->capacity;
	ptr_hash_sync_t old_entries = hash->entries;

	if (must_grow) {
		hash->capacity = (old_capacity * 3)/2 + 1;
	}
	hash->entries = (ptr_hash_sync_t)calloc(hash->capacity,sizeof(hash_sync_t));
	if (hash->entries == NULL) msgerror(E_NOMEM);
	for (i=0; i<hash->capacity; i++) hash->entries[i].first_entry = NULL;
	hash->size = 0;

	for (i=0; i<old_capacity; i++) {
		ptr_hash_sync_t sync_entry = &(old_entries[i]);
		ptr_hash_entry_t entry, next;
		entry = sync_entry->first_entry;
		while(entry != NULL) {
			next = entry->next;
			hash_put(hash, entry->key, entry->value);
			free(entry);
			entry = next;
		}
	}
	free(old_entries);
}

ptr_hash_t hash_create(uint32 capacity, float load_factor, _COMPARE *compare, _HASH_CODE *hash_code, _DESTROY *destroy_key, _DESTROY *destroy_value, _CLONE *clone_key, _CLONE *clone_value) {
	ptr_hash_t hash = NULL;
	uint32 i;

	hash = (ptr_hash_t)malloc(sizeof(hash_t));
	if (hash == NULL) msgerror(E_NOMEM);

	hash->entries = (ptr_hash_sync_t)calloc(capacity,sizeof(hash_sync_t));
	if (hash->entries == NULL) msgerror(E_NOMEM);
	for (i=0; i<capacity; i++) hash->entries[i].first_entry = NULL;

	hash->hash = 0;
	hash->size = 0;
	hash->capacity = capacity;
	hash->load_factor = load_factor;

	hash->compare = compare;
	hash->hash_code = hash_code;
	hash->destroy_key = destroy_key;
	hash->destroy_value = destroy_value;
	hash->clone_key = clone_key;
	hash->clone_value = clone_value;

	return hash;
}

ptr_hash_t hash_clone(ptr_hash_t hash) {
	uint32 i;
	void *key, *value;
	ptr_hash_t new_hash = hash_create(hash->capacity, hash->load_factor, hash->compare, hash->hash_code, (hash->clone_key!=NULL)?hash->destroy_key:NULL, (hash->clone_value!=NULL)?hash->destroy_value:NULL, hash->clone_key, hash->clone_value);

	for (i=0; i<hash->capacity; i++) {
		ptr_hash_sync_t sync_entry = &(hash->entries[i]);
		ptr_hash_entry_t entry;
		entry = sync_entry->first_entry;
		while(entry != NULL) {
			if (hash->clone_key != NULL) {
				key = hash->clone_key(entry->key);
			} else {
				key = entry->key;
			}
			if (hash->clone_value != NULL) {
				value = hash->clone_value(entry->value);
			} else {
				value = entry->value;
			}
			hash_put(new_hash, key, value);
			entry = entry->next;
		}
	}

	return new_hash;
}

bool hash_contains_key(ptr_hash_t hash, void *key) {
	ptr_hash_entry_t entry;
	ptr_hash_sync_t sync_entry;
	uint32 idx = (hash->hash_code(key)) % hash->capacity;

	sync_entry = &(hash->entries[idx]);
	entry = sync_entry->first_entry;
	while(entry != NULL) {
		if(hash->compare(&key, &entry->key) == 0) return TRUE;
		entry = entry->next;
	}
	
	return FALSE;
}

uint32 hash_hash_code(ptr_hash_t hash) {
	if (hash->hash == 0) {
		uint32 i, hash_value = 0;
		for (i=0; i<hash->capacity; i++) {
			ptr_hash_sync_t sync_entry = &(hash->entries[i]);
			ptr_hash_entry_t entry, next;
			entry = sync_entry->first_entry;
			while(entry != NULL) {
				next = entry->next;
				hash_value += hash->hash_code(entry->key);
				entry = next;
			}
		}
		hash->hash = hash_value%UINT_MAX;
	}
	return hash->hash;
}

int32 hash_compare(const void *value1, const void *value2) {
	ptr_hash_t *h1 = (ptr_hash_t*)value1;
	ptr_hash_t *h2 = (ptr_hash_t*)value2;
	if ((*h1)->compare != (*h2)->compare) msgerror(E_UNKNOWN);
	return ((*h1)->size - (*h2)->size);
}

void hash_set_hash_code(ptr_hash_t hash, _HASH_CODE *hash_code) {
	hash->hash_code = hash_code;
	_rehash(hash, FALSE);
}

void hash_put(ptr_hash_t hash, void *key, void *value) {
	if (hash->size >= (uint32)hash->capacity*hash->load_factor) _rehash(hash, TRUE);
	if(!hash_contains_key(hash, key)) {
		uint32 idx = (hash->hash_code(key)) % hash->capacity;
		ptr_hash_sync_t sync_entry = &(hash->entries[idx]);
		ptr_hash_entry_t entry = sync_entry->first_entry;
		ptr_hash_entry_t new_entry;
		
		new_entry = (ptr_hash_entry_t)malloc(sizeof(hash_entry_t));
		if (new_entry == NULL) msgerror(E_NOMEM);

		new_entry->key = key;
		new_entry->value = value;
		new_entry->next = NULL;

		if(entry == NULL) {
			sync_entry->first_entry = new_entry;
		} else {
			while(entry->next != NULL) entry = entry->next;
			entry->next = new_entry;
		}
		hash->size++;
		hash->hash = 0;
	} 
}

void hash_clear(ptr_hash_t hash) {
	uint32 i;

	for(i=0; i<hash->capacity; i++){
		ptr_hash_entry_t entry, next;
		ptr_hash_sync_t sync_entry = &(hash->entries[i]);

		entry = sync_entry->first_entry;
		while(entry != NULL) {
			next = entry->next;
			if (hash->destroy_key && entry->key != NULL) {
				hash->destroy_key(entry->key);
			}
			if (hash->destroy_value && entry->value != NULL) {
				hash->destroy_value(entry->value);
			}
			free(entry);
			entry = next;
		}
		sync_entry->first_entry = NULL;
	}
	hash->size = 0;
	hash->hash = 0;
}

void hash_destroy(ptr_hash_t hash) {
	hash_clear(hash);
	free(hash->entries);
	free(hash);
}

void* hash_remove(ptr_hash_t hash, void *key) {
	void *value = NULL;
	uint32 idx = (hash->hash_code(key)) % hash->capacity;
	ptr_hash_sync_t sync_entry;
	
	sync_entry = &(hash->entries[idx]);
	if(hash_contains_key(hash, key)) {
		ptr_hash_entry_t entry = sync_entry->first_entry, last = NULL;
		
		while(hash->compare(&entry->key, &key) != 0) {
			last = entry;
			entry = entry->next;
		}
		if(last == NULL) {
			sync_entry->first_entry = entry->next;
		} else {
			last->next = entry->next;
		}
		value = entry->value;
		free(entry);
		hash->size--;
		hash->hash = 0;
	}

	return value;
}

void* hash_get(ptr_hash_t hash, void *key) {
	ptr_hash_entry_t entry;
	uint32 idx = (hash->hash_code(key)) % hash->capacity;
	ptr_hash_sync_t sync_entry;
	
	sync_entry = &(hash->entries[idx]);
	if(!hash_contains_key(hash, key)) {
		return NULL;
	}

	entry = sync_entry->first_entry;
	while(hash->compare(&entry->key, &key) != 0) entry = entry->next;

	return entry->value;
}

ptr_vector_t hash_get_keys(ptr_hash_t hash) {
	ptr_vector_t result = vector_create(hash->size, hash->compare, hash->hash_code, NULL, hash->clone_key);
	uint32 i=0;

	for(i=0; i<hash->capacity; i++) {
		ptr_hash_sync_t sync_entry = &(hash->entries[i]);
		ptr_hash_entry_t entry = NULL;

		entry = sync_entry->first_entry;
		while(entry != NULL) {
			vector_add(result, entry->key);
			entry = entry->next;
		}
	}

	return result;
}

ptr_vector_t hash_get_values(ptr_hash_t hash) {
	ptr_vector_t result = vector_create(hash->size, NULL, NULL, NULL, NULL);
	uint32 i=0;

	for(i=0; i<hash->capacity; i++) {
		ptr_hash_sync_t sync_entry = &(hash->entries[i]);
		ptr_hash_entry_t entry = NULL;

		entry = sync_entry->first_entry;
		while(entry != NULL) {
			vector_add(result, entry->value);
			entry = entry->next;
		}
	}

	return result;
}

ptr_vector_t hash_get_values_by_key(ptr_hash_t hash, void *key) {
	ptr_hash_entry_t entry;
	ptr_hash_sync_t sync_entry;
	ptr_vector_t result = NULL;
	uint32 idx = (hash->hash_code(key)) % hash->capacity;
		
	result = vector_create(10, NULL, NULL, NULL, hash->clone_value);

	sync_entry = &(hash->entries[idx]);	
	entry = sync_entry->first_entry;
	while(entry != NULL) {
		vector_add(result, entry->value);
		entry = entry->next;
	}

	return result;
}

