
#ifndef _PROC_SYSCALLS_H
#define _PROC_SYSCALLS_H

#include <stdint.h>
#include <cpu/state.h>

#define SYS_TRACE 0
#define SYS_EXIT  1
#define SYS_YIELD 2

int64_t sys_trace(const char *message);
void sys_exit(cpu_state_t *state);
void sys_yield(cpu_state_t *state);

#endif
