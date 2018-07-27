
#include <stdlib/stdlib.h>
#include <mm/malloc.h>

void *realloc(void *ptr, size_t size)
{
  spin_lock(&malloc_lock);
  ptr = dlrealloc(ptr, size);
  spin_unlock(&malloc_lock);
  return ptr;
}
