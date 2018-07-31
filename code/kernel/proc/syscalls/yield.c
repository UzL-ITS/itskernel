
#include <proc/syscalls.h>
#include <cpu/state.h>
#include <proc/sched.h>

void sys_yield(cpu_state_t *state)
{
  state->regs[RAX] = 0; /* when yield() returns in userspace 0 is returned */
  sched_tick(state);
}
