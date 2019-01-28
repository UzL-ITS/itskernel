
#ifndef _PROC_SCHED_H
#define _PROC_SCHED_H

#include <cpu/state.h>
#include <proc/thread.h>
#include <stdbool.h>

// Starts the scheduler for the given core.
// The "bsp" flag should only be set for the initial boot core.
void sched_init(bool bsp);

/*
 * add/remove a thread from the scheduler's queue
 *
 * these functions should _not_ be called directly - use thread_suspend() and
 * thread_resume() instead, as they correctly deal with locking and updating
 * the thread's state.
 */
void sched_thread_resume(thread_t *thread);
void sched_thread_suspend(thread_t *thread);

void sched_tick(cpu_state_t *state);

#endif
