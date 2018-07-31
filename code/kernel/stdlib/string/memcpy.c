
#include <stdlib/string.h>

void *memcpy(void *dst, const void *src, size_t len)
{
  char *dst8 = (char *) dst;
  const char *src8 = (char *) src;

  while (len--)
    *dst8++ = *src8++;

  return dst;
}
