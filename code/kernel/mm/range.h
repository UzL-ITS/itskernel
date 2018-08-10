
#ifndef _MM_RANGE_H
#define _MM_RANGE_H

#include <mm/common.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool range_alloc(uintptr_t addr, size_t len, vm_acc_t flags);
void range_free(uintptr_t addr, size_t len);

// Range allocated a contiguous amount of pages.
// Callers should make sure that the given virtual address and allocation length are a multiple of one of the possible page sizes, to avoid waste of physical memory.
bool range_alloc_contiguous(uintptr_t addr_start, size_t len, vm_acc_t flags, uint64_t *physicalAddress);

#endif
