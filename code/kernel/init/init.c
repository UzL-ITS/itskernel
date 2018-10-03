#include <trace/trace.h>
#include <init/cmdline.h>
#include <trace/stacktrace.h>
#include <mm/common.h>
#include <mm/map.h>
#include <mm/phy32.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <mm/heap.h>
#include <mm/tlb.h>
#include <bus/isa.h>
#include <cpu/features.h>
#include <cpu/gdt.h>
#include <cpu/tss.h>
#include <cpu/idt.h>
#include <cpu/halt.h>
#include <intr/apic.h>
#include <intr/pic.h>
#include <intr/route.h>
#include <intr/fault.h>
#include <intr/nmi.h>
#include <panic/panic.h>
#include <smp/cpu.h>
#include <acpi/scan.h>
#include <smp/init.h>
#include <smp/mode.h>
#include <proc/sched.h>
#include <proc/syscall.h>
#include <proc/module.h>
#include <proc/idle.h>
#include <lock/intr.h>
#include <stdlib/string.h>
#include <stdbool.h>
#include <vbe/vbe.h>
#include <io/keyboard.h>
#include <pci/pci.h>
#include <fs/ramfs.h>

// Determines whether 1G pages are enabled (from mm/common.h). Is set in init().
bool enable1gPages;

static void print_banner(void)
{
	const int bannerWidth = 80;
	const char *bannerLine1 = "ITS Micro Kernel";
	const char *bannerLine2 = "based on Arc Operating System by Graham Edgecombe";

	int bannerLine1SpaceWidth = bannerWidth / 2 - strlen(bannerLine1) / 2;
	int bannerLine2SpaceWidth = bannerWidth / 2 - strlen(bannerLine2) / 2;

	char dashes[bannerWidth + 1];
	char bannerLine1Space[bannerLine1SpaceWidth + 1];
	char bannerLine2Space[bannerLine2SpaceWidth + 1];

	memset(dashes, '-', bannerWidth);
	dashes[bannerWidth] = '\0';

	memset(bannerLine1Space, ' ', bannerLine1SpaceWidth);
	bannerLine1Space[bannerLine1SpaceWidth] = '\0';

	memset(bannerLine2Space, ' ', bannerLine2SpaceWidth);
	bannerLine2Space[bannerLine2SpaceWidth] = '\0';

	trace_printf("%s\n", dashes);
	trace_printf("%s%s\n", bannerLine1Space, bannerLine1);
	trace_printf("%s%s\n", bannerLine2Space, bannerLine2);
	trace_printf("%s\n", dashes);
}

noreturn void init(uint32_t magic, multiboot_t *multiboot)
{
	/* convert physical 32-bit multiboot address to virtual address */
	multiboot = phy32_to_virt(multiboot);

	/* set up the BSP's percpu structure */
	cpu_bsp_init();

	/* parse command line arguments */
	cmdline_init(multiboot);

	// Initialize VBE for graphical rendering
	vbe_init(multiboot);

	/* initialize tracing */
	trace_init();

	/* print banner */
	print_banner();

	/* check the multiboot magic number */
	if(magic != MULTIBOOT_MAGIC)
		panic("invalid multiboot magic (expected %0#10x, got %0#10x)", MULTIBOOT_MAGIC, magic);

	/* set up stack traces */
	stacktrace_init(multiboot);

	/* set up the GDT, TSS, IDT and SYSCALL/RET for the boot core */
	gdt_init();
	tss_init();
	idt_bsp_init();
	syscall_init();

	/* scan CPU features */
	cpu_features_init();
	enable1gPages = cpu_feature_supported(FEATURE_1G_PAGE);

	/* map physical memory */
	trace_puts("Mapping physical memory...\n");
	list_t *map = mm_map_init(multiboot);

	/* set up the physical memory manager */
	trace_puts("Setting up the physical memory manager...\n");
	pmm_init(map);
	
	/* set up the virtual memory manager */
	trace_puts("Setting up the virtual memory manager...\n");
	vmm_init();

	/* set up the heap */
	trace_puts("Setting up the heap...\n");
	heap_init();

	// Output heap state
	//trace_puts("Heap alloc test...\n");
	heap_trace();
	/*uint64_t phys;
	uint64_t *cont = heap_alloc_contiguous(1024 * 1024 * 1024 + 512 * 1024 * 1024, VM_R, &phys);
	if(!cont)
		trace_puts("cont no allocation");
	else
	{
		trace_printf("cont at %016x / %016x\n", (uint64_t)cont, phys);
		heap_trace();
		for(int i = 0; i < (1024 * 1024 * 1024 + 512 * 1024 * 1024) / 8; ++i)
			cont[i] = i;
		trace_printf("Write test completed.\n");
		heap_free(cont);
		trace_printf("Free completed.\n");
	}
	heap_trace();
	trace_puts("Heap alloc test complete.\n");*/

	// Initialize VBE back buffers
	trace_puts("Initialize VBE back buffers...\n");
	vbe_init_back_buffers();	

	/* init ISA bus */
	isa_init();

	/* uniprocessor fallback flag */
	bool up_fallback = false;

	/* search for ACPI tables */
	trace_puts("Scanning ACPI tables...\n");
	if(!acpi_scan())
	{
		/* fall back to non-SMP mode using the PIC */
		trace_puts("Falling back to single processor mode...\n");
		pic_init();
		up_fallback = true;
	}
	else
		smp_mode = MODE_SMP_STARTING;

	/* set up the local APIC on the BSP if we are in SMP mode */
	if(!up_fallback)
		apic_init();
	else
	{
		/* enable BSP interrupts now the IDT and interrupt controllers are set up */
		// In SMP mode these are enabled by apic_init()
		intr_unlock();
	}

	/* route IPIs */
	panic_init();
	fault_init();
	tlb_init();
	
	/* set up idle process, this must be done before we are in SMP mode */
	idle_init();
	
	// Set interrupt stack pointer for bootstrap processor
    tss_set_rsp0(cpu_get()->idle_thread->rsp);
	
	/* set up symmetric multi-processing */
	if(!up_fallback)
	{
		trace_puts("Setting up SMP...\n");
		smp_init();
	}

	/* set up NMI routing, this must be done when we are in SMP mode */
	nmi_init();

	// Scan PCI devices
	pci_init();
	
	// Initialize I/O devices
	keyboard_init();
	
	// Initialize RAM file system
	ramfs_init();

	/* set up modules */
	// Do this BEFORE starting the scheduler, else we might run into a race condition where the scheduler interrupt fires too soon, leaving execution stuck in the idle() process
	trace_puts("Loading modules...\n");
	module_init(multiboot);

	/* set up the scheduler for this core, also needs SMP mode */
	trace_puts("Initializing scheduler...\n");
	sched_init();

	/* halt forever - the scheduler will take over from here */
	halt_forever();
}
