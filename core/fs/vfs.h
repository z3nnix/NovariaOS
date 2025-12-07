#ifndef VFS_H
#define VFS_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define MAX_FILES 64
#define MAX_FILENAME 32
#define MAX_FILE_SIZE 4096

typedef struct {
    char name[MAX_FILENAME];
    char data[MAX_FILE_SIZE];
    size_t size;
    bool used;
} vfs_file_t;

// Initialize the VFS
void vfs_init(void);

// Create or overwrite a file
int vfs_create(const char* filename, const char* data, size_t size);

// Read a file
const char* vfs_read(const char* filename, size_t* size);

// Delete a file
int vfs_delete(const char* filename);

// List all files
void vfs_list(void);

// Check if file exists
bool vfs_exists(const char* filename);

// Get file count
int vfs_count(void);

#endif // VFS_H
