
#include <bus/isa.h>
#include <cpu/port.h>
#include <trace/trace.h>
#include <panic/panic.h>
#include <stdlib/string.h>

static irq_tuple_t isa_irqs[ISA_INTR_LINES];

void isa_init(void)
{
  /*
   * The default mapping of ISA interrupt lines to GSI numbers is 1:1 but they
   * can also be overridden by MADT entries.
   *
   * ISA IRQs are edge-triggered and active high by default.
   */
  for (int line = 0; line < ISA_INTR_LINES; line++)
  {
    irq_tuple_t *tuple = &isa_irqs[line];
    tuple->irq = line;
    tuple->active_polarity = POLARITY_HIGH;
    tuple->trigger = TRIGGER_EDGE;
  }
}

irq_tuple_t *isa_irq(isa_line_t line)
{
  if (line >= ISA_INTR_LINES)
    panic("invalid ISA interrupt line %d", line);

  return &isa_irqs[line];
}
