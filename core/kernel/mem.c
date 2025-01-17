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

void mm_test() {
    kprint(":: starting memory test...\n\n", 7);
    kprint("Allocating 256 bytes of memory...\n", 7);
    void* ptr1 = allocateMemory(256);
    if (ptr1 != NULL) {
        kprint("Memory allocated successfully\n", 2);
    } else {
        kprint("Memory allocation failed\n", 4);
    }

    kprint("Allocating 512 bytes of memory...\n", 7);
    void* ptr2 = allocateMemory(512);
    if (ptr2 != NULL) {
        kprint("Memory allocated successfully\n", 2);
    } else {
        kprint("Memory allocation failed\n", 4);
    }

    kprint("Freeing 256 bytes of memory...\n", 7);
    freeMemory(ptr1);

    kprint("Freeing 512 bytes of memory...\n", 7);
    freeMemory(ptr2);

    kprint("\n:: Memory manager test completed\n", 7);
}