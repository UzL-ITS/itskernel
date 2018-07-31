
#include <stdlib/string.h>

void *memset(void *ptr, int value, size_t len)
{
  // TODO: volatile is a hacky workaround, gcc 4.8.0 at -O3 seems to optimize
  // away most of the code here...
  volatile char *ptr8 = (volatile char *) ptr;

  while (len--)
    *ptr8++ = value;

  return ptr;
}
