#include <stdio.h>
#include <stdlib.h>
#include "ramfs.h"

uint8_t data1[100000];
uint8_t data2[100000];

int main()
{
	ramfs_init();

	printf("Create directory 1: %d\n", ramfs_create_directory("/", "test"));
	printf("Create directory 2: %d\n", ramfs_create_directory("/test", "subtest1"));
	printf("Create directory 3: %d\n", ramfs_create_directory("/test/", "subtest2"));
	printf("Create directory 4: %d\n", ramfs_create_directory("/test/", "subtest1")); // FAIL
	printf("Create directory 5: %d\n", ramfs_create_directory("/test/subtest1/", "a"));
	printf("Create directory 6: %d\n", ramfs_create_directory("/test/subtest1/a", "b"));
	printf("Create directory 7: %d\n", ramfs_create_directory("/test/subtest1/b", "c")); // FAIL
	printf("Create directory 8: %d\n", ramfs_create_directory("/test/subtest1/a/b", "c"));
	printf("Create directory 9: %d\n", ramfs_create_directory("/a/b/c", "d")); // FAIL
	printf("Create directory 10: %d\n", ramfs_create_directory("/", "a"));
	printf("Create directory 11: %d\n", ramfs_create_directory("/a", "b"));
	printf("Create directory 12: %d\n", ramfs_create_directory("/a/b/", "c"));
	printf("Create directory 13: %d\n", ramfs_create_directory("/", "a2"));
	printf("Create directory 14: %d\n", ramfs_create_directory("/a2", "b"));
	printf("Create directory 15: %d\n", ramfs_create_directory("/a2/b/", "c"));
	printf("Create directory 16: %d\n", ramfs_create_directory("/a2/b/c", "d/e")); // FAIL

    for(int i = 0; i < 100000; ++i)
    {
        data1[i] = i % 256;
        data2[i] = 255 - i % 256;
    }

    ramfs_fd_t fd;
    printf("fopen 1: %d\n", ramfs_open("/test/test.txt", &fd, true));
    printf("fwrite 1: %d\n", ramfs_write(data1, 100, fd));
    printf("fwrite 2: %d\n", ramfs_write(data1 + 100, 9000, fd));
    printf("fwrite 3: %d\n", ramfs_write(data1 + 9100, 900, fd));
    printf("ftell 1: %d\n", ramfs_tell(fd));
    ramfs_seek(-100, RAMFS_SEEK_CURRENT, fd);
    printf("ftell 2: %d\n", ramfs_tell(fd));
    printf("fwrite 4: %d\n", ramfs_write(data2, 5000, fd));

    int fileLen = ramfs_tell(fd);
    printf("ftell 3: %d\n", fileLen);
    ramfs_seek(0, RAMFS_SEEK_START, fd);
    uint8_t *verif = malloc(fileLen);
    printf("fread 1: %d\n", ramfs_read(verif, fileLen, fd));
    for(int i = 0; i < 9900; ++i)
        if(verif[i] != data1[i])
        {
            printf("Violation at 1[%d]\n", i);
            break;
        }
    for(int i = 0; i < 5000; ++i)
        if(verif[9900 + i] != data2[i])
        {
            printf("Violation at 2[%d]\n", i);
            break;
        }

    ramfs_close(fd);

    // Create more dummy files
    uint64_t sizes[4] = { 256, 330 * 1024, 1023ull * 1024 * 1024, 2ull * 1024 * 1024 * 1024 };
    for(int i = 1; i < 4; ++i)
        sizes[i] += sizes[i - 1];
    for(int i = 0; i < 4; ++i)
    {
        printf("Writing file %d...\n", i);
        char filename[100];
        sprintf(filename, "/test/file%d.bin", i);
        ramfs_open(filename, &fd, true);
        uint64_t size = 0;
        while(size < sizes[i])
        {
            int writeSize = min(sizes[i] - size, 65536);
            ramfs_write(data1, writeSize, fd);
            size += writeSize;
        }
        ramfs_close(fd);
    }

    printf("Listing contents of /test:\n");
    char lsBuffer[1000];
    int len = ramfs_list("/test", lsBuffer, sizeof(lsBuffer) - 1);
    lsBuffer[len] = '\0';
    printf("%s\n", lsBuffer);



    printf("Done.");

/*	printf("Create file 1: %d\n", ramfs_create_file("/a/b/c", "test.txt", data1, sizeof(data1)));
	printf("Create file 2: %d\n", ramfs_create_file("/a/b/c", "test2.txt", data2, sizeof(data2)));
	printf("Create file 3: %d\n", ramfs_create_file("/", "a/test3.txt", data3, sizeof(data3))); // FAIL
	printf("Create file 4: %d\n", ramfs_create_file("/", "test3.txt", data3, sizeof(data3)));
	printf("Create file 5: %d\n", ramfs_create_file("/", "test3.txt", data3, sizeof(data3))); // FAIL

	void *data;
	int dataLength;
	printf("Get file 1: %d\n", ramfs_get_file("/a/b/c/test.txt", &data, &dataLength));
	printf("Get file 2: %d\n", ramfs_get_file("/a/b/c/test.txt1", &data, &dataLength)); // FAIL
	printf("Get file 3: %d\n", ramfs_get_file("/a/b/c/1test.txt", &data, &dataLength)); // FAIL
	printf("Get file 4: %d\n", ramfs_get_file("/a/b/c/test2.txt", &data, &dataLength));
	printf("Get file 5: %d\n", ramfs_get_file("bla.txt", &data, &dataLength)); // FAIL
	printf("Get file 6: %d\n", ramfs_get_file("/bla.txt", &data, &dataLength)); // FAIL
	printf("Get file 7: %d\n", ramfs_get_file("test3.txt", &data, &dataLength)); // FAIL
	printf("Get file 8: %d\n", ramfs_get_file("/test3.txt", &data, &dataLength));*/

//	ramfs_dump();

	getchar();
}