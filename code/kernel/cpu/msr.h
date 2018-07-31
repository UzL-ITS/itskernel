
#ifndef _CPU_MSR_H
#define _CPU_MSR_H

#include <stdint.h>

#define MSR_EFER           0xC0000080
#define MSR_FS_BASE        0xC0000100
#define MSR_GS_BASE        0xC0000101
#define MSR_GS_KERNEL_BASE 0xC0000102
#define MSR_APIC_BASE      0x0000001B
#define MSR_STAR           0xC0000081
#define MSR_LSTAR          0xC0000082
#define MSR_CSTAR          0xC0000083
#define MSR_SFMASK         0xC0000084

#define APIC_BASE_ENABLED 0x800
#define APIC_BASE_X2_MODE 0x400
#define APIC_BASE_BSP     0x100

uint64_t msr_read(uint32_t msr);
void msr_write(uint32_t msr, uint64_t value);

#endif
