
#ifndef _MM_SEQ_H
#define _MM_SEQ_H

#include <stddef.h>

/*
 * Allocates memory from a small 'pool' that can be used before the real
 * allocator has been set up. This memory can never be freed, and the pool is
 * small, so this should be used only when absolutely necessary.
 */
void *seq_alloc(size_t len);

void seq_trace(void);

#endif
