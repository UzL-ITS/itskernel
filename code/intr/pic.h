
#ifndef _INTR_PIC_H
#define _INTR_PIC_H

#include <stdbool.h>
#include <intr/common.h>
#include <defs/types.h>

void pic_init(void);
bool pic_spurious(irq_t irq);
void pic_mask(irq_t irq);
void pic_unmask(irq_t irq);
void pic_ack(irq_t irq);

#endif
