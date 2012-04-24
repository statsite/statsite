#include "libcompat.h"

unsigned int
sleep (unsigned int seconds CK_ATTRIBUTE_UNUSED)
{
  assert (0);
  return 0;
}
