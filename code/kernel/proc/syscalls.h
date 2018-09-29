
#ifndef _PROC_SYSCALLS_H
#define _PROC_SYSCALLS_H

#include <stdint.h>
#include <cpu/state.h>
#include <proc/msg.h>
#include <fs/ramfs.h>

// Prints the given string to kernel console. TODO remove, this is only for debugging
int64_t sys_trace(const char *message);

// TODO this does not do anything yet
void sys_exit(cpu_state_t *state);

// Switches to another thread, while the current one is e.g. waiting for messages (cooperative multitasking).
void sys_yield(cpu_state_t *state);

// Returns the type of the oldest non-processed message of the current process, if it exists.
msg_type_t sys_next_message_type();

// Returns and then deletes the oldest non-processed message, or 0 if there isn't any.
// The parameter messageBuffer must provide enough space to fit the given message, thus next_message_type() should be called first to determine the correct message size.
void sys_next_message(msg_header_t *messageBuffer);

// Changes the currently displayed process render context.
void sys_set_displayed_process(int contextId);

// Draws a rectangle at the specified position with the given dimensions.
int sys_vbe_rectangle(uint32_t posX, uint32_t posY, uint32_t width, uint32_t height);

// Renders the given character at the given render buffer position. Currently only ASCII characters (' ' ... '~') are supported.
int sys_vbe_render_char(uint32_t posX, uint32_t posY, char c);

// Returns the width of the render buffer.
uint32_t sys_vbe_get_screen_width();

// Returns the height of the render buffer.
uint32_t sys_vbe_get_screen_height();

// Sets the current foreground color.
void sys_vbe_set_front_color(uint8_t r, uint8_t g, uint8_t b);

// Sets the current background color.
void sys_vbe_set_back_color(uint8_t r, uint8_t g, uint8_t b);

// Allocates a scrollable back buffer, and returns true on success.
bool sys_vbe_allocate_scroll_buffer(uint32_t height);

// Scrolls the buffer to the specified position.
void sys_vbe_set_scroll_position(uint32_t y);

// Clears the buffer with the current background color.
void sys_vbe_clear();

// Allocates a 4K aligned block of memory of the given size (rounded up to a multiple of 4K).
void *sys_heap_alloc(int size);

// Frees the given allocated memory.
void sys_heap_free(void *addr);

// Starts a new thread and sets the instruction pointer to the given address.
void sys_run_thread(uint64_t rip);

// Exits the current thread.
void sys_exit_thread(cpu_state_t *state);

// Starts a new process based on the given ELF file data.
bool sys_start_process(uint8_t *program, int programLength);

// Returns the amount of elapsed milliseconds since system start.
uint64_t sys_get_elapsed_milliseconds();

// Returns the network card's MAC address in the given buffer of size 6.
void sys_get_network_mac_address(uint8_t *macBuffer);

// Checks for a new received network packet; if one is present, it is copied into the given buffer and its length is returned.
// The buffer should have the size of the device's MTU.
int sys_receive_network_packet(uint8_t *packetBuffer);

// Sends the given network packet.
void sys_send_network_packet(uint8_t *packet, int packetLength);

// Copies system information into the given buffer.
void sys_info(int infoId, uint8_t *buffer);

// Sets the core the current thread shall be run on.
void sys_set_affinity(int coreId);

// Creates a new directory under the given path.
ramfs_err_t sys_create_directory(const char *path, const char *name);

// Creates a new file at the given path.
ramfs_err_t sys_create_file(const char *path, const char *name, void *data, int dataLength);

// Returns the contents of the given file.
ramfs_err_t sys_get_file(const char *path, void *dataBuffer, int dataBufferLength);

// Returns the metadata of the given file.
ramfs_err_t sys_get_file_info(const char *path, int *dataLengthPtr);

// Dumps the entire file system tree into the given string buffer.
void sys_dump_files(char *buffer, int bufferLength);

// Returns the size of the string buffer that is needed to write the entire tree.
int sys_dump_files_get_buffer_size();
	
#endif
