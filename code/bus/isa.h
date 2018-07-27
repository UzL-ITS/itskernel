
#ifndef _BUS_ISA_H
#define _BUS_ISA_H

#include <defs/types.h>
#include <stdint.h>

/*
 * There are a maximum of 16 ISA interrupts as two PIC chips are used in
 * master-slave mode.
 */
#define ISA_INTR_LINES 16

typedef uint8_t isa_line_t;

/* Sets up the initial mapping of ISA interrupt lines to IRQ numbers. */
void isa_init(void);

/*
 * Returns a pointer to the tuple which describes a particular ISA interrupt
 * line.
 */
irq_tuple_t *isa_irq(isa_line_t line);

#endif
