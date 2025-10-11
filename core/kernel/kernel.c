#include <core/arch/multiboot.h>

#include <core/kernel/kstd.h>
#include <core/kernel/mem.h>

#include <core/kernel/nvm/nvm.h>

#include <core/drivers/serial.h>
#include <core/drivers/vga.h>
#include <core/drivers/timer.h>

#include <core/fs/ramfs.h>

#include <stddef.h>
#include <stdbool.h>

int counter = 0;
char content[32];

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

    kprint(":: Initializing memory manager...\n", 7);

    uint32_t available_memory = mb_info->mem_upper * 1024;
    initializeMemoryManager((void*)0x100000, available_memory);

    init_serial();
    pit_init();
    ramfs_init();
    nvm_init();
}