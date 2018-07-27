
#ifndef STDLIB_H
#define STDLIB_H

#include <stddef.h>

void *malloc(size_t size);
void *memalign(size_t alignment, size_t size);
void *calloc(size_t num, size_t size);
void *realloc(void *ptr, size_t size);
void free(void *ptr);

#endif
