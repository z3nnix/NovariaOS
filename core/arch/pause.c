// SPDX-License-Identifier: LGPL-3.0-or-later

void pause() {
    asm volatile ("hlt");
}

unsigned short inw(unsigned short port) {
    unsigned short result;
    asm volatile ("inw %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}