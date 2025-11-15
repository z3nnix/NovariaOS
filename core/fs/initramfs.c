// SPDX-License-Identifier: LGPL-3.0-or-later

#include <core/fs/initramfs.h>
#include <core/fs/ramfs.h>
#include <core/arch/multiboot.h>
#include <core/kernel/mem.h>
#include <stdint.h>
#include <stddef.h>
#include <core/kernel/kstd.h>
#include <core/kernel/mem.h>

#define MAX_PROGRAMS 64

static struct program programs[MAX_PROGRAMS];
static size_t program_count = 0;

void initramfs_load(multiboot_info_t* mb_info) {
    if (!(mb_info->flags & 0x08)) {
        kprint("No modules found in multiboot info\n", 14);
        return;
    }
    
    if (mb_info->mods_count == 0) {
        kprint("No modules to load\n", 14);
        return;
    }
    
    module_t* modules = (module_t*)mb_info->mods_addr;
    module_t* module = &modules[0];
    
    uint8_t *initramfs_data = (uint8_t*)module->mod_start;
    size_t initramfs_size = module->mod_end - module->mod_start;
    
    kprint(":: Loading initramfs..\n", 7);
    
    ramfs_init();
    
    size_t offset = 0;
    program_count = 0;
    
    while (offset < initramfs_size && program_count < MAX_PROGRAMS) {
        if (offset + 4 > initramfs_size) {
            kprint("Incomplete size field in initramfs\n", 14);
            break;
        }
        
        uint32_t prog_size = 0;
        for (int i = 0; i < 4; i++) {
            prog_size = (prog_size << 8) | initramfs_data[offset++];
        }
        
        if (prog_size == 0) {
            kprint("Zero-sized program encountered\n", 14);
            break;
        }
        
        if (prog_size > initramfs_size - offset) {
            kprint("Program size exceeds available data\n", 14);
            break;
        }
        
        programs[program_count].data = (const char*)&initramfs_data[offset];
        programs[program_count].size = prog_size;
        programs[program_count].ramfs_sector = -1;
        
        int sector = ramfs_write((const char*)&initramfs_data[offset], prog_size);
        if (sector >= 0) {
            programs[program_count].ramfs_sector = sector;
            
            kprint(":: Loaded program ", 7);
            
            char buf[16];
            memset(buf, 0, sizeof(buf));
            size_t n = program_count;
            char* p = buf;
            if (n == 0) {
                *p++ = '0';
            } else {
                char* start = p;
                while (n > 0) {
                    *p++ = '0' + n % 10;
                    n /= 10;
                }
                p--;
                while (start < p) {
                    char temp = *start;
                    *start = *p;
                    *p = temp;
                    start++;
                    p--;
                }
                p = buf + 15;
            }
            *p = '\0';
            kprint(buf, 7);
            
            kprint(" (size=", 7);
            
            memset(buf, 0, sizeof(buf));
            n = prog_size;
            p = buf;
            if (n == 0) {
                *p++ = '0';
            } else {
                char* start = p;
                while (n > 0) {
                    *p++ = '0' + n % 10;
                    n /= 10;
                }
                p--;
                while (start < p) {
                    char temp = *start;
                    *start = *p;
                    *p = temp;
                    start++;
                    p--;
                }
                p = buf + 15;
            }
            *p = '\0';
            kprint(buf, 7);
            kprint(", sector=", 7);
            
            memset(buf, 0, sizeof(buf));
            n = sector;
            p = buf;
            if (n == 0) {
                *p++ = '0';
            } else {
                char* start = p;
                while (n > 0) {
                    *p++ = '0' + n % 10;
                    n /= 10;
                }
                p--;
                while (start < p) {
                    char temp = *start;
                    *start = *p;
                    *p = temp;
                    start++;
                    p--;
                }
                p = buf + 15;
            }
            *p = '\0';
            kprint(buf, 7);
            kprint(")\n", 7);
        } else {
            kprint("Failed to load program to RamFS\n", 14);
        }
        
        program_count++;
        offset += prog_size;
        
        if (offset % 4 != 0) {
            offset += 4 - (offset % 4);
        }
    }

    kprint(":: Total programs loaded: ", 7);
    char count_buf[16];
    memset(count_buf, 0, sizeof(count_buf));
    size_t n = program_count;
    char* p = count_buf;
    if (n == 0) {
        *p++ = '0';
    } else {
        char* start = p;
        while (n > 0) {
            *p++ = '0' + n % 10;
            n /= 10;
        }
        p--;
        while (start < p) {
            char temp = *start;
            *start = *p;
            *p = temp;
            start++;
            p--;
        }
        p = count_buf + 15;
    }
    *p = '\0';
    kprint(count_buf, 7);
    kprint("\n", 7);
}

struct program* initramfs_get_program(size_t index) {
    if (index >= program_count) return NULL;
    return &programs[index];
}

size_t initramfs_get_count(void) {
    return program_count;
}

int initramfs_load_to_ramfs(size_t index) {
    if (index >= program_count) return -1;
    
    struct program* prog = &programs[index];
    
    if (prog->ramfs_sector >= 0) {
        return prog->ramfs_sector;
    }

    int sector = ramfs_write(prog->data, prog->size);
    if (sector >= 0) {
        prog->ramfs_sector = sector;
    }
    
    return sector;
}
