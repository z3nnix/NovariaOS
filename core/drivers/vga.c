// SPDX-License-Identifier: LGPL-3.0-or-later

#include <core/drivers/vga.h>

extern uint8_t inb(uint16_t port);
extern void outb(uint16_t port, uint8_t val);

#define LINES 25
#define COLUMNS_IN_LINE 80
#define BYTES_FOR_EACH_ELEMENT 2
#define SCREENSIZE (BYTES_FOR_EACH_ELEMENT * COLUMNS_IN_LINE * LINES)
#define SCROLL_BUFFER_LINES 100
#define SCROLL_BUFFER_SIZE (BYTES_FOR_EACH_ELEMENT * COLUMNS_IN_LINE * SCROLL_BUFFER_LINES)

unsigned int current_loc = 0;
char *video = (char*)0xb8000;

static char scroll_buffer[SCROLL_BUFFER_SIZE];
static int scroll_buffer_pos = 0;
static int scroll_offset = 0;
static char saved_screen[SCREENSIZE];

void terminal_set_cursor(int x, int y) {
    uint16_t pos = y * COLUMNS_IN_LINE + x;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void scroll() {
    if (scroll_buffer_pos >= SCROLL_BUFFER_LINES) {
        for (int i = 0; i < (SCROLL_BUFFER_LINES - 1) * COLUMNS_IN_LINE * BYTES_FOR_EACH_ELEMENT; i++) {
            scroll_buffer[i] = scroll_buffer[i + COLUMNS_IN_LINE * BYTES_FOR_EACH_ELEMENT];
        }
        scroll_buffer_pos = SCROLL_BUFFER_LINES - 1;
    }
    
    int buffer_offset = scroll_buffer_pos * COLUMNS_IN_LINE * BYTES_FOR_EACH_ELEMENT;
    for (int i = 0; i < COLUMNS_IN_LINE * BYTES_FOR_EACH_ELEMENT; i++) {
        scroll_buffer[buffer_offset + i] = video[i];
    }
    scroll_buffer_pos++;
    
    for (unsigned int i = COLUMNS_IN_LINE * BYTES_FOR_EACH_ELEMENT; i < SCREENSIZE; i++) {
        video[i - COLUMNS_IN_LINE * BYTES_FOR_EACH_ELEMENT] = video[i];
    }
    for (unsigned int i = SCREENSIZE - COLUMNS_IN_LINE * BYTES_FOR_EACH_ELEMENT; i < SCREENSIZE; i += 2) {
        video[i] = ' ';
        video[i + 1] = 0x07;
    }
    current_loc -= COLUMNS_IN_LINE * BYTES_FOR_EACH_ELEMENT;
    scroll_offset = 0;
}

// Print a string with the specified color
/* colors numbers:
       0  - black
      1  - blue
      2  - green
      3  - cyan
      4  - red
      5  - purple
      6  - brown
      7  - gray
      8  - dark gray
      9  - light blue
      10 - light green
      11 - light cyan
      12 - light red
      13 - light purple
      14 - yellow
      15 - white
   */
void vgaprint(const char *str, int color) {
    unsigned int i = 0;
    while (str[i] != '\0') {
        if (current_loc >= SCREENSIZE) {
            scroll();  // Scroll if the end of the screen is reached
        }
        video[current_loc++] = str[i++];  // Character
        video[current_loc++] = color;      // Character color
        
        // Update cursor position after printing each character
        int x = (current_loc / BYTES_FOR_EACH_ELEMENT) % COLUMNS_IN_LINE;
        int y = (current_loc / BYTES_FOR_EACH_ELEMENT) / COLUMNS_IN_LINE;
        terminal_set_cursor(x, y);
    }
}

// Move to the next line
void newline(void) {
    unsigned int line_size = BYTES_FOR_EACH_ELEMENT * COLUMNS_IN_LINE;
    current_loc += (line_size - current_loc % line_size);
    if (current_loc >= SCREENSIZE) {
        scroll();
    }
    
    // Update cursor position after moving to a new line
    int x = (current_loc / BYTES_FOR_EACH_ELEMENT) % COLUMNS_IN_LINE;
    int y = (current_loc / BYTES_FOR_EACH_ELEMENT) / COLUMNS_IN_LINE;
    terminal_set_cursor(x, y);
}

// Clear the screen
void clearscreen(void) {
    unsigned int i = 0;
    while (i < SCREENSIZE) {
        video[i++] = ' ';  // Fill the screen with spaces
        video[i++] = 0x07; // Text color (white on black)
    }
    current_loc = 0;  // Reset cursor position
    terminal_set_cursor(0, 0); // Set cursor to top left
}

// Enable blinking cursor
void enable_cursor() {
    outb(0x3D4, 0x0A);
    outb(0x3D5, (inb(0x3D5) & 0xC0) | 0); // Set cursor start scanline to 0

    outb(0x3D4, 0x0B);
    outb(0x3D5, (inb(0x3D5) & 0xE0) | 15); // Set cursor end scanline to 15
}

// Disable blinking cursor
void disable_cursor() {
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20); // Disable cursor by setting cursor start scanline bit 5 to 1
}

