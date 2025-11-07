#include <core/kernel/mem.h>

#define MAX_SECTORS 32
#define SECTOR_SIZE 512

extern void ramfs_init();
int ramfs_write(char* data);
char* ramfs_read(int sector); 
void ramfs_delete(int sector);