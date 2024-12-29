#include <string.h>
#include <stdint.h>

#include "idt.c"

#include <core/drivers/keyboard.c>
#include <core/drivers/vga.c>
#include <core/kernel/kstd.h>

#include <core/fs/ramfs.c>

void scanf(char *buffer, int max_length) {
    int index = 0;
    while (index < max_length - 1) {
        char c = __getch();
        if ((unsigned char)c < sizeof(keymap)) {
            c = keymap[(unsigned char)c];
            if (c != '\0') {
                if (c == '\n') {
                    break;
                }
                if (c == '\b') {
                    if (index > 0) {
                        index--;
                        current_loc -= 2;
                        video[current_loc] = ' ';
                        video[current_loc + 1] = 0x07;
                        int x = (current_loc / BYTES_FOR_EACH_ELEMENT) % COLUMNS_IN_LINE;
                        int y = (current_loc / BYTES_FOR_EACH_ELEMENT) / COLUMNS_IN_LINE;
                        terminal_set_cursor(x, y);
                    }
                } else {  
                    buffer[index++] = c; 
                    video[current_loc++] = c;  
                    video[current_loc++] = 0x07;
                    int x = (current_loc / BYTES_FOR_EACH_ELEMENT) % COLUMNS_IN_LINE;
                    int y = (current_loc / BYTES_FOR_EACH_ELEMENT) / COLUMNS_IN_LINE;
                    terminal_set_cursor(x, y);
                }
            }
        }
    }
    buffer[index] = '\0';  
    newline();
}

void test_syscall() {
    uint32_t syscall_number = 1;
    uint32_t arg1 = 42;         
    uint32_t result;

    asm volatile (
        "int $0x80"
        : "=a" (result)
        : "a" (syscall_number), "b" (arg1)
        : "memory"
    );
}

void kmain() {
    init_ramfs();
    
    kprint("NovariaOS. 0.0.3 indev build. TG: ", 15);
    kprint("@NovariaOS\n", 9);

    int file_id = ramfs_create("testfs.txt", false);
    const char* data = "Hello, RamFS!\n";
    int bytes_written = ramfs_write(file_id, data, strlen(data));
                
    char buffer[100];
    int bytes_read = ramfs_read(file_id, buffer, sizeof(buffer));
    kprint(buffer, 15);

    while(true) {
        kprint("# ", 7);
        char sometext[MAX_TEXT_SIZE];
        scanf(sometext, sizeof(sometext));
        if (strlen(sometext) == 0) {
            continue;
        }
        
        if (strcmp(sometext, "ramfs-test") == 0) {
            test_ramfs();
        } else if (strcmp(sometext, "syscall-test") == 0){
            test_syscall();
        } else if (strcmp(sometext, "clear") == 0) {
            clearscreen();
        } else if (strcmp(sometext, "shutdown") == 0) {
            acpi_off();
        } else {
            kprint("Unknown command: ", 7);
            kprint(sometext, 7);
            newline();
        }
    }
}