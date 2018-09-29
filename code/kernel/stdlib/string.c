
#include <stdlib/string.h>

int strcmp(const char *str1, const char *str2)
{
  const unsigned char *s1 = (const unsigned char *) str1;
  const unsigned char *s2 = (const unsigned char *) str2;

  while (*s1 && *s1 == *s2)
  {
    s1++;
    s2++;
  }

  return *s1 - *s2;
}

void *memset(void *ptr, int value, size_t len)
{
  // TODO: volatile is a hacky workaround, gcc 4.8.0 at -O3 seems to optimize
  // away most of the code here...
  volatile char *ptr8 = (volatile char *) ptr;

  while (len--)
    *ptr8++ = value;

  return ptr;
}

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

void *memcpy(void *dst, const void *src, size_t len)
{
  char *dst8 = (char *) dst;
  const char *src8 = (char *) src;

  while (len--)
    *dst8++ = *src8++;

  return dst;
}

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

void *memclr(void *ptr, size_t len)
{
  return memset(ptr, 0, len);
}

char *itoa(uint64_t value, char *str, int base)
{
	// Only support bases {0, 1} till {0, ..., 9, A, ..., Z}
	if(base <= 1 || base > 36)
		return 0;
	
	// Divide value by base, store remainder in string for each iteration
	// This produces a reversed string representation of value
	int pos = 0;
	do
	{
		// Add remainder to string
		// Use ASCII uppercase letters for digits > 9
		char c = '0' + (value % base);
		if(c > '9')
			c += 7;
		str[pos] = c;
		
		// Next multiple of base
		++pos;
	}
	while((value /= base) > 0);
	
	// Append terminating 0 and reverse resulting string
	str[pos] = '\0';
	strrev(str);
	return str;
}

char *strncpy(char *dest, const char *src, int n)
{
	// Copy string
	char *ret = dest;
	do
	{
		if(!n--)
			return ret;
	}
	while((*dest++ = *src++));
	
	// Pad destination string with zeroes, if necessary
	while(n--)
		*dest++ = 0;
	return ret;
}

size_t strlen(const char *str)
{
  const char *s;
  for (s = str; *s; s++);
  return (size_t) (s - str);
}

char *strrev(char *str)
{
	// Run through string from both sides and swap characters
	char *s1 = str;
	char *s2 = &str[strlen(str) - 1];
	while(s1 < s2)
	{
		// Swap
		char c = *s1;
		*s1 = *s2;
		*s2 = c;
		
		// Next
		++s1;
		--s2;
	}
	return str;
}

int strncmp(const char *str1, const char *str2, int n)
{
	// Do unsigned comparison
	const unsigned char *s1 = (const unsigned char *)str1;
	const unsigned char *s2 = (const unsigned char *)str2;
	
	// Compare character-wise; if *s2 == 0, the loop will exit
    while(n && *s1 && (*s1 == *s2))
    {
        ++s1;
        ++s2;
        --n;
    }
	
	// Length >= n and all characters equal?
    if(n == 0)
        return 0;
    else
        return *s1 - *s2;
}