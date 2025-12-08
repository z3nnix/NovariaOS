#ifndef USR_VFS_H
#define USR_VFS_H

#include <stddef.h>
#include <stdint.h>
#include <lib/nc/stdbool.h>

#define MAX_FILES 128
#define MAX_FILENAME 64
#define MAX_FILE_SIZE 4096

typedef enum {
    VFS_TYPE_FILE,
    VFS_TYPE_DIR
} vfs_entry_type_t;

typedef struct {
    char name[MAX_FILENAME];
    char data[MAX_FILE_SIZE];
    size_t size;
    bool used;
    vfs_entry_type_t type;
} vfs_file_t;

void vfs_init(void);
int vfs_create(const char* filename, const char* data, size_t size);
int vfs_mkdir(const char* dirname);
const char* vfs_read(const char* filename, size_t* size);
int vfs_delete(const char* filename);
bool vfs_exists(const char* filename);
bool vfs_is_dir(const char* path);
int vfs_count(void);
void vfs_list_dir(const char* dirname);
vfs_file_t* vfs_get_files(void);

#endif // USR_VFS_H
