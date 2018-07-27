
#include <stdlib/stdlib.h>
#include <mm/malloc.h>

void free(void *ptr)
{
  spin_lock(&malloc_lock);
  dlfree(ptr);
  spin_unlock(&malloc_lock);
}
