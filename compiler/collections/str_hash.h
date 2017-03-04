#ifndef _COLLECTIONS_STR_HASH_H_
#define _COLLECTIONS_STR_HASH_H_

#include "../collections/str.h"
#include "../collections/hash.h"

#define hash_create_str_container(size, load_factor) hash_create((size), (load_factor), (_COMPARE*) &str_compare, (_HASH_CODE*) &str_hash_code, NULL, NULL, NULL, NULL)

#endif // _COLLECTIONS_STR_HASH_H_