// Handle backspace - delete character at cursor position
void vga_backspace(void) {
    if (current_loc >= BYTES_FOR_EACH_ELEMENT) {
        current_loc -= BYTES_FOR_EACH_ELEMENT;
        
        video[current_loc] = ' ';
        video[current_loc + 1] = 0x07;
        
        int x = (current_loc / BYTES_FOR_EACH_ELEMENT) % COLUMNS_IN_LINE;
        int y = (current_loc / BYTES_FOR_EACH_ELEMENT) / COLUMNS_IN_LINE;
        terminal_set_cursor(x, y);
    }
}

void vga_scroll_up(void) {
    if (scroll_buffer_pos == 0) {
        return;
    }
    
    if (scroll_offset == 0) {
        for (int i = 0; i < SCREENSIZE; i++) {
            saved_screen[i] = video[i];
        }
    }
    
    scroll_offset++;
    
    int total_lines = scroll_buffer_pos;
    
    int max_scroll = total_lines;
    if (scroll_offset > max_scroll) {
        scroll_offset = max_scroll;
    }
    
    // Debug
    char debug[64];
    debug[0] = 'O'; debug[1] = 'f'; debug[2] = 'f'; debug[3] = '=';
    int off = scroll_offset;
    int idx = 4;
    if (off == 0) debug[idx++] = '0';
    else {
        char tmp[10];
        int j = 0;
        while (off > 0) { tmp[j++] = '0' + off % 10; off /= 10; }
        while (j > 0) debug[idx++] = tmp[--j];
    }
    debug[idx++] = ' '; debug[idx++] = 'T'; debug[idx++] = '=';
    off = total_lines;
    if (off == 0) debug[idx++] = '0';
    else {
        char tmp[10];
        int j = 0;
        while (off > 0) { tmp[j++] = '0' + off % 10; off /= 10; }
        while (j > 0) debug[idx++] = tmp[--j];
    }
    debug[idx++] = ' '; debug[idx++] = 'S'; debug[idx++] = '=';
    
    int start_line = total_lines - scroll_offset;
    if (start_line < 0) {
        start_line = 0;
    }
    
    off = start_line;
    if (off == 0) debug[idx++] = '0';
    else {
        char tmp[10];
        int j = 0;
        while (off > 0) { tmp[j++] = '0' + off % 10; off /= 10; }
        while (j > 0) debug[idx++] = tmp[--j];
    }
    debug[idx] = '\0';
    
    for (int i = 0; i < SCREENSIZE; i += 2) {
        video[i] = ' ';
        video[i + 1] = 0x07;
    }
    
    // Show debug at top
    for (int i = 0; debug[i] != '\0'; i++) {
        video[i * 2] = debug[i];
        video[i * 2 + 1] = 0x0E;
    }
    if (start_line < 0) {
        start_line = 0;
    }
    
    int lines_to_show = LINES;
    if (start_line + lines_to_show > total_lines) {
        lines_to_show = total_lines - start_line;
    }
    
    for (int line = 0; line < lines_to_show; line++) {
        int buffer_line_idx = start_line + line;
        if (buffer_line_idx >= 0 && buffer_line_idx < total_lines) {
            int buffer_offset = buffer_line_idx * COLUMNS_IN_LINE * BYTES_FOR_EACH_ELEMENT;
            int screen_offset = line * COLUMNS_IN_LINE * BYTES_FOR_EACH_ELEMENT;
            
            for (int i = 0; i < COLUMNS_IN_LINE * BYTES_FOR_EACH_ELEMENT; i++) {
                video[screen_offset + i] = scroll_buffer[buffer_offset + i];
            }
        }
    }
    
    int x = (current_loc / BYTES_FOR_EACH_ELEMENT) % COLUMNS_IN_LINE;
    int y = (current_loc / BYTES_FOR_EACH_ELEMENT) / COLUMNS_IN_LINE;
    terminal_set_cursor(x, y);
}

