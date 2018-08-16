
#ifndef _MM_RANGE_H
#define _MM_RANGE_H

#include <mm/common.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool range_alloc(uintptr_t addr, size_t len, vm_acc_t flags);
void range_free(uintptr_t addr, size_t len);

// Range allocated a contiguous amount of pages.
bool range_alloc_contiguous(uintptr_t virtualAddress, int size, int count, vm_acc_t flags, uint64_t *physicalAddress);

#endif
