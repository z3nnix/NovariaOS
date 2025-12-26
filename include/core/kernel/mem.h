#ifndef MEM_H
#define MEM_H

#include <stddef.h>

#include <core/kernel/kstd.h>
#include <core/drivers/serial.h>

extern void initializeMemoryManager(void);
extern void* memcpy(void* dest, const void* src, size_t n);
extern void* memset(void* s, int c, size_t n);
extern void* allocateMemory(size_t size);
extern void freeMemory(void* ptr);
extern void mm_test();
extern size_t getMemTotal(void);
extern size_t getMemFree(void);
extern size_t getMemAvailable(void);

// Aliases for convenience
#define kmalloc allocateMemory
#define kfree freeMemory

#endif
