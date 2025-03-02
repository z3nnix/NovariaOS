#ifndef NVM_H
#define NVM_H

#include <core/kernel/kstd.h>
#include <core/drivers/serial.h>
#include <stddef.h>

extern void nvm_init();
static void init_commands(void);
static void parse_command(const char* line);
static int my_atoi(const char* str);
static void my_strcpy(char* dest, const char* src);
static void my_strcat(char* dest, const char* src);
extern void clearscreen(void);

static const char apps[] = "";

#endif