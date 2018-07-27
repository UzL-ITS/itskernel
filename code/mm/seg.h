
#ifndef _MM_SEG_H
#define _MM_SEG_H

#include <mm/common.h>
#include <lock/spinlock.h>
#include <util/list.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum
{
  SEG_FREE,
  SEG_ALLOCATED
} seg_state_t;

typedef struct seg_block
{
  list_node_t node;
  uintptr_t start;
  uintptr_t end;
  seg_state_t state;
  vm_acc_t flags;
} seg_block_t;

typedef struct
{
  spinlock_t lock;
  list_t block_list;
} seg_t;

bool seg_init(seg_t *segments);
void seg_destroy(void);
bool seg_alloc_at(void *ptr, size_t size, vm_acc_t flags);
void *seg_alloc(size_t size, vm_acc_t flags);
void seg_free(void *ptr);
void seg_trace(void);

#endif
