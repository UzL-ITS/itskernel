
#ifndef _INTR_COMMON_H
#define _INTR_COMMON_H

/* the max number of interrupts */
#define INTERRUPTS 256

/* CPU exceptions */
#define FAULT0  0x00
#define FAULT1  0x01
#define FAULT2  0x02
#define FAULT3  0x03
#define FAULT4  0x04
#define FAULT5  0x05
#define FAULT6  0x06
#define FAULT7  0x07
#define FAULT8  0x08
#define FAULT9  0x09
#define FAULT10 0x0A
#define FAULT11 0x0B
#define FAULT12 0x0C
#define FAULT13 0x0D
#define FAULT14 0x0E
#define FAULT15 0x0F
#define FAULT16 0x10
#define FAULT17 0x11
#define FAULT18 0x12
#define FAULT19 0x13
#define FAULT20 0x14
#define FAULT21 0x15
#define FAULT22 0x16
#define FAULT23 0x17
#define FAULT24 0x18
#define FAULT25 0x19
#define FAULT26 0x1A
#define FAULT27 0x1B
#define FAULT28 0x1C
#define FAULT29 0x1D
#define FAULT30 0x1E
#define FAULT31 0x1F

/* master PIC IRQs */
#define IRQ0  0x20
#define IRQ1  0x21
#define IRQ2  0x22
#define IRQ3  0x23
#define IRQ4  0x24
#define IRQ5  0x25
#define IRQ6  0x26
#define IRQ7  0x27

/* slave PIC IRQs */
#define IRQ8  0x28
#define IRQ9  0x29
#define IRQ10 0x2A
#define IRQ11 0x2B
#define IRQ12 0x2C
#define IRQ13 0x2D
#define IRQ14 0x2E
#define IRQ15 0x2F

/* I/O APIC IRQs */
#define IRQ16 0x30
#define IRQ17 0x31
#define IRQ18 0x32
#define IRQ19 0x33
#define IRQ20 0x34
#define IRQ21 0x35
#define IRQ22 0x36
#define IRQ23 0x37

/* total number of IRQ interrupts */
#define IRQS 24

/*
 * note: I/O APICs can have more than 24 IRQs, and several I/O APICs can also
 * be used at the same time to support hundreds or even thousands of IRQs
 *
 * on systems where this is the case, IRQ24 will actually be mapped to the IRQ0
 * interrupt, IRQ25 will be mapped to IRQ1, etc. - essentially doing irq%24
 */

/*
 * reserved interrupt that should never be generated, used to indicate in
 * cpu_state_t if the state was actually pushed by a faux interrupt generated
 * by SYSCALL, and not automatically by the processor upon an interrupt
 */
#define NOT_INTR 0xFA

/* IPIs */
#define IPI_PANIC 0xFB
#define IPI_TLB   0xFC

/* LAPIC LVT interrupts */
#define LVT_TIMER 0xFD
#define LVT_ERROR 0xFE

/* LAPIC spurious interrupt */
#define SPURIOUS 0xFF /* in old processors bits 0-3 are hardwired to 1 */

#endif
