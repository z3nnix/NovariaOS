#include <core/kernel/nvm/nvm.h>

nvm_process_t processes[MAX_PROCESSES];
uint8_t current_process = 0;
uint32_t timer_ticks = 0;

void nvm_init() {
    for(int i = 0; i < MAX_PROCESSES; i++) {
        processes[i].active = false;
        processes[i].sp = 0;
        processes[i].ip = 0;
        processes[i].exit_code = 0;
    }
    kprint(":: NVM initialized\n", 7);
}

// Signature checking and process creation
int nvm_create_process(uint8_t* bytecode, uint32_t size) {
    if(bytecode[0] != 0x4E || bytecode[1] != 0x56 || 
       bytecode[2] != 0x4D || bytecode[3] != 0x30) {
        serial_print("Invalid NVM signature\n");
        return -1;
    }
    
    // Search free slot
    for(int i = 0; i < MAX_PROCESSES; i++) {
        if(!processes[i].active) {
            processes[i].bytecode = bytecode;
            processes[i].ip = 4;
            processes[i].size = size;
            processes[i].sp = 0;
            processes[i].active = true;
            processes[i].exit_code = 0;
            
            char buffer[32];
            serial_print("Process created with PID: ");
            itoa(i, buffer, 10);
            serial_print(buffer);
            serial_print("\n");
            return i;
        }
    }
    
    serial_print("No free process slots\n");
    return -1;
}

// Execute one instruction
bool nvm_execute_instruction(nvm_process_t* proc) {
    if(proc->ip >= proc->size) {
        serial_print("Instruction pointer out of bounds\n");
        proc->exit_code = -1;
        proc->active = false;
        return false;
    }
    
    uint8_t opcode = proc->bytecode[proc->ip++];
    
    switch(opcode) {
        case 0x00: // HALT
            proc->active = false;
            proc->exit_code = 0;
            serial_print("Process halted\n");
            return false;
            
        case 0x02: // PUSH_BYTE
            if(proc->ip < proc->size) {
                proc->stack[proc->sp++] = (int32_t)proc->bytecode[proc->ip++];
            }
            break;
            
        case 0x03: // PUSH_SHORT
            if(proc->ip + 1 < proc->size) {
                uint8_t low = proc->bytecode[proc->ip++];
                uint8_t high = proc->bytecode[proc->ip++];
                int32_t value = (high << 8) | low;
                // Sign extend for 16-bit value
                if(value & 0x8000) value |= 0xFFFF0000;
                proc->stack[proc->sp++] = value;
            }
            break;
            
        case 0x04: // PUSH_INT
            if(proc->ip + 3 < proc->size) {
                int32_t value = 0;
                for(int i = 0; i < 4; i++) {
                    value |= (proc->bytecode[proc->ip++] << (i * 8));
                }
                proc->stack[proc->sp++] = value;
            }
            break;
            
        case 0x50: // SYSCALL
            if(proc->ip < proc->size) {
                uint8_t syscall_id = proc->bytecode[proc->ip++];
                int32_t syscall_result = syscall_handler(syscall_id, proc);
                
                // For syscalls, which return argument - push that to stack
                /* EXAMPLE:
                if(syscall_id == SYS_GETC) {
                     proc->stack[proc->sp++] = syscall_result;
                 } 
                */
            }
            break;
            
        default:
            char buffer[32];
            serial_print("Unknown opcode: ");
            itoa(opcode, buffer, 16);
            serial_print(buffer);
            serial_print("\n");
            proc->exit_code = -1;
            proc->active = false;
            return false;
    }
    
    return true;
}

// Round Robin task manager
void nvm_scheduler_tick() {
    timer_ticks++;
    if(timer_ticks % TIME_SLICE_MS != 0) {
        return;
    }
    
    uint8_t start = current_process;
    do {
        current_process = (current_process + 1) % MAX_PROCESSES;
        if(processes[current_process].active) {
            break;
        }
    } while(current_process != start);
    
    if(processes[current_process].active) {
        nvm_execute_instruction(&processes[current_process]);
    }
}

void nvm_execute(uint8_t* bytecode, uint32_t size) {
    int pid = nvm_create_process(bytecode, size);
    if(pid >= 0) {
        char buffer[32];
        serial_print("NVM process started with PID: ");
        itoa(pid, buffer, 10);
        serial_print(buffer);
        serial_print("\n");
    }
}

// Function for get exit code
int32_t nvm_get_exit_code(uint8_t pid) {
    if(pid < MAX_PROCESSES && !processes[pid].active) {
        return processes[pid].exit_code;
    }
    return -1; // Procces still active or not exist
}

// Function for check process activity
bool nvm_is_process_active(uint8_t pid) {
    if(pid < MAX_PROCESSES) {
        return processes[pid].active;
    }
    return false;
}