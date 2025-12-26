#ifndef _KSTD_H
#define _KSTD_H

#include <stdbool.h>
#include <stddef.h>

void reverse(char* str, int length);
char* itoa(int num, char* str, int base);
char* strncpy(char *dest, const char *src, unsigned long n);
size_t strlen(const char* str);

#endif // _KSTD_H
