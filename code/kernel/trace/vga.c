
#include <trace/vga.h>
#include <trace/bda.h>
#include <cpu/port.h>
#include <mm/phy32.h>
#include <stdbool.h>
#include <stdlib/string.h>
#include <vbe/vbe.h>

// Row/column dimensions.
#define ROW_HEIGHT (VBE_FONT_CHARACTER_HEIGHT+2)
#define COL_WIDTH VBE_FONT_CHARACTER_WIDTH

// Width of a tab character.
#define TAB_WIDTH 4

// The current rendering position.
static uint16_t row = 0;
static uint16_t col = 0;

// The dimensions of the render area.
static uint16_t rowCount;
static uint16_t colCount;

// Determines whether a wrap around has happened.
static bool wrapAroundOccured = false;

void vga_init(void)
{
	// Calculate dimensions
	rowCount = vbe_get_screen_height() / ROW_HEIGHT;
	colCount = vbe_get_screen_width() / COL_WIDTH;
	
	// Set colors
	vbe_set_front_color(VBE_KERNEL_CONTEXT, 255, 255, 255);
	vbe_set_back_color(VBE_KERNEL_CONTEXT, 0, 0, 0);
}

void vga_puts(const char *str)
{
	// Draw each char
	char c;
	while((c = *str++))
		vga_putch(c);
}

void vga_putch(char c)
{
	// Act depending on character type, consider control chars
	switch (c)
	{
		case '\0':
		case '\f':
		case '\v':
		case '\a':
		{
			// Ignore
			break;
		}

		case '\n':
		{
			++row;

			// Fall through
		}
		__attribute__ ((fallthrough));
		case '\r':
		{
			col = 0;
			break;
		}

		case '\t':
		{
			// Skip positions till the next multiple of TAB_WIDTH
			int tmp = col % TAB_WIDTH;
			if(tmp != 0)
				col += (TAB_WIDTH - tmp);
			break;
		}

		case '\b':
		{
			// Delete preceding character, if there is any
			if(col > 0)
			{
				--col;
				vbe_render_char(VBE_KERNEL_CONTEXT, col * COL_WIDTH, row * ROW_HEIGHT, ' ');
			}
			break;
		}

		default:
		{
			// Just draw the character
			vbe_render_char(VBE_KERNEL_CONTEXT, col * COL_WIDTH, row * ROW_HEIGHT, c);
			++col;
			break;
		}
	}

	// Line wrap
	if(col >= colCount)
	{
		col = 0;
		++row;
	}

	// When the last row is reached, wrap around to the top one (ring buffer)
	// A nice scrolling terminal will be available once the scheduler is up.
	if(row >= rowCount)
	{
		row = 0;
		wrapAroundOccured = true;
	}

	// New row? -> If wrap around has occured, clear the current and the next row
	if(col == 0 && wrapAroundOccured)
	{
		// Set clear color
		vbe_set_front_color(VBE_KERNEL_CONTEXT, 0, 0, 0);

		// Clear current row
		vbe_rectangle(VBE_KERNEL_CONTEXT, 0, row * ROW_HEIGHT, colCount * COL_WIDTH, ROW_HEIGHT);

		// Clear next row, if there is any
		if(row < rowCount - 1)
			vbe_rectangle(VBE_KERNEL_CONTEXT, 0, (row + 1) * ROW_HEIGHT, colCount * COL_WIDTH, ROW_HEIGHT);

		// Reset color
		vbe_set_front_color(VBE_KERNEL_CONTEXT, 255, 255, 255);
	}
}