
#include <stdlib/stdlib.h>
#include <mm/malloc.h>

void *memalign(size_t alignment, size_t size)
{
  spin_lock(&malloc_lock);
  void *ptr = dlmemalign(alignment, size);
  spin_unlock(&malloc_lock);
  return ptr;
}
