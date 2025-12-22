// SPDX-License-Identifier: LGPL-3.0-or-later

#include <core/kernel/kstd.h>
#include <lib/bootloader/limine.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

volatile struct limine_framebuffer_request fb_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0,
    .response = NULL
};

static struct {
    struct limine_framebuffer *fb;
    uint32_t *fb_addr;
    uint64_t width;
    uint64_t height;
    uint64_t pitch;
    uint64_t pitch_pixels;
    uint32_t bg_color;
    uint32_t fg_color;
    uint32_t cursor_x;
    uint32_t cursor_y;
    uint32_t char_width;
    uint32_t char_height;
    bool initialized;
} fb_info = {0};

// Simple 8x16 font
static const uint8_t font[128][16] = {
    [0 ... 31] = {0},
    
    [32] = {0},
    
    // !"#$%&'
    [33] = {0x18, 0x3C, 0x3C, 0x18, 0x18, 0x00, 0x18, 0x00},
    
    // Numbers 0-9
    [48] = {0x3C, 0x66, 0x6E, 0x76, 0x66, 0x66, 0x3C, 0x00}, // 0
    [49] = {0x18, 0x38, 0x18, 0x18, 0x18, 0x18, 0x7E, 0x00}, // 1
    [50] = {0x3C, 0x66, 0x06, 0x0C, 0x18, 0x30, 0x7E, 0x00}, // 2
    [51] = {0x3C, 0x66, 0x06, 0x1C, 0x06, 0x66, 0x3C, 0x00}, // 3
    [52] = {0x0C, 0x1C, 0x3C, 0x6C, 0x7E, 0x0C, 0x0C, 0x00}, // 4
    [53] = {0x7E, 0x60, 0x7C, 0x06, 0x06, 0x66, 0x3C, 0x00}, // 5
    [54] = {0x3C, 0x66, 0x60, 0x7C, 0x66, 0x66, 0x3C, 0x00}, // 6
    [55] = {0x7E, 0x06, 0x0C, 0x18, 0x30, 0x30, 0x30, 0x00}, // 7
    [56] = {0x3C, 0x66, 0x66, 0x3C, 0x66, 0x66, 0x3C, 0x00}, // 8
    [57] = {0x3C, 0x66, 0x66, 0x3E, 0x06, 0x66, 0x3C, 0x00}, // 9
    
    // letters A-Z
    [65] = {0x3C, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00}, // A
    [66] = {0x7C, 0x66, 0x66, 0x7C, 0x66, 0x66, 0x7C, 0x00}, // B
    [67] = {0x3C, 0x66, 0x60, 0x60, 0x60, 0x66, 0x3C, 0x00}, // C
    [68] = {0x78, 0x6C, 0x66, 0x66, 0x66, 0x6C, 0x78, 0x00}, // D
    [69] = {0x7E, 0x60, 0x60, 0x78, 0x60, 0x60, 0x7E, 0x00}, // E
    [70] = {0x7E, 0x60, 0x60, 0x78, 0x60, 0x60, 0x60, 0x00}, // F
    [71] = {0x3C, 0x66, 0x60, 0x6E, 0x66, 0x66, 0x3C, 0x00}, // G
    [72] = {0x66, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00}, // H
    [73] = {0x3C, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00}, // I
    [74] = {0x1E, 0x0C, 0x0C, 0x0C, 0x0C, 0x6C, 0x38, 0x00}, // J
    [75] = {0x66, 0x6C, 0x78, 0x70, 0x78, 0x6C, 0x66, 0x00}, // K
    [76] = {0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x7E, 0x00}, // L
    [77] = {0x63, 0x77, 0x7F, 0x6B, 0x63, 0x63, 0x63, 0x00}, // M
    [78] = {0x66, 0x76, 0x7E, 0x7E, 0x6E, 0x66, 0x66, 0x00}, // N
    [79] = {0x3C, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00}, // O
    [80] = {0x7C, 0x66, 0x66, 0x7C, 0x60, 0x60, 0x60, 0x00}, // P
    [81] = {0x3C, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x0E, 0x00}, // Q
    [82] = {0x7C, 0x66, 0x66, 0x7C, 0x78, 0x6C, 0x66, 0x00}, // R
    [83] = {0x3C, 0x66, 0x60, 0x3C, 0x06, 0x66, 0x3C, 0x00}, // S
    [84] = {0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00}, // T
    [85] = {0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00}, // U
    [86] = {0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x18, 0x00}, // V
    [87] = {0x63, 0x63, 0x63, 0x6B, 0x7F, 0x77, 0x63, 0x00}, // W
    [88] = {0x66, 0x66, 0x3C, 0x18, 0x3C, 0x66, 0x66, 0x00}, // X
    [89] = {0x66, 0x66, 0x66, 0x3C, 0x18, 0x18, 0x18, 0x00}, // Y
    [90] = {0x7E, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x7E, 0x00}, // Z
    
    // a-z
    [97] = {0x00, 0x00, 0x3C, 0x06, 0x3E, 0x66, 0x3E, 0x00}, // a
    [98] = {0x60, 0x60, 0x7C, 0x66, 0x66, 0x66, 0x7C, 0x00}, // b
    [99] = {0x00, 0x00, 0x3C, 0x66, 0x60, 0x66, 0x3C, 0x00}, // c
    [100] = {0x06, 0x06, 0x3E, 0x66, 0x66, 0x66, 0x3E, 0x00}, // d
    [101] = {0x00, 0x00, 0x3C, 0x66, 0x7E, 0x60, 0x3C, 0x00}, // e
    [102] = {0x1C, 0x30, 0x7C, 0x30, 0x30, 0x30, 0x30, 0x00}, // f
    [103] = {0x00, 0x00, 0x3E, 0x66, 0x66, 0x3E, 0x06, 0x3C}, // g
    [104] = {0x60, 0x60, 0x7C, 0x66, 0x66, 0x66, 0x66, 0x00}, // h
    [105] = {0x18, 0x00, 0x38, 0x18, 0x18, 0x18, 0x3C, 0x00}, // i
    [106] = {0x0C, 0x00, 0x0C, 0x0C, 0x0C, 0x0C, 0x6C, 0x38}, // j
    [107] = {0x60, 0x60, 0x66, 0x6C, 0x78, 0x6C, 0x66, 0x00}, // k
    [108] = {0x38, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00}, // l
    [109] = {0x00, 0x00, 0x76, 0x7F, 0x6B, 0x63, 0x63, 0x00}, // m
    [110] = {0x00, 0x00, 0x7C, 0x66, 0x66, 0x66, 0x66, 0x00}, // n
    [111] = {0x00, 0x00, 0x3C, 0x66, 0x66, 0x66, 0x3C, 0x00}, // o
    [112] = {0x00, 0x00, 0x7C, 0x66, 0x66, 0x7C, 0x60, 0x60}, // p
    [113] = {0x00, 0x00, 0x3E, 0x66, 0x66, 0x3E, 0x06, 0x06}, // q
    [114] = {0x00, 0x00, 0x7C, 0x66, 0x60, 0x60, 0x60, 0x00}, // r
    [115] = {0x00, 0x00, 0x3E, 0x60, 0x3C, 0x06, 0x7C, 0x00}, // s
    [116] = {0x30, 0x30, 0x7C, 0x30, 0x30, 0x30, 0x1C, 0x00}, // t
    [117] = {0x00, 0x00, 0x66, 0x66, 0x66, 0x66, 0x3E, 0x00}, // u
    [118] = {0x00, 0x00, 0x66, 0x66, 0x66, 0x3C, 0x18, 0x00}, // v
    [119] = {0x00, 0x00, 0x63, 0x6B, 0x7F, 0x36, 0x22, 0x00}, // w
    [120] = {0x00, 0x00, 0x66, 0x3C, 0x18, 0x3C, 0x66, 0x00}, // x
    [121] = {0x00, 0x00, 0x66, 0x66, 0x66, 0x3E, 0x06, 0x3C}, // y
    [122] = {0x00, 0x00, 0x7E, 0x0C, 0x18, 0x30, 0x7E, 0x00}, // z
    
    [46] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00}, // .
    [44] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x30}, // ,
    [58] = {0x00, 0x18, 0x18, 0x00, 0x00, 0x18, 0x18, 0x00}, // :
    [59] = {0x00, 0x18, 0x18, 0x00, 0x00, 0x18, 0x18, 0x30}, // ;
    [63] = {0x3C, 0x66, 0x0C, 0x18, 0x18, 0x00, 0x18, 0x00}, // ?
};

