
#include <proc/syscalls.h>
#include <acpi/power.h>

void sys_reset()
{
	power_reset();
}