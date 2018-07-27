
#include <stdlib/string.h>
#include <stdint.h>

void *memmove(void *dst, const void *src, size_t len)
{
  if (src == dst)
    return dst;

  const void *src_end = (const void *) ((uintptr_t) src + len);
  if (src < dst && dst < src_end)
  {
    char *dst8 = (char *) dst;
    const char *src8 = (const char *) src;

    for (ptrdiff_t i = len - 1; i >= 0; i--)
      dst8[i] = src8[i];

    return dst;
  }

  return memcpy(dst, src, len);
}
