#ifndef INITRAMFS_H
#define INITRAMFS_H

#include <stdint.h>
#include <stddef.h>
#include <core/arch/multiboot.h>

struct program {
    const char* data;
    size_t size;
    int ramfs_sector; 
};

void initramfs_load(multiboot_info_t* mb_info);
struct program* initramfs_get_program(size_t index);
size_t initramfs_get_count(void);
int initramfs_load_to_ramfs(size_t index);
void initramfs_list_programs(void);

#endif