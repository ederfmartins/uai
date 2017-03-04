#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define MEMCPY(dst, src, s)							\
  ((s) == sizeof (int)								\
   ? (*(int *) (dst) = *(int *) (src), (dst))		\
   : memcpy (dst, src, s))

typedef int (*__compar_fn_t)(const void * , const void * );

static void
msort(void * b, size_t n, size_t s, __compar_fn_t cmp, char *t)
{
  char *tmp;
  char *b1, *b2;
  size_t n1, n2;
  
  if (n <= 1)
    return;
  
  n1 = n / 2;
  n2 = n - n1;
  b1 = (char *) b;
  b2 = (char *) b + (n1 * s);
  
  msort (b1, n1, s, cmp, t);
  msort (b2, n2, s, cmp, t);
  
  tmp = t;
  
  while (n1 > 0 && n2 > 0)
    {
      if ((*cmp) (b1, b2) <= 0)
        {
          MEMCPY (tmp, b1, s);
          b1 += s;
          --n1;
        }
      else
        {
          MEMCPY (tmp, b2, s);
          b2 += s;
          --n2;
        }
      tmp += s;
    }
  if (n1 > 0)
    memcpy (tmp, b1, n1 * s);
  memcpy (b, t, (n - n2) * s);
}
