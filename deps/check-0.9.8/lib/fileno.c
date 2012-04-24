#include "libcompat.h"

int fileno(FILE *stream CK_ATTRIBUTE_UNUSED)
{
  assert (0);
  return 0;
}

