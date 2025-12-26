// SPDX-License-Identifier: LGPL-3.0-or-later

#include <core/fs/vfs.h>
#include <core/kernel/kstd.h>
#include <core/kernel/vge/fb_render.h>

// Helper function to get string length
static int write_strlen(const char* str) {
    int len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

int write_main(int argc, char** argv) {
    if (argc < 3) {
        kprint("Usage: write <filename> <content>\n\n", 12);
        return 1;
    }
    
    // Combine all arguments after filename into content
    char content[4096];
    int pos = 0;
    
    for (int i = 2; i < argc && pos < 4095; i++) {
        int len = write_strlen(argv[i]);
        for (int j = 0; j < len && pos < 4095; j++) {
            content[pos++] = argv[i][j];
        }
        if (i < argc - 1 && pos < 4095) {
            content[pos++] = ' ';
        }
    }
    content[pos] = '\0';
    
    int result = vfs_create(argv[1], content, pos);
    
    if (result >= 0) {
        return 0;
    } else if (result == -1) {
        kprint("Error: Filename too long\n\n", 12);
    } else if (result == -2) {
        kprint("Error: File too large\n\n", 12);
    } else {
        kprint("Error: No space left\n\n", 12);
    }
    
    return 1;
}
