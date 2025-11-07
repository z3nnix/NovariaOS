#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include <core/kernel/kstd.h>
#include <stdint.h>

#define MULTIBOOT_FLAG_MODS   0x00000008

typedef struct multiboot_info {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
} multiboot_info_t;

typedef struct module {
    uint32_t mod_start;
    uint32_t mod_end;
    uint32_t string;
    uint32_t reserved;
} module_t;

#endif // MULTIBOOT_H