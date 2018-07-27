
#ifndef _CPU_IDT_H
#define _CPU_IDT_H

#include <stdint.h>

typedef struct
{
  uint16_t len;
  uint64_t addr;
} __attribute__((__packed__)) idtr_t;

void idt_bsp_init(void);
void idt_ap_init(void);
void idtr_install(idtr_t *idtr);

#endif