void init_fb(void) {
    if (fb_info.initialized) return;
    
    if (fb_request.response == NULL || 
        fb_request.response->framebuffer_count == 0) {
        return;
    }
    
    fb_info.fb = fb_request.response->framebuffers[0];
    fb_info.fb_addr = (uint32_t*)fb_info.fb->address;
    fb_info.width = fb_info.fb->width;
    fb_info.height = fb_info.fb->height;
    fb_info.pitch = fb_info.fb->pitch;
    fb_info.pitch_pixels = fb_info.pitch / 4;
    
    fb_info.char_width = 8;
    fb_info.char_height = 16;
    
    fb_info.bg_color = 0x00000000; // Черный
    fb_info.fg_color = 0x00FFFFFF; // Белый
    
    fb_info.cursor_x = 0;
    fb_info.cursor_y = 0;
    
    fb_info.initialized = true;
    
    clear_screen();
}

static void put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (x >= fb_info.width || y >= fb_info.height) return;
    
    uint32_t *pixel = &fb_info.fb_addr[y * fb_info.pitch_pixels + x];
    *pixel = color;
}

static void draw_char(uint32_t x, uint32_t y, char c, uint32_t color) {
    if (c < 0 || c >= 128) return;
    
    const uint8_t *glyph = font[(int)c];
    
    for (uint32_t row = 0; row < 8; row++) {
        uint8_t row_data = glyph[row];
        for (uint32_t col = 0; col < 8; col++) {
            if (row_data & (1 << (7 - col))) {
                put_pixel(x + col, y + row, color);
            }
        }
    }
}

