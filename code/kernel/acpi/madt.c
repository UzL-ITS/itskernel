
#include <acpi/madt.h>
#include <bus/isa.h>
#include <intr/apic.h>
#include <intr/pic.h>
#include <intr/ioapic.h>
#include <intr/nmi.h>
#include <smp/cpu.h>
#include <util/container.h>
#include <panic/panic.h>
#include <stdbool.h>
#include <stddef.h>
#include <trace/trace.h>

static void madt_flags_to_trigger(irq_tuple_t *tuple, uint16_t flags)
{
  if ((flags & MADT_INTR_POLARITY_HIGH) == MADT_INTR_POLARITY_HIGH)
    tuple->active_polarity = POLARITY_HIGH;
  else if ((flags & MADT_INTR_POLARITY_LOW) == MADT_INTR_POLARITY_LOW)
    tuple->active_polarity = POLARITY_LOW;

  if ((flags & MADT_INTR_TRIGGER_EDGE) == MADT_INTR_TRIGGER_EDGE)
    tuple->trigger = TRIGGER_EDGE;
  else if ((flags & MADT_INTR_TRIGGER_LEVEL) == MADT_INTR_TRIGGER_LEVEL)
    tuple->trigger = TRIGGER_LEVEL;
}

void madt_scan(madt_t *madt)
{
  /* the 32-bit address of the LAPIC */
  uint64_t lapic_addr = madt->lapic_addr;

  /* a flag indicating if the BSP has been found in the table yet */
  bool bsp = true;

  /* iterate through the MADT entries */
  uintptr_t ptr = (uintptr_t) &madt->entries[0];
  uintptr_t ptr_end = (uintptr_t) madt + madt->header.len;
  while (ptr < ptr_end)
  {
    madt_entry_t *entry = (madt_entry_t *) ptr;
    ptr += entry->len;

    switch (entry->type)
    {
      case MADT_TYPE_INTR:
        {
          uint8_t bus = entry->intr.bus;
          uint8_t line = entry->intr.irq;
          uint32_t gsi = entry->intr.gsi;
          uint16_t flags = entry->intr.flags;

		  trace_printf("MADT INTR\t   bus=0x%02x  line=0x%02x  gsi=0x%08x  flags=0x%04x\n", bus, line, gsi, flags);
          if (bus == MADT_INTR_BUS_ISA)
          {
            if (line >= ISA_INTR_LINES)
              panic("ISA interrupt line out of range: %d", line);

            irq_tuple_t *tuple = isa_irq(line);
            tuple->irq = gsi;

            madt_flags_to_trigger(tuple, flags);
          }
        }
        break;

      case MADT_TYPE_LAPIC_ADDR:
        /* override the LAPIC address with a 64-bit one */
        lapic_addr = entry->lapic_addr.addr;
		trace_printf("MADT LADDR\t   addr=0x%016x\n", lapic_addr);
        break;

      case MADT_TYPE_LAPIC:
		trace_printf("MADT LAPIC\t   id=0x%02x  apicid=0x%02x  flags=0x%08x\n", entry->lapic.id, entry->lapic.apic_id, entry->lapic.flags);
        if (entry->lapic.flags & MADT_LAPIC_FLAGS_ENABLED)
        {
          uint8_t id  = entry->lapic.id;
          uint8_t apic_id = entry->lapic.apic_id;

          if (bsp)
          {
            bsp = false;

            /*
             * ACPI spec states that the first LAPIC entry is the BSP, fill out
             * the extra bits not done by cpu_bsp_init() here
             */
            cpu_t *cpu_bsp = cpu_get();
            cpu_bsp->lapic_id = apic_id;
            cpu_bsp->acpi_id = id;
          }
          else
          {
            if (!cpu_ap_init(apic_id, id))
              panic("failed to register AP");
          }
        }
        break;

      case MADT_TYPE_IOAPIC:
        {
          uint8_t id = entry->ioapic.id;
          uint32_t addr = entry->ioapic.addr;
          uint32_t gsi_base = entry->ioapic.gsi_base;
		  trace_printf("MADT IOAPIC\t   id=0x%02x  addr=0x%08x  gsi=0x%08x\n", id, addr, gsi_base);
          if (!ioapic_init(id, addr, gsi_base))
            panic("failed to register I/O APIC");
        }
        break;

      case MADT_TYPE_NMI:
        {
          uint32_t gsi = entry->nmi.gsi;
          uint16_t flags = entry->nmi.flags;
		  
		  trace_printf("MADT NMI \t   gsi=0x%08x  flags=0x%04x\n", gsi, flags);

          irq_tuple_t tuple;
          tuple.irq = gsi;
          tuple.active_polarity = POLARITY_HIGH;
          tuple.trigger = TRIGGER_EDGE;

          madt_flags_to_trigger(&tuple, flags);

          if (!nmi_add(tuple))
            panic("failed to register NMI");
        }
        break;
    }
  }

  /* mask the PICs if they are present */
  if (madt->flags & MADT_FLAGS_PCAT)
    pic_init();

  /* initialise the local APIC */
  xapic_init(lapic_addr);

  /* perform a second pass to route local NMIs */
  ptr = (uintptr_t) &madt->entries[0];
  while (ptr < ptr_end)
  {
    madt_entry_t *entry = (madt_entry_t *) ptr;
    ptr += entry->len;

    switch (entry->type)
    {
      case MADT_TYPE_LNMI:
        {
          uint8_t id = entry->lnmi.id;
          uint8_t lintn = entry->lnmi.lintn;
          // TODO: deal with the flags
          //uint16_t flags = entry->lnmi.flags;

          if (lintn != 0 && lintn != 1)
            panic("LINTn can only be set to 0 or 1");

          if (id == 0xFF)
          {
            /* magic value which indicates NMI is connected to all APICs */
            list_for_each(&cpu_list, node)
            {
              cpu_t *cpu = container_of(node, cpu_t, node);
              cpu->apic_lint_nmi[lintn] = true;
            }
          }
          else
          {
            /* find the local APIC id for this NMI */
            cpu_t *cpu = 0;
            list_for_each(&cpu_list, node)
            {
              cpu = container_of(node, cpu_t, node);
              if (cpu->acpi_id == id)
                break;
            }
 
            if (!cpu)
              panic("failed to find local APIC referred to by local NMI entry");

            cpu->apic_lint_nmi[lintn] = true;
          }
        }
        break;
    }
  }
}
