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

uint8_t nvm_bytecode[] = {
    0x4E, 0x56, 0x4D, 0x30,  // NVM0 signature

    0x02, 0x0F,              // PUSH_BYTE 15
    0x02, 0x1B,              // PUSH_BYTE 27
    0x10,                    // ADD  -> 42

    0x03, 0x00, 0x01,        // PUSH_SHORT 256  (0x0100 -> bytes: 0x00, 0x01)
    0x03, 0xFE, 0x00,        // PUSH_SHORT 254  (0x00FE -> bytes: 0xFE, 0x00)
    0x10,                    // ADD  -> 510

    0x02, 0x0A,              // PUSH_BYTE 10
    0x11,                    // SUB  -> 510
    0x06,
    0x14,

    0x50, 0x01,
    0x00                     // HALT
};


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

    nvm_execute(nvm_bytecode, sizeof(nvm_bytecode));
    pit_polling_loop();
}