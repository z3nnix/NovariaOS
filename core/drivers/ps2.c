#include <core/drivers/keymap.h>

extern char inb(int port);
char __getch() {
    char symbol;
    while (1) {
        if ((inb(0x64) & 0x01) == 1) {  // Check for input
            symbol = inb(0x60);  // Read the character
            return symbol;
        }
    }
}