#include <core/fs/ramfs.h>

char sectors[MAX_SECTORS][SECTOR_SIZE];
bool used[MAX_SECTORS];
int next_free = 0;

void ramfs_init() {
    for(int i = 0; i < MAX_SECTORS; i++) {
        used[i] = false;
    }
    kprint(":: RamFS initialized\n", 7);
}

int ramfs_write(char* data) {
    for(int i = 0; i < MAX_SECTORS; i++) {
        if(!used[i]) {
            strncpy(sectors[i], data, SECTOR_SIZE-1);
            sectors[i][SECTOR_SIZE-1] = '\0';
            used[i] = true;
            return i;
        }
    }
    return -1;
}

char* ramfs_read(int sector) {
    if(sector < 0 || sector >= MAX_SECTORS || !used[sector]) {
        return NULL;
    }
    return sectors[sector];
}

void ramfs_delete(int sector) {
    if(sector >= 0 && sector < MAX_SECTORS) {
        used[sector] = false;
        sectors[sector][0] = '\0';
    }
}