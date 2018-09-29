
#ifndef _CPU_CPUID_H
#define _CPU_CPUID_H

#include <stdint.h>

#define CPUID_VENDOR       0x00000000
#define CPUID_FEATURES     0x00000001
#define CPUID_EXT_VENDOR   0x80000000
#define CPUID_EXT_FEATURES 0x80000001

#define CPUID_EXT_FEATURE_EDX_1GB_PAGE 0x04000000

void cpu_id(uint32_t code, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx);

void cpu_id_special(uint32_t eaxIn, uint32_t ecxIn, uint32_t *eaxOut, uint32_t *ebxOut, uint32_t *ecxOut, uint32_t *edxOut);

#endif
