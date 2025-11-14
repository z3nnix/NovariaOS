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
    uint8_t* bytecode;      // Bytecode pointer
    uint32_t ip;            // Instruction Pointer
    int32_t stack[STACK_SIZE];     // Data stack
    uint32_t sp;            // Stack Pointer (changed to 32-bit)
    bool active;            // Process is active?
    uint32_t size;          // Bytecode size
    int32_t exit_code;      // Exit code

    int32_t locals[MAX_LOCALS];    // Local variables

    // CAPS
    uint16_t capabilities[MAX_CAPS];  // list of caps
    uint8_t caps_count;               // count active caps
    uint8_t pid;                      // Process ID
    
    // Message system
    bool blocked;           // Process blocked waiting for message
    uint8_t wakeup_reason;  // Reason for wakeup
} nvm_process_t;

extern nvm_process_t processes[MAX_PROCESSES];
extern uint8_t current_process;
extern uint32_t timer_ticks;

void nvm_init();
void nvm_execute(uint8_t* bytecode, uint32_t size, uint16_t* capabilities, uint8_t caps_count);
void nvm_scheduler_tick();
bool nvm_is_process_active(uint8_t pid);
int32_t nvm_get_exit_code(uint8_t pid);

#endif