
#include <stdlib/stdlib.h>
#include <mm/malloc.h>

void *malloc(size_t size)
{
  spin_lock(&malloc_lock);
  void *ptr = dlmalloc(size);
  spin_unlock(&malloc_lock);
  return ptr;
}
