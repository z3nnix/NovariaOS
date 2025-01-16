// multiboot.h
#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include <stdint.h>

typedef struct multiboot_info {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    // Другие поля...
} multiboot_info_t;

#endif // MULTIBOOT_H