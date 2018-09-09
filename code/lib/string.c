/*
ITS kernel standard library string functions.
*/

/* INCLUDES */

#include <string.h>



/* VARIABLES */




/* FUNCTIONS */

int strlen(const char *str)
{
	// Run through chars until 0 terminator is found
	const char *c = str;
	while(*c)
		++c;
	return (int)(c - str);
}

char *strrev(char *str)
{
	// Run through string from both sides and swap characters
	char *s1 = str;
	char *s2 = &str[strlen(str) - 1];
	while(s1 < s2)
	{
		// Swap
		char c = *s1;
		*s1 = *s2;
		*s2 = c;
		
		// Next
		++s1;
		--s2;
	}
	return str;
}

char *strchr(char *str, int c)
{
	// Find character
    while(*str != (char)c)
        if(!*str++)
            return 0;
    return str;
}

uint64_t atoi(const char *str)
{
	// Add up digits from left to right
	uint64_t i = 0;
	while(is_digit(*str))
		i = i * 10 + (uint64_t)((*str++) - '0');
	return i;
}

char *itoa(uint64_t value, char *str, int base)
{
	// Only support bases {0, 1} till {0, ..., 9, A, ..., Z}
	if(base <= 1 || base > 36)
		return 0;
	
	// Divide value by base, store remainder in string for each iteration
	// This produces a reversed string representation of value
	int pos = 0;
	do
	{
		// Add remainder to string
		// Use ASCII uppercase letters for digits > 9
		char c = '0' + (value % base);
		if(c > '9')
			c += 7;
		str[pos] = c;
		
		// Next multiple of base
		++pos;
	}
	while((value /= base) > 0);
	
	// Append terminating 0 and reverse resulting string
	str[pos] = '\0';
	strrev(str);
	return str;
}

bool is_digit(char c)
{
	return ('0' <= c && c <= '9');
}

int strcmp(const char *str1, const char *str2)
{
	// Do unsigned comparison
	const unsigned char *s1 = (const unsigned char *)str1;
	const unsigned char *s2 = (const unsigned char *)str2;

	// Compare character-wise; if *s2 == 0, the loop will exit
	while(*s1 && (*s1 == *s2))
	{
		++s1;
		++s2;
	}
	return *s1 - *s2;
}

int strncmp(const char *str1, const char *str2, int n)
{
	// Do unsigned comparison
	const unsigned char *s1 = (const unsigned char *)str1;
	const unsigned char *s2 = (const unsigned char *)str2;
	
	// Compare character-wise; if *s2 == 0, the loop will exit
    while(n && *s1 && (*s1 == *s2))
    {
        ++s1;
        ++s2;
        --n;
    }
	
	// Length >= n and all characters equal?
    if(n == 0)
        return 0;
    else
        return *s1 - *s2;
}

char *strncpy(char *dest, const char *src, int n)
{
	// Copy string
	char *ret = dest;
	do
	{
		if(!n--)
			return ret;
	}
	while((*dest++ = *src++));
	
	// Pad destination string with zeroes, if necessary
	while(n--)
		*dest++ = 0;
	return ret;
}