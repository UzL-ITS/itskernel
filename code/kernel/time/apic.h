
#ifndef _TIME_APIC_H
#define _TIME_APIC_H

#include <intr/route.h>


void apic_timer_install_handler(intr_handler_t handler);
void apic_monotonic(int ms);

#endif
