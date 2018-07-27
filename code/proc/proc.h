
#ifndef _PROC_PROC_H
#define _PROC_PROC_H

#include <mm/seg.h>
#include <proc/thread.h>
#include <lock/spinlock.h>
#include <util/list.h>
#include <stdint.h>

typedef enum
{
  PROC_RUNNING
} proc_state_t;

typedef struct proc
{
  /* state */
  proc_state_t state;

  /* physical address of the pml4 table of this process */
  uintptr_t pml4_table;

  /* vmm address space lock */
  spinlock_t vmm_lock;

  /* list of threads in this process */
  list_t thread_list;

  /* memory segments */
  seg_t segments;
} proc_t;

proc_t *proc_create(void);
proc_t *proc_get(void);
void proc_switch(proc_t *proc);

/*
 * add/remove a thread from this process.
 *
 * these methods should _not_ be called directly. to make a new thread, use
 * thread_create() and specify the proc to attach it to.
 *
 * to stop a thread, use thread_kill(). it will be removed from the process
 * automatically after it is dead.
 */
void proc_thread_add(proc_t *proc, thread_t *thread);
void proc_thread_remove(proc_t *proc, thread_t *thread);

void proc_destroy(proc_t *proc);

#endif
