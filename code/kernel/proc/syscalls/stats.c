
#include <proc/syscalls.h>
#include <smp/cpu.h>

uint64_t sys_get_elapsed_milliseconds()
{
	// Get counter of bootstrap CPU
	cpu_t *cpu = cpu_get_bsp();
	return cpu->elapsedMsSinceStart;
}