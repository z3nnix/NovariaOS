// SPDX-License-Identifier: LGPL-3.0-or-later

#include <core/fs/vfs.h>
#include <core/kernel/kstd.h>
#include <core/kernel/mem.h>

static vfs_file_t files[MAX_FILES];

// Helper function to compare strings
static int strcmp(const char* str1, const char* str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(unsigned char*)str1 - *(unsigned char*)str2;
}

// Helper function to copy string
static void strcpy(char* dest, const char* src) {
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
}

// Helper function to get string length
static int strlen(const char* str) {
    int len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

void vfs_init(void) {
    for (int i = 0; i < MAX_FILES; i++) {
        files[i].used = false;
        files[i].size = 0;
        files[i].name[0] = '\0';
        files[i].data[0] = '\0';
    }
    kprint(":: VFS initialized\n", 7);
}

int vfs_create(const char* filename, const char* data, size_t size) {
    if (strlen(filename) >= MAX_FILENAME) {
        return -1; // Filename too long
    }
    
    if (size > MAX_FILE_SIZE) {
        return -2; // File too large
    }
    
    // Check if file already exists
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used && strcmp(files[i].name, filename) == 0) {
            // Overwrite existing file
            memcpy(files[i].data, data, size);
            files[i].size = size;
            return i;
        }
    }
    
    // Create new file
    for (int i = 0; i < MAX_FILES; i++) {
        if (!files[i].used) {
            strcpy(files[i].name, filename);
            memcpy(files[i].data, data, size);
            files[i].size = size;
            files[i].used = true;
            return i;
        }
    }
    
    return -3; // No space left
}

const char* vfs_read(const char* filename, size_t* size) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used && strcmp(files[i].name, filename) == 0) {
            if (size) *size = files[i].size;
            return files[i].data;
        }
    }
    
    if (size) *size = 0;
    return NULL;
}

int vfs_delete(const char* filename) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used && strcmp(files[i].name, filename) == 0) {
            files[i].used = false;
            files[i].size = 0;
            files[i].name[0] = '\0';
            files[i].data[0] = '\0';
            return 0;
        }
    }
    
    return -1; // File not found
}

void vfs_list(void) {
    int count = 0;
    
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used) {
            count++;
        }
    }
    
    if (count == 0) {
        return;
    }
    
    char buf[32];
    kprint("Files: ", 7);
    itoa(count, buf, 10);
    kprint(buf, 7);
    kprint("\n", 7);
    
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used) {
            kprint("  ", 7);
            kprint(files[i].name, 11);
            kprint(" (", 7);
            itoa(files[i].size, buf, 10);
            kprint(buf, 7);
            kprint(" bytes)\n", 7);
        }
    }
}

bool vfs_exists(const char* filename) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used && strcmp(files[i].name, filename) == 0) {
            return true;
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
