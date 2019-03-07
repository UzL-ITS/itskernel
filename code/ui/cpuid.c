/*
System capability.

Implementation partially from https://gist.github.com/macton/4dd5fec2113be284796e
*/

/* INCLUDES */

#include "cpuid.h"
#include <io.h>
#include <memory.h>
#include <internal/syscall/syscalls.h>
#include <internal/cpu/cpuid.h>


/* FUNCTIONS */

void print_topology()
{
	// Retrieve processor count
	uint32_t processorCount;
	sys_info(0, (uint8_t *)&processorCount);
	printf_locked("CPU count: %d\n", processorCount);
	
	// Retrieve topology data
	uint32_t *topologyBuffer = (uint32_t *)malloc(processorCount * 12);
	sys_info(1, (uint8_t *)topologyBuffer);
	printf_locked("CPU topology data:\n");
	for(uint32_t p = 0; p < processorCount; ++p)
	{
		// Print data of this processor
		printf_locked("    CPU #%d:\n", p);
		printf_locked("        Package: %d\n", topologyBuffer[3 * p + 0]);
		printf_locked("        Core: %d\n", topologyBuffer[3 * p + 1]);
		printf_locked("        Thread: %d\n", topologyBuffer[3 * p + 2]);
	}
	
	// Done
	free(topologyBuffer);
}

// Prints the given feature name, if the "test" parameter is non-zero.
static void print_feature(uint32_t test, const char *name)
{
	printf_locked("%s - %s\n", (test) ? "[yes]" : "[no] ", name);
}

// Reinterprets and prints the given unsigned 32-bit integer as 4 characters.
void print_uint32_chars(uint32_t a)
{
	char *aChars = (char *)&a;
	printf_locked("%c%c%c%c", aChars[0], aChars[1], aChars[2], aChars[3]);
}

