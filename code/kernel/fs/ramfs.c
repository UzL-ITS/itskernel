/*
ITS kernel RAM file system implementation.
Directories are stored as linked list, where each maintains a pointer to the next directory on the same level,
and to its first and last child directories on the next level.
*/

/* INCLUDES */

#include <fs/ramfs.h>
#include <stdlib/stdlib.h>
#include <stdlib/string.h>
#include <lock/spinlock.h>
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
    uint8_t *block;

    // Size of the block.
    uint64_t blockSize;

    // Length of the contained data.
    uint64_t dataLength;

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
    uint64_t dataLength;

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

    // Absolute read/write position.
    uint64_t position;

    // Current block.
    ramfs_file_block_entry_t *currentBlock;

    // Relative position in current block.
    uint64_t currentBlockPosition;

} ramfs_file_handle_t;


/* VARIABLES */

// Minimal size of a file block.
#define FILE_MIN_BLOCK_SIZE 4096

// Alignment of the file block size.
#define FILE_BLOCK_SIZE_ALIGNMENT 64

// The root directory.
static ramfs_directory_t *root;

// The total amount of directories and files.
static int totalEntryCount = 0;

// Lock to protect file system structure on simultaneous access.
static spinlock_t ramfsLock = SPIN_UNLOCKED;

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
    spin_lock(&ramfsLock);
}

static void release_lock()
{
    spin_unlock(&ramfsLock);
}

// Traverses the directory tree according to the given path and returns a pointer to the most deeply nested directory.
static ramfs_err_t get_directory(const char *path, int pathLength, ramfs_directory_t **directoryPtr)
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

static ramfs_err_t get_file_entry(const char *path, const char **fileNameStartPtr, ramfs_file_t **fileEntryPtr, ramfs_directory_t **directoryEntryPtr)
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
    *fileNameStartPtr = fileNameStart;

    // Get containing directory
    ramfs_directory_t *directory;
    ramfs_err_t err = get_directory(path, pathLength - fileNameLength, &directory);
    if(err != RAMFS_ERR_OK)
        return err;
    if(directoryEntryPtr)
        *directoryEntryPtr = directory;

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

