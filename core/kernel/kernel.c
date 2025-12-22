// SPDX-License-Identifier: LGPL-3.0-or-later

#include <core/kernel/kstd.h>
#include <core/kernel/mem.h>
#include <core/kernel/nvm/nvm.h>
#include <core/kernel/nvm/caps.h>
#include <core/drivers/serial.h>
#include <core/drivers/fb.h>
#include <core/drivers/timer.h>
#include <core/drivers/keyboard.h>
#include <core/drivers/cdrom.h>
#include <core/kernel/shell.h>
#include <core/kernel/log.h>
#include <core/fs/ramfs.h>
#include <core/fs/initramfs.h>
#include <core/fs/iso9660.h>
#include <core/fs/vfs.h>
#include <usr/bin/userspace_init.h>
#include <stddef.h>
#include <stdbool.h>
#include <lib/bootloader/limine.h>


static volatile struct limine_memmap_request memmap_request = {
    .id = { LIMINE_COMMON_MAGIC, 0x67cf3d9d378a806f, 0xe304acdfc50c3c62 },
    .revision = 0
};

static volatile struct limine_module_request module_request = {
    .id = { LIMINE_COMMON_MAGIC, 0x3e7e279702be32af, 0xca1c4f3bd1280cee },
    .revision = 0
};

static volatile struct limine_hhdm_request hhdm_request = {
    .id = { LIMINE_COMMON_MAGIC, 0x48dcf1cb8ad2b852, 0x63984e959a98244b },
    .revision = 0
};

static volatile struct limine_rsdp_request rsdp_request = {
    .id = { LIMINE_COMMON_MAGIC, 0xc5e77b6b397e7b43, 0x27637845accdcf3c },
    .revision = 0
};

static volatile struct limine_bootloader_info_request bootloader_info_request = {
    .id = { LIMINE_COMMON_MAGIC, 0xf55038d8e2a1202f, 0x279426fcf5f59740 },
    .revision = 0
};

static volatile struct limine_executable_address_request kernel_address_request = {
    .id = { LIMINE_COMMON_MAGIC, 0x71ba76863cc55f63, 0xb2644a48c516a487 },
    .revision = 0
};

static volatile struct limine_mp_request smp_request = {
    .id = { LIMINE_COMMON_MAGIC, 0x95a67b819a1b857e, 0xa0b61b723b6a73e0 },
    .revision = 0,
    .flags = 0
};

static volatile struct limine_paging_mode_request paging_mode_request = {
    .id = { LIMINE_COMMON_MAGIC, 0x95c1a0edab0944cb, 0xa4e5cb3842f7488a },
    .revision = 0
};

void limine_smp_entry(struct limine_mp_info *info) {
    // This function is called on each additional CPU
    // For now, just halt the CPU
    while (1) {
        asm volatile("hlt");
    }
}

void kmain() {
    // enable_cursor();

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
    // initializeMemoryManager();

    LOG_DEBUG("test\n");
    init_serial();
    // ramfs_init();
    // vfs_init();
    // syslog_init();
    // keyboard_init();
    
    // LOG_DEBUG("System initialization started\n");
    
    // cdrom_init();
    
    // void* iso_location = NULL;
    // size_t iso_size = 0;
    
    // if (module_request.response != NULL && module_request.response->module_count > 0) {
    //     LOG_DEBUG("Checking Limine modules for ISO9660...\n");
        
    //     for (uint64_t i = 0; i < module_request.response->module_count; i++) {
    //         struct limine_file *module = module_request.response->modules[i];
            
    //         if (module->size > 0x8005) {
    //             char* sig = (char*)module->address + 0x8001;
    //             if (sig[0] == 'C' && sig[1] == 'D' && sig[2] == '0' && 
    //                 sig[3] == '0' && sig[4] == '1') {
    //                 iso_location = (void*)module->address;
    //                 iso_size = module->size;
    //                 LOG_DEBUG("Found ISO9660 in module\n");
    //                 break;
    //             }
    //         }
    //     }
    // }
    
    // if (iso_location) {
    //     cdrom_set_iso_data(iso_location, iso_size);
    //     iso9660_init(iso_location, iso_size);
    //     LOG_DEBUG("ISO9660 filesystem mounted\n");

    //     iso9660_mount_to_vfs("/", "/");
    //     LOG_DEBUG("ISO contents mounted to /\n");
    // } else {
    //     LOG_DEBUG(":: ISO9660 filesystem not found\n");
    // }
    
    // initramfs_load_limine(&module_request);
    // LOG_DEBUG("Initramfs loaded\n");
    // nvm_init();
    // LOG_DEBUG("NVM initialized\n");
    // userspace_init_programs();
    // LOG_DEBUG("Userspace programs registered\n");

    // // Print additional Limine information
    // if (kernel_address_request.response) {
    //     LOG_DEBUG("Kernel loaded at: 0x%lx\n", kernel_address_request.response->physical_base);
    //     LOG_DEBUG("Kernel virtual base: 0x%lx\n", kernel_address_request.response->virtual_base);
    // }

    // if (smp_request.response) {
    //     LOG_DEBUG("SMP: %lu CPUs available\n", smp_request.response->cpu_count);

    //     // Set entry point for additional CPUs
    //     for (uint64_t i = 0; i < smp_request.response->cpu_count; i++) {
    //         if (smp_request.response->cpus[i]->lapic_id != smp_request.response->bsp_lapic_id) {
    //             smp_request.response->cpus[i]->goto_address = (limine_goto_address)&limine_smp_entry;
    //         }
    //     }
    // }

    // if (paging_mode_request.response) {
    //     LOG_DEBUG("Paging mode: %u\n", paging_mode_request.response->mode);
    // }

    // while(true) {
    //     nvm_scheduler_tick();
    // }
}