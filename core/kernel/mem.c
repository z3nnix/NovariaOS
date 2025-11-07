#include <core/kernel/mem.h>
#include <stddef.h>
#include <stdint.h>
#include <stdalign.h>
#include <stdbool.h>

typedef struct MemoryBlock {
    uint32_t magic; 
    size_t size; 
    struct MemoryBlock* next;
    uint32_t checksum;
} MemoryBlock;

#define MAGIC_ALLOC      0xABCD1234
#define MAGIC_FREE       0xDCBA5678
#define ALIGNMENT        8
#define MIN_BLOCK_SIZE   (sizeof(MemoryBlock) + ALIGNMENT)

static MemoryBlock* freeList = NULL;
static void* poolStart = NULL;
static size_t poolSizeTotal = 0;

static void panic(const char* message);
static void mergeFreeBlocks();
static bool validateBlock(MemoryBlock* block);
static uint32_t calculateChecksum(MemoryBlock* block);

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
    
    char num_buf[32];
    char* p = num_buf;
    int n = int_part;
    
    if (n == 0) {
        *p++ = '0';
    } else {
        while (n > 0) {
            *p++ = '0' + n % 10;
            n /= 10;
        }
    }
    p--;

    char* buf_ptr = buffer;
    while (p >= num_buf) {
        *buf_ptr++ = *p--;
    }
    *buf_ptr++ = '.';
    *buf_ptr++ = '0' + frac_part;
    *buf_ptr++ = ' ';
    
    const char* unit = units[unit_index];
    while (*unit) {
        *buf_ptr++ = *unit++;
    }
    *buf_ptr = '\0';
}

void initializeMemoryManager(void* memoryPool, size_t poolSize) {
    if (memoryPool == NULL) {
        panic("Memory pool is NULL");
    }
    
    if (poolSize < MIN_BLOCK_SIZE) {
        char buffer[64];
        formatMemorySize(poolSize, buffer);
        kprint("Kernel panic - Memory pool too small (", 4);
        kprint(buffer, 4);
        kprint(")\n", 4);
        panic("Memory pool too small");
    }

    poolStart = memoryPool;
    poolSizeTotal = poolSize;

    freeList = (MemoryBlock*)memoryPool;
    freeList->magic = MAGIC_FREE;
    freeList->size = poolSize - sizeof(MemoryBlock);
    freeList->next = NULL;
    freeList->checksum = calculateChecksum(freeList);

    char buffer[64];
    formatMemorySize(freeList->size, buffer);
    kprint(":: Memory initialized (", 7);
    kprint(buffer, 7);
    kprint(")\n", 7);
}

