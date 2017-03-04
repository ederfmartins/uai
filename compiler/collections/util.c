#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>

#include "util.h"

#include <errno.h>

extern int errno;

/*#ifndef _QUIET*/  
static const char *errmsgs[] = {      /* Mensagens de erro */
  /* E_NONE        0 */  "sem erro\n",
  /* E_NOMEM      -1 */  "memoria insuficiente '%s'\n",
  /* E_FOPEN      -2 */  "nao consegue abrir arquivo '%s'\n",
  /* E_FREAD      -3 */  "erro de leitura no arquivo '%s'\n",
  /* E_FWRITE     -4 */  "erro de escrita no arquivo '%s'\n",
  /* E_OPTION     -5 */  "opcao desconhecida '%s'\n",
  /* E_OPTARG     -6 */  "opcao de argumento invalida -'%s'\n",
  /* E_FINEXIST   -7 */  "arquivo de entrada inexistente '%s'\n",
  /* E_UNKNOWN    -8 */  "erro desconhecido '%s'\n",
  /* E_NOTFOUND   -9 */  "elemento nao encontrado '%s'\n",
  /* E_PARSEFAIL -10 */  "parsing problems: '%s'\n"
};
/* #endif */

void msgerror (int code, ...) {                          
	va_list    args;   
	const char *msg;  

	if ((code > 0) || (code < E_UNKNOWN)) code = E_UNKNOWN;

	msg = errmsgs[-code];     
	//fprintf(stderr, "\n%s: ", prgname);
	va_start(args, code);         
	vfprintf(stderr, msg, args); 
	va_end(args);

	fprintf(stderr, "errno = %d\n", errno);

	exit(code);
}

uint32 actual_line_size = 128;

char *read_line(FILE *file) {
	register uint32 size;
	char *line;
	
	line = (char*)calloc(actual_line_size,sizeof(char));
	if (line == NULL) msgerror(E_NOMEM);

	fgets(line, actual_line_size, file);
	size = strlen(line);
	while (!feof(file) && line[size-1] != '\n') { 
		char *comp_line;
		uint32 new_size = (actual_line_size * 2) / 3;

		actual_line_size += new_size;

		comp_line = (char*)calloc(new_size,sizeof(char));
		if (comp_line == NULL) msgerror(E_NOMEM);

		fgets(comp_line, new_size, file);

		line = (char*)realloc(line, actual_line_size*sizeof(char));
		if (line == NULL) msgerror(E_NOMEM);

		strcat(line, comp_line);
		size = strlen(line);

		free(comp_line);
	}	
	
	if (feof(file) && size == 0) {
		free(line);
		line = NULL;
	} else {
		line[size-1] = '\0';
	}	
	
	return line;
}

double elapsed_time(const struct timeval tbegin, const struct timeval tend) {
	double begin, end;

	begin = tbegin.tv_sec + (tbegin.tv_usec/MICROSEC);
	end = tend.tv_sec + (tend.tv_usec/MICROSEC);

	return (double)(end - begin);
}

//--------------------------------------------------------------------------------


#define MIN_MERGESORT_LIST_SIZE    32

void mergesort_array(int a[], int size, int temp[]) {
    int i1, i2, tempi;
    if (size < MIN_MERGESORT_LIST_SIZE) {
        /* Use insertion sort */
        int i;
        for (i=0; i < size; i++) {
            int j, v = a[i];
            for (j = i - 1; j >= 0; j--) {
               if (a[j] <= v) break;
                a[j + 1] = a[j];
            }
            a[j + 1] = v;
        }
        return;
    }

    mergesort_array(a, size/2, temp);
    mergesort_array(a + size/2, size - size/2, temp);
    i1 = 0;
    i2 = size/2;
    tempi = 0;
    while (i1 < size/2 && i2 < size) {
        if (a[i1] < a[i2]) {
            temp[tempi] = a[i1];
            i1++;
        } else {
            temp[tempi] = a[i2];
            i2++;
        }
        tempi++;
    }

    while (i1 < size/2) {
        temp[tempi] = a[i1];
        i1++;
        tempi++;
    }
    while (i2 < size) {
        temp[tempi] = a[i2];
        i2++;
        tempi++;
    }

    memcpy(a, temp, size*sizeof(int));
}

void mergesort___(void *base, size_t nmemb, size_t size, int(*compar)(const void *, const void *))
{

}

