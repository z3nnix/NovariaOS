// SPDX-License-Identifier: LGPL-3.0-or-later

void pause() {
    asm volatile ("hlt");
}