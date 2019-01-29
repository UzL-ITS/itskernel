/*
ITS kernel RAM file system implementation.
Directories are stored as linked list, where each maintains a pointer to the next directory on the same level,
and to its first and last child directories on the next level.
*/

/* INCLUDES */

#include "ramfs.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>


/* TYPES */

// Represents a pointer to a file block.
typedef struct ramfs_file_block_entry_s
{
	// Pointer to the previous block.
	struct ramfs_file_block_entry_s *prev;

	// Pointer to the next block.
	struct ramfs_file_block_entry_s *next;

	// Pointer to the associated memory.
	void *block;

	// Size of the block.
	int blockSize;

	// Length of the contained data.
	int dataLength;

} ramfs_file_block_entry_t;

// Represents a file system file entry.
typedef struct ramfs_file_s
{
	// Name of this file.
	char name[64];

	// Determines whether this file is currently open.
	bool isOpen;

	// Pointer to the file contents.
	ramfs_file_block_entry_t *firstBlock;
	ramfs_file_block_entry_t *lastBlock;

	// Size of the first file block.
	int dataLength;

	// The next file in the same directory.
	struct ramfs_file_s *next;

} ramfs_file_t;

// Represents a file system directory entry.
typedef struct ramfs_directory_s
{
	// Name of this directory.
	char name[64];

	// The next directory on the current level.
	struct ramfs_directory_s *next;

	// The first sub directory.
	struct ramfs_directory_s *firstChild;

	// The last sub directory.
	struct ramfs_directory_s *lastChild;

	// The first contained file.
	struct ramfs_file_s *firstFile;

	// The last contained file.
	struct ramfs_file_s *lastFile;

} ramfs_directory_t;

// Contains information about an open file.
typedef struct
{
	// Determines whether the handle is active.
	bool active;

	// Pointer to the open file.
	ramfs_file_t *file;

	// Read/write position.
	int position;

} ramfs_file_handle_t;


/* VARIABLES */

// Minimal size of a file block.
#define FILE_MIN_BLOCK_SIZE 4096

// The root directory.
static ramfs_directory_t *root;

// The total amount of directories and files.
static int totalEntryCount = 0;

// Lock to protect file system structure on simultaneous access.
static int ramfsLock = 0;

// Active file handles.
#define FILE_HANDLE_COUNT 64
static ramfs_file_handle_t fileHandles[FILE_HANDLE_COUNT];


/* FUNCTIONS */

void ramfs_init()
{
	// Initialize root directory
	root = (ramfs_directory_t *)malloc(sizeof(ramfs_directory_t));
	root->name[0] = '\0';
	root->next = 0;
	root->firstChild = 0;
	root->lastChild = 0;
	root->firstFile = 0;
	root->lastFile = 0;
	totalEntryCount = 1;

	// Reset all handles
	for(int i = 0; i < FILE_HANDLE_COUNT; ++i)
		fileHandles[i].active = false;
}

static void acquire_lock()
{
	// Placeholder
	if(ramfsLock)
		printf("acquire_lock error\n");
	else
		ramfsLock = 1;
}

static void release_lock()
{
	// Placeholder
	if(!ramfsLock)
		printf("release_lock error\n");
	else
		ramfsLock = 0;
}

// Traverses the directory tree according to the given path and returns a pointer to the most deeply nested directory.
static ramfs_err_t traverse_tree(const char *path, int pathLength, ramfs_directory_t **directoryPtr)
{
	// Path should start with / (root directory)
	ramfs_directory_t *currentDir = root;
	int index = 0;
	if(pathLength == 0 || path[index++] != '/')
		return RAMFS_ERR_INVALID_PATH_FORMAT;

	// Iterate through sub directories
	while(1)
	{
		// Ignore obsolete slashes
		while(index < pathLength && path[index] == '/')
			++index;
		if(index == pathLength)
			break;

		// Get name of next directory
		int nameStartIndex = index;
		while(index < pathLength && path[index] != '/')
			++index;
		int nameLength = index - nameStartIndex;
		if(nameLength == 0)
			break;

		// Iterate through sub directories
		ramfs_directory_t *currentSubDir = currentDir->firstChild;
		while(currentSubDir)
		{
			// Correct name?
			if(strncmp(&path[nameStartIndex], currentSubDir->name, nameLength) == 0)
			{
				// This is the next current directory
				currentDir = currentSubDir;
				break;
			}

			// Next sub directory
			currentSubDir = currentSubDir->next;
		}
		if(!currentSubDir)
			return RAMFS_ERR_DIRECTORY_DOES_NOT_EXIST;

		// Done?
		++index;
		if(index == pathLength)
			break;
	}

	// Return pointer to last sub directory
	*directoryPtr = currentDir;
	return RAMFS_ERR_OK;
}

