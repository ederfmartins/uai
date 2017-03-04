#ifndef _STR_H_

#define _STR_H_

#include "util.h"

int32 str_compare(const void*, const void*);

char* str_clone(const char*);
uint32 str_hash_code(char*);
void str_destroy(char*);

void str_trim(char*);
void str_to_lower(char*);
void str_to_upper(char*);
void str_remove_tags(char*);

#endif
