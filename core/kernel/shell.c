// SPDX-License-Identifier: LGPL-3.0-or-later

#include <core/kernel/shell.h>
#include <core/kernel/kstd.h>
#include <core/drivers/keyboard.h>
#include <core/kernel/vge/fb.h>
#include <core/kernel/mem.h>
#include <core/fs/initramfs.h>
#include <core/fs/iso9660.h>
#include <core/fs/vfs.h>
#include <core/kernel/nvm/nvm.h>
#include <core/kernel/nvm/caps.h>
#include <core/kernel/userspace.h>

#define MAX_COMMAND_LENGTH 256
#define MAX_PATH_LENGTH 64

static char current_working_directory[MAX_PATH_LENGTH] = "/";

static int strcmp(const char* str1, const char* str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(unsigned char*)str1 - *(unsigned char*)str2;
}


static void cmd_help(void) {
    kprint("\nBuilt-in commands:\n", 10);
    kprint("  help     - Show this help message\n", 7);
    kprint("  memtest  - Test memory allocation\n", 7);
    kprint("  list     - List loaded NVM programs\n", 7);
    kprint("  progs    - List userspace programs\n", 7);
    kprint("  pwd      - Print working directory\n", 7);
    kprint("  ls       - List directory contents\n", 7);
    kprint("  cat      - Display file contents\n", 7);
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

static void cmd_memtest(void) {
    kprint("\nTesting memory allocation...\n", 7);
    
    void* ptr1 = allocateMemory(128);
    if (ptr1) {
        kprint("Allocated 128 bytes at ", 7);
        char buf[32];
        itoa((unsigned int)(unsigned long)ptr1, buf, 16);
        kprint(buf, 7);
        kprint("\n", 7);
        freeMemory(ptr1);
        kprint("Freed memory\n", 7);
    } else {
        kprint("Failed to allocate memory\n", 12);
    }
    kprint("\n", 7);
}

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

static void cmd_progs(void) {
    kprint("\n", 7);
    userspace_list();
    kprint("\n", 7);
}

static void cmd_isols(const char* args) {
    if (!iso9660_is_initialized()) {
        kprint("\nISO9660 filesystem is not initialized\n\n", 14);
        return;
    }
    
    const char* path = args;
    while (*path == ' ') path++;
    
    if (*path == '\0') {
        path = "/";
    }
    
    kprint("\n", 7);
    iso9660_list_dir(path);
    kprint("\n", 7);
}

static void cmd_cat(const char* args) {
    const char* path = args;
    while (*path == ' ') path++;

    if (*path == '\0') {
        kprint("\nUsage: cat <filename>\n\n", 12);
        return;
    }

    size_t size;
    const char* data = vfs_read(path, &size);

    if (data == NULL) {
        kprint("\nError: File '", 12);
        kprint(path, 12);
        kprint("' not found\n", 12);
        return;
    }

    kprint("\n", 7);
    for (size_t i = 0; i < size; i++) {
        char c[2] = {data[i], '\0'};
        if (data[i] == '\n') {
            kprint("\n", 7);
        } else {
            kprint(c, 15);
        }
    }
    kprint("\n", 7);
}

static void cmd_ls(const char* args) {
    const char* path = args;
    while (*path == ' ') path++;

    if (*path == '\0') {
        path = current_working_directory;
    }

    vfs_file_t* files = vfs_get_files();
    int dir_len = strlen(path);

    char normalized_dir[64];
    size_t i = 0;
    while (path[i] && i < 63) {
        normalized_dir[i] = path[i];
        i++;
    }
    normalized_dir[i] = '\0';

    if (dir_len > 1 && normalized_dir[dir_len - 1] == '/') {
        normalized_dir[dir_len - 1] = '\0';
        dir_len--;
    }

    int found_count = 0;
    kprint("\n", 7);

    for (size_t i = 0; i < 128; i++) {
        if (!files[i].used) continue;

        const char* name = files[i].name;
        bool should_show = false;
        const char* display_name = name;

        if (dir_len == 1 && normalized_dir[0] == '/') {
            if (name[0] == '/' && name[1] != '\0') {
                int slash_count = 0;
                for (int j = 1; name[j] != '\0'; j++) {
                    if (name[j] == '/') slash_count++;
                }
                if (slash_count == 0) {
                    should_show = true;
                    display_name = name + 1;
                }
            }
        } else {
            size_t name_len = strlen(name);

            if (name_len > dir_len && name[dir_len] == '/') {
                bool match = true;
                for (size_t j = 0; j < dir_len; j++) {
                    if (name[j] != normalized_dir[j]) {
                        match = false;
                        break;
                    }
                }
                if (match) {
                    size_t slash_count = 0;
                    for (size_t j = dir_len + 1; name[j] != '\0'; j++) {
                        if (name[j] == '/') slash_count++;
                    }
                    if (slash_count == 0) {
                        should_show = true;
                        display_name = name + dir_len + 1;
                    }
                }
            }
        }

        if (should_show) {
            found_count++;
            if (files[i].type == VFS_TYPE_DIR) {
                kprint(display_name, 9);
                kprint("/", 9);
            } else {
                kprint(display_name, 7);
            }
            kprint("    ", 7);
        }
    }

    if (found_count == 0) {
        kprint("(empty)", 7);
    }

    kprint("\n\n", 7);
}

static void cmd_isocat(const char* args) {
    if (!iso9660_is_initialized()) {
        kprint("\nISO9660 filesystem is not initialized\n\n", 14);
        return;
    }

    const char* path = args;
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

static int parse_command(const char* command, char* argv[], int max_args) {
    int argc = 0;
    static char cmd_buf[MAX_COMMAND_LENGTH];
    
    int i = 0;
    while (command[i] && i < MAX_COMMAND_LENGTH - 1) {
        cmd_buf[i] = command[i];
        i++;
    }
    cmd_buf[i] = '\0';
    
    char* p = cmd_buf;
    while (*p && argc < max_args) {
        while (*p == ' ') p++;
        
        if (*p == '\0') break;
        
        argv[argc++] = p;
        
        while (*p && *p != ' ') p++;
        
        if (*p) {
            *p = '\0';
            p++;
        }
    }
    
    return argc;
}

static int should_delay_prompt = 0;
static int delay_ticks = 0;

static void execute_command(const char* command) {
    while (*command == ' ') command++;
    
    if (*command == '\0') {
        return;
    }
    
    char* argv[16];
    int argc = parse_command(command, argv, 16);
    
    if (argc == 0) return;
    
    if (strcmp(argv[0], "help") == 0) {
        cmd_help();
    } else if (strcmp(argv[0], "memtest") == 0) {
        cmd_memtest();
    } else if (strcmp(argv[0], "list") == 0) {
        cmd_list();
    } else if (strcmp(argv[0], "progs") == 0) {
        cmd_progs();
    } else if (strcmp(argv[0], "pwd") == 0) {
        kprint(current_working_directory, 11);
        kprint("\n", 7);
    } else if (strcmp(argv[0], "ls") == 0) {
        if (argc > 1) {
            cmd_ls(argv[1]);
        } else {
            cmd_ls("");
        }
    } else if (strcmp(argv[0], "cat") == 0) {
        if (argc > 1) {
            cmd_cat(argv[1]);
        } else {
            kprint("\nUsage: cat <filename>\n\n", 12);
        }
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
        char bin_path[64];
        int len = strlen(argv[0]);
        
        bin_path[0] = '/';
        bin_path[1] = 'b';
        bin_path[2] = 'i';
        bin_path[3] = 'n';
        bin_path[4] = '/';
        
        for (size_t i = 0; i < len && i < 58; i++) {
            bin_path[5 + i] = argv[0][i];
        }
        
        bin_path[5 + len] = '.';
        bin_path[6 + len] = 'b';
        bin_path[7 + len] = 'i';
        bin_path[8 + len] = 'n';
        bin_path[9 + len] = '\0';
        
        if (vfs_exists(bin_path)) {
            size_t size;
            const char* data = vfs_read(bin_path, &size);
            
            if (data && size > 0) {
                should_delay_prompt = 1;
                delay_ticks = 20;
                nvm_execute((int8_t*)data, size, (int16_t[]){CAP_ALL}, 1);
                return;
            } else {
                kprint("Error: Failed to read program file\n", 12);
            }
        } else if (userspace_exists(argv[0])) {
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
    size_t i = 0;
    while (path[i] != '\0' && i < MAX_PATH_LENGTH - 1) {
        current_working_directory[i] = path[i];
        i++;
    }
    current_working_directory[i] = '\0';
}

void shell_init(void) {
    current_working_directory[0] = '/';
    should_delay_prompt = 0;
    delay_ticks = 0;
    
    kprint("Type 'help' for available commands.\n\n", 7);
}

void shell_run(void) {
    char command[MAX_COMMAND_LENGTH];
    
    while (1) {
        nvm_scheduler_tick();
        
        if (should_delay_prompt) {
            if (delay_ticks > 0) {
                delay_ticks--;
                continue;
            } else {
                should_delay_prompt = 0;
            }
        }
        
        kprint("(host)-[", 7);
        kprint(current_working_directory, 2);
        kprint("] ", 7);
        kprint("# ", 2);
        
        keyboard_getline(command, MAX_COMMAND_LENGTH);
        
        execute_command(command);
    }
}