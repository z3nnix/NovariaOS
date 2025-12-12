// SPDX-License-Identifier: LGPL-3.0-or-later

#include <core/fs/vfs.h>
#include <core/kernel/kstd.h>
#include <core/kernel/shell.h>

// Helper: compare strings
static int ls_strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

// Helper: string length
static int ls_strlen(const char* s) {
    int len = 0;
    while (s[len]) len++;
    return len;
}

// Helper: compare with length
static int ls_strncmp(const char* s1, const char* s2, int n) {
    while (n > 0 && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

int ls_main(int argc, char** argv) {
    const char* path;
    
    if (argc > 1) {
        path = argv[1];
    } else {
        path = shell_get_cwd();
    }
    
    vfs_file_t* files = vfs_get_files();
    int dir_len = ls_strlen(path);
    
    char normalized_dir[64];
    int i = 0;
    while (path[i] && i < 63) {
        normalized_dir[i] = path[i];
        i++;
    }
    normalized_dir[i] = '\0';
    
    if (dir_len > 1 && normalized_dir[dir_len - 1] == '/') {
        normalized_dir[dir_len - 1] = '\0';
        dir_len--;
    }
    
    int found_count = 0;
    
    for (int i = 0; i < 128; i++) {
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
            int name_len = ls_strlen(name);
            
            if (name_len > dir_len && name[dir_len] == '/') {
                bool match = true;
                for (int j = 0; j < dir_len; j++) {
                    if (name[j] != normalized_dir[j]) {
                        match = false;
                        break;
                    }
                }
                if (match) {
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
        }
        
        if (should_show) {
            found_count++;
            if (files[i].type == VFS_TYPE_DIR) {
                kprint(display_name, 9);
                kprint("/", 9);
            } else {
                kprint(display_name, 7);
            }
            kprint("    ", 7);
        }
    }
    
    if (found_count == 0) {
        kprint("(empty)", 7);
    }

    kprint("\n", 7);
    return 0;
}
