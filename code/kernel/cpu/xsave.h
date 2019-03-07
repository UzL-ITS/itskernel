#pragma once

#include <stdint.h>

// Returns a sufficient memory block for XSAVE.
void* xsave_alloc(void);

// Free XSAVE memory block.
void xsave_free(void *mem);

// Saves the entire vector register state to the given memory block.
void xsave(void *mem);

// Restores the vector register state from the given memory block.
void xrstor(void *mem);