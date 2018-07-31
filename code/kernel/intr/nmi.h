
#ifndef _INTR_NMI_H
#define _INTR_NMI_H

#include <defs/types.h>
#include <stdbool.h>

void nmi_init(void);
bool nmi_add(irq_tuple_t tuple);

#endif
