
#ifndef _SMP_CPU_H
#define _SMP_CPU_H

#include <cpu/gdt.h>
#include <cpu/tss.h>
#include <proc/proc.h>
#include <proc/thread.h>
#include <util/list.h>
#include <defs/types.h>
#include <stdbool.h>
#include <stdint.h>
#include <mm/tlb.h>

typedef struct cpu
{
	/*
	* cpu_get() relies on the fact that the first 8 bytes in the structure
	* points to itself
	*/
	struct cpu *self;

	/*
	* the hold count of the 'interrupt lock' (intr_enable/disable()).
	* intr_stub() relies on the fact that the second 8 bytes in the structure
	* are occupied by this field
	*/
	uint64_t intr_mask_count;

	/*
	* the current thread running on this cpu. syscall_stub() relies on the fact
	* that this is the third group of 8 bytes in this structure
	*/
	thread_t *thread;

	/* global CPU list node */
	list_node_t node;

	/* BSP flag */
	bool bsp;

	/* the local APIC and ACPI ids of this processor */
	cpu_lapic_id_t lapic_id;
	cpu_acpi_id_t acpi_id;

	/* the gdt and gdtr for this processor */
	gdtr_t gdtr;
	gdt_descriptor_t gdt_descriptors[GDT_DESCRIPTORS];

	/* the tss for this processor */
	tss_t tss;

	/* current process running on this cpu */
	proc_t *proc;

	/* idle thread for this cpu */
	thread_t *idle_thread;

	/* number of APIC ticks per millisecond */
	uint32_t apic_ticks_per_ms;

	// Total amount of elapsed milliseconds since the scheduler was started on this CPU (accurate to around 10ms).
	uint64_t elapsedMsSinceStart;

	/* flags indicating if LINTn should be programmed as NMIs */
	bool apic_lint_nmi[2];

	// The CPU core ID.
	int coreId;
	
	// The CPU's TLB queue.
	tlb_op_t tlbOperationQueue[TLB_OP_QUEUE_SIZE];
	
	// The current entry count of the CPU's TLB queue.
	int tlbOperationQueueLength;
} cpu_t;

extern list_t cpu_list;
extern int cpuCount;

void cpu_bsp_init(void);
bool cpu_ap_init(cpu_lapic_id_t lapic_id, cpu_acpi_id_t acpi_id);
void cpu_ap_install(cpu_t *cpu);

// Returns the current processor data.
cpu_t *cpu_get(void);

// Returns the bootstrap processor data.
cpu_t *cpu_get_bsp();

#endif
