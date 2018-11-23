
#include <proc/syscalls.h>
#include <vbe/vbe.h>
#include <proc/proc.h>
#include <trace/trace.h>

static int get_vbe_context()
{
	// Get current process
	proc_t *proc = proc_get();
	
	// Retrieve VBE context ID
	return proc->vbeContext;
}

int sys_vbe_rectangle(uint32_t posX, uint32_t posY, uint32_t width, uint32_t height)
{
	return vbe_rectangle(get_vbe_context(), posX, posY, width, height);
}

int sys_vbe_render_char(uint32_t posX, uint32_t posY, char c)
{
	return vbe_render_char(get_vbe_context(), posX, posY, c);
}

uint32_t sys_vbe_get_screen_width()
{
	return vbe_get_screen_width();
}

uint32_t sys_vbe_get_screen_height()
{
	return vbe_get_screen_height();
}

void sys_vbe_set_front_color(uint8_t r, uint8_t g, uint8_t b)
{
	vbe_set_front_color(get_vbe_context(), r, g, b);
}

void sys_vbe_set_back_color(uint8_t r, uint8_t g, uint8_t b)
{
	vbe_set_back_color(get_vbe_context(), r, g, b);
}

bool sys_vbe_allocate_scroll_buffer(uint32_t height)
{
	return vbe_allocate_scroll_buffer(get_vbe_context(), height);
}

void sys_vbe_set_scroll_position(uint32_t y)
{
	vbe_set_scroll_position(get_vbe_context(), y);
}

void sys_vbe_clear()
{
	vbe_clear(get_vbe_context());
}