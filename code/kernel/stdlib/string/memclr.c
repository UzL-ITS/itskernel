
#include <stdlib/string.h>

void *memclr(void *ptr, size_t len)
{
  return memset(ptr, 0, len);
}
