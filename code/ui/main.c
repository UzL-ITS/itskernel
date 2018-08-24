/*
Kernel UI process main file.
*/

/* INCLUDES */

#include <io.h>
#include <internal/syscall/syscalls.h>


/* FUNCTIONS */

// Handles a Fn key press.
static void handle_window_switch(vkey_t keyCode, bool shiftPressed)
{
	// Switch to given render context
	sys_set_displayed_process(keyCode - VKEY_F1);
}

int main()
{
	// Initialize I/O
	io_init(1000);
	printf("--- ITS Micro Kernel :: UI PROCESS ---\n");
	
	// Install handler for render context switch
	printf("Installing context switch handlers...");
	for(int c = VKEY_F1; c <= VKEY_F12; ++c)
		register_keypress_handler(c, &handle_window_switch);
	printf("OK\n");
	
	printf("Alloc test...");
	uint8_t *mem = sys_heap_alloc(4096);
	for(int i = 0; i < 4096; ++i)
		mem[i] = 1;
	int sum = 0;
	for(int i = 0; i < 4096; ++i)
		sum += mem[i];
	if(sum == 4096)
		printf("OK\n");
	else
		printf("Failed (%d)\n", sum);
	
	// test
	getline();
	
	return 0;
}