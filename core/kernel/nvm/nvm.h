#include <stdint.h>
#include <stdbool.h>

#ifndef _NVM_H
#define _NVM_H

#define MAX_PROCESSES 32768
#define TIME_SLICE_MS 2
#define MAX_CAPS 16

#define MAX_PROCESSES 32768
#define TIME_SLICE_MS 2
#define MAX_CAPS 16

// NVM process structure
typedef struct {
    uint8_t* bytecode;      // Bytecode pointer
    uint32_t ip;            // Instruction Pointer
    int32_t stack[256];     // Data stack
    uint16_t sp;            // Stack Pointer
    uint16_t local_vars[64]; // Local variables
    bool active;            // Process is active?
    uint32_t size;          // Bytecode size
    int32_t exit_code;      // Exit code

    int32_t locals[256];    // 256 local variables (index 0-256)

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

#endif