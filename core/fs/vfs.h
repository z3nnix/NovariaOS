// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef _VFS_H
#define _VFS_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#define MAX_FILES 256
#define MAX_HANDLES 128
#define MAX_FILENAME 256
#define MAX_FILE_SIZE 65536

#define VFS_READ   0x01
#define VFS_WRITE  0x02
#define VFS_CREAT  0x04
#define VFS_APPEND 0x08

#define VFS_SEEK_SET 0
#define VFS_SEEK_CUR 1
#define VFS_SEEK_END 2

#define VFS_TYPE_FILE 0
#define VFS_TYPE_DIR  1

typedef int64_t off_t;
typedef int64_t ssize_t;

typedef struct {
    char name[MAX_FILENAME];
    char data[MAX_FILE_SIZE];
    size_t size;
    bool used;
    uint8_t type;
} vfs_file_t;

typedef struct {
    int fd;
    vfs_file_t* file;
    size_t position;
    int flags;
    bool used;
} vfs_handle_t;

void vfs_init(void);
int vfs_create(const char* filename, const char* data, size_t size);
const char* vfs_read(const char* filename, size_t* size);
int vfs_open(const char* filename, int flags);
ssize_t vfs_readfd(int fd, void* buf, size_t count);
ssize_t vfs_writefd(int fd, const void* buf, size_t count);
int vfs_close(int fd);
off_t vfs_seek(int fd, off_t offset, int whence);
int vfs_delete(const char* filename);
void vfs_list(void);
bool vfs_exists(const char* filename);
int vfs_count(void);

int vfs_mkdir(const char* dirname);
bool vfs_is_dir(const char* path);
void vfs_list_dir(const char* dirname);
vfs_file_t* vfs_get_files(void);

#endif