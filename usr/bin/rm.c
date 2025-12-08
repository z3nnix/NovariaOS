// SPDX-License-Identifier: LGPL-3.0-or-later

#include "vfs.h"
#include <core/kernel/kstd.h>

int rm_main(int argc, char** argv) {
    if (argc < 2) {
        kprint("\nUsage: rm <filename>\n\n", 12);
        return 1;
    }
    
    int result = vfs_delete(argv[1]);
    
    if (result == 0) {
        kprint("\nFile '", 7);
        kprint(argv[1], 11);
        kprint("' deleted successfully\n\n", 7);
        return 0;
    } else {
        kprint("\nError: File '", 12);
        kprint(argv[1], 12);
        kprint("' not found\n\n", 12);
        return 1;
    }
}
