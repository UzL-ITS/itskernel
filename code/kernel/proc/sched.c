
#include <proc/sched.h>
#include <cpu/halt.h>
#include <proc/proc.h>
#include <smp/cpu.h>
#include <smp/mode.h>
#include <time/apic.h>
#include <time/pit.h>
#include <util/container.h>
#include <util/list.h>
#include <panic/panic.h>
#include <stdlib/string.h>

#define SCHED_TIMESLICE 10 /* 10ms = 100Hz */

/* a list of threads that are ready to run */
static spinlock_t thread_queue_lock = SPIN_UNLOCKED;
static list_t thread_queue = LIST_EMPTY;

void sched_init(void)
{
  // Start scheduler timer for this CPU
  if (smp_mode == MODE_SMP)
    apic_monotonic(SCHED_TIMESLICE, &sched_tick);
  else
    pit_monotonic(SCHED_TIMESLICE, &sched_tick);
}

void sched_thread_resume(thread_t *thread)
{
  spin_lock(&thread_queue_lock);
  list_add_tail(&thread_queue, &thread->sched_node);
  spin_unlock(&thread_queue_lock);
}

void sched_thread_suspend(thread_t *thread)
{
  spin_lock(&thread_queue_lock);
  list_remove(&thread_queue, &thread->sched_node);
  spin_unlock(&thread_queue_lock);
}

void sched_tick(cpu_state_t *state)
{
  cpu_t *cpu = cpu_get();

  /* figure out what thread is currently running on the CPU */
  thread_t *cur_thread = cpu->thread;
  thread_t *new_thread = 0;

  spin_lock(&thread_queue_lock);

  /* add the current thread to the queue if it is runnable */
  if (cur_thread && cur_thread->state == THREAD_RUNNING)
  {
    list_add_tail(&thread_queue, &cur_thread->sched_node);
  }

  /* pick the next thread to run */
  list_node_t *head = thread_queue.head;
  if (head)
  {
    new_thread = container_of(head, thread_t, sched_node);
    list_remove(&thread_queue, head);
  }

  spin_unlock(&thread_queue_lock);

  /* if there is no new thread, switch to the idle thread */
  if (!new_thread)
    new_thread = cpu->idle_thread;

  /* check if we're actually switching threads */
  if (cur_thread != new_thread)
  {
    /* actually swap the pointers over */
    cpu->thread = new_thread;

    /* save the register file for the current thread */
    if (cur_thread)
    {
      memcpy(cur_thread->regs, state->regs, sizeof(state->regs));
      cur_thread->rip = state->rip;
      cur_thread->rsp = state->rsp;
      cur_thread->rflags = state->rflags;
      cur_thread->cs = state->cs;
      cur_thread->ss = state->ss;
    }

    /* restore the register file for the new thread */
    memcpy(state->regs, new_thread->regs, sizeof(state->regs));
    state->rip = new_thread->rip;
    state->rsp = new_thread->rsp;
    state->rflags = new_thread->rflags;
    state->cs = new_thread->cs;
    state->ss = new_thread->ss;

    /* if we're switcing between processes, we need to switch address spaces */
    if (!cur_thread || cur_thread->proc != new_thread->proc)
      proc_switch(new_thread->proc); /* (this also sets cpu->proc) */

    /* write new kernel stack pointer into the TSS */
    tss_set_rsp0(new_thread->kernel_rsp);
  }
}
