/*
ITS kernel standard library terminal implementation.
*/

/* INCLUDES */

#include <stdbool.h>
#include <internal/terminal/terminal.h>
#include <internal/syscall/syscalls.h>


/* VARIABLES */

// Row/column dimensions.
// TODO retrieve via syscall
#define VBE_FONT_CHARACTER_WIDTH 8
#define VBE_FONT_CHARACTER_HEIGHT 13
#define ROW_HEIGHT (VBE_FONT_CHARACTER_HEIGHT+2)
#define COLUMN_WIDTH VBE_FONT_CHARACTER_WIDTH

// Terminal inner padding (distance to border of render area).
#define TERMINAL_PADDING 2

// Width of a tab character.
#define TAB_WIDTH 4

// Size of the scrollbar.
#define SCROLLBAR_WIDTH 8
#define SCROLLBAR_BAR_MIN_HEIGHT 4
#define SCROLLBAR_ARROW_KEY_OFFSET 20

// Colors.
#define COLOR_BACKGROUND 0,0,0
#define COLOR_BACKGROUND_WRAP_AROUND 20,20,20
#define COLOR_FOREGROUND 255,255,255
#define COLOR_SCROLLBAR_BACKGROUND 60,60,60
#define COLOR_SCROLLBAR_FOREGROUND 200,200,200

// Render window dimensions (in pixels).
static uint32_t renderWindowWidth;
static uint32_t renderWindowHeight;

// Full terminal height (in pixels).
static uint32_t terminalHeight;

// The current printing position (in characters).
static uint32_t currentColumn = 0;
static uint32_t currentRow = 0;

// The current scroll position (in pixels).
static uint32_t scrollY = 0;

// The maximum scroll position (in pixels).
static uint32_t scrollYMax = 0;

// The size of the actual scrollbar "bar".
static uint32_t scrollbarBarHeight = 0;

// The dimensions of the terminal typesetting area (in characters).
static uint32_t terminalColumnCount;
static uint32_t terminalRowCount;

// Signals whether a wrap around has occured.
static bool wrapAroundOccured = false;

// Signals the current wrap around color.
static bool wrapAroundColor = false;


/* FUNCTIONS */

// Draws the scrollbar for the current scroll position.
static void draw_scrollbar()
{
	// Calculate render dimensions
	uint32_t posX = renderWindowWidth - SCROLLBAR_WIDTH;
	uint32_t posY = scrollY;
	uint32_t barOffsetY = (scrollY * renderWindowHeight) / terminalHeight;
	
	// Render background
	sys_vbe_set_front_color(COLOR_SCROLLBAR_BACKGROUND);
	sys_vbe_rectangle(posX, posY, SCROLLBAR_WIDTH, renderWindowHeight);
	
	// Render bar
	sys_vbe_set_front_color(COLOR_SCROLLBAR_FOREGROUND);
	sys_vbe_rectangle(posX, posY + barOffsetY, SCROLLBAR_WIDTH, scrollbarBarHeight);
	
	// Reset color
	sys_vbe_set_front_color(COLOR_FOREGROUND);
}

void terminal_init(int lines)
{
	// Retrieve window dimensions
	renderWindowWidth = sys_vbe_get_screen_width();
	renderWindowHeight = sys_vbe_get_screen_height();
	
	// Store terminal typeset area size
	terminalColumnCount = (renderWindowWidth - TERMINAL_PADDING - TERMINAL_PADDING - SCROLLBAR_WIDTH) / COLUMN_WIDTH;
	terminalRowCount = lines;
	
	// Create scrollable buffer
	terminalHeight = lines * ROW_HEIGHT + 2 * TERMINAL_PADDING;
	if(!sys_vbe_allocate_scroll_buffer(terminalHeight))
	{
		// Print error to kernel console
		sys_kputs("Could not allocate scroll buffer.\n");
		return;
	}
	
	// Clear buffer
	sys_vbe_set_front_color(COLOR_FOREGROUND);
	sys_vbe_set_back_color(COLOR_BACKGROUND);
	sys_vbe_clear();
	
	// Draw scrollbar for current display
	scrollbarBarHeight = (renderWindowHeight * renderWindowHeight) / terminalHeight;
	if(scrollbarBarHeight < SCROLLBAR_BAR_MIN_HEIGHT)
		scrollbarBarHeight = SCROLLBAR_BAR_MIN_HEIGHT;
	scrollYMax = terminalHeight - renderWindowHeight;
	draw_scrollbar();
}

