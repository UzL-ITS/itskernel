#include <stdio.h>
#include "ramfs.h"

char data1[500];
char data2[200];
char data3[600];

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

	printf("Create file 1: %d\n", ramfs_create_file("/a/b/c", "test.txt", data1, sizeof(data1)));
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
	printf("Get file 8: %d\n", ramfs_get_file("/test3.txt", &data, &dataLength));

	ramfs_dump();

	getchar();
}