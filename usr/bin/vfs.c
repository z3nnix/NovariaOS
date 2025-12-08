// SPDX-License-Identifier: LGPL-3.0-or-later
// VFS in userspace

#include "vfs.h"
#include <lib/nc/stdlib.h>

static vfs_file_t files[MAX_FILES];

// Helper function to compare strings
static int vfs_strcmp(const char* str1, const char* str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(unsigned char*)str1 - *(unsigned char*)str2;
}

// Helper function to copy string
static void vfs_strcpy(char* dest, const char* src) {
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
}

// Helper function to get string length
static int vfs_strlen(const char* str) {
    int len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

// Helper function to copy memory
static void vfs_memcpy(void* dest, const void* src, size_t n) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    while (n--) {
        *d++ = *s++;
    }
}

static int vfs_strncmp(const char* s1, const char* s2, int n) {
    while (n > 0 && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

void vfs_init(void) {
    for (int i = 0; i < MAX_FILES; i++) {
        files[i].used = false;
        files[i].size = 0;
        files[i].name[0] = '\0';
        files[i].data[0] = '\0';
        files[i].type = VFS_TYPE_FILE;
    }

    vfs_mkdir("/home");
    vfs_mkdir("/tmp");
    vfs_mkdir("/var");
    vfs_mkdir("/var/log");
    vfs_mkdir("/var/cache");
}

int vfs_mkdir(const char* dirname) {
    if (vfs_strlen(dirname) >= MAX_FILENAME) {
        return -1;
    }
    
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used && vfs_strcmp(files[i].name, dirname) == 0) {
            if (files[i].type == VFS_TYPE_DIR) {
                return i;
            }
            return -2;
        }
    }
    
    for (int i = 0; i < MAX_FILES; i++) {
        if (!files[i].used) {
            vfs_strcpy(files[i].name, dirname);
            files[i].size = 0;
            files[i].used = true;
            files[i].type = VFS_TYPE_DIR;
            return i;
        }
    }
    
    return -3;
}

int vfs_create(const char* filename, const char* data, size_t size) {
    if (vfs_strlen(filename) >= MAX_FILENAME) {
        return -1;
    }
    
    if (size > MAX_FILE_SIZE) {
        return -2;
    }
    
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used && vfs_strcmp(files[i].name, filename) == 0) {
            if (files[i].type == VFS_TYPE_DIR) {
                return -4;
            }
            vfs_memcpy(files[i].data, data, size);
            files[i].size = size;
            return i;
        }
    }
    
    for (int i = 0; i < MAX_FILES; i++) {
        if (!files[i].used) {
            vfs_strcpy(files[i].name, filename);
            vfs_memcpy(files[i].data, data, size);
            files[i].size = size;
            files[i].used = true;
            files[i].type = VFS_TYPE_FILE;
            return i;
        }
    }
    
    return -3;
}

const char* vfs_read(const char* filename, size_t* size) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used && vfs_strcmp(files[i].name, filename) == 0) {
            if (size) *size = files[i].size;
            return files[i].data;
        }
    }
    
    if (size) *size = 0;
    return NULL;
}

int vfs_delete(const char* filename) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used && vfs_strcmp(files[i].name, filename) == 0) {
            files[i].used = false;
            files[i].size = 0;
            files[i].name[0] = '\0';
            files[i].data[0] = '\0';
            return 0;
        }
    }
    
    return -1; // File not found
}

bool vfs_exists(const char* filename) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used && vfs_strcmp(files[i].name, filename) == 0) {
            return true;
        }
    }
    return false;
}

bool vfs_is_dir(const char* path) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used && vfs_strcmp(files[i].name, path) == 0) {
            return files[i].type == VFS_TYPE_DIR;
        }
    }
    return false;
}

int vfs_count(void) {
    int count = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used) {
            count++;
        }
    }
    return count;
}

vfs_file_t* vfs_get_files(void) {
    return files;
}

void vfs_list_dir(const char* dirname) {
    int dir_len = vfs_strlen(dirname);
    
    char normalized_dir[MAX_FILENAME];
    vfs_strcpy(normalized_dir, dirname);
    if (dir_len > 1 && normalized_dir[dir_len - 1] == '/') {
        normalized_dir[dir_len - 1] = '\0';
        dir_len--;
    }
    
    for (int i = 0; i < MAX_FILES; i++) {
        if (!files[i].used) continue;
        
        const char* name = files[i].name;
        bool should_show = false;
        const char* display_name = name;
        
        if (dir_len == 1 && normalized_dir[0] == '/') {
            if (name[0] == '/' && name[1] != '\0') {
                int slash_count = 0;
                for (int j = 1; name[j] != '\0'; j++) {
                    if (name[j] == '/') slash_count++;
                }
                if (slash_count == 0) {
                    should_show = true;
                    display_name = name + 1;
                }
            }
        } else {
            int name_len = vfs_strlen(name);
            if (name_len > dir_len && 
                name[dir_len] == '/' &&
                vfs_strncmp(name, normalized_dir, dir_len) == 0) {
                int slash_count = 0;
                for (int j = dir_len + 1; name[j] != '\0'; j++) {
                    if (name[j] == '/') slash_count++;
                }
                if (slash_count == 0) {
                    should_show = true;
                    display_name = name + dir_len + 1;
                }
            }
        }
        
        if (should_show) {
            // Use external print functions since we're in userspace
            // This will be handled by ls command
        }
    }
}
