
#ifndef _TIME_APIC_H
#define _TIME_APIC_H

#include <intr/route.h>

void apic_monotonic(int ms, intr_handler_t handler);

#endif
