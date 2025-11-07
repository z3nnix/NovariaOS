#include <core/fs/initramfs.h>
#include <core/fs/ramfs.h>
#include <core/arch/multiboot.h>
#include <core/kernel/mem.h>
#include <stdint.h>
#include <stddef.h>
#include <core/kernel/kstd.h>

#define MAX_PROGRAMS 64

static struct program programs[MAX_PROGRAMS];
static size_t program_count = 0;

void initramfs_load(multiboot_info_t* mb_info) {
    if (!(mb_info->flags & 0x08)) {  // MULTIBOOT_INFO_MODS
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
        uint32_t prog_size = 0;
        for (int i = 0; i < 4; i++) {
            prog_size = (prog_size << 8) | initramfs_data[offset++];
        }
        
        if (prog_size == 0 || prog_size > initramfs_size - offset) {
            break;
        }
        
        programs[program_count].data = &initramfs_data[offset];
        programs[program_count].size = prog_size;
        
        if (prog_size <= SECTOR_SIZE) {
            char* sector_data = (char*)allocateMemory(SECTOR_SIZE);
            if (sector_data) {
                memcpy(sector_data, &initramfs_data[offset], prog_size);
                
                int sector = ramfs_write(sector_data);
                
                freeMemory(sector_data);
            }
        }
        
        program_count++;
        offset += prog_size;
    }

    char buf[32];
    memset(buf, 0, sizeof(buf));

    int n = program_count;
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
        p = buf + 32;
    }
    *p = '\0';
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
    
    if (prog->size <= SECTOR_SIZE) {
        char* sector_data = (char*)allocateMemory(SECTOR_SIZE);
        if (!sector_data) return -1;
        
        memcpy(sector_data, prog->data, prog->size);
        int sector = ramfs_write(sector_data);
        freeMemory(sector_data);
        
        return sector;
    }
    
    return -1;
}