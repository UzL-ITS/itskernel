/*
Kernel UI process main file.
*/

/* INCLUDES */

#include <app.h>
#include <io.h>
#include <internal/syscall/syscalls.h>
#include <threading/thread.h>
#include <internal/keyboard/keyboard.h>
#include <memory.h>


/* FUNCTIONS */

// Handles a Fn key press.
static void handle_window_switch(vkey_t keyCode, bool shiftPressed)
{
	// Switch to given render context
	sys_set_displayed_process(keyCode - VKEY_F1);
}

void main()
{
	// Initialize library
	_start();
	
	// Banner
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
	
	printf("Type something: ");
	char *input = getline();
	printf("You typed \"%s\"\n", input);
	free(input);
	
	printf("Done.\n");
		
	// Exit with return code
	_end(0);
}