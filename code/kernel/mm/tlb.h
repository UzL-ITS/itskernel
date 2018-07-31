
#ifndef _MM_TLB_H
#define _MM_TLB_H

#include <stdint.h>

#define TLB_OP_INVLPG 0x0
#define TLB_OP_FLUSH  0x1

typedef struct
{
  int type;
  uintptr_t addr;
} tlb_op_t;

void tlb_init(void);

void tlb_transaction_init(void);

void tlb_transaction_queue_invlpg(uintptr_t addr);
void tlb_transaction_queue_flush(void);

void tlb_transaction_rollback(void);
void tlb_transaction_commit(void);

#endif
