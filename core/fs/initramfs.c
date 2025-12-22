// SPDX-License-Identifier: LGPL-3.0-or-later

#include <core/fs/initramfs.h>
#include <core/fs/ramfs.h>

#include <core/kernel/mem.h>
#include <stdint.h>
#include <stddef.h>
#include <core/kernel/kstd.h>
#include <core/kernel/mem.h>

#define MAX_PROGRAMS 64

static struct program programs[MAX_PROGRAMS];
static size_t program_count = 0;

void initramfs_load(struct limine_module_request* module_request) {
    initramfs_load_limine(module_request);
}

void initramfs_load_limine(volatile struct limine_module_request* module_request) {
    if (!module_request->response || module_request->response->module_count == 0) {
        kprint("No modules found in Limine info\n", 14);
        return;
    }

    // Find initramfs module (skip ISO if present)
    struct limine_file* initramfs_module = NULL;
    for (uint64_t i = 0; i < module_request->response->module_count; i++) {
        struct limine_file* module = module_request->response->modules[i];
        // Skip modules that are too small to be initramfs
        if (module->size > 1024) {  // Minimum reasonable size
            initramfs_module = module;
            break;
        }
    }

    if (!initramfs_module) {
        kprint("No suitable initramfs module found\n", 14);
        return;
    }

    int8_t *initramfs_data = (int8_t*)initramfs_module->address;
    size_t initramfs_size = initramfs_module->size;
    
    kprint(":: Loading initramfs..\n", 7);
    
    size_t offset = 0;
    program_count = 0;
    
    while (offset < initramfs_size && program_count < MAX_PROGRAMS) {
        if (offset + 4 > initramfs_size) {
            kprint("Incomplete size field in initramfs\n", 14);
            break;
        }
        
        int32_t prog_size = 0;
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
