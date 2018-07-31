
#ifndef _INTR_APIC_H
#define _INTR_APIC_H

#include <defs/types.h>
#include <stdbool.h>
#include <stdint.h>

/* global xAPIC/x2APIC enable */
bool xapic_init(uintptr_t addr); /* init in xAPIC mode using MMIO */
void x2apic_init(void);          /* init in x2APIC mode using MSRs */

/* enable this CPU's APIC */
void apic_init(void);

/* acknowledge an interrupt */
void apic_ack(void);

/* IPIs */
void apic_ipi_init(cpu_lapic_id_t id);
void apic_ipi_startup(cpu_lapic_id_t id, uint8_t trampoline_addr);
void apic_ipi_fixed(cpu_lapic_id_t id, intr_t intr);
void apic_ipi_all(intr_t intr);
void apic_ipi_all_exc_self(intr_t intr);
void apic_ipi_self(intr_t intr);

#endif
