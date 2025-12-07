#ifndef _VGA_H_
#define _VGA_H_

#include <stdint.h>

extern void disable_cursor();
extern void enable_cursor();
extern void clearscreen(void);
extern void newline(void);
extern void vgaprint(const char *str, int color);
extern void terminal_set_cursor(int x, int y);
extern void vga_backspace(void);

#endif