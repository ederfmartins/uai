#include <stdlib.h>

#include "int.h"

int32 int_compare(const void *i1, const void *i2) {
	int32 *ii1 = (int32*)i1;
	int32 *ii2 = (int32*)i2;
	return *ii1-*ii2;
}

int int_compare2(const void *i1, const void *i2) {	//DCMir - marreta
    int32 *ii1 = (int32*)i1;
    int32 *ii2 = (int32*)i2;
    return *ii1-*ii2;
}

uint32 int_hash_code(const void *key) {
	int32 i = (int32)key;
	
	return i;
}

void int_destroy(void* value) {
	// dummy function
}

void* int_clone(void* value) {
	// dummy function
	return value;
}

uint32* int_replace_range(uint32* source, uint32 *source_size, const uint32* range, uint32 range_size, const uint32* subs, uint32 subs_size) {
	uint32 i, size;
	uint32 *dest, dest_index = 0, dest_size = 0;

	if (range_size > subs_size) 
		msgerror(E_UNKNOWN, "int_replace_range: subs size must be greater or equal than range size");

	if (range_size < subs_size) {
		size = *source_size+(subs_size-range_size);
	} else {
		size = *source_size;
	}

	dest = (uint32*)malloc(size*sizeof(uint32));
	if (dest == NULL) msgerror(E_NOMEM);
	
	for (i=0; i<*source_size; i++) {
		if (bsearch(&source[i], range, range_size, sizeof(uint32), int_compare2) == NULL) 
			dest[dest_index++] = source[i];
	}
	dest_size = dest_index;
		
	for (i=0; i<subs_size; i++) {
		if (bsearch(&subs[i], dest, dest_size, sizeof(uint32), int_compare2) == NULL) 
			dest[dest_index++] = subs[i];
	}	

	*source_size = dest_index;
	free(source);
		
	qsort((uint32*)dest, *source_size, sizeof(uint32), int_compare2);

	return dest;

}	
