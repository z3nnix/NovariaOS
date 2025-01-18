#ifndef MEM_H
#define MEM_H

#include <stddef.h>
#include <core/arch/multiboot.h>
#include <core/kernel/kstd.h>

extern void initializeMemoryManager(void* memoryPool, size_t size);
extern void* allocateMemory(size_t size);
extern void freeMemory(void* ptr);
void mm_test();

#endif