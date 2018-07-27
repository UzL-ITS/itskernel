
#include <proc/syscalls.h>
#include <proc/sched.h>
#include <proc/thread.h>

void sys_exit(cpu_state_t *state)
{
  // TODO use status code:
  // int status = state->regs[RDI];

  // TODO real impl which actually kills the whole proc

  thread_t *thread = thread_get();
  thread_kill(thread);
  sched_tick(state);
}
