
#ifndef _MM_VMM_H
#define _MM_VMM_H

#include <mm/common.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void vmm_init(void);
bool vmm_init_pml4(uintptr_t pml4_table_addr);

bool vmm_touch(uintptr_t virt, int size);

bool vmm_map(uintptr_t virt, uintptr_t phy, vm_acc_t flags);
bool vmm_maps(uintptr_t virt, uintptr_t phy, vm_acc_t flags, int size);

uintptr_t vmm_unmap(uintptr_t virt);
uintptr_t vmm_unmaps(uintptr_t virt, int size);

void vmm_untouch(uintptr_t virt, int size);

bool vmm_map_range(uintptr_t virt, uintptr_t phy, size_t len, vm_acc_t flags);
void vmm_unmap_range(uintptr_t virt, size_t len);

int vmm_size(uintptr_t virt);

#endif