ramfs_err_t ramfs_create_directory(const char *path, const char *name)
{
	// The directory name must not contain any reserved characters
	for(const char *n = name; *n; ++n)
		if(*n == '/')
			return RAMFS_ERR_ILLEGAL_CHARACTER;

	acquire_lock();

	// Get sub directory where the new directory shall be placed in
	ramfs_directory_t *parentDirectory;
	int pathLength = strlen(path);
	ramfs_err_t err = traverse_tree(path, pathLength, &parentDirectory);
	if(err != RAMFS_ERR_OK)
	{
		release_lock();
		return err;
	}

	// Check whether directory exists already
	for(ramfs_directory_t *subDir = parentDirectory->firstChild; subDir; subDir = subDir->next)
		if(strcmp(subDir->name, name) == 0)
		{
			release_lock();
			return RAMFS_ERR_DIRECTORY_EXISTS;
		}

	// Create directory
	ramfs_directory_t *directory = (ramfs_directory_t *)malloc(sizeof(ramfs_directory_t));
	strncpy(directory->name, name, sizeof(directory->name));
	directory->next = 0;
	directory->firstChild = 0;
	directory->lastChild = 0;
	directory->firstFile = 0;
	directory->lastFile = 0;

	// Add directory to parent directory
	if(parentDirectory->lastChild)
		parentDirectory->lastChild->next = directory; // Update last sub directory of parent
	else
		parentDirectory->firstChild = directory; // The parent directory does not have any sub directories yet
	parentDirectory->lastChild = directory;
	++totalEntryCount;

	// Done
	release_lock();
	return RAMFS_ERR_OK;
}

ramfs_err_t ramfs_create_file(const char *path, const char *name)
{
	// The file name must not contain any reserved characters
	for(const char *n = name; *n; ++n)
		if(*n == '/')
			return RAMFS_ERR_ILLEGAL_CHARACTER;

	acquire_lock();

	// Get sub directory where the new file shall be placed in
	ramfs_directory_t *directory;
	int pathLength = strlen(path);
	ramfs_err_t err = traverse_tree(path, pathLength, &directory);
	if(err != RAMFS_ERR_OK)
	{
		release_lock();
		return err;
	}

	// Check whether file exists already
	for(ramfs_file_t *f = directory->firstFile; f; f = f->next)
		if(strcmp(f->name, name) == 0)
		{
			release_lock();
			return RAMFS_ERR_FILE_EXISTS;
		}

	// Allocate first file block
	ramfs_file_block_entry_t *firstBlockEntry = malloc(sizeof(ramfs_file_block_entry_t));
	firstBlockEntry->next = 0;
	firstBlockEntry->prev = 0;
	firstBlockEntry->block = malloc(FILE_MIN_BLOCK_SIZE);
	firstBlockEntry->blockSize = FILE_MIN_BLOCK_SIZE;
	firstBlockEntry->dataLength = 0;

	// Create file
	ramfs_file_t *file = (ramfs_file_t *)malloc(sizeof(ramfs_file_t));
	strncpy(file->name, name, sizeof(file->name));
	file->isOpen = false;
	file->firstBlock = firstBlockEntry;
	file->lastBlock = firstBlockEntry;
	file->dataLength = 0;
	file->next = 0;

	// Add file to directory
	if(directory->lastFile)
		directory->lastFile->next = file; // Update last file entry of directory
	else
		directory->firstFile = file; // The directory does not have any files yet
	directory->lastFile = file;
	++totalEntryCount;

	// Done
	release_lock();
	return RAMFS_ERR_OK;
}

