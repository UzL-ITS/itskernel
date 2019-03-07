/*
AMD memory encryption tests.
*/

/* INCLUDES */

#include <app.h>
#include <io.h>
#include <memory.h>
#include <stdbool.h>
#include <stdint.h>
#include <internal/syscall/syscalls.h>


/* FUNCTIONS */

static inline uint64_t popcount64(uint64_t x)
{
  uint64_t m1 = 0x5555555555555555ll;
  uint64_t m2 = 0x3333333333333333ll;
  uint64_t m4 = 0x0F0F0F0F0F0F0F0Fll;
  uint64_t h01 = 0x0101010101010101ll;

  x -= (x >> 1) & m1;
  x = (x & m2) + ((x >> 2) & m2);
  x = (x + (x >> 4)) & m4;

  return (x * h01) >> 56;
}

const int tab64[64] = {
    63,  0, 58,  1, 59, 47, 53,  2,
    60, 39, 48, 27, 54, 33, 42,  3,
    61, 51, 37, 40, 49, 18, 28, 20,
    55, 30, 34, 11, 43, 14, 22,  4,
    62, 57, 46, 52, 38, 26, 32, 41,
    50, 36, 17, 19, 29, 10, 13, 21,
    56, 45, 25, 31, 35, 16,  9, 12,
    44, 24, 15,  8, 23,  7,  6,  5};

int log2_64 (uint64_t value)
{
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    value |= value >> 32;
    return tab64[((uint64_t)((value - (value >> 1))*0x07EDD5E59A4E28C2)) >> 58];
}

static inline void clflush(volatile void *p)
{
	__asm volatile ("clflush (%0)" :: "r"(p));
}

static void flush_page(uint8_t *p)
{
	for(int i = 0; i < 4096; i += 64)
		clflush(&p[i]);
}

void main()
{
	// Initialize library
	_start();
	
	sys_hugepage_mode(false);
			
	uint64_t vectors[] = {
		0b0000000000000000000000000001000000000000000000000000000000000000ull, // 36
		0b0000000000000000000000000000100000000000000000000000000000000000ull, // 35
		0b0000000000000000000000000000010000000000000000000000000000000000ull, // 34
		0b0000000000000000000000000000001000000000000000000000000000000000ull, // 33
		0b0000000000000000000000000000000100000000000000000000000000000000ull, // 32
		0b0000000000000000000000000000000010000000000000000000000000000000ull, // 31
		0b0000000000000000000000000000000001000000000000000000000000000000ull, // 30
		0b0000000000000000000000000000000000100000000000000000000000000000ull,
		0b0000000000000000000000000000000000010000000000000000000000000000ull,
		0b0000000000000000000000000000000000001000000000000000000000000000ull,
		0b0000000000000000000000000000000000000100000000000000000000000000ull,
		0b0000000000000000000000000000000000000010000000000000000000000000ull,
		0b0000000000000000000000000000000000000001000000000000000000000000ull,
		0b0000000000000000000000000000000000000000100000000000000000000000ull,
		0b0000000000000000000000000000000000000000010000000000000000000000ull,
		0b0000000000000000000000000000000000000000001000000000000000000000ull, // 21
		0b0000000000000000000000000000000000000001000100000000000000000000ull, // 24 | 20
		0b0000000000000000000000000000000000000001000010000000000000000000ull,
		0b0000000000000000000000000000000000000001000001000000000000000000ull,
		0b0000000000000000000000000000000000000001000000100000000000000000ull,
		0b0000000000000000000000000000000000000001000000010000000000000000ull,
		0b0000000000000000000000000000000000000001000000001000000000000000ull,
		0b0000000000000000000000000000000000000001000000000100000000000000ull,
		0b0000000000000000000000000000000000000001000000000010000000000000ull,
		0b0000000000000000000000000000000000000001000000000001000000000000ull,  // 24 | 12
	};
	int vectorCount = 25;
	uint64_t vectorsVirt[25] = { 0 };
	
	uint64_t addresses[1000];
	uint64_t available;
	int i = 0;
	uint64_t size = 256 * 1024 * 1024ull;
	while((available = get_available_physical_memory()) > size + 16 * 1024 * 1024)
	{
		addresses[i] = (uint64_t)malloc(size);
		printf_locked("Allocated at 0x%016llx, 0x%016llx bytes were available\n", addresses[i], available);
		for(uint64_t a = 0; a < size; a += 4096)
		{
			uint64_t phys = get_physical_address(addresses[i] + a);
			
			for(int v = 0; v < vectorCount; ++v)
				if(vectors[v] == phys)
				{
					vectorsVirt[v] = addresses[i] + a;
				}
		}
		
		++i;
	}
	
	printf_locked("Vectors: \n");
	for(int v = 0; v < vectorCount; ++v)
		printf_locked("0x%016llx: %s\n", vectors[v], vectorsVirt[v] ? "yes" : "no");
	
	printf_locked("Prepare encryption\n");
	uint8_t block[16];
	uint64_t cflag;
	{
		uint64_t baseVecVirt = vectorsVirt[0];
		uint8_t *baseVec = (uint8_t *)vectorsVirt[0];
		
		uint32_t buf[2];
		sys_info(3, (uint8_t *)buf);
		uint32_t eax = buf[0];
		uint32_t ebx = buf[1];
		printf_locked("EAX = %08x   EBX = %08x\n", eax, ebx);
		
		int bit = ebx & 0x3F;
		cflag = 1ull << bit;
		
		printf_locked("1: %016llx\n", sys_page_flags(baseVecVirt, 0x0, false));
		
		flush_page(baseVec);
		printf_locked("2: %016llx\n", sys_page_flags(baseVecVirt, cflag, true));
		
		memset(baseVec, 0, 4096);
		baseVec[15] = 0xFF;
		
		flush_page(baseVec);
		printf_locked("3: %016llx\n", sys_page_flags(baseVecVirt, cflag, false));
		
		memcpy(block, baseVec, sizeof(block));
	}
	
	printf_locked("Encrypting\n");
	fs_fd_t fd;
	fopen("/pages.bin", &fd, true);
	for(int v = 0; v < vectorCount; ++v)
	{
		uint8_t *vPage = (uint8_t *)vectorsVirt[v];
		
		for(int i = 0; i < 4096; i += sizeof(block))
			memcpy(&vPage[i], block, sizeof(block));
		
		flush_page(vPage);
		printf_locked("4: %016llx\n", sys_page_flags(vectorsVirt[v], cflag, true));
		
		fwrite((uint8_t *)&vectors[v], 8, fd);
		fwrite(vPage, 4096, fd);
	}
	fclose(fd);
	
	sys_hugepage_mode(true);
	
	printf_locked("Free\n");
	if(--i >= 0)
		free((uint8_t *)addresses[i]);
	printf("Freed enough memory for networking, but not all - restart!\n");
	
	printf_locked("Done\n");
	
	// Exit with return code
	_end(0);
}