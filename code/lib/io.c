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

fs_err_t fopen(const char *path, fs_fd_t *fdPtr)
{
	// Call system function
	return sys_fs_open(path, fdPtr);
}

void fclose(fs_fd_t fd)
{
	// Call system function
	sys_fs_close(fd);
}

uint64_t fread(uint8_t *buffer, uint64_t length, fs_fd_t fd)
{
	// Call system function
	return sys_fs_read(buffer, length, fd);
}

uint64_t fwrite(uint8_t *buffer, uint64_t length, fs_fd_t fd)
{
	// Call system function
	return sys_fs_write(buffer, length, fd);
}

uint64_t ftell(fs_fd_t fd)
{
	// Call system function
	return sys_fs_tell(fd);
}

void fseek(int64_t offset, fs_seek_whence_t whence, fs_fd_t fd)
{
	// Call system function
	sys_fs_seek(offset, whence, fd);
}

fs_err_t create_directory(const char *path, const char *name)
{
	// Call system function
	return sys_fs_create_directory(path, name);
}

fs_err_t test_directory(const char *path, const char *name)
{
	// Call system function
	return sys_fs_test_directory(path, name);
}

int flist(const char *path, char *buffer, int bufferLength)
{
	// Call system function
	return sys_fs_list(path, buffer, bufferLength);
}
