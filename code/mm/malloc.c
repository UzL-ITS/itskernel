
#include <mm/malloc.h>
#include <mm/heap.h>
#include <stdlib/string.h>

spinlock_t malloc_lock = SPIN_UNLOCKED;

/*
 * note: these are only functions required for dlmalloc to work, for the bulk
 * of the allocator see dlmalloc.c
 */

void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off)
{
  vm_acc_t vm_flags = 0;
  if (prot & PROT_READ)
    vm_flags |= VM_R;
  if (prot & PROT_WRITE)
    vm_flags |= VM_W;
  if (prot & PROT_EXEC)
    vm_flags |= VM_X;

  void *ptr = heap_alloc(len, vm_flags);
  if (!ptr)
    return MAP_FAILED;

  memclr(ptr, len);
  return ptr;
}

int munmap(void *addr, size_t len)
{
  heap_free(addr);
  return 0;
}
