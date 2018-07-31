
#include <stdlib/stdlib.h>
#include <mm/malloc.h>

void *calloc(size_t num, size_t size)
{
  spin_lock(&malloc_lock);
  void *ptr = dlcalloc(num, size);
  spin_unlock(&malloc_lock);
  return ptr;
}
