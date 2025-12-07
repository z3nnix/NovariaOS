// SPDX-License-Identifier: LGPL-3.0-or-later

#include <core/arch/multiboot.h>
#include <core/kernel/kstd.h>
#include <core/kernel/mem.h>
#include <core/kernel/nvm/nvm.h>
#include <core/kernel/nvm/caps.h>
#include <core/drivers/serial.h>
#include <core/drivers/vga.h>
#include <core/drivers/timer.h>
#include <core/drivers/keyboard.h>
#include <core/kernel/shell.h>
#include <core/fs/ramfs.h>
#include <core/fs/initramfs.h>
#include <core/fs/vfs.h>
#include <userspace/userspace_init.h>
#include <stddef.h>
#include <stdbool.h>

void kmain(multiboot_info_t* mb_info) {
    enable_cursor();

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
    vfs_init();
    keyboard_init();
    
    initramfs_load(mb_info);
    
    nvm_init();
    
    // Initialize userspace programs
    userspace_init_programs();
    
    // Execute programs from initramfs
    size_t program_count = initramfs_get_count();
    if (program_count > 0) {
        for (size_t i = 0; i < program_count; i++) {
            struct program* prog = initramfs_get_program(i);
            if (prog && prog->size > 0) {
                char buf[32];
                int n = i;
                char* p = buf;
                if (n == 0) {
                    *p++ = '0';
                } else {
                    char* start = p;
                    while (n > 0) {
                        *p++ = '0' + n % 10;
                        n /= 10;
                    }
                    p--;
                    while (start < p) {
                        char temp = *start;
                        *start = *p;
                        *p = temp;
                        start++;
                        p--;
                    }
                }
                *p = '\0';
                
                nvm_execute(prog->data, prog->size, (uint16_t[]){CAP_ALL}, 1);
            }
        }
        
        // Wait for all initramfs programs to complete before starting shell
        bool any_active = true;
        int max_cycles = 100000; // Prevent infinite loop
        int cycles = 0;
        while (any_active && cycles < max_cycles) {
            any_active = false;
            for (int i = 0; i < MAX_PROCESSES; i++) {
                if (nvm_is_process_active(i)) {
                    any_active = true;
                    break;
                }
            }
            if (any_active) {
                nvm_scheduler_tick();
                cycles++;
            }
        }
    } else {
        kprint(":: No programs found in initramfs\n", 14);
    }
    
    // Start the shell
    shell_init();
    shell_run();
    
    // If shell exits, continue with scheduler
    while(true) {
        nvm_scheduler_tick();
    }
}
