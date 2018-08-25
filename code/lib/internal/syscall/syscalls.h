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

// Draws a rectangle at the specified position with the given dimensions.
int sys_vbe_rectangle(uint32_t posX, uint32_t posY, uint32_t width, uint32_t height);

// Renders the given character at the given render buffer position. Currently only ASCII characters (' ' ... '~') are supported.
int sys_vbe_render_char(uint32_t posX, uint32_t posY, char c);

// Returns the width of the render buffer.
uint32_t sys_vbe_get_screen_width();

// Returns the height of the render buffer.
uint32_t sys_vbe_get_screen_height();

// Sets the current foreground color.
void sys_vbe_set_front_color(uint8_t r, uint8_t g, uint8_t b);

// Sets the current background color.
void sys_vbe_set_back_color(uint8_t r, uint8_t g, uint8_t b);

// Allocates a scrollable back buffer, and returns true on success.
bool sys_vbe_allocate_scroll_buffer(uint32_t height);

// Scrolls the buffer to the specified position.
void sys_vbe_set_scroll_position(uint32_t y);

// Clears the buffer with the current background color.
void sys_vbe_clear();

// Allocates a 4K aligned block of memory of the given size (rounded up to a multiple of 4K).
void *sys_heap_alloc(int size);

// Frees the given allocated memory.
void sys_heap_free(void *addr);

// Starts a new thread and sets the instruction pointer to the given address.
void sys_run_thread(uint64_t rip);

// Exits the current thread.
void sys_exit_thread();