#include <stdint.h>
#include <stdbool.h>

#ifndef _NVM_H
#define _NVM_H

#define MAX_PROCESSES 32768
#define TIME_SLICE_MS 2

// NVM procces structure
typedef struct {
    uint8_t* bytecode;      //  Bytecode pointer
    uint32_t ip;            //  Instruction Pointer
    int32_t stack[256];     //  Data stack
    uint16_t sp;            //  Stack Pointer
    uint16_t local_vars[64]; // Local variables
    bool active;            //  Procces is active?
    uint32_t size;          //  Bytecode size
    int32_t exit_code;      //  Exit code
} nvm_process_t;

extern nvm_process_t processes[MAX_PROCESSES];
extern uint8_t current_process;
extern uint32_t timer_ticks;

void nvm_init();
void nvm_execute(uint8_t* bytecode, uint32_t size);
void nvm_scheduler_tick();

#endif