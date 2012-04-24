#include "libcompat.h"

int
pipe (int *fildes CK_ATTRIBUTE_UNUSED)
{
  assert (0);
  return 0;
}
