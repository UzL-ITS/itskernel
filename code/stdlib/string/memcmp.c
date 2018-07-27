
#include <stdlib/string.h>

int memcmp(const void *ptr1, const void *ptr2, size_t len)
{
  const unsigned char *p1 = (const unsigned char *) ptr1;
  const unsigned char *p2 = (const unsigned char *) ptr2;

  while (len--)
  {
    if (*p1 != *p2)
      return *p1 - *p2;

    p1++;
    p2++;
  }

  return 0;

}