static ramfs_err_t ramfs_get_file_entry(const char *path, ramfs_file_t **fileEntryPtr)
{
	// The path must not end with a slash
	int pathLength = strlen(path);
	if(pathLength == 0 || path[pathLength - 1] == '/')
		return RAMFS_ERR_INVALID_PATH_FORMAT;

	// Strip file name from given path
	const char *fileNameStart = &path[pathLength - 1];
	int fileNameLength = 1;
	while(pathLength >= fileNameLength && *fileNameStart != '/')
	{
		--fileNameStart;
		++fileNameLength;
	}
	if(pathLength < fileNameLength)
		return RAMFS_ERR_INVALID_PATH_FORMAT;

	// Skip slash
	++fileNameStart;
	--fileNameLength;

	// Get containing directory
	ramfs_directory_t *directory;
	ramfs_err_t err = traverse_tree(path, pathLength - fileNameLength, &directory);
	if(err != RAMFS_ERR_OK)
		return err;

	// Find file in directory
	for(ramfs_file_t *file = directory->firstFile; file; file = file->next)
		if(strcmp(file->name, fileNameStart) == 0)
		{
			// Return file info struct
			*fileEntryPtr = file;
			return RAMFS_ERR_OK;
		}

	// Not found
	return RAMFS_ERR_FILE_DOES_NOT_EXIST;
}

ramfs_err_t ramfs_get_file(const char *path, void *dataBuffer, int dataBufferLength)
{
	acquire_lock();

	// Get file info struct
	ramfs_file_t *file;
	ramfs_err_t err = ramfs_get_file_entry(path, &file);
	if(err != RAMFS_ERR_OK)
	{
		release_lock();
		return err;
	}

	// Check buffer size
	if(dataBufferLength < file->dataLength)
	{
		release_lock();
		return RAMFS_ERR_BUFFER_TOO_SMALL;
	}

	// Copy data
	memcpy(dataBuffer, file->data, file->dataLength);

	release_lock();
	return RAMFS_ERR_OK;
}

ramfs_err_t ramfs_get_file_info(const char *path, int *dataLengthPtr)
{
	acquire_lock();

	// Get file info struct
	ramfs_file_t *file;
	ramfs_err_t err = ramfs_get_file_entry(path, &file);
	if(err != RAMFS_ERR_OK)
	{
		release_lock();
		return err;
	}

	// Copy infos
	*dataLengthPtr = file->dataLength;

	release_lock();
	return RAMFS_ERR_OK;
}

// Helper function to recursively dump a directory.
static int dump_recursive(ramfs_directory_t *directory, int level, char *buffer, int bufferPos)
{
	// Indent name of directory
	for(int l = 0; l < 2 * level; ++l)
		buffer[bufferPos++] = ' ';

	// Name of directory
	int directoryNameLength = strlen(directory->name);
	strncpy(&buffer[bufferPos], directory->name, directoryNameLength);
	bufferPos += directoryNameLength;
	buffer[bufferPos++] = '/';

	// New line
	buffer[bufferPos++] = '\n';

	// Print files
	for(ramfs_file_t *file = directory->firstFile; file; file = file->next)
	{
		// Indent
		for(int l = 0; l < 2 * (level + 1); ++l)
			buffer[bufferPos++] = ' ';

		// Name
		int fileNameLength = strlen(file->name);
		strncpy(&buffer[bufferPos], file->name, fileNameLength);
		bufferPos += fileNameLength;

		// Size begin
		strncpy(&buffer[bufferPos], " (", 1 + 1);
		bufferPos += 1 + 1;

		// Size
		char sizeBuffer[16];
		itoa(file->dataLength, sizeBuffer, 10);
		int sizeLength = strlen(sizeBuffer);
		strncpy(&buffer[bufferPos], sizeBuffer, sizeLength);
		bufferPos += sizeLength;

		// Size end
		strncpy(&buffer[bufferPos], " bytes)\n", 1 + 5 + 1 + 1);
		bufferPos += 1 + 5 + 1 + 1;
	}

	// Print sub directories
	for(ramfs_directory_t *subDir = directory->firstChild; subDir; subDir = subDir->next)
		bufferPos = dump_recursive(subDir, level + 1, buffer, bufferPos);

	return bufferPos;
}

