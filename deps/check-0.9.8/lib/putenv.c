#include "libcompat.h"

int
putenv (const char *string CK_ATTRIBUTE_UNUSED);
{
  assert (0);
  return 0;
}
