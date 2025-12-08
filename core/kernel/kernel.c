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
#include <core/drivers/cdrom.h>
#include <core/kernel/shell.h>
#include <core/kernel/syslog.h>
#include <core/fs/ramfs.h>
#include <core/fs/initramfs.h>
#include <core/fs/iso9660.h>
#include <usr/bin/vfs.h>
#include <usr/bin/userspace_init.h>
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
    syslog_init();
    
    char buf[16];
    char mem_msg[64];
    mem_msg[0] = 'M'; mem_msg[1] = 'e'; mem_msg[2] = 'm'; mem_msg[3] = 'o';
    mem_msg[4] = 'r'; mem_msg[5] = 'y'; mem_msg[6] = ':'; mem_msg[7] = ' ';
    itoa(available_memory / 1024 / 1024, buf, 10);
    int i = 0;
    while (buf[i]) {
        mem_msg[8 + i] = buf[i];
        i++;
    }
    mem_msg[8 + i] = ' ';
    mem_msg[9 + i] = 'M';
    mem_msg[10 + i] = 'B';
    mem_msg[11 + i] = '\n';
    mem_msg[12 + i] = '\0';
    syslog_write(mem_msg);
    keyboard_init();
    
    syslog_write("System initialization started\n");
    
    cdrom_init();
    
    void* iso_location = NULL;
    size_t iso_size = 0;
    
    // Check multiboot modules for ISO9660
    if (mb_info->flags & MULTIBOOT_FLAG_MODS && mb_info->mods_count > 0) {
        module_t* modules = (module_t*)mb_info->mods_addr;
        syslog_write("Checking multiboot modules for ISO9660...\n");
        
        for (uint32_t i = 0; i < mb_info->mods_count; i++) {
            uint32_t mod_start_addr = modules[i].mod_start;
            uint32_t mod_end_addr = modules[i].mod_end;
            
            if (mod_start_addr == 0 || mod_end_addr == 0 || mod_end_addr <= mod_start_addr) {
                continue;
            }
            
            void* mod_start = (void*)mod_start_addr;
            uint32_t mod_size = mod_end_addr - mod_start_addr;
            
            if (mod_size > 0x8005) {
                char* sig = (char*)mod_start + 0x8001;
                if (sig[0] == 'C' && sig[1] == 'D' && sig[2] == '0' && 
                    sig[3] == '0' && sig[4] == '1') {
                    iso_location = mod_start;
                    iso_size = mod_size;
                    syslog_print(":: Found ISO9660 in module\n", 7);
                    char num[8];
                    itoa(i, num, 10);
                    syslog_write(num);
                    syslog_write("\n");
                    break;
                }
            }
        }
    }
    
    if (iso_location) {
        cdrom_set_iso_data(iso_location, iso_size);
        iso9660_init(iso_location, iso_size);
        syslog_write("ISO9660 filesystem mounted\n");

        iso9660_mount_to_vfs("/", "/");
        syslog_write("ISO contents mounted to /\n");
    } else {
        syslog_print(":: ISO9660 filesystem not found\n", 14);
    }
    
    initramfs_load(mb_info);
    syslog_write("Initramfs loaded\n");
    nvm_init();
    syslog_write("NVM initialized\n");
    userspace_init_programs();
    syslog_write("Userspace programs registered\n");
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
                
                nvm_execute((uint8_t*)prog->data, prog->size, (uint16_t[]){CAP_ALL}, 1);
            }
        }
    } else {
        syslog_print(":: No programs found in initramfs\n", 14);
    }
    
    syslog_write("System initialization complete\n");
    shell_init();
    shell_run();
    
    while(true) {
        nvm_scheduler_tick();
    }
}
