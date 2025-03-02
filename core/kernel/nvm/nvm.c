#include <core/kernel/nvm/nvm.h>

#define MAX_CALLS      10
#define MAX_ARG_LEN    64
#define ERROR_BUF_LEN  32

typedef void (*command_handler_t)(const char* arg);

typedef struct {
    int id;
    command_handler_t handler;
} command_entry_t;

static command_entry_t commands[MAX_CALLS];
static char shared_buffer[MAX_ARG_LEN];
static bool initialized = false;

static int my_isdigit(char c) {
    return c >= '0' && c <= '9';
}

static int my_isspace(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static int my_atoi(const char* str) {
    int res = 0;
    while (*str && my_isdigit(*str)) {
        res = res * 10 + (*str - '0');
        str++;
    }
    return res;
}

static void my_strcpy(char* dest, const char* src) {
    while (*src) *dest++ = *src++;
    *dest = '\0';
}

static void my_strcat(char* dest, const char* src) {
    while (*dest) dest++;
    my_strcpy(dest, src);
}

void nvm_init() {
    if (initialized) return;
    
    kprint(":: NVM subsystem initialized\n", 0x07);
    init_commands();
    initialized = true;
}

void nvm_execute(const char* command) {
    if (!initialized) {
        serial_print("ERROR: NVM Subsystem not initialized\n");
        return;
    }
    
    if (command == NULL) {
        serial_print("ERROR: Null instruction\n");
        return;
    }
    
    parse_command(command);
}

static void call1_handler(const char* arg) {
    if (arg) {
        kprint(arg, 0x07);
        kprint("\n", 0x07);
    }
}

static void call2_handler(const char* arg) {
    (void)arg;
    clearscreen();
}

static void init_commands() {
    for (int i = 0; i < MAX_CALLS; i++) {
        commands[i].id = 0;
        commands[i].handler = NULL;
    }
    
    commands[0] = (command_entry_t){1, call1_handler};
    commands[1] = (command_entry_t){2, call2_handler};
}

static command_handler_t find_handler(int call_id) {
    for (int i = 0; i < MAX_CALLS; i++) {
        if (commands[i].id == call_id) {
            return commands[i].handler;
        }
    }
    return NULL;
}

static void parse_command(const char* line) {
    const char* ptr = line;
    int call_id = 0;
    
    while (*ptr && !my_isdigit(*ptr)) ptr++;
    if (!*ptr) return;
    
    call_id = my_atoi(ptr);
    
    while (*ptr && *ptr != ':') ptr++;
    
    char* arg = NULL;
    if (*ptr == ':') {
        ptr++;
        while (*ptr && (my_isspace(*ptr) || *ptr == '"')) ptr++;
        
        int i = 0;
        while (*ptr && *ptr != '"' && i < MAX_ARG_LEN-1) {
            shared_buffer[i++] = *ptr++;
        }
        shared_buffer[i] = '\0';
        arg = shared_buffer;
    }
    
    command_handler_t handler = find_handler(call_id);
    if (handler) {
        handler(arg);
    } else {
        char error_buf[ERROR_BUF_LEN];
        my_strcpy(error_buf, "NVM ERROR: Unknown call ");
        char num_buf[16];
        itoa(call_id, num_buf, 10);
        my_strcat(error_buf, num_buf);
        my_strcat(error_buf, "\n");
        serial_print(error_buf);
    }
}