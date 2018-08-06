
#ifndef _PROC_SYSCALLS_H
#define _PROC_SYSCALLS_H

#include <stdint.h>
#include <cpu/state.h>
#include <proc/msg.h>

#define SYS_TRACE 0
#define SYS_EXIT  1
#define SYS_YIELD 2

// Prints the given string to kernel console. TODO remove, this is only for debugging
int64_t sys_trace(const char *message);

// TODO this does not do anything yet
void sys_exit(cpu_state_t *state);

// Switches to another thread, while the current one is e.g. waiting for messages (cooperative multitasking).
void sys_yield(cpu_state_t *state);

// Returns the type of the oldest non-processed message of the current process, if it exists.
msg_type_t sys_next_message_type();

// Returns and then deletes the oldest non-processed message, or 0 if there isn't any.
// The parameter messageBuffer must provide enough space to fit the given message, thus next_message_type() should be called first to determine the correct message size.
void sys_next_message(msg_header_t *messageBuffer);

// Changes the currently displayed process render context.
void sys_set_displayed_process(int contextId);

#endif
