/*
ITS kernel RAM file system implementation.
Directories are stored as linked list, where each maintains a pointer to the next directory on the same level,
and to its first and last child directories on the next level.
*/

/* INCLUDES */

#include <fs/ramfs.h>
#include <memory.h>
#include <io.h>
#include <string.h>


/* TYPES */

// Represents a file system file entry.
typedef struct ramfs_file_s
{
	// Name of this file.
	char name[256];

	// Pointer to the file contents.
	void *data;

	// Size of the file contents.
	int dataLength;

	// The next file in the same directory.
	struct ramfs_file_s *next;

} ramfs_file_t;

// Represents a file system directory entry.
typedef struct ramfs_directory_s
{
	// Name of this directory.
	char name[256];

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


/* VARIABLES */

// The root directory.
ramfs_directory_t *root;


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

	// Get sub directory where the new directory shall be placed in
	ramfs_directory_t *parentDirectory;
	int pathLength = strlen(path);
	ramfs_err_t err = traverse_tree(path, pathLength, &parentDirectory);
	if(err != RAMFS_ERR_OK)
		return err;

	// Check whether directory exists already
	for(ramfs_directory_t *subDir = parentDirectory->firstChild; subDir; subDir = subDir->next)
		if(strcmp(subDir->name, name) == 0)
			return RAMFS_ERR_DIRECTORY_EXISTS;

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

	// Done
	return RAMFS_ERR_OK;
}

ramfs_err_t ramfs_create_file(const char *path, const char *name, void *data, int dataLength)
{
	// The file name must not contain any reserved characters
	for(const char *n = name; *n; ++n)
		if(*n == '/')
			return RAMFS_ERR_ILLEGAL_CHARACTER;

	// Get sub directory where the new file shall be placed in
	ramfs_directory_t *directory;
	int pathLength = strlen(path);
	ramfs_err_t err = traverse_tree(path, pathLength, &directory);
	if(err != RAMFS_ERR_OK)
		return err;

	// Check whether file exists already
	for(ramfs_file_t *f = directory->firstFile; f; f = f->next)
		if(strcmp(f->name, name) == 0)
			return RAMFS_ERR_FILE_EXISTS;

	// Create file
	ramfs_file_t *file = (ramfs_file_t *)malloc(sizeof(ramfs_file_t));
	strncpy(file->name, name, sizeof(file->name));
	file->data = data;
	file->dataLength = dataLength;
	file->next = 0;

	// Add file to directory
	if(directory->lastFile)
		directory->lastFile->next = file; // Update last file entry of directory
	else
		directory->firstFile = file; // The directory does not have any files yet
	directory->lastFile = file;

	// Done
	return RAMFS_ERR_OK;
}

ramfs_err_t ramfs_get_file(const char *path, void **dataPtr, int *dataLengthPtr)
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
			// Return file contents
			if(dataPtr)
				*dataPtr = file->data;
			if(dataLengthPtr)
				*dataLengthPtr = file->dataLength;
			return RAMFS_ERR_OK;
		}

	// Not found
	return RAMFS_ERR_FILE_DOES_NOT_EXIST;
}

// Helper function to recursively dump a directory.
static void dump_recursive(ramfs_directory_t *directory, int level)
{
	// Print name of directory
	for(int l = 0; l < level; ++l)
		printf("  ");
	printf("%s/\n", directory->name);

	// Print files
	for(ramfs_file_t *file = directory->firstFile; file; file = file->next)
	{
		for(int l = 0; l < level + 1; ++l)
			printf("  ");
		printf("%s (%d bytes)\n", file->name, file->dataLength);
	}

	// Print sub directories
	for(ramfs_directory_t *subDir = directory->firstChild; subDir; subDir = subDir->next)
		dump_recursive(subDir, level + 1);
}

void ramfs_dump()
{
	// Run recursively through directory tree
	dump_recursive(root, 0);
}