void* allocateMemory(size_t size) {
    if (size == 0 || size > poolSizeTotal - sizeof(MemoryBlock)) {
        return NULL;
    }
    
    size = (size + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
    
    MemoryBlock* prev = NULL;
    MemoryBlock* curr = freeList;

    while (curr != NULL) {
        if (!validateBlock(curr)) {
            panic("Corrupted block in free list");
            return NULL;
        }

        if (curr->size >= size) {
            if (curr->size >= size + MIN_BLOCK_SIZE) {
                MemoryBlock* newBlock = (MemoryBlock*)((char*)curr + sizeof(MemoryBlock) + size);
                
                newBlock->magic = MAGIC_FREE;
                newBlock->size = curr->size - size - sizeof(MemoryBlock);
                newBlock->next = curr->next;
                newBlock->checksum = calculateChecksum(newBlock);
                
                curr->size = size;
                curr->next = newBlock;
            }

            if (prev) prev->next = curr->next;
            else freeList = curr->next;
            
            curr->magic = MAGIC_ALLOC;
            curr->checksum = calculateChecksum(curr);
            return (void*)((char*)curr + sizeof(MemoryBlock));
        }
        
        prev = curr;
        curr = curr->next;
    }
    return NULL;
}

void freeMemory(void* ptr) {
    if (ptr == NULL) return;
    
    MemoryBlock* block = (MemoryBlock*)((char*)ptr - sizeof(MemoryBlock));
    
    if ((char*)block < (char*)poolStart || 
        (char*)block + block->size + sizeof(MemoryBlock) > (char*)poolStart + poolSizeTotal) {
        panic("Invalid memory address in free");
    }
    
    if (!validateBlock(block) || block->magic != MAGIC_ALLOC) {
        panic("Double free or corrupted block");
    }
    
    for (MemoryBlock* curr = freeList; curr; curr = curr->next) {
        if (curr == block) {
            panic("Double free detected");
            return;
        }
    }

    block->magic = MAGIC_FREE;
    block->next = freeList;
    block->checksum = calculateChecksum(block);
    freeList = block;
    
    mergeFreeBlocks();
}

static void mergeFreeBlocks() {
    MemoryBlock* curr = freeList;
    while (curr && curr->next) {
        if (!validateBlock(curr) || !validateBlock(curr->next)) {
            panic("Corrupted block during merge");
        }
        
        MemoryBlock* next = curr->next;
        if ((char*)curr + sizeof(MemoryBlock) + curr->size == (char*)next) {
            curr->size += sizeof(MemoryBlock) + next->size;
            curr->next = next->next;
            curr->checksum = calculateChecksum(curr);
        } else {
            curr = curr->next;
        }
    }
}

static bool validateBlock(MemoryBlock* block) {
    if (block == NULL) return false;
    
    if ((char*)block < (char*)poolStart || 
        (char*)block + sizeof(MemoryBlock) > (char*)poolStart + poolSizeTotal) {
        return false;
    }
    
    if (block->magic != MAGIC_ALLOC && block->magic != MAGIC_FREE) {
        return false;
    }

    return block->checksum == calculateChecksum(block);
}

static uint32_t calculateChecksum(MemoryBlock* block) {
    uint32_t sum = 0;
    uint8_t* bytes = (uint8_t*)block;
    for (size_t i = 0; i < sizeof(MemoryBlock) - sizeof(uint32_t); i++) {
        sum = (sum << 3) ^ bytes[i];
    }
    return sum;
}

static void panic(const char* message) {
    kprint("KERNEL PANIC: ", 4);
    kprint(message, 4);
    kprint("\n", 4);
    
    pause();
}

void mm_test() {
    kprint(":: Starting memory test...\n\n", 7);
    
    kprint("Allocating 256 bytes...\n", 7);
    void* ptr1 = allocateMemory(256);
    if (ptr1 != NULL) {
        kprint("Allocation 1 OK\n", 2);
        freeMemory(ptr1);
        kprint("Free 1 OK\n", 2);
    } else {
        panic("Allocation 1 failed");
    }

    kprint("\nAllocating 512 bytes...\n", 7);
    void* ptr2 = allocateMemory(512);
    void* ptr3 = allocateMemory(512);
    if (ptr2 && ptr3) {
        kprint("Allocation 2-3 OK\n", 2);
        freeMemory(ptr2);
        freeMemory(ptr3);
        kprint("Free 2-3 OK\n", 2);
    } else {
        panic("Allocation 2-3 failed");
    }

    kprint("\nTesting edge cases...\n", 7);
    void* ptr4 = allocateMemory(poolSizeTotal - sizeof(MemoryBlock) - 1);
    if (ptr4) {
        kprint("Large allocation OK\n", 2);
        freeMemory(ptr4);
    }

    kprint("\n:: Memory test completed\n", 7);
}

void* memcpy(void* dest, const void* src, size_t n) {
    char* d = (char*)dest;
    const char* s = (const char*)src;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dest;
}

void* memset(void* s, int c, size_t n) {
    char* p = (char*)s;
    for (size_t i = 0; i < n; i++) {
        p[i] = (char)c;
    }
    return s;
}