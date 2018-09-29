/*
ITS kernel standard library I/O interface.
*/

/* INCLUDES */

#include <io.h>
#include <stdbool.h>
#include <internal/keyboard/keyboard.h>
#include <internal/terminal/terminal.h>
#include <string.h>
#include <memory.h>
#include <internal/syscall/syscalls.h>


/* VARIABLES */

// Determines whether initialization has completed.
static bool initDone = false;


/* FUNCTIONS */

void io_init(int lines)
{
	// Only initialize once
	if(initDone)
		return;
	
	// Initialize printf_locked mutex
	printf_init();
	
	// Initialize keyboard
	keyboard_init();
	
	// Initialize terminal
	terminal_init(lines);
	
	// Initialized
	initDone = true;
}

char *getline()
{
	// Initialized?
	if(!initDone)
		return 0;
	
	// Temporary buffer for input characters
	char inputBuffer[2048];
	size_t inputSize = 0;
	
	// Read key presses until ENTER is encountered
	while(true)
	{
		// Read next key press
		bool shiftPressed;
		vkey_t key = receive_keypress(&shiftPressed);
		
		// Modify buffer depending on key code
		if(key_is_printable_character(key))
		{
			// Add printable character to input buffer and show it on the terminal
			if(inputSize < sizeof(inputBuffer))
			{
				char keyChar = key_to_character(key, shiftPressed);
				inputBuffer[inputSize++] = keyChar;
				terminal_putc(keyChar);
			}
		}
		else if(key == VKEY_ENTER)
		{
			// Print line break
			terminal_putc('\n');
			
			// Copy line string into return array
			char *line = malloc(inputSize + 1);
			memcpy(line, inputBuffer, inputSize);
			line[inputSize] = '\0';
			return line;
		}
		else if(key == VKEY_BACKSPACE)
		{
			// Remove last character from buffer and terminal
			// TODO (low priority) this will produce buggy terminal output since terminal_putc can only
			//      remove characters from the current terminal line, causing problems with line wrapping
			if(inputSize > 0)
			{
				inputBuffer[--inputSize] = '\0';
				terminal_putc('\b');
			}
		}
	}
	return 0;
}

fs_err_t create_directory(const char *path, const char *name)
{
	// Call system function
	return sys_create_directory(path, name);
}

fs_err_t create_file(const char *path, const char *name, void *data, int dataLength)
{
	// Call system function
	return sys_create_file(path, name, data, dataLength);
}

fs_err_t get_file(const char *path, void **dataPtr, int *dataLengthPtr)
{
	// Get file size
	fs_err_t err = sys_get_file_info(path, dataLengthPtr);
	if(err != RAMFS_ERR_OK)
		return err;
		
	// Allocate memory for file contents
	*dataPtr = malloc(*dataLengthPtr);
	
	// Get file contents
	return sys_get_file(path, *dataPtr, *dataLengthPtr);
}

void dump_files()
{
	// Allocate buffer
	int lsBufferSize = sys_dump_files_get_buffer_size();
	char *lsBuffer = malloc(lsBufferSize);
	
	// Retrieve and print file system tree
	sys_dump_files(lsBuffer, lsBufferSize);
	printf("%s\n", lsBuffer);
	
	// Done
	free(lsBuffer);
}