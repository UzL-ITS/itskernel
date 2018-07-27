
#ifndef _TRACE_UART_H
#define _TRACE_UART_H

/* initializes the uart driver */
void uart_init(void);

/* writes a character to the serial port */
void uart_putch(char c);

/* writes a string to the serial port */
void uart_puts(const char *str);

#endif
