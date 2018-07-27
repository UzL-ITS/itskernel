
#ifndef _VBE_H
#define _VBE_H

#include <init/multiboot.h>
#include <vbe/res/font.h>

// Initializes VBE with the given multiboot2 information block.
void vbe_init(multiboot_t *multiboot);

// Draws a rectangle at the specified position with the given dimensions.
int vbe_rectangle(uint16_t posX, uint16_t posY, uint16_t width, uint16_t height);

// Renders the given character at the given render buffer position. Currently only ASCII characters (' ' ... '~') are supported.
int vbe_render_char(uint16_t posX, uint16_t posY, char c);

// Returns the width of the render buffer.
uint16_t vbe_get_screen_width();

// Returns the height of the render buffer.
uint16_t vbe_get_screen_height();

// Sets the foreground color.
void vbe_set_front_color(uint8_t r, uint8_t g, uint8_t b);

// Sets the background color.
void vbe_set_back_color(uint8_t r, uint8_t g, uint8_t b);

#endif