
#ifndef _TIME_PIT_H
#define _TIME_PIT_H

#include <intr/route.h>

void pit_timer_install_handler(intr_handler_t handler);
void pit_monotonic(int ms);

// Waits for the given amount of milliseconds.
// This function depends on interrupts, so do not use interrupt locks around its calls!
void pit_mdelay(int ms);

#endif