void print_capabilities()
{
	uint32_t eax;
	uint32_t ebx;
	uint32_t ecx;
	uint32_t edx;

	cpuid(0, &eax, &ebx, &ecx, &edx);
	uint32_t ids = eax;

	cpuid(0x80000000, &eax, &ebx, &ecx, &edx);
	uint32_t ex_ids = eax;

	// Vendor
	{
		// On AMD, 0x80000000 also returns Vendor ID; On Intel 0x80000000 is empty.
		cpuid(0x00000000, &eax, &ebx, &ecx, &edx);
		printf_locked("Vendor    = ");
		print_uint32_chars(ebx);
		print_uint32_chars(edx);
		print_uint32_chars(ecx);
		printf_locked("\n");
	}
	
	// Model name
	if(ex_ids >= 0x80000004)
	{
		printf_locked("Processor = ");
		cpuid(0x80000002, &eax, &ebx, &ecx, &edx);
		print_uint32_chars(eax);
		print_uint32_chars(ebx);
		print_uint32_chars(ecx);
		print_uint32_chars(edx);
		cpuid(0x80000003, &eax, &ebx, &ecx, &edx);
		print_uint32_chars(eax);
		print_uint32_chars(ebx);
		print_uint32_chars(ecx);
		print_uint32_chars(edx);
		cpuid(0x80000004, &eax, &ebx, &ecx, &edx);
		print_uint32_chars(eax);
		print_uint32_chars(ebx);
		print_uint32_chars(ecx);
		print_uint32_chars(edx);
		printf_locked("\n");
	}
	printf_locked("\n");

	// Features
	if(ids >= 1)
	{
		cpuid(1, &eax, &ebx, &ecx, &edx);
		print_feature(edx & (1 <<  0), "FPU           (Floating-point Unit on-chip)");
		print_feature(edx & (1 <<  1), "VME           (Virtual Mode Extension)");
		print_feature(edx & (1 <<  2), "DE            (Debugging Extension)");
		print_feature(edx & (1 <<  3), "PSE           (Page Size Extension)");
		print_feature(edx & (1 <<  4), "TSC           (Time Stamp Counter)");
		print_feature(edx & (1 <<  5), "MSR           (Model Specific Registers)");
		print_feature(edx & (1 <<  6), "PAE           (Physical Address Extension)");
		print_feature(edx & (1 <<  7), "MCE           (Machine Check Exception)");
		print_feature(edx & (1 <<  8), "CX8           (CMPXCHG8 Instructions)");
		print_feature(edx & (1 <<  9), "APIC          (On-chip APIC hardware)");
		print_feature(edx & (1 << 11), "SEP           (Fast System Call)");
		print_feature(edx & (1 << 12), "MTRR          (Memory type Range Registers)");
		print_feature(edx & (1 << 13), "PGE           (Page Global Enable)");
		print_feature(edx & (1 << 14), "MCA           (Machine Check Architecture)");
		print_feature(edx & (1 << 15), "CMOV          (Conditional Move Instruction)");
		print_feature(edx & (1 << 16), "PAT           (Page Attribute Table)");
		print_feature(edx & (1 << 17), "PSE36         (36bit Page Size Extension");
		print_feature(edx & (1 << 18), "PSN           (Processor Serial Number)");
		print_feature(edx & (1 << 19), "CLFSH         (CFLUSH Instruction)");
		print_feature(edx & (1 << 21), "DS            (Debug Store)");
		print_feature(edx & (1 << 22), "ACPI          (Thermal Monitor & Software Controlled Clock)");
		print_feature(edx & (1 << 23), "MMX           (Multi-Media Extension)");
		print_feature(edx & (1 << 24), "FXSR          (Fast Floating Point Save & Restore)");
		print_feature(edx & (1 << 25), "SSE           (Streaming SIMD Extension 1)");
		print_feature(edx & (1 << 26), "SSE2          (Streaming SIMD Extension 2)");
		print_feature(edx & (1 << 27), "SS            (Self Snoop)");
		print_feature(edx & (1 << 28), "HTT           (Hyper Threading Technology)");
		print_feature(edx & (1 << 29), "TM            (Thermal Monitor)");
		print_feature(edx & (1 << 31), "PBE           (Pend Break Enabled)");
		print_feature(ecx & (1 <<  0), "SSE3          (Streaming SMD Extension 3)");
		print_feature(ecx & (1 <<  3), "MW            (Monitor Wait Instruction");
		print_feature(ecx & (1 <<  4), "CPL           (CPL-qualified Debug Store)");
		print_feature(ecx & (1 <<  5), "VMX           (Virtual Machine Extensions)");
		print_feature(ecx & (1 <<  7), "EST           (Enchanced Speed Test)");
		print_feature(ecx & (1 <<  8), "TM2           (Thermal Monitor 2)");
		print_feature(ecx & (1 <<  9), "SSSE3         (Supplemental Streaming SIMD Extensions 3)");
		print_feature(ecx & (1 << 10), "L1            (L1 Context ID)");
		print_feature(ecx & (1 << 12), "FMA3          (Fused Multiply-Add 3-operand Form)");
		print_feature(ecx & (1 << 13), "CAE           (Compare And Exchange 16B)");
		print_feature(ecx & (1 << 19), "SSE41         (Streaming SIMD Extensions 4.1)");
		print_feature(ecx & (1 << 20), "SSE42         (Streaming SIMD Extensions 4.2)");
		print_feature(ecx & (1 << 23), "POPCNT        (Advanced Bit Manipulation - Bit Population Count Instruction)");
		print_feature(ecx & (1 << 25), "AES           (Advanced Encryption Standard)");
		print_feature(ecx & (1 << 26), "XSAVE         (XSAVE/XRSTOR Instructions)");
		print_feature(ecx & (1 << 27), "OSXSAVE       (XSAVE/XRSTOR Enabled)");
		print_feature(ecx & (1 << 28), "AVX           (Advanced Vector Extensions)");
		print_feature(ecx & (1 << 30), "RDRAND        (Random Number Generator)");
	}
	
	// Features
	if(ids >= 7)
	{
		cpuid(7, &eax, &ebx, &ecx, &edx);
		print_feature(ebx & (1 <<  5), "AVX2          (Advanced Vector Extensions 2)");
		print_feature(ebx & (1 <<  3), "BMI1          (Bit Manipulations Instruction Set 1)");
		print_feature(ebx & (1 <<  8), "BMI2          (Bit Manipulations Instruction Set 2)");
		print_feature(ebx & (1 << 19), "ADX           (Multi-Precision Add-Carry Instruction Extensions)");
		print_feature(ebx & (1 << 16), "AVX512F       (512-bit extensions to Advanced Vector Extensions Foundation)");
		print_feature(ebx & (1 << 26), "AVX512PFI     (512-bit extensions to Advanced Vector Extensions Prefetch Instructions)");
		print_feature(ebx & (1 << 27), "AVX512ERI     (512-bit extensions to Advanced Vector Extensions Exponential and Reciprocal Instructions)");
		print_feature(ebx & (1 << 28), "AVX512CDI     (512-bit extensions to Advanced Vector Extensions Conflict Detection Instructions)");
		print_feature(ebx & (1 << 29), "SHA           (Secure Hash Algorithm)");
	}
	
	// Features
	if(ex_ids >= 0x80000001)
	{
		cpuid(0x80000001, &eax, &ebx, &ecx, &edx);
		print_feature(edx & (1 << 29), "X64           (64-bit Extensions/Long mode)");
		print_feature(ecx & (1 <<  5), "LZCNT         (Advanced Bit Manipulation - Leading Zero Bit Count Instruction)");
		print_feature(ecx & (1 <<  6), "SSE4A         (Streaming SIMD Extensions 4a)");
		print_feature(ecx & (1 << 16), "FMA4          (Fused Multiply-Add 4-operand Form)"); 
		print_feature(ecx & (1 << 11), "XOP           (Extended Operations)");
		print_feature(ecx & (1 << 21), "TBM           (Trailing Bit Manipulation Instruction)");
		print_feature(ecx & (1 << 15), "LWP           (Light Weight Profiling Support)");
		print_feature(ecx & (1 << 13), "WDT           (Watchdog Timer Support)");
		print_feature(ecx & (1 << 10), "IBS           (Instruction Based Sampling)");
		print_feature(ecx & (1 <<  8), "3DNOWPREFETCH (PREFETCH and PREFETCHW instruction support)");
		print_feature(ecx & (1 <<  7), "MISALIGNSSE   (Misaligned SSE mode)");
		print_feature(ecx & (1 <<  2), "SVM           (Secure Virtual Machine)");
		print_feature(ecx & (1 <<  0), "LAHFSAHF      (LAHF and SAHF instruction support in 64-bit mode)");
	}
}