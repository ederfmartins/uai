#include <stdlib.h>
#include <limits.h>
#include <ctype.h>
#include <string.h>

#include "str.h"

int32 str_compare(const void *v1, const void *v2) {
	char **c1 = (char**)v1, **c2 = (char**)v2;
	return strcmp(*c1, *c2);
}

char* str_clone(const char* str) {
	return strdup(str);
}

uint32 str_hash_code(char *key) {
	char *char_key = (char*)key;
	uint32 h = 0, off = 0;
	int32 len;
	
	len = strlen(char_key);
	if (len > 0) {
		do {
			h = 31*h + (uint32)char_key[off++];
		} while (--len > 0);
	}
	// if(h < 0) h = (-1) * h;
	return h%UINT_MAX;
}

void str_destroy(char *value) {
	free(value);
}

void str_trim(char *str) {
	uint32 i, len;
	int32 j;

	len = strlen(str);
	i = 0;
	while((i < len) && ((str[i] == ' ') || (str[i] == '\t') || 
			(str[i] == '\n') || (str[i] == '\r'))) {
		i++;
	}
	j = len - 1;
	while((j >= 0) && ((str[j] == ' ') || (str[j] == '\t') ||
			(str[i] == '\n') || (str[i] == '\r'))) {
		j--;
	}

	if (j >= 0) str[j + 1] = '\0';
	
	if (i <= j) {
		strcpy(str, str + i);
	} else {
		strcpy(str, "");
	}
}

void str_to_lower(char *str) {
	uint32 i, len = strlen(str);
	for(i=0; i<len; i++) {
		str[i] = (char)tolower((int)str[i]);
	}
}

void str_to_upper(char *str) {
	uint32 i, len = strlen(str);
	for(i=0; i<len; i++) {
		str[i] = (char)toupper((int)str[i]);
	}
}

void str_remove_tags(char *str) {
	uint32 begin = 0, end = 0, i, j;
	bool opened = FALSE;
	
	for (i=0; i<strlen(str); i++) {
		if (str[i] == '<') {
			begin = i;
			opened = TRUE;
		} else if (opened && str[i] == '>') {
			end = i;
			opened = FALSE;
			for (j=begin; j<=end; j++) {
				str[j] = ' ';
			}
		}
	}
	str_trim(str);
}
