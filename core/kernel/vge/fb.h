#ifndef _FB_H_
#define _FB_H_

#include <stdint.h>
#include <core/kernel/vge/fb_render.h>

extern void disable_cursor();
extern void enable_cursor();
extern void clear_screen(void);
extern void newline(void);
extern void vgaprint(const char *str, int color);
extern void terminal_set_cursor(int x, int y);
extern void vga_backspace(void);
extern void vga_scroll_up(void);
extern void vga_scroll_down(void);
extern int init_vge_font(void);

#endif