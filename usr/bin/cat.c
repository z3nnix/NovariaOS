// SPDX-License-Identifier: LGPL-3.0-or-later

#include "vfs.h"
#include <core/kernel/kstd.h>

int cat_main(int argc, char** argv) {
    if (argc < 2) {
        kprint("Usage: cat <filename>\n\n", 12);
        return 1;
    }
    
    size_t size;
    const char* data = vfs_read(argv[1], &size);
    
    if (data == NULL) {
        kprint("\nError: File '", 12);
        kprint(argv[1], 12);
        kprint("' not found\n", 12);
        return 1;
    }
    
    for (size_t i = 0; i < size; i++) {
        char c[2] = {data[i], '\0'};
        if (data[i] == '\n') {
            kprint("\n", 7);
        } else {
            kprint(c, 15);
        }
    }
    kprint("\n", 7);
    
    return 0;
}