void ramfs_dump(char *buffer, int bufferLength)
{
	acquire_lock();

	// Check buffer size
	if(bufferLength < ramfs_dump_get_buffer_size())
	{
		// Output error message
		strncpy(buffer, "Wrong buffer size", 5 + 1 + 6 + 1 + 4);
		release_lock();
		return;
	}

	// Run recursively through directory tree
	int bufferPos = dump_recursive(root, 0, buffer, 0);
	buffer[bufferPos] = '\0';
	release_lock();
}

int ramfs_dump_get_buffer_size()
{
	// 64 bytes for the entry name and 64 bytes for padding / other information
	return totalEntryCount * 128;
}

ramfs_err_t ramfs_open(const char *path, ramfs_fd_t *fdPtr)
{
	acquire_lock();

	// Check for available file handle
	int fd = RAMFS_FD_INVALID;
	for(int i = 0; i < FILE_HANDLE_COUNT; ++i)
		if(!fileHandles[i].active)
		{
			fd = i;
			break;
		}
	if(fd == RAMFS_FD_INVALID)
	{
		release_lock();
		return RAMFS_ERR_TOO_MANY_OPEN_FILES;
	}

	// TODO create file if not exists

	// Get file info struct
	ramfs_file_t *file;
	ramfs_err_t err = ramfs_get_file_entry(path, &file);
	if(err != RAMFS_ERR_OK)
	{
		release_lock();
		return err;
	}

	// Initialize handle
	ramfs_file_handle_t *handle = &fileHandles[fd];
	handle->active = true;
	handle->file = file;
	handle->position = 0;
	*fdPtr = fd;
	file->isOpen = true;

	// Done
	release_lock();
	return RAMFS_ERR_OK;
}

void ramfs_close(ramfs_fd_t fd)
{
	// Validate handle
	if(fd < 0 || fd >= FILE_HANDLE_COUNT || !fileHandles[fd].active)
		return;

	// Close handle
	fileHandles[fd].file->isOpen = false;
	fileHandles[fd].active = false;
}

int ramfs_read(void *buffer, int length, ramfs_fd_t fd)
{
	// The file is marked as "open", thus no one else can access it in parallel -> no locking needed

	// Validate handle
	if(fd < 0 || fd >= FILE_HANDLE_COUNT || !fileHandles[fd].active)
		return -1;

	// Get handle
	ramfs_file_handle_t *handle = &fileHandles[fd];

	// Calculate effective length
	int readLength = length;
	if(readLength < 0)
		readLength = 0;
	else if(handle->position + readLength > handle->file->dataLength)
		readLength = handle->file->dataLength - handle->position;

	// Copy data
	if(readLength > 0)
		memcpy(buffer, handle->file->data, readLength);
	return readLength;
}

int ramfs_write(void *buffer, int length, ramfs_fd_t fd)
{
	// Validate handle
	if(fd < 0 || fd >= FILE_HANDLE_COUNT || !fileHandles[fd].active)
		return -1;


}

int ramfs_tell(ramfs_fd_t fd)
{
	// Validate handle
	if(fd < 0 || fd >= FILE_HANDLE_COUNT || !fileHandles[fd].active)
		return -1;

	// Retrieve position
	return fileHandles[fd].position;
}

void ramfs_seek(int position, ramfs_fd_t fd)
{
	// Validate handle
	if(fd < 0 || fd >= FILE_HANDLE_COUNT || !fileHandles[fd].active)
		return -1;

	// Check whether position is valid
	ramfs_file_handle_t *handle = &fileHandles[fd];
	if(position < 0)
		position = 0;
	else if(position >= handle->file->dataLength)
		position = handle->file->dataLength - 1;

	// Update position
	handle->position = position;
}
