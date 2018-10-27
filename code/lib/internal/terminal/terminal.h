#pragma once

/*
ITS kernel standard library terminal implementation.
*/

/* INCLUDES */

#include <stdint.h>
#include <internal/keyboard/keycodes.h>


/* TYPES */




/* DECLARATIONS */

// Initializes the terminal with the given amount of lines.
void terminal_init(int lines);

// Writes the given char to the terminal.
void terminal_putc(char c);

// Writes the given string to the terminal.
void terminal_puts(const char *str);

// Handles the given navigation key press.
void terminal_handle_navigation_key(vkey_t keyCode);

// Draws a rectangle relative to the current position.
void terminal_draw(uint32_t x, uint32_t y, uint32_t width, uint32_t height);