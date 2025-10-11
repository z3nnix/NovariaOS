#ifndef _KSTD_H
#define _KSTD_H

#include <stdbool.h>

void reverse(char* str, int length);
char* itoa(int num, char* str, int base);
void kprint(const char *str, int color);
void strncpy(char *dest, const char *src, unsigned int n);

#endif // _KSTD_H