
#ifndef _INTR_ROUTE_H
#define _INTR_ROUTE_H

#include <defs/types.h>
#include <cpu/state.h>
#include <stdbool.h>

typedef void (*intr_handler_t)(cpu_state_t *state);

void intr_route_init(void);

/* dispatches an interrupt */
void intr_dispatch(cpu_state_t *state);

/* route by interrupt id */
bool intr_route_intr(intr_t intr, intr_handler_t handler);
void intr_unroute_intr(intr_t intr, intr_handler_t handler);

/* route an IRQ to an interrupt */
bool intr_route_irq_to(irq_tuple_t *tuple, intr_t intr);
void intr_unroute_irq_to(irq_tuple_t *tuple, intr_t intr);

/*
 * route an IRQ to an interrupt handler directly without worrying about picking
 * an interrupt id - this function is a combination of the above functions
 */
bool intr_route_irq(irq_tuple_t *tuple, intr_handler_t handler);
void intr_unroute_irq(irq_tuple_t *tuple, intr_handler_t handler);

#endif
