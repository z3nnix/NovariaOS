#include <stddef.h>
#include <stdint.h>

typedef struct MemoryBlock {
    size_t size;
    struct MemoryBlock* next;
} MemoryBlock;

static MemoryBlock* freeList = NULL;

typedef struct multiboot_info {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
} multiboot_info_t;

void formatMemorySize(size_t size, char* buffer) {
    const char* units[] = {"B", "KB", "MB", "GB"};
    int unit_index = 0;
    double formatted_size = (double)size;

    while (formatted_size >= 1024 && unit_index < 3) {
        formatted_size /= 1024;
        unit_index++;
    }

    int int_part = (int)formatted_size;
    int frac_part = (int)((formatted_size - int_part) * 10);
    itoa(int_part, buffer, 10);
    int len = 0;
    while (buffer[len] != '\0') len++;
    buffer[len] = '.';
    buffer[len + 1] = '0' + frac_part;
    buffer[len + 2] = ' ';
    buffer[len + 3] = units[unit_index][0];
    if (units[unit_index][1] != '\0') {
        buffer[len + 4] = units[unit_index][1];
        buffer[len + 5] = '\0';
    } else {
        buffer[len + 4] = '\0';
    }
}

void initializeMemoryManager(void* memoryPool, size_t poolSize) {
    freeList = (MemoryBlock*)memoryPool;
    freeList->size = poolSize - sizeof(MemoryBlock);
    freeList->next = NULL;

    char buffer[64];
    formatMemorySize(freeList->size, buffer);
    kprint(":: memory detected (", 7);
    kprint(buffer, 7);
    kprint(")\n", 7);
}

void* allocateMemory(size_t size) {
    MemoryBlock* current = freeList;
    MemoryBlock* previous = NULL;

    while (current != NULL) {
        if (current->size >= size) {
            if (current->size >= size + sizeof(MemoryBlock)) {
                MemoryBlock* newBlock = (MemoryBlock*)((char*)current + sizeof(MemoryBlock) + size);
                newBlock->size = current->size - size - sizeof(MemoryBlock);
                newBlock->next = current->next;
                current->size = size;
                current->next = newBlock;
            }
            if (previous != NULL) {
                previous->next = current->next;
            } else {
                freeList = current->next;
            }
            return (void*)((char*)current + sizeof(MemoryBlock));
        }
        previous = current;
        current = current->next;
    }
    return NULL;
}

void freeMemory(void* ptr) {
    if (ptr == NULL) {
        return;
    }
    MemoryBlock* block = (MemoryBlock*)((char*)ptr - sizeof(MemoryBlock));
    block->next = freeList;
    freeList = block;
}

void stressTestMemoryManager() {
    void* allocations[1000];
    
    for (int i = 0; i < 1000; i++) {
        size_t size = (i % 256) + 1;
        allocations[i] = allocateMemory(size);
        if (allocations[i] == NULL) {
            kprint("Memory allocation failed\n", 15);
        }
    }

    for (int i = 0; i < 1000; i++) {
        if (allocations[i] != NULL) {
            freeMemory(allocations[i]);
        }
    }

    for (int i = 0; i < 1000; i++) {
        size_t size = ((i + 128) % 256) + 1;
        allocations[i] = allocateMemory(size);
        if (allocations[i] == NULL) {
            kprint("Memory allocation failed\n", 15);
        }
    }

    for (int i = 0; i < 1000; i++) {
        if (allocations[i] != NULL) {
            freeMemory(allocations[i]);
        }
    }

    kprint(":: memory stress test completed\n", 7);
}