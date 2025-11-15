// SPDX-License-Identifier: LGPL-3.0-or-later

#include <core/fs/ramfs.h>
#include <core/kernel/kstd.h>
#include <core/kernel/mem.h>

static ramfs_sector_t sectors[MAX_SECTORS];

void ramfs_init() {
    for(int i = 0; i < MAX_SECTORS; i++) {
        sectors[i].used = false;
        sectors[i].size = 0;
        sectors[i].data[0] = '\0';
    }
    kprint(":: RamFS initialized\n", 7);
}

int ramfs_write(const char* data, size_t size) {
    if (size > SECTOR_SIZE) {
        kprint("Error: Data too large for sector\n", 14);
        return -1;
    }
    
    for(int i = 0; i < MAX_SECTORS; i++) {
        if(!sectors[i].used) {
            memcpy(sectors[i].data, data, size);
            sectors[i].size = size;
            sectors[i].used = true;
            
            return i;
        }
    }
    
    kprint("Error: No free sectors in RamFS\n", 14);
    return -1;
}

const char* ramfs_read(int sector, size_t* size) {
    if(sector < 0 || sector >= MAX_SECTORS || !sectors[sector].used) {
        if (size) *size = 0;
        return NULL;
    }
    
    if (size) *size = sectors[sector].size;
    return sectors[sector].data;
}

void ramfs_delete(int sector) {
    if(sector >= 0 && sector < MAX_SECTORS) {
        sectors[sector].used = false;
        sectors[sector].size = 0;
        sectors[sector].data[0] = '\0';
    }
}

size_t ramfs_get_size(int sector) {
    if(sector < 0 || sector >= MAX_SECTORS || !sectors[sector].used) {
        return 0;
    }
    return sectors[sector].size;
}
