
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
	trace_printf("SYSCALL: sys_vbe_rectangle(%d, %d, %d, %d)\n", posX, posY, width, height);
	int ret = vbe_rectangle(get_vbe_context(), posX, posY, width, height);
	trace_printf("    returns %d\n", ret);
	return ret;
}

int sys_vbe_render_char(uint32_t posX, uint32_t posY, char c)
{
	trace_printf("SYSCALL: vbe_render_char(%d, %d, '%c')\n", posX, posY, c);
	int ret = vbe_render_char(get_vbe_context(), posX, posY, c);
	trace_printf("    returns %d\n", ret);
	return ret;
}

uint32_t sys_vbe_get_screen_width()
{
	uint32_t ret = vbe_get_screen_width();
	trace_printf("    returns %d\n", ret);
	return ret;
}

uint32_t sys_vbe_get_screen_height()
{
	uint32_t ret = vbe_get_screen_height();
	trace_printf("    returns %d\n", ret);
	return ret;
}

void sys_vbe_set_front_color(uint8_t r, uint8_t g, uint8_t b)
{
	trace_printf("SYSCALL: vbe_set_front_color(%d, %d, %d)\n", r, g, b);
	vbe_set_front_color(get_vbe_context(), r, g, b);
}

void sys_vbe_set_back_color(uint8_t r, uint8_t g, uint8_t b)
{
	trace_printf("SYSCALL: sys_vbe_set_back_color(%d, %d, %d)\n", r, g, b);
	vbe_set_back_color(get_vbe_context(), r, g, b);
}

bool sys_vbe_allocate_scroll_buffer(uint32_t height)
{
	trace_printf("SYSCALL: sys_vbe_allocate_scroll_buffer(%d)\n", height);
	bool ret = vbe_allocate_scroll_buffer(get_vbe_context(), height);
	trace_printf("    returns %d\n", (int)ret);
	return ret;
}

void sys_vbe_set_scroll_position(uint32_t y)
{
	trace_printf("SYSCALL: sys_vbe_set_scroll_position(%d)\n", y);
	vbe_set_scroll_position(get_vbe_context(), y);
}

void sys_vbe_clear()
{
	vbe_clear(get_vbe_context());
}