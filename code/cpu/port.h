
#ifndef _CPU_PORT_H
#define _CPU_PORT_H

#include <stdint.h>

/* waits an I/O cycle */
void iowait(void);

/* I/O port input and output functions */
uint8_t inb(uint16_t port);
uint16_t inw(uint16_t port);
uint32_t inl(uint16_t port);
uint8_t inb_p(uint16_t port);
uint16_t inw_p(uint16_t port);
uint32_t inl_p(uint16_t port);
void outb(uint16_t port, uint8_t value);
void outw(uint16_t port, uint16_t value);
void outl(uint16_t port, uint32_t value);
void outb_p(uint16_t port, uint8_t value);
void outw_p(uint16_t port, uint16_t value);
void outl_p(uint16_t port, uint32_t value);

#endif
