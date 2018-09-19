
#ifndef _TIME_PIT_H
#define _TIME_PIT_H

#include <intr/route.h>

void pit_timer_install_handler(intr_handler_t handler);
void pit_monotonic(int ms);
void pit_mdelay(int ms);

#endif
