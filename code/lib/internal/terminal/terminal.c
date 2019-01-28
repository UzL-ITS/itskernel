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
#define SCROLLBAR_CURRENT_LINE_HEIGHT 3
#define SCROLLBAR_ARROW_KEY_OFFSET 20

// Default colors.
const color_t COLOR_DEFAULT_BACKGROUND = { 0, 0, 0 };
const color_t COLOR_DEFAULT_FOREGROUND = { 255, 255, 255 };
const color_t COLOR_DEFAULT_SCROLLBAR_BACKGROUND = { 60, 60, 60 };
const color_t COLOR_DEFAULT_SCROLLBAR_BAR = { 200, 200, 200 };
const color_t COLOR_DEFAULT_SCROLLBAR_CURRENT_LINE = { 0, 200, 0 };

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

// Denotes whether a terminal wrap around has occured.
static bool wrapAroundOccured = false;

// The dimensions of the terminal typesetting area (in characters).
static uint32_t terminalColumnCount;
static uint32_t terminalRowCount;

// Current colors.
static color_t frontColor = COLOR_DEFAULT_FOREGROUND;
static color_t backColor = COLOR_DEFAULT_BACKGROUND;


/* FUNCTIONS */

// Sets the given color as active draw front color.
static void apply_front_color(color_t color)
{
	sys_vbe_set_front_color(color.r, color.g, color.b);
}

// Sets the given color as active draw back color.
static void apply_back_color(color_t color)
{
	sys_vbe_set_back_color(color.r, color.g, color.b);
}

// Draws the scrollbar for the current scroll position.
static void draw_scrollbar()
{
	// Calculate render position
	uint32_t posX = renderWindowWidth - SCROLLBAR_WIDTH;
	uint32_t posY = scrollY;
	
	// Render background
	apply_front_color(COLOR_DEFAULT_SCROLLBAR_BACKGROUND);
	sys_vbe_rectangle(posX, posY, SCROLLBAR_WIDTH, renderWindowHeight);
	
	// Render bar
	uint32_t barOffsetY = (scrollY * renderWindowHeight) / terminalHeight; // Order of multiplication and division reversed, to avoid loss of precision
	apply_front_color(COLOR_DEFAULT_SCROLLBAR_BAR);
	sys_vbe_rectangle(posX, posY + barOffsetY, SCROLLBAR_WIDTH, scrollbarBarHeight);
	
	// Render line of cursor
	uint32_t cursorOffsetY = (currentRow * ROW_HEIGHT * renderWindowHeight) / terminalHeight;
	apply_front_color(COLOR_DEFAULT_SCROLLBAR_CURRENT_LINE);
	sys_vbe_rectangle(posX, posY + cursorOffsetY, SCROLLBAR_WIDTH, SCROLLBAR_CURRENT_LINE_HEIGHT);
	
	// Reset color
	apply_front_color(frontColor);
}

// Draws the cursor at the current position, using the current foreground color.
static void draw_cursor()
{
	// Draw cursor
	sys_vbe_rectangle(TERMINAL_PADDING + currentColumn * COLUMN_WIDTH, TERMINAL_PADDING + currentRow * ROW_HEIGHT, VBE_FONT_CHARACTER_WIDTH, VBE_FONT_CHARACTER_HEIGHT);
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
	apply_front_color(frontColor);
	apply_back_color(backColor);
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
	// Clear cursor
	apply_back_color(backColor);
	apply_front_color(backColor);
	draw_cursor();
	apply_front_color(frontColor);
	
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
			draw_scrollbar();
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
			// Go one character back, if possible
			// There is no need to clear the character, since the cursor will be drawn at this position
			if(currentColumn > 0)
				--currentColumn;
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
		draw_scrollbar();
	}

	// When the last row is reached, wrap around to the top one (ring buffer)
	if(currentRow >= terminalRowCount)
	{
		currentRow = 0;
		draw_scrollbar();
		wrapAroundOccured = true;
	}

	// New row? -> If wrap around has occured, clear the current and the next row
	if(currentColumn == 0 && wrapAroundOccured)
	{
		// Set clear color
		apply_front_color(backColor);

		// Clear current row
		sys_vbe_rectangle(TERMINAL_PADDING, TERMINAL_PADDING + currentRow * ROW_HEIGHT, terminalColumnCount * COLUMN_WIDTH, ROW_HEIGHT);

		// Clear next row, if there is any
		if(currentRow + 1 < terminalRowCount)
			sys_vbe_rectangle(TERMINAL_PADDING, TERMINAL_PADDING + (currentRow + 1) * ROW_HEIGHT, terminalColumnCount * COLUMN_WIDTH, ROW_HEIGHT);

		// Restore color
		apply_front_color(frontColor);
	}
	
	// Draw cursor
	draw_cursor();
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

void terminal_set_front_color(color_t color)
{
	// Set color
	frontColor = color;
}

void terminal_set_back_color(color_t color)
{
	// Set color
	backColor = color;
}

void terminal_reset_colors()
{
	// Set default colors
	frontColor = COLOR_DEFAULT_FOREGROUND;
	backColor = COLOR_DEFAULT_BACKGROUND;
}