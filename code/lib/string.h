#pragma once

/*
ITS kernel standard library string functions.
*/

/* INCLUDES */

#include <stdbool.h>
#include <stdint.h>


/* TYPES */




/* DECLARATIONS */

// Returns the length of the given string.
int strlen(const char *str);

// Reverses the given string.
// Returns str on success.
char *strrev(char *str);

// Converts the given string into an unsigned 64-bit integer.
uint64_t atoi(const char *str);

// Converts the given unsigned integer into a string, which is written into the given buffer (make sure this is long enough!).
// Returns str on success.
char *itoa(uint64_t value, char *str, int base);

// Returns whether the given char is an ASCII digit (0-9).
bool is_digit(char c);

// Compares the given strings and returns 0 if they are equal.
int strcmp(const char *str1, const char *str2);

// Compares the first n characters of the given strings and returns 0 if they are equal.
int strncmp(const char *str1, const char *str2, int n);