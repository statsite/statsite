/* AC_FUNC_REALLOC in configure defines realloc to rpl_realloc if
   realloc (p, 0) or realloc (0, n) is NULL to provide GNU
   compatibility */

#include "libcompat.h"

/* realloc has been defined to rpl_realloc, so first undo that */
#undef realloc
     
/* this gives us the real realloc to use below */
void *realloc (void *p, size_t n);
     
/* force realloc(p, 0) and realloc (NULL, n) to return a valid pointer */
void *
rpl_realloc (void *p, size_t n)
{
  if (n == 0)
    n = 1;
  if (p == 0)
    return malloc (n);
  return realloc (p, n);
}
