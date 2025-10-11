#include <core/kernel/mem.h>

extern void ramfs_init();
int ramfs_write(char* data);
char* ramfs_read(int sector); 
void ramfs_delete(int sector);