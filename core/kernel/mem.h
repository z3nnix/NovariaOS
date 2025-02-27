#ifndef MEM_H
#define MEM_H

#include <stddef.h>
#include <core/arch/multiboot.h>
#include <core/arch/pause.h>
#include <core/kernel/kstd.h>
#include <core/drivers/serial.h>

extern void initializeMemoryManager(void* memoryPool, size_t size);
extern void* allocateMemory(size_t size);
extern void freeMemory(void* ptr);
extern void mm_test();
extern void pause();

#endif