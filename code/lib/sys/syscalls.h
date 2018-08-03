#pragma once

/*
This file defines functions for all available system calls; implementations can be found in syscalls.s.
*/

#include <stdint.h>
#include <sys/msg.h>

// TEST Prints the given string to kernel console.
uint64_t testprintf(const char *str);

// Returns the type of the oldest non-processed message, if it exists.
msg_type_t next_message_type();

// Returns and then deletes the oldest non-processed message, or 0 if there isn't any.
// The parameter messageBuffer must provide enough space to fit the given message, thus next_message_type() should be called first to determine the correct message size.
void next_message(msg_header_t *messageBuffer);

// Changes the currently displayed process render context.
void set_displayed_process(int contextId);