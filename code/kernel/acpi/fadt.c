
#include <acpi/fadt.h>
#include <trace/trace.h>
#include <mm/mmio.h>
#include <panic/panic.h>
#include <acpi/power.h>

void fadt_scan(fadt_t *fadt)
{
	trace_printf("Scanning FADT...\n");
	
	// Initialize power management
	power_init(fadt);
	
	trace_printf("FADT scan successful.\n");
}