ramfs_err_t ramfs_create_directory(const char *path, const char *name)
{
    // The directory name must not contain any reserved characters
	if(*name == '.' && *(name + 1) == '.')
		return RAMFS_ERR_ILLEGAL_CHARACTER;
    for(const char *n = name; *n; ++n)
        if(*n == '/')
            return RAMFS_ERR_ILLEGAL_CHARACTER;

    acquire_lock();

    // Get sub directory where the new directory shall be placed in
    ramfs_directory_t *parentDirectory;
    int pathLength = strlen(path);
    ramfs_err_t err = get_directory(path, pathLength, &parentDirectory);
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
    directory->name[sizeof(directory->name) - 1] = '\0';
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

ramfs_err_t ramfs_test_directory(const char *path, const char *name)
{
    // The directory name must not contain any reserved characters
	if(*name == '.' && *(name + 1) == '.')
		return RAMFS_ERR_ILLEGAL_CHARACTER;
    for(const char *n = name; *n; ++n)
        if(*n == '/')
            return RAMFS_ERR_ILLEGAL_CHARACTER;

    acquire_lock();

    // Get sub directory where the new directory shall be placed in
    ramfs_directory_t *parentDirectory;
    int pathLength = strlen(path);
    ramfs_err_t err = get_directory(path, pathLength, &parentDirectory);
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

    // Directory does not exist yet
    release_lock();
    return RAMFS_ERR_DIRECTORY_DOES_NOT_EXIST;
}

static ramfs_err_t create_file(ramfs_directory_t *directory, const char *name, ramfs_file_t **fileEntryPtr)
{
    // The file name must not contain any reserved characters
    for(const char *n = name; *n; ++n)
        if(*n == '/')
            return RAMFS_ERR_ILLEGAL_CHARACTER;

    // Check whether file exists already
    for(ramfs_file_t *f = directory->firstFile; f; f = f->next)
        if(strcmp(f->name, name) == 0)
            return RAMFS_ERR_FILE_EXISTS;

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
    file->name[sizeof(file->name) - 1] = '\0';
    file->isOpen = false;
    file->firstBlock = firstBlockEntry;
    file->lastBlock = firstBlockEntry;
    file->dataLength = 0;
    file->next = 0;
    *fileEntryPtr = file;

    // Add file to directory
    if(directory->lastFile)
        directory->lastFile->next = file; // Update last file entry of directory
    else
        directory->firstFile = file; // The directory does not have any files yet
    directory->lastFile = file;
    ++totalEntryCount;

    // Done
    return RAMFS_ERR_OK;
}

ramfs_err_t ramfs_open(const char *path, ramfs_fd_t *fdPtr, bool create)
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

    // Get file info struct
    const char *fileNameStart; // Position of filename in path string
    ramfs_file_t *file;
    ramfs_directory_t *directory;
    ramfs_err_t err = get_file_entry(path, &fileNameStart, &file, &directory);
    if(err == RAMFS_ERR_FILE_DOES_NOT_EXIST && create)
    {
        // File does not exist yet, create it
        err = create_file(directory, fileNameStart, &file);
    }
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
    handle->currentBlock = file->firstBlock;
    handle->currentBlockPosition = 0;
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

uint64_t ramfs_read(uint8_t *buffer, uint64_t length, ramfs_fd_t fd)
{
    // The file is marked as "open", thus no one else can access it in parallel -> no locking needed

    // Validate handle
    if(fd < 0 || fd >= FILE_HANDLE_COUNT || !fileHandles[fd].active)
        return -1;

    // Get handle
    ramfs_file_handle_t *handle = &fileHandles[fd];

    // Calculate effective length
    uint64_t readLength = length;
	if(handle->position + readLength > handle->file->dataLength)
        readLength = handle->file->dataLength - handle->position;

    // Copy data
    uint64_t remainingLength = readLength;
    uint64_t bufferPos = 0;
    while(remainingLength > 0)
    {
        // Read remainder of current block
        if(handle->currentBlockPosition < handle->currentBlock->dataLength)
        {
            // Read entire block or only parts of it?
            uint64_t blockLen = handle->currentBlock->dataLength - handle->currentBlockPosition;
            if(remainingLength < blockLen)
            {
                // Read part of block
                memcpy(buffer + bufferPos, handle->currentBlock->block + handle->currentBlockPosition, remainingLength);
                bufferPos += remainingLength;
                handle->currentBlockPosition += remainingLength;
                handle->position += remainingLength;
                remainingLength = 0;

                // Done
                break;
            }

            // Copy remainder of block
            memcpy(buffer + bufferPos, handle->currentBlock->block + handle->currentBlockPosition, blockLen);
            bufferPos += blockLen;
            remainingLength -= blockLen;
            handle->position += blockLen;
        }

        // Next block
        handle->currentBlock = handle->currentBlock->next;
        handle->currentBlockPosition = 0;
    }

    return readLength;
}

uint64_t ramfs_write(uint8_t *buffer, uint64_t length, ramfs_fd_t fd)
{
    // Validate handle
    if(fd < 0 || fd >= FILE_HANDLE_COUNT || !fileHandles[fd].active)
        return -1;

    // Get handle
    ramfs_file_handle_t *handle = &fileHandles[fd];

    // Copy data
    uint64_t remainingLength = length;
    uint64_t bufferPos = 0;
    while(remainingLength > 0)
    {
        // Is there any remaining space in the current block?
        if(handle->currentBlockPosition < handle->currentBlock->blockSize)
        {
            // Copy to current block
            uint64_t blockSpace = handle->currentBlock->blockSize - handle->currentBlockPosition;
            if(blockSpace >= remainingLength)
            {
                // This block suffices, copy data
                memcpy(handle->currentBlock->block + handle->currentBlockPosition, buffer + bufferPos, remainingLength);
                bufferPos += remainingLength;
                handle->position += remainingLength;
                handle->currentBlockPosition += remainingLength;
                uint64_t oldDataLength = handle->currentBlock->dataLength;
                handle->currentBlock->dataLength = handle->currentBlockPosition;
                handle->file->dataLength += handle->currentBlock->dataLength - oldDataLength;
                remainingLength = 0;
                break;
            }

            // Fill remainder of block
            memcpy(handle->currentBlock->block + handle->currentBlockPosition, buffer + bufferPos, blockSpace);
            bufferPos += blockSpace;
            handle->position += blockSpace;
            uint64_t oldDataLength = handle->currentBlock->dataLength;
            handle->currentBlock->dataLength = handle->currentBlockPosition + blockSpace;
            handle->file->dataLength += handle->currentBlock->dataLength - oldDataLength;
            remainingLength -= blockSpace;
        }

        // Is there a next block?
        if(!handle->currentBlock->next)
        {
            // Calculate size of needed block
            uint64_t newBlockSize = FILE_BLOCK_SIZE_ALIGNMENT * (1 + remainingLength / FILE_BLOCK_SIZE_ALIGNMENT);
            if(newBlockSize < FILE_MIN_BLOCK_SIZE)
                newBlockSize = FILE_MIN_BLOCK_SIZE;

            // Allocate new block
            ramfs_file_block_entry_t *newBlockEntry = malloc(sizeof(ramfs_file_block_entry_t));
            newBlockEntry->next = 0;
            newBlockEntry->prev = handle->currentBlock;
            handle->currentBlock->next = newBlockEntry;
            newBlockEntry->block = malloc(newBlockSize);
            newBlockEntry->blockSize = newBlockSize;
            newBlockEntry->dataLength = 0;
            handle->file->lastBlock = newBlockEntry;
        }

        // Go to next block
        handle->currentBlock = handle->currentBlock->next;
        handle->currentBlockPosition = 0;
    }
    return length;
}

uint64_t ramfs_tell(ramfs_fd_t fd)
{
    // Validate handle
    if(fd < 0 || fd >= FILE_HANDLE_COUNT || !fileHandles[fd].active)
        return -1;

    // Retrieve position
    return fileHandles[fd].position;
}

void ramfs_seek(int64_t offset, ramfs_seek_whence_t whence, ramfs_fd_t fd)
{
    // Validate handle
    if(fd < 0 || fd >= FILE_HANDLE_COUNT || !fileHandles[fd].active)
        return;

    // Calculate target position
    ramfs_file_handle_t *handle = &fileHandles[fd];
    uint64_t position = 0;
    if(whence == RAMFS_SEEK_START && offset >= 0)
        position = offset;
    else if(whence == RAMFS_SEEK_CURRENT && (offset >= 0 || (int64_t)handle->position >= -offset))
        position = (uint64_t)(handle->position + offset);
    else if(whence == RAMFS_SEEK_END && offset <= 0 && (int64_t)handle->file->dataLength >= -offset)
        position = handle->file->dataLength + offset;
    if(position > handle->file->dataLength)
        position = handle->file->dataLength;

    // Find suitable block
    ramfs_file_block_entry_t *currBlock = handle->file->firstBlock;
    uint64_t skip = position;
    while(skip > 0)
    {
        // Skip entire block?
        if(skip > currBlock->dataLength)
        {
            skip -= currBlock->dataLength;
            currBlock = currBlock->next;
        }
        else
            break;
    }

    // Update position
    handle->currentBlock = currBlock;
    handle->currentBlockPosition = skip;
    handle->position = position;
}

ramfs_err_t ramfs_delete(const char *path)
{
    acquire_lock();

    // Get file info struct
    const char *fileNameStart; // Position of filename in path string
    ramfs_file_t *file;
    ramfs_directory_t *directory;
    ramfs_err_t err = get_file_entry(path, &fileNameStart, &file, &directory);
    if(err != RAMFS_ERR_OK)
    {
        release_lock();
        return err;
    }

    // The file must not be open
    if(file->isOpen)
    {
        release_lock();
        return RAMFS_ERR_FILE_ALREADY_OPEN;
    }

    // Remove file from list
    for(ramfs_file_t *f = directory->firstFile; f; f = f->next)
    {
        if(f == file)
            break;
        if(f->next == file)
        {
            f->next = file->next;
            if(directory->lastFile == file)
                directory->lastFile = f;
            break;
        }
    }
    if(directory->firstFile == file)
    {
        directory->firstFile = file->next;
        if(directory->lastFile == file)
            directory->lastFile = file->next;
    }

    // Free file blocks
    ramfs_file_block_entry_t *currentBlock = file->firstBlock;
    while(currentBlock)
    {
        // Free block data
        free(currentBlock->block);

        // Get address of next block, then free current one
        ramfs_file_block_entry_t *nextBlock = currentBlock->next;
        free(currentBlock);
        currentBlock = nextBlock;
    }

    // Free file info structure
    free(file);

    // Done
    release_lock();
    return RAMFS_ERR_OK;
}

int ramfs_list(const char *path, char *buffer, int bufferLength)
{
    acquire_lock();

    // Get directory info
    ramfs_directory_t *directory;
    if(get_directory(path, strlen(path), &directory) != RAMFS_ERR_OK)
        return 0;

    // Current writing position in buffer
    int bufferPos = 0;
    bool overflow = false;

    // Length of the file size string in front of the files names
    const int fileSizeStringLength = 4 + 1 + 1 + 1 + 2; // e.g. "1014.7 KB"
    char fileSizeString[4 + 1 + 1 + 1 + 2]; // Buffer for building those strings

    // Print directory list
    ramfs_directory_t *currentSubDirectory = directory->firstChild;
    while(currentSubDirectory)
    {
        // Enough space left in buffer?
        int nameLength = strlen(currentSubDirectory->name);
        int lineLength = 1 + fileSizeStringLength + 4 + nameLength + 1 + 1;
        if(bufferPos + lineLength > bufferLength)
        {
            overflow = true;
            break;
        }

        // Spacing
        buffer[bufferPos++] = ' ';
        for(int i = 0; i < fileSizeStringLength; ++i)
            buffer[bufferPos++] = ' ';
        strncpy(&buffer[bufferPos], "    ", 4);
        bufferPos += 4;

        // Output sub directory name
        strncpy(&buffer[bufferPos], currentSubDirectory->name, nameLength);
        bufferPos += nameLength;

        // Slash
        buffer[bufferPos++] = '/';

        // New line
        buffer[bufferPos++] = '\n';

        // Next sub directory
        currentSubDirectory = currentSubDirectory->next;
    }

    // Print file list
    ramfs_file_t *currentFile = directory->firstFile;
    while(currentFile)
    {
        // Calculate file size string
        int fileSizeStringPos = 0;
        char *fileSizeNames[5] = { "B ", "KB", "MB", "GB", "TB" };
        int fileSizeOrders[5];
        fileSizeOrders[0] = (int)(currentFile->dataLength >> 00) & 1023; // B
        fileSizeOrders[1] = (int)(currentFile->dataLength >> 10) & 1023; // KB
        fileSizeOrders[2] = (int)(currentFile->dataLength >> 20) & 1023; // MB
        fileSizeOrders[3] = (int)(currentFile->dataLength >> 30) & 1023; // GB
        fileSizeOrders[4] = (int)(currentFile->dataLength >> 40) & 1023; // TB
        for(int i = 4; i >= 0; --i)
        {
            // Non-zero value in current order of magnitude? -> print
            if(fileSizeOrders[i] != 0 || i == 0)
            {
                // Convert number to string
                itoa(fileSizeOrders[i], &fileSizeString[0], 10);
                fileSizeStringPos = strlen(&fileSizeString[0]);

                // Non-zero value in next smaller order? -> print first decimal digit
                if(i > 0 && fileSizeOrders[i - 1] != 0)
                {
                    // Add decimal point
                    fileSizeString[fileSizeStringPos++] = '.';

                    // Convert number to decimal fraction
                    char nextOrderDecimal[5];
                    itoa((fileSizeOrders[i - 1] * 1000) / 1024, nextOrderDecimal, 10);

                    // Copy first digit
                    fileSizeString[fileSizeStringPos++] = nextOrderDecimal[0];
                }

                // Add unit
                fileSizeString[fileSizeStringPos++] = ' ';
                strncpy(&fileSizeString[fileSizeStringPos], fileSizeNames[i], 2);
                fileSizeStringPos += 2;

                break;
            }
        }

        // Enough space left in buffer?
        int nameLength = strlen(currentFile->name);
        int lineLength = 1 + fileSizeStringLength + 4 + nameLength + 1;
        if(bufferPos + lineLength > bufferLength)
        {
            overflow = true;
            break;
        }

        // Spacing
        buffer[bufferPos++] = ' ';

        // Pad right file size string
        for(int i = 0; i < fileSizeStringLength - fileSizeStringPos; ++i)
            buffer[bufferPos++] = ' ';

        // Copy file size string
        strncpy(&buffer[bufferPos], fileSizeString, fileSizeStringPos);
        bufferPos += fileSizeStringPos;

        // Spacing
        strncpy(&buffer[bufferPos], "    ", 4);
        bufferPos += 4;

        // Copy file name
        strncpy(&buffer[bufferPos], currentFile->name, nameLength);
        bufferPos += nameLength;

        // New line
        buffer[bufferPos++] = '\n';

        // Next file
        currentFile = currentFile->next;
    }

    // If an overflow would have happened, indicate this in the output
    if(overflow)
    {
        if(bufferPos > bufferLength - 3)
            bufferPos = bufferLength - 3;
        buffer[bufferPos++] = '~';
        buffer[bufferPos++] = '~';
        buffer[bufferPos++] = '~';
    }

    // Done
    release_lock();
    return bufferPos;
}