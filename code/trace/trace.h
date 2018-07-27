/*
NOTE: This functions should only be used on startup! Use the console functions for later outputs, once the scheduler is running.
*/

#ifndef _TRACE_H
#define _TRACE_H

#include <stdarg.h>

void trace_init(void);
void trace_putch(char c);
void trace_puts(const char *str);
void trace_printf(const char *fmt, ...);
void trace_vprintf(const char *fmt, va_list args);

#endif
