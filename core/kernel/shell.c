// SPDX-License-Identifier: LGPL-3.0-or-later

#include <core/kernel/shell.h>
#include <core/kernel/kstd.h>
#include <core/drivers/keyboard.h>
#include <core/drivers/vga.h>
#include <core/kernel/mem.h>
#include <core/fs/initramfs.h>
#include <core/fs/vfs.h>
#include <core/kernel/nvm/nvm.h>
#include <core/kernel/nvm/caps.h>
#include <core/kernel/userspace.h>

#define MAX_COMMAND_LENGTH 256

// Helper function to compare strings
static int strcmp(const char* str1, const char* str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(unsigned char*)str1 - *(unsigned char*)str2;
}

// Helper function to compare strings with length limit
static int strncmp(const char* str1, const char* str2, int n) {
    while (n > 0 && *str1 && (*str1 == *str2)) {
        str1++;
        str2++;
        n--;
    }
    if (n == 0) return 0;
    return *(unsigned char*)str1 - *(unsigned char*)str2;
}

// Helper function to get string length
static int strlen(const char* str) {
    int len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

// Command: help
static void cmd_help(void) {
    kprint("\nBuilt-in commands:\n", 10);
    kprint("  help     - Show this help message\n", 7);
    kprint("  info     - Show system information\n", 7);
    kprint("  memtest  - Test memory allocation\n", 7);
    kprint("  list     - List loaded NVM programs\n", 7);
    kprint("  run      - Run a NVM program by index\n", 7);
    kprint("  progs    - List userspace programs\n", 7);
    kprint("\nUserspace programs:\n", 10);
    kprint("  Use 'progs' to see available programs\n", 7);
    kprint("\n", 7);
}


// Command: info
static void cmd_info(void) {
    kprint("\nNovariaOS v0.1\n", 10);
    kprint("Kernel with NVM support\n", 7);
    kprint("Architecture: x86\n", 7);
    kprint("\n", 7);
}

// Command: memtest
static void cmd_memtest(void) {
    kprint("\nTesting memory allocation...\n", 7);
    
    void* ptr1 = allocateMemory(128);
    if (ptr1) {
        kprint("Allocated 128 bytes at ", 7);
        char buf[32];
        itoa((int)ptr1, buf, 16);
        kprint(buf, 7);
        kprint("\n", 7);
        freeMemory(ptr1);
        kprint("Freed memory\n", 7);
    } else {
        kprint("Failed to allocate memory\n", 12);
    }
    kprint("\n", 7);
}

// Command: list
static void cmd_list(void) {
    size_t count = initramfs_get_count();
    
    kprint("\nLoaded programs: ", 7);
    char buf[32];
    itoa(count, buf, 10);
    kprint(buf, 7);
    kprint("\n", 7);
    
    for (size_t i = 0; i < count; i++) {
        struct program* prog = initramfs_get_program(i);
        if (prog) {
            kprint("  [", 7);
            itoa(i, buf, 10);
            kprint(buf, 7);
            kprint("] Size: ", 7);
            itoa(prog->size, buf, 10);
            kprint(buf, 7);
            kprint(" bytes\n", 7);
        }
    }
    kprint("\n", 7);
}

// Command: run
static void cmd_run(const char* args) {
    // Parse the program index
    int index = 0;
    const char* p = args;
    
    // Skip leading spaces
    while (*p == ' ') p++;
    
    // Parse number
    while (*p >= '0' && *p <= '9') {
        index = index * 10 + (*p - '0');
        p++;
    }
    
    size_t count = initramfs_get_count();
    if (index >= 0 && (size_t)index < count) {
        struct program* prog = initramfs_get_program(index);
        if (prog && prog->size > 0) {
            kprint("\nRunning program ", 7);
            char buf[32];
            itoa(index, buf, 10);
            kprint(buf, 7);
            kprint("...\n", 7);
            
            nvm_execute(prog->data, prog->size, (uint16_t[]){CAP_ALL}, 1);
            
            kprint("Program finished.\n", 7);
        } else {
            kprint("\nError: Invalid program\n", 12);
        }
    } else {
        kprint("\nError: Invalid program index\n", 12);
    }
    kprint("\n", 7);
}

// Command: progs
static void cmd_progs(void) {
    kprint("\n", 7);
    userspace_list();
    kprint("\n", 7);
}

// Parse command line into argc/argv
static int parse_command(const char* command, char* argv[], int max_args) {
    int argc = 0;
    static char cmd_buf[MAX_COMMAND_LENGTH];
    
    // Copy command to buffer
    int i = 0;
    while (command[i] && i < MAX_COMMAND_LENGTH - 1) {
        cmd_buf[i] = command[i];
        i++;
    }
    cmd_buf[i] = '\0';
    
    // Parse into tokens
    char* p = cmd_buf;
    while (*p && argc < max_args) {
        // Skip leading spaces
        while (*p == ' ') p++;
        
        if (*p == '\0') break;
        
        // Start of argument
        argv[argc++] = p;
        
        // Find end of argument
        while (*p && *p != ' ') p++;
        
        // Null-terminate
        if (*p) {
            *p = '\0';
            p++;
        }
    }
    
    return argc;
}

// Parse and execute a command
static void execute_command(const char* command) {
    // Skip leading spaces
    while (*command == ' ') command++;
    
    // Empty command
    if (*command == '\0') {
        return;
    }
    
    // Parse command
    char* argv[16];
    int argc = parse_command(command, argv, 16);
    
    if (argc == 0) return;
    
    // Check built-in commands first
    if (strcmp(argv[0], "help") == 0) {
        cmd_help();
    } else if (strcmp(argv[0], "info") == 0) {
        cmd_info();
    } else if (strcmp(argv[0], "memtest") == 0) {
        cmd_memtest();
    } else if (strcmp(argv[0], "list") == 0) {
        cmd_list();
    } else if (strcmp(argv[0], "run") == 0) {
        if (argc > 1) {
            cmd_run(argv[1]);
        } else {
            kprint("\nUsage: run <index>\n\n", 12);
        }
    } else if (strcmp(argv[0], "progs") == 0) {
        cmd_progs();
    } else {
        // Try to execute as userspace program
        if (userspace_exists(argv[0])) {
            int ret = userspace_exec(argv[0], argc, argv);
            if (ret != 0) {
                kprint("\nProgram exited with code ", 12);
                char buf[16];
                itoa(ret, buf, 10);
                kprint(buf, 12);
                kprint("\n", 12);
            }
        } else {
            kprint("\nUnknown command: ", 12);
            kprint(argv[0], 12);
            kprint("\nType 'help' for available commands or 'progs' for programs.\n", 7);
        }
    }
}

// Initialize the shell
void shell_init(void) {
    kprint("Type 'help' for available commands.\n\n", 7);
}

// Run the shell main loop
void shell_run(void) {
    char command[MAX_COMMAND_LENGTH];
    
    while (1) {
        // Run NVM scheduler to execute background processes
        nvm_scheduler_tick();
        
        // Print prompt
        kprint("novaria> ", 11);
        
        // Read command
        keyboard_getline(command, MAX_COMMAND_LENGTH);
        
        // Execute command
        execute_command(command);
    }
}
