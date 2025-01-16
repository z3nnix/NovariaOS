#include <string.h>
#include <stdint.h>

#include <core/kernel/idt.h>
#include <core/kernel/kstd.h>
#include <core/kernel/multiboot.h>

extern void disable_cursor();
void kmain(multiboot_info_t* mb_info) {
    disable_cursor();
    const char* ascii_art[] = {
        " _   _                      _        ___  ____  ",
        "| \\ | | _____   ____ _ _ __(_) __ _ / _ \\/ ___| ",
        "|  \\| |/ _ \\ \\ / / _` | '__| |/ _` | | | \\___ \\ ",
        "| |\\  | (_) \\ V / (_| | |  | | (_| | |_| |___) |",
        "|_| \\_|\\___/ \\_/ \\__,_|_|  |_|\\__,_|\\___/|____/ "
    };

    for (int i = 0; i < sizeof(ascii_art) / sizeof(ascii_art[0]); i++) {
        kprint(ascii_art[i], 15);
        kprint("\n", 15);
    }

    kprint("                                 TG: ", 15);
    kprint("@NovariaOS\n", 9);
    
    size_t total_memory = (mb_info->mem_upper + 1024) * 1024;
    initializeMemoryManager((void*)0x100000, total_memory);

    void* allocated_mem = allocateMemory(1024);
    if (allocated_mem != NULL) {
        // ???
    }

    freeMemory(allocated_mem);
    stressTestMemoryManager();
}