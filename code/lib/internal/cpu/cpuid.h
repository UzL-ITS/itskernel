#pragma once

/*
CPUID commands.
*/

/* INCLUDES */

#include <stdint.h>


/* TYPES */



/* DECLARATIONS */

// Runs the CPUID instruction with the given input register EAX, and writes the output registers into the given variables.
void cpuid(uint32_t eaxIn, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx);

// Runs the CPUID instruction with the given input registers EAX and ECX, and writes the output registers into the given variables.
void cpuid_ext(uint32_t eaxIn, uint32_t ecxIn, uint32_t *eaxOut, uint32_t *ebxOut, uint32_t *ecxOut, uint32_t *edxOut);