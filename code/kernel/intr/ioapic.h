
#ifndef _INTR_IOAPIC_H
#define _INTR_IOAPIC_H

#include <stdbool.h>
#include <stdint.h>
#include <util/list.h>
#include <defs/types.h>

#define IOAPIC_MAX_IRQS 240

typedef struct ioapic
{
  /* global I/O APIC list node */
  list_node_t node;

  /* the id of this I/O APIC */
  ioapic_id_t id;

  /* the MMIO registers */
  uint32_t *reg, *val;

  /* the base IRQ number */
  irq_t irq_base;

  /* the number of IRQs this I/O APIC routes */
  irq_t irqs;
} ioapic_t;

extern list_t ioapic_list;

bool ioapic_init(ioapic_id_t id, uintptr_t addr, irq_t irq_base);
void ioapic_route(ioapic_t *apic, irq_tuple_t *tuple, intr_t intr);
void ioapic_route_nmi(ioapic_t *apic, irq_tuple_t *tuple);
void ioapic_mask(ioapic_t *apic, irq_tuple_t *tuple);

#endif
