
#ifndef _TIME_PIT_H
#define _TIME_PIT_H

#include <intr/route.h>

void pit_monotonic(int ms, intr_handler_t handler);
void pit_mdelay(int ms);

#endif
