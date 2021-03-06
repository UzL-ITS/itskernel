#include "pmm.h"
#include "common.h"
#include <stdlib.h>
#include <stdio.h>

#include <malloc.h>

static uint64_t addresses[400000] = { 0 };

int main()
{
	uint64_t size = 1 * 1024 * 1024 * 1024;
	uint64_t mem = (uint64_t)malloc(size);
	if(!mem)
		printf("malloc failed, errno %d\n", errno);
	else
	{
		uint64_t memAligned = (mem + 0xFFF) & ~0xFFF;
		pmm_init(11 * 4096, size - (memAligned - mem), memAligned, 4096); // Avoid ambiguity between 0 pointers and real address 0x00000000

		pmm_dump_stack("R:\\dump1.bin");

		int addr = 0;

		for(int j = 0; j < 400000; ++j)
		{
			addresses[addr++] = pmm_allocs(0);
		}

		pmm_dump_stack("R:\\dump2.bin");

		for(int j = 0; j < 400000; ++j)
			if(addresses[--addr])
				pmm_frees(0, addresses[addr]);

		for(int i = 0; i < 5; ++i)
		{
			for(int j = 0; j < 95; ++j)
				addresses[addr++] = pmm_allocs(1);
			for(int j = 0; j < 500; ++j)
				addresses[addr++] = pmm_allocs(0);
		}

		pmm_dump_stack("R:\\dump3.bin");

		for(int i = 0; i < 5; ++i)
		{
			for(int j = 0; j < 500; ++j)
				if(addresses[--addr])
					pmm_frees(0, addresses[addr]);
			for(int j = 0; j < 95; ++j)
				if(addresses[--addr])
					pmm_frees(1, addresses[addr]);
		}

		pmm_dump_stack("R:\\dump4.bin");

		for(int j = 0; j < 200000; ++j)
			addresses[addr++] = pmm_allocs(0);

		pmm_dump_stack("R:\\dump5.bin");

		for(int j = 0; j < 200000; ++j)
			if(addresses[--addr])
				pmm_frees(0, addresses[addr]);

		pmm_dump_stack("R:\\dump6.bin");

        uint64_t contiguous = pmm_alloc_contiguous(SIZE_2M, 100);

        pmm_dump_stack("R:\\dump7.bin");
	}

	printf("Finished.\n");
	getchar();
	return 0;
}

