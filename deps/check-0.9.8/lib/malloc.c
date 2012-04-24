/* AC_FUNC_MALLOC in configure defines malloc to rpl_malloc if
   malloc (0) is NULL to provide GNU compatibility */

#include "libcompat.h"

/* malloc has been defined to rpl_malloc, so first undo that */
#undef malloc

/* this gives us the real malloc to use below */
void *malloc (size_t n);

/* force malloc(0) to return a valid pointer */
void *
rpl_malloc (size_t n)
{
  if (n == 0)
    n = 1;
  return malloc (n);
}
