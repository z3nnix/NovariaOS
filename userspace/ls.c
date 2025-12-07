// SPDX-License-Identifier: LGPL-3.0-or-later

#include <core/fs/vfs.h>
#include <core/kernel/kstd.h>

int ls_main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    kprint("\n", 7);
    vfs_list();
    kprint("\n", 7);
    
    return 0;
}
