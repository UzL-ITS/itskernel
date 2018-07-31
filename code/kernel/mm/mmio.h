
#ifndef _MM_MMIO_H
#define _MM_MMIO_H

#include <mm/common.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Maps an arbitrary physical address into the kernel's virtual address space.
 * This address and length do not need to be page aligned, this is dealt with
 * by the function itself. However, aligned memory is preferable to avoid
 * wasted virtual address space.
 *
 * The main purpose of this function is for mapping memory-mapped I/O devices
 * into the kernel's virtual memory, however, it is also used in a few other
 * miscellaneous ways e.g. for mapping another task's PML4 table into virtual
 * memory temporarily to copy from it.
 */
void *mmio_map(uintptr_t phy, size_t len, vm_acc_t flags);

/* Unmaps a memory-mapped I/O area. */
void mmio_unmap(void *virt, size_t len);

#endif
