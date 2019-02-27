
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

// Renders the given pixel data at the specified position.
int sys_vbe_draw(uint32_t *pixels, uint32_t posX, uint32_t posY, uint32_t width, uint32_t height);

// Allocates a 4K aligned block of memory of the given size (rounded up to a multiple of 4K).
void *sys_heap_alloc(int size);

// Frees the given allocated memory.
void sys_heap_free(void *addr);

// Starts a new thread and sets the instruction pointer to the given address.
void sys_run_thread(uint64_t rip, const char *name);

// Exits the current thread.
void sys_exit_thread(cpu_state_t *state);

// Runs the given executable in a new process.
bool sys_start_process(const char *programPath);

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

// Dumps system information into the given file.
void sys_dump(int infoId, const char *filePath);

// Sets the core the current thread shall be run on.
void sys_set_affinity(int coreId);

// Resolves the underlying physical address of the given virtual address.
uint64_t sys_virt_to_phy(uint64_t addr);

// Resets the CPU using the ACPI.
void sys_reset();

// Custom system call for experiments.
// Note: This system call must not change the current thread or address space!
uint8_t *sys_custom(int param);

// Opens the given file and stores the descriptor in the given variable.
// If the file does not exist and the "create" flag is set, it is created.
ramfs_err_t sys_fs_open(const char *path, ramfs_fd_t *fdPtr, bool create);

// Closes the given file.
void sys_fs_close(ramfs_fd_t fd);

// Reads the given amount of bytes into the given buffer.
uint64_t sys_fs_read(uint8_t *buffer, uint64_t length, ramfs_fd_t fd);

// Writes the given amount of bytes.
uint64_t sys_fs_write(uint8_t *buffer, uint64_t length, ramfs_fd_t fd);

// Returns the current position in the file.
uint64_t sys_fs_tell(ramfs_fd_t fd);

// Moves to the given position in the file.
void sys_fs_seek(int64_t offset, ramfs_seek_whence_t whence, ramfs_fd_t fd);

// Creates a new directory at the given path.
ramfs_err_t sys_fs_create_directory(const char *path, const char *name);

// Tries to access the directory at the given path and returns suitable error codes (RAMFS_ERR_DIRECTORY_EXISTS vs. RAMFS_ERR_DIRECTORY_DOES_NOT_EXIST).
ramfs_err_t sys_fs_test_directory(const char *path, const char *name);

// Retrieves a list of the contents of the given directory.
// Note: This function does NOT append a terminating 0-byte.
int sys_fs_list(const char *path, char *buffer, int bufferLength);

// Deletes the given file.
ramfs_err_t sys_fs_delete(const char *path);
	
#endif
