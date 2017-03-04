#ifndef _INT_H_

#define _INT_H_

#include "util.h"

int32 int_compare(const void*, const void*);
uint32 int_hash_code(const void*);
void int_destroy(void*);
void* int_clone(void*);

uint32* int_replace_range(uint32*, uint32*, const uint32*, uint32, const uint32*, uint32);

#endif
