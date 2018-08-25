/*
Kernel UI process main file.
*/

/* INCLUDES */

#include <io.h>
#include <internal/syscall/syscalls.h>
#include <thread.h>


/* FUNCTIONS */

// Handles a Fn key press.
static void handle_window_switch(vkey_t keyCode, bool shiftPressed)
{
	// Switch to given render context
	sys_set_displayed_process(keyCode - VKEY_F1);
}

static void test_thread(void *args)
{
	for(int i = 0; i < 2000000000 - (int)args * 100000000; ++i)
		;
	printf("thread %p end!\n", args);
}

int main()
{
	// Initialize I/O
	io_init(1000);
	threading_init();
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
	
	
	run_thread(test_thread, 0);
	run_thread(test_thread, 1);
	run_thread(test_thread, 2);
	run_thread(test_thread, 3);
	
	// test
	getline();
	
	return 0;
}