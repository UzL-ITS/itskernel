
#include <smp/init.h>
#include <smp/cpu.h>
#include <smp/mode.h>
#include <cpu/gdt.h>
#include <cpu/tss.h>
#include <cpu/idt.h>
#include <cpu/pause.h>
#include <cpu/halt.h>
#include <cpu/tlb.h>
#include <cpu/cpuid.h>
#include <lock/barrier.h>
#include <lock/intr.h>
#include <lock/spinlock.h>
#include <intr/apic.h>
#include <mm/vmm.h>
#include <time/pit.h>
#include <proc/sched.h>
#include <proc/syscall.h>
#include <util/container.h>
#include <trace/trace.h>
#include <panic/panic.h>
#include <stdlib/stdlib.h>
#include <stdlib/string.h>
#include <smp/topology.h>

#define TRAMPOLINE_BASE 0x1000
#define IDLE_STACK_SIZE	8192
#define IDLE_STACK_ALIGN 16

/* some variables used to exchange data between the BSP and APs */
static bool ack_sipi = false;
static cpu_t *booted_cpu;

/* a counter of ready CPUs, smp_init() blocks until all APs are ready */
static int ready_cpus = 1;
static spinlock_t ready_cpus_lock = SPIN_UNLOCKED;

/* a flag used to indicate that the system is now in SMP mode */
static bool mode_switched = false;
static spinlock_t mode_switch_lock = SPIN_UNLOCKED;

static void print_cpu_info(cpu_t *cpu)
{
	const char *str = cpu->bsp ? ", bsp" : "";
	trace_printf(" => CPU (struct at %0#18x, id %0#10x%s)\n", cpu, cpu->lapic_id, str);
}

/* bring up an AP */
static void smp_boot(cpu_t *cpu)
{
	/* figure out where the trampoline is */
	extern int trampoline_start, trampoline_end, trampoline_stack;
	size_t trampoline_len = (uintptr_t) &trampoline_end - (uintptr_t) &trampoline_start;
	
	/* map the trampoline into low memory */
	if (!vmm_map_range(TRAMPOLINE_BASE, TRAMPOLINE_BASE, trampoline_len, VM_R | VM_W | VM_X))
		panic("couldn't map SMP trampoline code");

	/* allocate a stack for this AP */
	void *idle_stack = memalign(IDLE_STACK_ALIGN, IDLE_STACK_SIZE);
	if (!idle_stack)
		panic("couldn't allocate AP stack");

	/* set up this cpu's bootstrap stack */
	uint64_t *rsp = (uint64_t *) &trampoline_stack;
	*rsp = (uint64_t) idle_stack + IDLE_STACK_SIZE;

	/* set the pointer to the cpu struct of the cpu we are booting */
	booted_cpu = cpu;
	
	/* copy the trampoline into low memory */
	memcpy((void *) TRAMPOLINE_BASE, &trampoline_start, trampoline_len);
	
	/* reset the ack flag */
	ack_sipi = false;
	barrier();
	
	/* send INIT IPI */
	apic_ipi_init(cpu->lapic_id);
	pit_mdelay(10);
	
	/* send STARTUP IPI */
	uint8_t vector = TRAMPOLINE_BASE / FRAME_SIZE;
	apic_ipi_startup(cpu->lapic_id, vector);
	pit_mdelay(1);

	/* send STARTUP IPI again */
	if (!ack_sipi)
	{
		apic_ipi_startup(cpu->lapic_id, vector);
		pit_mdelay(1);
	}

	/* wait for the AP to come up */
	while (!ack_sipi)
		pause_once();

	/* unmap the trampoline */
	vmm_unmap_range(TRAMPOLINE_BASE, trampoline_len);
}

void smp_init(void)
{
	// Initialize CPU topology storage
	topology_prepare(cpuCount);
	
	/* bring up all of the APs */
	list_for_each(&cpu_list, node)
	{
		cpu_t *cpu = container_of(node, cpu_t, node);
		
		if(!cpu->bsp)
			smp_boot(cpu);
		else
		{
			// Retrieve topology information
			topology_init(cpu);
	
			print_cpu_info(cpu);
		}
	}

	/* wait for all CPUs to be ready */
	int ready;
	do
	{
		spin_lock(&ready_cpus_lock);
		ready = ready_cpus;
		spin_unlock(&ready_cpus_lock);
	} while (ready != cpu_list.size);

	/* switch to SMP mode */
	smp_mode = MODE_SMP;

	/* now let the APs carry on */
	spin_lock(&mode_switch_lock);
	mode_switched = true;
	spin_unlock(&mode_switch_lock);
}

void smp_ap_init(void)
{
	/* save the per-cpu data area pointer so we can ack the SIPI straight away */
	cpu_ap_install(booted_cpu);

	/* print a message to indicate the AP has been booted */
	cpu_t *cpu = cpu_get();
	print_cpu_info(cpu);

	/* acknowledge the STARTUP IPI */
	ack_sipi = true;
	barrier();

	/* now start the real work! - set up the GDT, TSS, IDT and SYSCALL/RET */
	gdt_init();
	tss_init();
	tss_set_rsp0(cpu->idle_thread->rsp);
	idt_ap_init(); /* we re-use the same IDT for every CPU */
	syscall_init();

	/* set up the local APIC on this CPU */
	apic_init();

	/* flush the TLB (as up until this point we won't have received TLB shootdowns) */
	tlb_flush();

	/* increment the ready counter */
	spin_lock(&ready_cpus_lock);
	ready_cpus++;
	spin_unlock(&ready_cpus_lock);

	/* wait for the switch to SMP mode */
	bool ready;
	do
	{
		spin_lock(&mode_switch_lock);
		ready = mode_switched;
		spin_unlock(&mode_switch_lock);
	} while (!ready);
	
	// Retrieve topology information
	topology_init(cpu);

	/* set up the scheduler */
	sched_init();

	/* halt forever - the scheduler will take over from here */
	halt_forever();
}
