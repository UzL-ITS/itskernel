
#ifndef _VBE_FONT_H
#define _VBE_FONT_H

#include <stdint.h>

#define VBE_FONT_CHARACTER_WIDTH 8
#define VBE_FONT_CHARACTER_HEIGHT 13

extern uint8_t font_data[95][VBE_FONT_CHARACTER_HEIGHT];

#endif