void vga_scroll_down(void) {
    if (scroll_offset == 0) {
        return;
    }
    
    scroll_offset--;
    
    // Debug
    char debug[64];
    debug[0] = 'D'; debug[1] = 'N'; debug[2] = ' '; debug[3] = 'O'; debug[4] = '=';
    int off = scroll_offset;
    int idx = 5;
    if (off == 0) debug[idx++] = '0';
    else {
        char tmp[10];
        int j = 0;
        while (off > 0) { tmp[j++] = '0' + off % 10; off /= 10; }
        while (j > 0) debug[idx++] = tmp[--j];
    }
    debug[idx] = '\0';
    
    if (scroll_offset == 0) {
        for (int i = 0; i < SCREENSIZE; i++) {
            video[i] = saved_screen[i];
        }
        // Show debug
        for (int i = 0; debug[i] != '\0'; i++) {
            video[i * 2] = debug[i];
            video[i * 2 + 1] = 0x0C;
        }
    } else {
        for (int i = 0; i < SCREENSIZE; i += 2) {
            video[i] = ' ';
            video[i + 1] = 0x07;
        }
        
        // Show debug
        for (int i = 0; debug[i] != '\0'; i++) {
            video[i * 2] = debug[i];
            video[i * 2 + 1] = 0x0C;
        }
        
        int total_lines = scroll_buffer_pos;
        
        int start_line = total_lines - scroll_offset;
        if (start_line < 0) {
            start_line = 0;
        }
        
        int lines_to_show = LINES;
        if (start_line + lines_to_show > total_lines) {
            lines_to_show = total_lines - start_line;
        }
        
        for (int line = 0; line < lines_to_show; line++) {
            int buffer_line_idx = start_line + line;
            if (buffer_line_idx >= 0 && buffer_line_idx < total_lines) {
                int buffer_offset = buffer_line_idx * COLUMNS_IN_LINE * BYTES_FOR_EACH_ELEMENT;
                int screen_offset = line * COLUMNS_IN_LINE * BYTES_FOR_EACH_ELEMENT;
                
                for (int i = 0; i < COLUMNS_IN_LINE * BYTES_FOR_EACH_ELEMENT; i++) {
                    video[screen_offset + i] = scroll_buffer[buffer_offset + i];
                }
            }
        }
    }
    
    int x = (current_loc / BYTES_FOR_EACH_ELEMENT) % COLUMNS_IN_LINE;
    int y = (current_loc / BYTES_FOR_EACH_ELEMENT) / COLUMNS_IN_LINE;
    terminal_set_cursor(x, y);
}