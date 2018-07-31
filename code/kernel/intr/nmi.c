
#include <intr/nmi.h>
#include <intr/common.h>
#include <intr/route.h>
#include <util/container.h>
#include <util/list.h>
#include <panic/panic.h>
#include <stdlib/stdlib.h>

static list_t nmi_list;

typedef struct
{
  irq_tuple_t tuple;
  list_node_t node;
} nmi_t;

static void nmi_handle(cpu_state_t *state)
{
  panic("non-maskable interrupt - possible hardware failure?");
}

void nmi_init(void)
{
  if (!intr_route_intr(FAULT2, &nmi_handle))
    panic("failed to route NMI");

  list_for_each(&nmi_list, node)
  {
    nmi_t *nmi = container_of(node, nmi_t, node);

    /*
     * route the NMI to interrupt 2, this must be done using a raw IRQ entry
     * this way to avoid collisions with another IRQ - the NMI handler *always*
     * panic()s, so it cannot be shared
     */
    if (!intr_route_irq_to(&nmi->tuple, FAULT2))
      panic("failed to route NMI");
  }

  // TODO: free unused nmi structures?
}

bool nmi_add(irq_tuple_t tuple)
{
  nmi_t *nmi = malloc(sizeof(*nmi));
  if (!nmi)
    return false;

  nmi->tuple = tuple;
  list_add_tail(&nmi_list, &nmi->node);
  return true;
}
