#include <core/drivers/vga.h>

extern uint8_t inb(uint16_t port);
extern void outb(uint16_t port, uint8_t val);

#define LINES 25
#define COLUMNS_IN_LINE 80
#define BYTES_FOR_EACH_ELEMENT 2
#define SCREENSIZE (BYTES_FOR_EACH_ELEMENT * COLUMNS_IN_LINE * LINES)

unsigned int current_loc = 0;  // Current position in video memory
char *video = (char*)0xb8000;   // Address of VGA video memory

void terminal_set_cursor(int x, int y) {
    uint16_t pos = y * COLUMNS_IN_LINE + x;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

// Scroll the screen
void scroll() {
    for (unsigned int i = COLUMNS_IN_LINE * BYTES_FOR_EACH_ELEMENT; i < SCREENSIZE; i++) {
        video[i - COLUMNS_IN_LINE * BYTES_FOR_EACH_ELEMENT] = video[i];
    }
    for (unsigned int i = SCREENSIZE - COLUMNS_IN_LINE * BYTES_FOR_EACH_ELEMENT; i < SCREENSIZE; i += 2) {
        video[i] = ' ';  // Clear the last line
        video[i + 1] = 0x07;  // Text color
    }
    current_loc -= COLUMNS_IN_LINE * BYTES_FOR_EACH_ELEMENT; // Move cursor up
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