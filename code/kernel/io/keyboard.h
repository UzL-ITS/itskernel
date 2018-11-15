#pragma once

#include <io/keycodes.h>

// Initializes the keyboard driver.
void keyboard_init();

// Polls the keyboard.
// Is called by the scheduler timer as a workaround for a broken 8042 emulation.
void keyboard_poll();