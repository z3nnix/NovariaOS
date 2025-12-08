// SPDX-License-Identifier: LGPL-3.0-or-later

#include <core/drivers/keyboard.h>
#include <core/drivers/vga.h>
#include <core/kernel/kstd.h>

extern uint8_t inb(uint16_t port);
extern void outb(uint16_t port, uint8_t val);

// Keyboard data port and status port
#define KEYBOARD_DATA_PORT    0x60
#define KEYBOARD_STATUS_PORT  0x64

// Circular buffer for keyboard input
static char keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static volatile int buffer_read_pos = 0;
static volatile int buffer_write_pos = 0;

// Scancode to ASCII mapping (US layout)
static const char scancode_to_ascii[] = {
    0,   27,  '1', '2', '3',  '4', '5', '6', '7', '8', '9', '0', '-',  '=', '\b',
    '\t', 'q', 'w', 'e', 'r',  't', 'y', 'u', 'i', 'o', 'p', '[', ']',  '\n',
    0,    'a', 's', 'd', 'f',  'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0,    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',  0,
    '*',  0,   ' '
};

// Shifted characters
static const char scancode_to_ascii_shifted[] = {
    0,   27,  '!', '@', '#',  '$', '%', '^', '&', '*', '(', ')', '_',  '+', '\b',
    '\t', 'Q', 'W', 'E', 'R',  'T', 'Y', 'U', 'I', 'O', 'P', '{', '}',  '\n',
    0,    'A', 'S', 'D', 'F',  'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0,    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',  0,
    '*',  0,   ' '
};

static bool shift_pressed = false;
static bool caps_lock = false;
static bool ctrl_pressed = false;
static bool extended_scancode = false;

// Add a character to the buffer
static void keyboard_buffer_push(char c) {
    int next_pos = (buffer_write_pos + 1) % KEYBOARD_BUFFER_SIZE;
    if (next_pos != buffer_read_pos) {
        keyboard_buffer[buffer_write_pos] = c;
        buffer_write_pos = next_pos;
    }
}

// Read a character from the buffer
static char keyboard_buffer_pop(void) {
    if (buffer_read_pos == buffer_write_pos) {
        return 0; // Buffer is empty
    }
    char c = keyboard_buffer[buffer_read_pos];
    buffer_read_pos = (buffer_read_pos + 1) % KEYBOARD_BUFFER_SIZE;
    return c;
}

// Handle keyboard interrupt
void keyboard_handler(void) {
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);
    
    if (scancode == 0xE0) {
        extended_scancode = true;
        return;
    }
    
    if (scancode & 0x80) {
        scancode &= 0x7F;
        
        // Check for shift release
        if (scancode == 0x2A || scancode == 0x36) {
            shift_pressed = false;
        }
        
        // Check for ctrl release
        if (scancode == 0x1D) {
            ctrl_pressed = false;
        }
        
        extended_scancode = false;
        return;
    }
    
    if (extended_scancode) {
        extended_scancode = false;
        
        if (scancode == 0x49) {
            vga_scroll_up();
            return;
        } else if (scancode == 0x51) {
            vga_scroll_down();
            return;
        } else if (scancode == 0x48) {
            if (ctrl_pressed) {
                vga_scroll_up();
            }
            return;
        } else if (scancode == 0x50) {
            if (ctrl_pressed) {
                vga_scroll_down();
            }
            return;
        }
        return;
    }
    
    if (scancode == 0x2A || scancode == 0x36) {
        shift_pressed = true;
        return;
    }
    
    // Check for ctrl press
    if (scancode == 0x1D) {
        ctrl_pressed = true;
        return;
    }
    
    // Check for caps lock
    if (scancode == 0x3A) {
        caps_lock = !caps_lock;
        return;
    }
    
    // Convert scancode to ASCII
    char ascii = 0;
    if (scancode < sizeof(scancode_to_ascii)) {
        if (shift_pressed) {
            ascii = scancode_to_ascii_shifted[scancode];
        } else {
            ascii = scancode_to_ascii[scancode];
            
            // Handle caps lock for letters
            if (caps_lock && ascii >= 'a' && ascii <= 'z') {
                ascii = ascii - 'a' + 'A';
            }
        }
        
        // Handle Ctrl combinations
        if (ctrl_pressed && ascii >= 'a' && ascii <= 'z') {
            ascii = ascii - 'a' + 1; // Ctrl+A = 1, Ctrl+B = 2, etc.
        } else if (ctrl_pressed && ascii >= 'A' && ascii <= 'Z') {
            ascii = ascii - 'A' + 1;
        }
        
        if (ascii != 0) {
            keyboard_buffer_push(ascii);
        }
    }
}

// Poll the keyboard (since we don't have interrupts set up yet)
static void keyboard_poll(void) {
    uint8_t status = inb(KEYBOARD_STATUS_PORT);
    if (status & 0x01) { // Check if data is available
        keyboard_handler();
    }
}

// Initialize the keyboard
void keyboard_init(void) {
    buffer_read_pos = 0;
    buffer_write_pos = 0;
    shift_pressed = false;
    caps_lock = false;
    ctrl_pressed = false;
    extended_scancode = false;
}

// Check if a character is available
bool keyboard_has_char(void) {
    keyboard_poll();
    return buffer_read_pos != buffer_write_pos;
}

// Read a character (blocking)
char keyboard_getchar(void) {
    extern void nvm_scheduler_tick(void);
    
    while (!keyboard_has_char()) {
        keyboard_poll();
        // Run NVM scheduler while waiting for input
        nvm_scheduler_tick();
    }
    return keyboard_buffer_pop();
}

// Read a line of input
void keyboard_getline(char* buffer, int max_length) {
    int pos = 0;
    
    while (pos < max_length - 1) {
        char c = keyboard_getchar();
        
        if (c == '\n') {
            buffer[pos] = '\0';
            kprint("\n", 7);
            return;
        } else if (c == '\b') {
            if (pos > 0) {
                pos--;
                vga_backspace(); // Use proper backspace function
            }
        } else if (c >= 32 && c <= 126) { // Printable characters
            buffer[pos++] = c;
            char temp[2] = {c, '\0'};
            kprint(temp, 15);
        }
    }
    
    buffer[pos] = '\0';
}
