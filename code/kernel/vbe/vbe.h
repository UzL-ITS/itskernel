
#ifndef _VBE_H
#define _VBE_H

#include <init/multiboot.h>
#include <vbe/res/font.h>

// The identifier of the kernel drawing context.
#define VBE_KERNEL_CONTEXT 0

// Initializes VBE with the given multiboot2 information block.
void vbe_init(multiboot_t *multiboot);

// Initializes the background buffers (needed for blitting) using the virtual memory manager.
void vbe_init_back_buffers();

// Re-renders the entire screen from back buffer.
void vbe_redraw();

// Creates a new VBE drawing context and returns its identifier.
int vbe_create_context();

// Sets the currently displayed VBE drawing context and redraws the scene.
void vbe_show_context(int contextId);

// Draws a rectangle at the specified position with the given dimensions.
int vbe_rectangle(int contextId, uint16_t posX, uint16_t posY, uint16_t width, uint16_t height);

// Renders the given character at the given render buffer position. Currently only ASCII characters (' ' ... '~') are supported.
int vbe_render_char(int contextId, uint16_t posX, uint16_t posY, char c);

// Returns the width of the render buffer.
uint16_t vbe_get_screen_width();

// Returns the height of the render buffer.
uint16_t vbe_get_screen_height();

// Sets the foreground color.
void vbe_set_front_color(int contextId, uint8_t r, uint8_t g, uint8_t b);

// Sets the background color.
void vbe_set_back_color(int contextId, uint8_t r, uint8_t g, uint8_t b);

#endif