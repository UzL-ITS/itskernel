
#include <syscalls.h>

uint64_t printf(const char *str)
{
	return testprintf(str);
}