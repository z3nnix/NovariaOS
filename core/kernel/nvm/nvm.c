// SPDX-License-Identifier: LGPL-3.0-or-later

#include <core/kernel/nvm/syscall.h>
#include <core/kernel/kstd.h>
#include <core/drivers/serial.h>
#include <core/kernel/nvm/nvm.h>
#include <core/kernel/nvm/caps.h>

nvm_process_t processes[MAX_PROCESSES];
uint8_t current_process = 0;
uint32_t timer_ticks = 0;

int32_t syscall_handler(uint8_t syscall_id, nvm_process_t* proc);

void nvm_init() {
    for(int i = 0; i < MAX_PROCESSES; i++) {
        processes[i].active = false;
        processes[i].sp = 0;
        processes[i].ip = 0;
        processes[i].exit_code = 0;
        processes[i].caps_count = 0;
    }

    kprint(":: NVM initialized\n", 7);
}

// Signature checking and process creation
int nvm_create_process(uint8_t* bytecode, uint32_t size, uint16_t initial_caps[], uint8_t caps_count) {
    if(bytecode[0] != 0x4E || bytecode[1] != 0x56 || 
       bytecode[2] != 0x4D || bytecode[3] != 0x30) {
        serial_print("Invalid NVM signature\n");
        return -1;
    }
    
    for(int i = 0; i < MAX_PROCESSES; i++) {
        if(!processes[i].active) {
            processes[i].bytecode = bytecode;
            processes[i].ip = 4;
            processes[i].size = size;
            processes[i].sp = 0;
            processes[i].active = true;
            processes[i].exit_code = 0;
            processes[i].pid = i;
            processes[i].caps_count = 0;

            // Initializing capabilities
            for(int j = 0; j < caps_count && j < MAX_CAPS; j++) {
                processes[i].capabilities[j] = initial_caps[j];
            }
            processes[i].caps_count = caps_count;
            
            for(int j = 0; j < MAX_LOCALS; j++) {
                processes[i].locals[j] = 0;
            }
            
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
        // Basic:
        case 0x00: // HALT
            proc->active = false;
            proc->exit_code = 0;
            serial_print("Process halted\n");
            return false;
        
        case 0x01: // NOP
            break;
            
        case 0x02: // PUSH32 (was PUSH_BYTE)
            if(proc->ip + 3 < proc->size) {
                uint32_t value = (proc->bytecode[proc->ip] << 24) |
                                (proc->bytecode[proc->ip + 1] << 16) |
                                (proc->bytecode[proc->ip + 2] << 8) |
                                proc->bytecode[proc->ip + 3];
                proc->ip += 4;
                
                if(proc->sp < STACK_SIZE) {
                    proc->stack[proc->sp++] = (int32_t)value;
                    
                    // debug
                    char dbg[64];
                    serial_print("DEBUG PUSH32: value=0x");
                    itoa(value, dbg, 16);
                    serial_print(dbg);
                    serial_print(" (");
                    itoa((int32_t)value, dbg, 10);
                    serial_print(dbg);
                    serial_print(") at ip=");
                    itoa(proc->ip, dbg, 10);
                    serial_print(dbg);
                    serial_print("\n");
                } else {
                    serial_print("Stack overflow in PUSH32\n");
                    proc->exit_code = -1;
                    proc->active = false;
                    return false;
                }
            } else {
                serial_print("PUSH32: not enough bytes\n");
                proc->exit_code = -1;
                proc->active = false;
                return false;
            }
            break;

        case 0x04: // POP
            if(proc->sp > 0) {
                proc->sp--;
            } else {
                serial_print("Stack underflow in POP\n");
                proc->exit_code = -1;
                proc->active = false;
                return false;
            }
            break;

        case 0x05: // DUP
            if(proc->sp == 0) {
                serial_print("Stack underflow in DUP\n");
                proc->exit_code = -1;
                proc->active = false;
                return false;
            }
            if(proc->sp >= STACK_SIZE) {
                serial_print("Stack overflow in DUP\n");
                proc->exit_code = -1;
                proc->active = false;
                return false;
            }
            
            proc->stack[proc->sp] = proc->stack[proc->sp - 1];
            proc->sp++;
            break;
        
        case 0x06: // SWAP
            if(proc->sp >= 2) {
                int32_t top = proc->stack[proc->sp - 1];
                int32_t second = proc->stack[proc->sp - 2];
                proc->stack[proc->sp - 2] = top;
                proc->stack[proc->sp - 1] = second;
            } else {
                serial_print("Stack underflow in SWAP\n");
                proc->exit_code = -1;
                proc->active = false;
                return false;
            }
            break;

        // Arithmetic:
        case 0x10: // ADD
            if(proc->sp >= 2) {
                int32_t top = proc->stack[proc->sp - 1];
                int32_t second = proc->stack[proc->sp - 2];
                int32_t result = top + second; 
                
                proc->stack[proc->sp - 2] = result;
                proc->sp--;
            } else {
                serial_print("Stack underflow in ADD\n");
                proc->exit_code = -1;
                proc->active = false;
                return false;
            }
            break;

        case 0x11: // SUB
            if(proc->sp >= 2) {
                int32_t top = proc->stack[proc->sp - 1];
                int32_t second = proc->stack[proc->sp - 2];
                int32_t result = second - top;
                
                proc->stack[proc->sp - 2] = result;
                proc->sp--;
            } else {
                serial_print("Stack underflow in SUB\n");
                proc->exit_code = -1;
                proc->active = false;
                return false;
            }
            break;

        case 0x12: // MUL
            if(proc->sp >= 2) {
                int32_t top = proc->stack[proc->sp - 1];
                int32_t second = proc->stack[proc->sp - 2];
                int32_t result = second * top;
                
                proc->stack[proc->sp - 2] = result;
                proc->sp--;
            } else {
                serial_print("Stack underflow in MUL\n");
                proc->exit_code = -1;
                proc->active = false;
                return false;
            }
            break;

        case 0x13: // DIV
            if(proc->sp >= 2) {
                int32_t top = proc->stack[proc->sp - 1];
                int32_t second = proc->stack[proc->sp - 2];
                int32_t result;

                if(top != 0) {
                    result = second / top;
                    proc->stack[proc->sp - 2] = result;
                    proc->sp--;
                } else {
                    serial_print("Zero division\n");
                    proc->exit_code = -1;
                    proc->active = false;
                    return false;
                }
            } else {
                serial_print("Stack underflow in DIV\n");
                proc->exit_code = -1;
                proc->active = false;
                return false;
            }
            break;

        case 0x14: // MOD
            if(proc->sp >= 2) {
                int32_t top = proc->stack[proc->sp - 1];
                int32_t second = proc->stack[proc->sp - 2];
                int32_t result = second % top;
                
                proc->stack[proc->sp - 2] = result;
                proc->sp--;
            } else {
                serial_print("Stack underflow in MOD\n");
                proc->exit_code = -1;
                proc->active = false;
                return false;
            }
            break;
        
        // Comparisons:
        case 0x20: // CMP
            if(proc->sp >= 2) {
                int32_t top = proc->stack[proc->sp - 1];
                int32_t second = proc->stack[proc->sp - 2];
                int32_t result;

                if(second < top) {
                    result = -1;
                } else if (top == second) {
                    result = 0;
                } else {
                    result = 1;
                }
                
                proc->stack[proc->sp - 2] = result;
                proc->sp--;
            } else {
                serial_print("Stack underflow in CMP\n");
                proc->exit_code = -1;
                proc->active = false;
                return false;
            }
            break;

        case 0x21: // EQ
            if(proc->sp >= 2) {
                int32_t top = proc->stack[proc->sp - 1];
                int32_t second = proc->stack[proc->sp - 2];
                int32_t result = (top == second) ? 1 : 0;
                
                proc->stack[proc->sp - 2] = result;
                proc->sp--;
            } else {
                serial_print("Stack underflow in EQ\n");
                proc->exit_code = -1;
                proc->active = false;
                return false;
            }
            break;

        case 0x22: // NEQ
            if(proc->sp >= 2) {
                int32_t top = proc->stack[proc->sp - 1];
                int32_t second = proc->stack[proc->sp - 2];
                int32_t result = (top != second) ? 1 : 0;
                
                proc->stack[proc->sp - 2] = result;
                proc->sp--;
            } else {
                serial_print("Stack underflow in NEQ\n");
                proc->exit_code = -1;
                proc->active = false;
                return false;
            }
            break;

        case 0x23: // GT
            if(proc->sp >= 2) {
                int32_t top = proc->stack[proc->sp - 1];
                int32_t second = proc->stack[proc->sp - 2];
                int32_t result = (second > top) ? 1 : 0;
                
                proc->stack[proc->sp - 2] = result;
                proc->sp--;
            } else {
                serial_print("Stack underflow in GT\n");
                proc->exit_code = -1;
                proc->active = false;
                return false;
            }
            break;

        case 0x24: // LT
            if(proc->sp >= 2) {
                int32_t top = proc->stack[proc->sp - 1];
                int32_t second = proc->stack[proc->sp - 2];
                int32_t result = (second < top) ? 1 : 0;
                
                proc->stack[proc->sp - 2] = result;
                proc->sp--;
            } else {
                serial_print("Stack underflow in LT\n");
                proc->exit_code = -1;
                proc->active = false;
                return false;
            }
            break;

        // Flow control (32-bit addresses):
        case 0x30: // JMP32
            if(proc->ip + 3 < proc->size) {
                uint32_t addr = (proc->bytecode[proc->ip] << 24) |
                               (proc->bytecode[proc->ip + 1] << 16) |
                               (proc->bytecode[proc->ip + 2] << 8) |
                               proc->bytecode[proc->ip + 3];
                proc->ip += 4;
                
                if(addr >= 4 && addr < proc->size) {
                    proc->ip = addr;
                } else {
                    serial_print("JMP32: invalid address\n");
                    proc->exit_code = -1;
                    proc->active = false;
                    return false;
                }
            }
            break;

        case 0x31: // JZ32
            if (proc->sp > 0) {
                int32_t value = proc->stack[--proc->sp];
                if (proc->ip + 3 < proc->size) {
                    uint32_t addr = (proc->bytecode[proc->ip] << 24) |
                                   (proc->bytecode[proc->ip + 1] << 16) |
                                   (proc->bytecode[proc->ip + 2] << 8) |
                                   proc->bytecode[proc->ip + 3];
                    proc->ip += 4;
                    
                    if (value == 0) {
                        if (addr >= 4 && addr < proc->size) {
                            proc->ip = addr;
                        } else {
                            serial_print("JZ32: invalid address\n");
                            proc->exit_code = -1;
                            proc->active = false;
                            return false;
                        }
                    }
                } else {
                    serial_print("JZ32: not enough bytes for address\n");
                    proc->exit_code = -1;
                    proc->active = false;
                    return false;
                }
            } else {
                serial_print("JZ32: stack underflow\n");
                proc->exit_code = -1;
                proc->active = false;
                return false;
            }
            break;

        case 0x32: // JNZ32
            if (proc->sp > 0) {
                int32_t value = proc->stack[--proc->sp];
                if (proc->ip + 3 < proc->size) {
                    uint32_t addr = (proc->bytecode[proc->ip] << 24) |
                                   (proc->bytecode[proc->ip + 1] << 16) |
                                   (proc->bytecode[proc->ip + 2] << 8) |
                                   proc->bytecode[proc->ip + 3];
                    proc->ip += 4;
                    
                    if (value != 0) {
                        if (addr >= 4 && addr < proc->size) {
                            proc->ip = addr;
                        } else {
                            serial_print("JNZ32: invalid address\n");
                            proc->exit_code = -1;
                            proc->active = false;
                            return false;
                        }
                    }
                } else {
                    serial_print("JNZ32: not enough bytes for address\n");
                    proc->exit_code = -1;
                    proc->active = false;
                    return false;
                }
            } else {
                serial_print("JNZ32: stack underflow\n");
                proc->exit_code = -1;
                proc->active = false;
                return false;
            }
            break;

        case 0x33: // CALL32
            if(proc->ip + 3 < proc->size) {
                uint32_t addr = (proc->bytecode[proc->ip] << 24) |
                               (proc->bytecode[proc->ip + 1] << 16) |
                               (proc->bytecode[proc->ip + 2] << 8) |
                               proc->bytecode[proc->ip + 3];
                proc->ip += 4;
                
                if(proc->sp < STACK_SIZE - 1) {
                    proc->stack[proc->sp++] = proc->ip;
                    
                    if(addr >= 4 && addr < proc->size) {
                        proc->ip = addr;
                    } else {
                        serial_print("CALL32: invalid address\n");
                        proc->exit_code = -1;
                        proc->active = false;
                        return false;
                    }
                } else {
                    serial_print("CALL32: stack overflow\n");
                    proc->exit_code = -1;
                    proc->active = false;
                    return false;
                }
            } else {
                serial_print("CALL32: not enough bytes for address\n");
                proc->exit_code = -1;
                proc->active = false;
                return false;
            }
            break;

        case 0x34: // RET
            if(proc->sp > 0) {
                uint32_t return_addr = (uint32_t)proc->stack[--proc->sp];
                
                if(return_addr >= 4 && return_addr < proc->size) {
                    proc->ip = return_addr;
                } else {
                    serial_print("RET: invalid return address\n");
                    proc->exit_code = -1;
                    proc->active = false;
                    return false;
                }
            } else {
                serial_print("RET: stack underflow\n");
                proc->exit_code = -1;
                proc->active = false;
                return false;
            }
            break;

        // Memory:
        case 0x40: // LOAD
            if(proc->ip < proc->size) {
                uint8_t var_index = proc->bytecode[proc->ip++];
                
                if(var_index < MAX_LOCALS) {
                    int32_t value = proc->locals[var_index];
                    
                    if(proc->sp < STACK_SIZE) {
                        proc->stack[proc->sp++] = value;
                    } else {
                        serial_print("LOAD: stack overflow\n");
                        proc->exit_code = -1;
                        proc->active = false;
                        return false;
                    }
                } else {
                    serial_print("LOAD: invalid variable index\n");
                    proc->exit_code = -1;
                    proc->active = false;
                    return false;
                }
            }
            break;

        case 0x41: // STORE
            if(proc->ip < proc->size) {
                uint8_t var_index = proc->bytecode[proc->ip++];
                
                if(var_index < MAX_LOCALS && proc->sp > 0) {
                    int32_t value = proc->stack[--proc->sp];
                    proc->locals[var_index] = value;
                } else {
                    serial_print("STORE: invalid index or stack underflow\n");
                    proc->exit_code = -1;
                    proc->active = false;
                    return false;
                }
            }
            break;

        // Memory absolute access:
        case 0x44: // LOAD_ABS - load from absolute memory address
            if (!caps_has_capability(proc, CAP_DRV_ACCESS)) {
                serial_print("required caps not received\n");
                proc->exit_code = -1;
                proc->active = false;
                return false;
            }

            if(proc->sp > 0) {
                uint32_t addr = (uint32_t)proc->stack[proc->sp - 1]; // get address from stack

                if((addr >= 0x100000 && addr < 0xFFFFFFFF) || 
                (addr >= 0xB8000 && addr <= 0xB8FA0)) {
                    int32_t value = *(int32_t*)addr;
                    proc->stack[proc->sp - 1] = value;

                    // Debug
                    char dbg[64];
                    serial_print("DEBUG LOAD_ABS: addr=0x");
                    itoa(addr, dbg, 16);
                    serial_print(dbg);
                    serial_print(" -> value=0x");
                    itoa(value, dbg, 16);
                    serial_print(dbg);
                    serial_print("\n");
                } else {
                    serial_print("LOAD_ABS: invalid memory address\n");
                    proc->exit_code = -1;
                    proc->active = false;
                    return false;
                }
            } else {
                serial_print("LOAD_ABS: stack underflow\n");
                proc->exit_code = -1;
                proc->active = false;
                return false;
            }
            break;

        case 0x45: // STORE_ABS - store to absolute memory address
            if (!caps_has_capability(proc, CAP_DRV_ACCESS)) {
                serial_print("required caps not received\n");
                proc->exit_code = -1;
                proc->active = false;
                return false;
            }

            if(proc->sp >= 2) {
                uint32_t addr = (uint32_t)proc->stack[proc->sp - 2]; // address
                int32_t value = proc->stack[proc->sp - 1]; // value

                if((addr >= 0x100000 && addr < 0xFFFFFFFF) || 
                (addr >= 0xB8000 && addr <= 0xB8FA0)) {
                    *(int32_t*)addr = value;
                    proc->sp -= 2;
                    
                    // Debug
                    char dbg[64];
                    serial_print("DEBUG STORE_ABS: addr=0x");
                    itoa(addr, dbg, 16);
                    serial_print(dbg);
                    serial_print(" <- value=0x");
                    itoa(value, dbg, 16);
                    serial_print(dbg);
                    serial_print("\n");
                } else {
                    serial_print("STORE_ABS: invalid memory address\n");
                    proc->exit_code = -1;
                    proc->active = false;
                    return false;
                }
            } else {
                serial_print("STORE_ABS: stack underflow\n");
                proc->exit_code = -1;
                proc->active = false;
                return false;
            }
            break;

        // System calls:
        case 0x51: // BREAK
            serial_print("BREAK: debug stop\n");
            char dbg[64];
            serial_print("IP: ");
            itoa(proc->ip, dbg, 10);
            serial_print(dbg);
            serial_print(", SP: ");
            itoa(proc->sp, dbg, 10);
            serial_print(dbg);
            serial_print("\n");
            break;
            
        case 0x50: // SYSCALL
            if(proc->ip < proc->size) {
                uint8_t syscall_id = proc->bytecode[proc->ip++];
                syscall_handler(syscall_id, proc);
            }
            break;
            
        default:
            char buffer[32];
            serial_print("Unknown opcode: 0x");
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
    uint8_t original = current_process;

    do {
        current_process = (current_process + 1) % MAX_PROCESSES;
        if(processes[current_process].active && !processes[current_process].blocked) {
            break;
        }
    } while(current_process != start);
    
    if(processes[current_process].active && !processes[current_process].blocked) {
        if (processes[current_process].ip < processes[current_process].size) {
            nvm_execute_instruction(&processes[current_process]);
        } else {
            serial_print("Process ");
            char buffer[32];
            itoa(processes[current_process].pid, buffer, 10);
            serial_print(buffer);
            serial_print(" reached end of code - terminating\n");
            processes[current_process].active = false;
            processes[current_process].exit_code = 0;
        }
    } else {
        current_process = original;
    }
}

void nvm_execute(uint8_t* bytecode, uint32_t size, uint16_t* capabilities, uint8_t caps_count) {
    int pid = nvm_create_process(bytecode, size, capabilities, caps_count);
    if(pid >= 0) {
        char buffer[32];
        serial_print("NVM 32-bit process started with PID: ");
        itoa(pid, buffer, 10);
        serial_print(buffer);
        serial_print(" | CAPS: ");
        
        for(int i = 0; i < caps_count; i++) {
            itoa(capabilities[i], buffer, 16);
            serial_print("0x");
            serial_print(buffer);
            if(i < caps_count - 1) serial_print(", ");
        }
        serial_print("\n");
    }
}

// Function for get exit code
int32_t nvm_get_exit_code(uint8_t pid) {
    if(pid < MAX_PROCESSES && !processes[pid].active) {
        return processes[pid].exit_code;
    }
    return -1;
}

// Function for check process activity
bool nvm_is_process_active(uint8_t pid) {
    if(pid < MAX_PROCESSES) {
        return processes[pid].active;
    }
    return false;
}