void terminal_putc(char c)
{
	// Act depending on character type, consider control chars
	switch(c)
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
			// Next row
			++currentRow;
		}
		__attribute__ ((fallthrough));
		case '\r':
		{
			// Go to line start
			currentColumn = 0;
			break;
		}

		case '\t':
		{
			// Skip positions till the next multiple of TAB_WIDTH
			int tmp = currentColumn % TAB_WIDTH;
			if(tmp != 0)
				currentColumn += (TAB_WIDTH - tmp);
			break;
		}

		case '\b':
		{
			// Delete preceding character, if there is any
			if(currentColumn > 0)
			{
				--currentColumn;
				sys_vbe_render_char(TERMINAL_PADDING + currentColumn * COLUMN_WIDTH, TERMINAL_PADDING + currentRow * ROW_HEIGHT, ' ');
			}
			break;
		}

		default:
		{
			// Just draw the character
			sys_vbe_render_char(TERMINAL_PADDING + currentColumn * COLUMN_WIDTH, TERMINAL_PADDING + currentRow * ROW_HEIGHT, c);
			++currentColumn;
			break;
		}
	}

	// Line wrap
	if(currentColumn >= terminalColumnCount)
	{
		currentColumn = 0;
		++currentRow;
	}

	// When the last row is reached, wrap around to the top one (ring buffer)
	// A nice scrolling terminal will be available once the scheduler is up.
	if(currentRow >= terminalRowCount)
	{
		currentRow = 0;
		wrapAroundOccured = true;
		wrapAroundColor = !wrapAroundColor;
	}

	// New row? -> If wrap around has occured, clear the current and the next row
	if(currentColumn == 0 && wrapAroundOccured)
	{
		// Set clear color
		if(wrapAroundColor)
			sys_vbe_set_front_color(COLOR_BACKGROUND_WRAP_AROUND);
		else
			sys_vbe_set_front_color(COLOR_BACKGROUND);

		// Clear current row
		sys_vbe_rectangle(TERMINAL_PADDING, TERMINAL_PADDING + currentRow * ROW_HEIGHT, terminalColumnCount * COLUMN_WIDTH, ROW_HEIGHT);

		// Clear next row, if there is any
		if(currentRow + 1 < terminalRowCount)
			sys_vbe_rectangle(TERMINAL_PADDING, TERMINAL_PADDING + (currentRow + 1) * ROW_HEIGHT, terminalColumnCount * COLUMN_WIDTH, ROW_HEIGHT);

		// Reset color
		sys_vbe_set_front_color(COLOR_FOREGROUND);
	}
}

void terminal_puts(const char *str)
{
	// Print each char
	char c;
	while((c = *str++))
		terminal_putc(c);
}

void terminal_handle_navigation_key(vkey_t keyCode)
{	
	// Calculate new scroll position
	uint32_t newScrollY = scrollY;
	switch(keyCode)
	{
		case VKEY_CURSORUP:
			if(newScrollY >= SCROLLBAR_ARROW_KEY_OFFSET)
				newScrollY -= SCROLLBAR_ARROW_KEY_OFFSET;
			else
				newScrollY = 0;
			break;
		case VKEY_CURSORDOWN:
			newScrollY += SCROLLBAR_ARROW_KEY_OFFSET;
			break;
			
		case VKEY_PAGEUP:
			if(newScrollY >= renderWindowHeight)
				newScrollY -= renderWindowHeight;
			else
				newScrollY = 0;
			break;
		case VKEY_PAGEDOWN:
			newScrollY += renderWindowHeight;
			break;
			
		case VKEY_HOME:
			newScrollY = 0;
			break;
		case VKEY_END:
			newScrollY = scrollYMax;
			break;
			
		default:
			return;
	}
	
	// Position valid?
	if(newScrollY > scrollYMax)
		newScrollY = scrollYMax;
	if(newScrollY != scrollY)
	{
		// Scroll
		sys_vbe_set_scroll_position(newScrollY);
		scrollY = newScrollY;
		draw_scrollbar();
	}
}

void terminal_draw(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
	// Draw rectangle relative to current position
	sys_vbe_rectangle(TERMINAL_PADDING + currentColumn * COLUMN_WIDTH + x, TERMINAL_PADDING + currentRow * ROW_HEIGHT + y, width, height);
}