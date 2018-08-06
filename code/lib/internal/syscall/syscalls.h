#pragma once

/*
This file defines functions for all available system calls; implementations can be found in syscalls.s.
*/

#include <stdint.h>
#include <internal/syscall/msg.h>

// Prints the given string to kernel console. TODO remove, this is only for debugging
uint64_t sys_kputs(const char *str);

// TODO this does not do anything yet
int sys_exit();

// Switches to another thread, while the current one is e.g. waiting for messages (cooperative multitasking).
int sys_yield();

// Returns the type of the oldest non-processed message, if it exists.
msg_type_t sys_next_message_type();

// Returns and then deletes the oldest non-processed message, or 0 if there isn't any.
// The parameter messageBuffer must provide enough space to fit the given message, thus next_message_type() should be called first to determine the correct message size.
void sys_next_message(msg_header_t *messageBuffer);

// Changes the currently displayed process render context.
void sys_set_displayed_process(int contextId);