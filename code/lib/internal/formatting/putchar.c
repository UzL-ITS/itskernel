/*
Implements _putchar() to enable the printf library to write to the terminal.
*/

/* INCLUDES */

#include <internal/formatting/printf.h>
#include <internal/terminal/terminal.h>


/* FUNCTIONS */

void _putchar(char c)
{
	terminal_putc(c);
}
