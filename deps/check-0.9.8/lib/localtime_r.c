#include "libcompat.h"
  
struct tm *
localtime_r (const time_t *clock, struct tm *result)
{
  struct tm *now = localtime (clock);
  if (now == NULL)
    {
      return NULL;
    }
  else
    {
      *result = *now;
    }
  return result;
}
