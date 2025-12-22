// SPDX-License-Identifier: LGPL-3.0-or-later

#include <core/kernel/vge/fb.h>

int clear_main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    clear_screen();
    return 0;
}
