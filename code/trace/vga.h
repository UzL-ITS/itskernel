
#ifndef _TRACE_VGA_H
#define _TRACE_VGA_H

// Initializes the VGA/VBE driver.
void vga_init(void);

// Renders the given character.
void vga_putch(char c);

// Renders the given string (including certain control characters).
void vga_puts(const char *str);

#endif
