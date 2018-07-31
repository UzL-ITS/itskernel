
#ifndef _CPU_TLB_H
#define _CPU_TLB_H

#include <stdint.h>

void tlb_invlpg(uintptr_t address);
void tlb_flush(void);

#endif
