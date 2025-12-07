// SPDX-License-Identifier: LGPL-3.0-or-later

#include <core/fs/vfs.h>
#include <core/kernel/kstd.h>

int ls_main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    vfs_list();
    
    return 0;
}
