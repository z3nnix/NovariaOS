#include <stdint.h>
#include <stdbool.h>

#ifndef _NVM_H
#define _NVM_H

#define MAX_PROCESSES 32768
#define TIME_SLICE_MS 2
#define MAX_CAPS 16
#define STACK_SIZE 256
#define MAX_LOCALS 256

// NVM process structure
typedef struct {
    int8_t* bytecode;      // Bytecode pointer
    int32_t ip;            // Instruction Pointer
    int32_t stack[STACK_SIZE];     // Data stack
    int32_t sp;            // Stack Pointer (changed to 32-bit)
    bool active;            // Process is active?
    int32_t size;          // Bytecode size
    int32_t exit_code;      // Exit code

    int32_t locals[MAX_LOCALS];    // Local variables

    // CAPS
    int16_t capabilities[MAX_CAPS];  // list of caps
    int8_t caps_count;               // count active caps
    int8_t pid;                      // Process ID
    
    // Message system
    bool blocked;           // Process blocked waiting for message
    int8_t wakeup_reason;  // Reason for wakeup
} nvm_process_t;

extern nvm_process_t processes[MAX_PROCESSES];
extern int8_t current_process;
extern int32_t timer_ticks;

void nvm_init();
void nvm_execute(int8_t* bytecode, int32_t size, int16_t* capabilities, int8_t caps_count);
void nvm_scheduler_tick();
bool nvm_is_process_active(int8_t pid);
int32_t nvm_get_exit_code(int8_t pid);

#endif