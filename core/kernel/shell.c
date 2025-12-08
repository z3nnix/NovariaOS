// SPDX-License-Identifier: LGPL-3.0-or-later

// This is a temporary shell. In the near future, it will be rewritten in a language that compiles to NVM.

#include <core/kernel/shell.h>
#include <core/kernel/kstd.h>
#include <core/drivers/keyboard.h>
#include <core/drivers/vga.h>
#include <core/kernel/mem.h>
#include <core/fs/initramfs.h>
#include <core/fs/iso9660.h>
#include <core/kernel/nvm/nvm.h>
#include <core/kernel/nvm/caps.h>
#include <core/kernel/userspace.h>

#define MAX_COMMAND_LENGTH 256
#define MAX_PATH_LENGTH 64

static char current_working_directory[MAX_PATH_LENGTH] = "/";

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
    kprint("  pwd      - Print working directory\n", 7);
    kprint("\nISO9660 commands:\n", 10);
    kprint("  isols    - List files in ISO9660 directory\n", 7);
    kprint("  isocat   - Show ISO9660 file content\n", 7);
    kprint("\nKeyboard shortcuts: (may unstable now)\n", 10);
    kprint("  Ctrl+Up     - Scroll screen up\n", 7);
    kprint("  Ctrl+Down   - Scroll screen down\n", 7);
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
    const char* p = args;
    while (*p == ' ') p++;

    if (*p == '\0') {
        kprint("\nError: Please specify program path\n", 12);
        kprint("Usage: run <filepath>\n", 7);
        kprint("\n", 7);
        return;
    }

    char filepath[256];
    int i = 0;

    while (*p != '\0' && *p != ' ' && i < 255) {
        filepath[i++] = *p++;
    }
    filepath[i] = '\0';

    if (!vfs_exists(filepath)) {
        kprint("\nError: File '", 12);
        kprint(filepath, 12);
        kprint("' not found\n", 12);
        kprint("\n", 7);
        return;
    }

    size_t size;
    const char* data = vfs_read(filepath, &size);
    
    if (data && size > 0) {
        nvm_execute((uint8_t*)data, size, (uint16_t[]){CAP_ALL}, 1);
        
    } else {
        kprint("Error: Failed to read program file or file is empty\n", 12);
    }
    
    kprint("\n", 7);
}

// Command: progs
static void cmd_progs(void) {
    kprint("\n", 7);
    userspace_list();
    kprint("\n", 7);
}

// Command: isols
static void cmd_isols(const char* args) {
    if (!iso9660_is_initialized()) {
        kprint("\nISO9660 filesystem is not initialized\n\n", 14);
        return;
    }
    
    const char* path = args;
    // Skip leading spaces
    while (*path == ' ') path++;
    
    // Default to root if no path given
    if (*path == '\0') {
        path = "/";
    }
    
    kprint("\n", 7);
    iso9660_list_dir(path);
    kprint("\n", 7);
}

// Command: isocat
static void cmd_isocat(const char* args) {
    if (!iso9660_is_initialized()) {
        kprint("\nISO9660 filesystem is not initialized\n\n", 14);
        return;
    }
    
    const char* path = args;
    // Skip leading spaces
    while (*path == ' ') path++;
    
    if (*path == '\0') {
        kprint("\nUsage: isocat <path>\n\n", 12);
        return;
    }
    
    size_t size;
    const void* data = iso9660_find_file(path, &size);
    
    if (!data) {
        kprint("\nFile not found: ", 14);
        kprint(path, 14);
        kprint("\n\n", 14);
        return;
    }
    
    kprint("\n", 7);
    kprint("File: ", 7);
    kprint(path, 11);
    kprint(" (", 7);
    char buf[32];
    itoa(size, buf, 10);
    kprint(buf, 7);
    kprint(" bytes)\n", 7);
    kprint("Content:\n", 7);
    
    // Print file content (limit to first 1024 bytes for safety)
    size_t print_size = size > 1024 ? 1024 : size;
    const char* text = (const char*)data;
    for (size_t i = 0; i < print_size; i++) {
        char c[2];
        c[0] = text[i];
        c[1] = '\0';
        
        if (text[i] >= 32 && text[i] < 127) {
            kprint(c, 7);
        } else if (text[i] == '\n') {
            kprint("\n", 7);
        } else if (text[i] == '\t') {
            kprint("    ", 7);
        } else {
            kprint(".", 7);
        }
    }
    
    if (size > 1024) {
        kprint("\n... (truncated, showing first 1024 bytes)\n", 7);
    }
    
    kprint("\n\n", 7);
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
            kprint("\nUsage: run <path>\n\n", 12);
        }
    } else if (strcmp(argv[0], "progs") == 0) {
        cmd_progs();
    } else if (strcmp(argv[0], "pwd") == 0) {
        kprint(current_working_directory, 11);
        kprint("\n", 7);
    } else if (strcmp(argv[0], "isols") == 0) {
        if (argc > 1) {
            cmd_isols(argv[1]);
        } else {
            cmd_isols("/");
        }
    } else if (strcmp(argv[0], "isocat") == 0) {
        if (argc > 1) {
            cmd_isocat(argv[1]);
        } else {
            kprint("\nUsage: isocat <path>\n\n", 12);
        }
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
            kprint(argv[0], 7);
            kprint(": command not found\n", 7);
        }
    }
}

const char* shell_get_cwd(void) {
    return current_working_directory;
}

void shell_set_cwd(const char* path) {
    int i = 0;
    while (path[i] != '\0' && i < MAX_PATH_LENGTH - 1) {
        current_working_directory[i] = path[i];
        i++;
    }
    current_working_directory[i] = '\0';
}

void shell_init(void) {
    current_working_directory[0] = '/';
    
    kprint("Type 'help' for available commands.\n\n", 7);
}

// Run the shell main loop
void shell_run(void) {
    char command[MAX_COMMAND_LENGTH];
    
    while (1) {
        // Run NVM scheduler to execute background processes
        nvm_scheduler_tick();
        
        // Print prompt
        kprint("(host)-[", 7);
        kprint(current_working_directory, 2);
        kprint("] ", 7);
        kprint("# ", 2);
        
        // Read command
        keyboard_getline(command, MAX_COMMAND_LENGTH);
        
        // Execute command
        execute_command(command);
    }
}
