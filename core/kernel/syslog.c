// SPDX-License-Identifier: LGPL-3.0-or-later

#include <core/kernel/syslog.h>
#include <core/kernel/kstd.h>
#include <core/drivers/serial.h>
#include <usr/bin/vfs.h>

#define MAX_LOG_SIZE 4000

static char log_buffer[MAX_LOG_SIZE];
static size_t log_size = 0;

static int syslog_strlen(const char* str) {
    int len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

void syslog_init(void) {
    log_buffer[0] = '\0';
    log_size = 0;
    
    const char* init_msg = "=== NovariaOS System Log ===\n";
    int len = syslog_strlen(init_msg);
    
    for (int i = 0; i < len && log_size < MAX_LOG_SIZE - 1; i++) {
        log_buffer[log_size++] = init_msg[i];
    }
    log_buffer[log_size] = '\0';
    vfs_create("/var/log/system.log", log_buffer, log_size);
}

void syslog_write(const char* message) {
    if (!message) return;
    
    int len = syslog_strlen(message);
    
    for (int i = 0; i < len && log_size < MAX_LOG_SIZE - 1; i++) {
        log_buffer[log_size++] = message[i];
    }
    log_buffer[log_size] = '\0';
    
    serial_print(message);
    
    vfs_create("/var/log/system.log", log_buffer, log_size);
}

void syslog_print(const char* message, int color) {
    kprint(message, color);
    syslog_write(message);
}
