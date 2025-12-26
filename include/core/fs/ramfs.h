#ifndef RAMFS_H
#define RAMFS_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define MAX_SECTORS 256
#define SECTOR_SIZE 4096

typedef struct {
    char data[SECTOR_SIZE];
    size_t size;
    bool used;
} ramfs_sector_t;

void ramfs_init(void);
int ramfs_write(const char* data, size_t size);
const char* ramfs_read(int sector, size_t* size);
void ramfs_delete(int sector);
size_t ramfs_get_size(int sector);

#endif