void clear_screen(void) {
    init_fb();
    
    for (uint32_t y = 0; y < fb_info.height; y++) {
        for (uint32_t x = 0; x < fb_info.width; x++) {
            put_pixel(x, y, fb_info.bg_color);
        }
    }
    
    fb_info.cursor_x = 0;
    fb_info.cursor_y = 0;
}

void newline(void) {
    init_fb();
    
    fb_info.cursor_x = 0;
    fb_info.cursor_y += fb_info.char_height;
    
    if (fb_info.cursor_y + fb_info.char_height > fb_info.height) {
        uint32_t scroll_lines = fb_info.char_height;
        uint32_t *src = &fb_info.fb_addr[scroll_lines * fb_info.pitch_pixels];
        uint32_t *dst = fb_info.fb_addr;
        uint64_t size = (fb_info.height - scroll_lines) * fb_info.pitch_pixels * 4;

        for (uint64_t i = 0; i < size / 4; i++) {
            dst[i] = src[i];
        }

        uint32_t last_line_start = (fb_info.height - scroll_lines) * fb_info.pitch_pixels;
        for (uint64_t i = 0; i < scroll_lines * fb_info.pitch_pixels; i++) {
            fb_info.fb_addr[last_line_start + i] = fb_info.bg_color;
        }
        
        fb_info.cursor_y = fb_info.height - fb_info.char_height;
    }
}

