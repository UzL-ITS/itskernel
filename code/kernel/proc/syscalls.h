
#ifndef _PROC_SYSCALLS_H
#define _PROC_SYSCALLS_H

#include <stdint.h>
#include <cpu/state.h>
#include <proc/msg.h>

#define SYS_TRACE 0
#define SYS_EXIT  1
#define SYS_YIELD 2

int64_t sys_trace(const char *message);
void sys_exit(cpu_state_t *state);
void sys_yield(cpu_state_t *state);

// Returns the type of the oldest non-processed message of the current process, if it exists.
msg_type_t sys_next_message_type();

// Returns and then deletes the oldest non-processed message of the current process, or 0 if there isn't any.
void sys_next_message(msg_header_t *messageBuffer);

// Changes the currently displayed process render context.
void sys_set_displayed_process(int contextId);

#endif
