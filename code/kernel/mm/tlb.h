
#ifndef _MM_TLB_H
#define _MM_TLB_H

#include <stdint.h>

// The different TLB operation types.
typedef enum
{
	// Invalidate the entry for the given virtual address.
	TLB_OP_INVLPG = 0,
	
	// Flush the whole TLB.
	TLB_OP_FLUSH = 1
} tlb_op_type_t;

// Describes a TLB operation queue entry.
typedef struct
{
	// The operation type.
	tlb_op_type_t type;
	
	// The address for the invalidation operation.
	uintptr_t addr;
} tlb_op_t;

// The size of the TLB operation queue.
#define TLB_OP_QUEUE_SIZE 16

void tlb_init(void);

void tlb_transaction_init(void);

void tlb_transaction_queue_invlpg(uintptr_t addr);

void tlb_transaction_commit(void);

#endif
