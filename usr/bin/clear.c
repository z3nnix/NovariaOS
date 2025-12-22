// SPDX-License-Identifier: LGPL-3.0-or-later

#include <core/drivers/fb.h>

int clear_main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    clearscreen();
    return 0;
}
