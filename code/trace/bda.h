
#ifndef _BDA_H
#define _BDA_H

#include <stdint.h>

#define BDA_VGA_MODE  0x49
#define BDA_VGA_PORT  0x63
#define BDA_EBDA      0x0E
#define BDA_COM1_PORT 0x00
#define BDA_COM2_PORT 0x02
#define BDA_COM3_PORT 0x04
#define BDA_COM4_PORT 0x06

uint8_t bda_read(uint8_t off);
uint16_t bda_reads(uint8_t off);

#endif
