// SPDX-License-Identifier: LGPL-3.0-or-later

#include <core/kernel/kstd.h>
#include <core/kernel/vge/fb_render.h>

int echo_main(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        kprint(argv[i], 15);
        if (i < argc - 1) {
            kprint(" ", 15);
        }
    }
    
    kprint("\n", 7);
    return 0;
}
