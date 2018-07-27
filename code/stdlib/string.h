
#ifndef STRING_H
#define STRING_H

#include <stddef.h>

int strcmp(const char *str1, const char *str2);
size_t strlen(const char *str);
void *memclr(void *ptr, size_t len);
void *memset(void *ptr, int value, size_t len);
void *memcpy(void *dst, const void *src, size_t len);
void *memmove(void *dst, const void *src, size_t len);
int memcmp(const void *ptr1, const void *ptr2, size_t len);

#endif
