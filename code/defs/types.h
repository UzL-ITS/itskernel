
#ifndef _TYPES_H
#define _TYPES_H

#include <stdint.h>

/*
 * IRQ number type
 *
 * This type is distinct from intr_t as the amd64 architecture limits the
 * number of interrupts to 256. However, in a system with several I/O APICs, it
 * is possible to have far more than 256 IRQ lines.
 *
 * IRQ numbers are 32 bits in the ACPI tables, hence this value being 32 bits.
 */
typedef uint32_t irq_t;

/* interrupt number type, this is 8 bits as expected for amd64 */
typedef uint8_t intr_t;

/* CPU ID (ACPI and local APIC) types */
typedef uint32_t cpu_acpi_id_t;
typedef uint32_t cpu_lapic_id_t;

/* I/O APIC id type */
typedef uint8_t ioapic_id_t;

/* polarity type */
typedef enum
{
  POLARITY_HIGH,
  POLARITY_LOW
} polarity_t;

/* trigger type */
typedef enum
{
  TRIGGER_EDGE,
  TRIGGER_LEVEL
} trigger_t;

/* a tuple consisting of an IRQ number, active polarity and trigger type */
typedef struct
{
  /* I/O APIC IRQ number */
  irq_t irq;

  /* the polarity and trigger type */
  polarity_t active_polarity;
  trigger_t trigger;
} irq_tuple_t;

#endif
