
#ifndef _VBE_H
#define _VBE_H

#include <stdbool.h>
#include <init/multiboot.h>
#include <vbe/res/font.h>

// The identifier of the kernel drawing context.
#define VBE_KERNEL_CONTEXT 0

// Initializes VBE with the given multiboot2 information block.
void vbe_init(multiboot_t *multiboot);

// Initializes the background buffers (needed for blitting) using the virtual memory manager.
void vbe_init_back_buffers();

// Creates a new VBE drawing context and returns its identifier.
int vbe_create_context();

// Sets the currently displayed VBE drawing context and redraws the scene.
// Should ONLY be called from proc.c, where appropriate locking is used.
void vbe_show_context(int contextId);

// Draws a rectangle at the specified position with the given dimensions.
int vbe_rectangle(int contextId, uint32_t posX, uint32_t posY, uint32_t width, uint32_t height);

// Renders the given character at the given render buffer position. Currently only ASCII characters (' ' ... '~') are supported.
int vbe_render_char(int contextId, uint32_t posX, uint32_t posY, char c);

// Returns the width of the render buffer.
uint32_t vbe_get_screen_width();

// Returns the height of the render buffer.
uint32_t vbe_get_screen_height();

// Sets the foreground color.
void vbe_set_front_color(int contextId, uint8_t r, uint8_t g, uint8_t b);

// Sets the background color.
void vbe_set_back_color(int contextId, uint8_t r, uint8_t g, uint8_t b);

// Allocates a scrollable back buffer for the given context, and returns true on success.
bool vbe_allocate_scroll_buffer(int contextId, uint32_t height);

// Scrolls the buffer of the given context to the specified position.
void vbe_set_scroll_position(int contextId, uint32_t y);

// Clears the buffer of the given context with its current background color.
void vbe_clear(int contextId);

// Returns a pointer to the primary render buffer of the given context. Only for debugging and experiments.
// Note: Drawing to this buffer does not immediately update the screen.
uint32_t* vbe_debug_get_render_buffer(int contextId);

#endif