void putchar(char c, int color) {
    init_fb();
    
    if (c == '\n') {
        newline();
        return;
    }
    
    if (c == '\t') {
        fb_info.cursor_x += fb_info.char_width * 4;
        return;
    }
    
    if (fb_info.cursor_x + fb_info.char_width > fb_info.width) {
        newline();
    }

    draw_char(fb_info.cursor_x, fb_info.cursor_y, c, color);

    fb_info.cursor_x += fb_info.char_width;
}

void vgaprint(const char *str, int color) {
    init_fb();
    
    uint32_t fb_color;

    switch (color & 0xF) {
        case 0: fb_color = 0x00000000; break;
        case 1: fb_color = 0x000000AA; break;
        case 2: fb_color = 0x0000AA00; break;
        case 3: fb_color = 0x0000AAAA; break; 
        case 4: fb_color = 0x00AA0000; break;
        case 5: fb_color = 0x00AA00AA; break;
        case 6: fb_color = 0x00AA5500; break;
        case 7: fb_color = 0x00AAAAAA; break; 
        case 8: fb_color = 0x00555555; break; 
        case 9: fb_color = 0x005555FF; break; 
        case 10: fb_color = 0x0055FF55; break;
        case 11: fb_color = 0x0055FFFF; break;
        case 12: fb_color = 0x00FF5555; break;
        case 13: fb_color = 0x00FF55FF; break;
        case 14: fb_color = 0x00FFFF55; break;
        case 15: fb_color = 0x00FFFFFF; break;
        default: fb_color = 0x00FFFFFF; break;
    }
    
    while (*str) {
        putchar(*str, fb_color);
        str++;
    }
}

void kprint(const char *str, int color) {
    vgaprint(str, color);
}

void set_bg_color(uint32_t color) {
    fb_info.bg_color = color;
}

void set_fg_color(uint32_t color) {
    fb_info.fg_color = color;
}

void set_cursor_pos(uint32_t x, uint32_t y) {
    init_fb();
    
    fb_info.cursor_x = x * fb_info.char_width;
    fb_info.cursor_y = y * fb_info.char_height;
    
    if (fb_info.cursor_x >= fb_info.width) fb_info.cursor_x = 0;
    if (fb_info.cursor_y >= fb_info.height) fb_info.cursor_y = 0;
}

uint32_t get_screen_width_chars(void) {
    init_fb();
    return fb_info.width / fb_info.char_width;
}

uint32_t get_screen_height_chars(void) {
    init_fb();
    return fb_info.height / fb_info.char_height;
}

void draw_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color) {
    init_fb();
    
    for (uint32_t dy = 0; dy < height; dy++) {
        for (uint32_t dx = 0; dx < width; dx++) {
            put_pixel(x + dx, y + dy, color);
        }
    }
}

void draw_line(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t color) {
    init_fb();
    
    int dx = (x2 > x1) ? (x2 - x1) : (x1 - x2);
    int dy = (y2 > y1) ? (y2 - y1) : (y1 - y2);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    
    while (1) {
        put_pixel(x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}

void reverse(char* str, int length) {
    int start = 0;
    int end = length - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

char* strncpy(char *dest, const char *src, unsigned int n) {
    unsigned int i = 0;
    while (i < n && src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    while (i < n) {
        dest[i] = '\0';
        i++;
    }
    return dest;
}

char* itoa(int num, char* str, int base) {
    int i = 0;
    bool is_negative = false;

    // Handle 0 explicitly, since it is a special case
    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }

    // Handle negative numbers only for base 10
    if (num < 0 && base == 10) {
        is_negative = true;
        num = -num;
    }

    // Process individual digits
    while (num != 0) {
        int rem = num % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0'; // Convert to character
        num /= base;
    }

    // If the number is negative, append '-'
    if (is_negative) {
        str[i++] = '-';
    }

    str[i] = '\0'; // Null-terminate the string

    // Reverse the string
    reverse(str, i);

    return str;
}