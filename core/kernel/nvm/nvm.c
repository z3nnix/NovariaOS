#include <core/kernel/kstd.h>
#include <core/drivers/serial.h>
#include <core/kernel/nvm/nvm.h>

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

            for(int j = 0; j < sizeof(processes[i].locals)/sizeof(processes[i].locals[0]); j++) {
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
            serial_print("NVM: NOP called\n");
            break;
            
        case 0x02: // PUSH_BYTE
            if(proc->ip < proc->size) {
                uint8_t value = proc->bytecode[proc->ip++];
                proc->stack[proc->sp++] = (int32_t)value;
                
                // debug
                char dbg[64];
                serial_print("DEBUG PUSH_BYTE: value=");
                itoa(value, dbg, 10);
                serial_print(dbg);
                serial_print(" at ip=");
                itoa(proc->ip, dbg, 10);
                serial_print(dbg);
                serial_print("\n");
            }
            break;
            
        case 0x03: // PUSH_SHORT (instrumented, little-endian)
            if (proc->ip + 1 < proc->size) {
                // read bytes
                uint8_t low  = proc->bytecode[proc->ip++];
                uint8_t high = proc->bytecode[proc->ip++];

                // debug print: show raw bytes and ip/sp
                char dbg[128];
                serial_print("DEBUG PUSH_SHORT read: low=0x");
                itoa(low, dbg, 16); serial_print(dbg);
                serial_print(" high=0x");
                itoa(high, dbg, 16); serial_print(dbg);
                serial_print(" ip:"); itoa(proc->ip, dbg, 10); serial_print(dbg);
                serial_print(" sp:"); itoa(proc->sp, dbg, 10); serial_print(dbg);
                serial_print("\n");

                uint16_t u16 = (uint16_t)(((uint16_t)high << 8) | (uint16_t)low);
                int32_t value = (int32_t)u16; // no sign extension

                // debug value
                serial_print("DEBUG PUSH_SHORT value (u16) = ");
                itoa(u16, dbg, 10); serial_print(dbg);
                serial_print(" (hex 0x"); itoa(u16, dbg, 16); serial_print(dbg);
                serial_print(") -> stored as ");
                itoa(value, dbg, 10); serial_print(dbg);
                serial_print("\n");

                proc->stack[proc->sp++] = value;
            } else {
                serial_print("PUSH_SHORT: not enough bytes\n");
                proc->exit_code = -1;
                proc->active = false;
                return false;
            }
            break;


        case 0x04: // POP
            if(proc->sp > 0) {
                int32_t popped_value = proc->stack[--proc->sp];
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
            if(proc->sp >= sizeof(proc->stack)/sizeof(proc->stack[0])) {
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
                
                // Debug
                char dbg[64];
                serial_print("DEBUG ADD: ");
                itoa(second, dbg, 10); serial_print(dbg);
                serial_print(" + ");
                itoa(top, dbg, 10); serial_print(dbg);
                serial_print(" = ");
                itoa(result, dbg, 10); serial_print(dbg);
                serial_print("\n");
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
                
                // Debug
                char dbg[64];
                serial_print("DEBUG SUB: ");
                itoa(second, dbg, 10); serial_print(dbg);
                serial_print(" - ");
                itoa(top, dbg, 10); serial_print(dbg);
                serial_print(" = ");
                itoa(result, dbg, 10); serial_print(dbg);
                serial_print("\n");
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
                
                // Debug
                char dbg[64];
                serial_print("DEBUG MUL: ");
                itoa(second, dbg, 10); serial_print(dbg);
                serial_print(" * ");
                itoa(top, dbg, 10); serial_print(dbg);
                serial_print(" = ");
                itoa(result, dbg, 10); serial_print(dbg);
                serial_print("\n");
            } else {
                serial_print("Stack underflow in SUB\n");
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
                    
                    // Debug
                    char dbg[64];
                    serial_print("DEBUG DIV: ");
                    itoa(second, dbg, 10); serial_print(dbg);
                    serial_print(" / ");
                    itoa(top, dbg, 10); serial_print(dbg);
                    serial_print(" = ");
                    itoa(result, dbg, 10); serial_print(dbg);
                    serial_print("\n");
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
                
                // Debug
                char dbg[64];
                serial_print("DEBUG MOD: ");
                itoa(second, dbg, 10); serial_print(dbg);
                serial_print(" % ");
                itoa(top, dbg, 10); serial_print(dbg);
                serial_print(" = ");
                itoa(result, dbg, 10); serial_print(dbg);
                serial_print("\n");
            } else {
                serial_print("Stack underflow in MOD\n");
                proc->exit_code = -1;
                proc->active = false;
                return false;
            }
            break;
        
        // Сomparisons:
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
                
                // Debug
                char dbg[64];
                serial_print("DEBUG CMP: ");
                itoa(top, dbg, 10); serial_print(dbg);
                serial_print(" | ");
                itoa(second, dbg, 10); serial_print(dbg);
                serial_print(" | ");
                itoa(result, dbg, 10); serial_print(dbg);
                serial_print("\n");
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
                int32_t result;

                if(top == second) {
                    result = 1;
                } else {
                    result = 0;
                }
                
                proc->stack[proc->sp - 2] = result;
                proc->sp--;
                
                // Debug
                char dbg[64];
                serial_print("DEBUG EQ: ");
                itoa(top, dbg, 10); serial_print(dbg);
                serial_print(" | ");
                itoa(second, dbg, 10); serial_print(dbg);
                serial_print(" | ");
                itoa(result, dbg, 10); serial_print(dbg);
                serial_print("\n");
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
                int32_t result;

                if(top != second) {
                    result = 1;
                } else {
                    result = 0;
                }
                
                proc->stack[proc->sp - 2] = result;
                proc->sp--;
                
                // Debug
                char dbg[64];
                serial_print("DEBUG NEQ: ");
                itoa(top, dbg, 10); serial_print(dbg);
                serial_print(" | ");
                itoa(second, dbg, 10); serial_print(dbg);
                serial_print(" | ");
                itoa(result, dbg, 10); serial_print(dbg);
                serial_print("\n");
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
                int32_t result;

                if(top < second) {
                    result = 1;
                } else {
                    result = 0;
                }
                
                proc->stack[proc->sp - 2] = result;
                proc->sp--;
                
                // Debug
                char dbg[64];
                serial_print("DEBUG GT: ");
                itoa(top, dbg, 10); serial_print(dbg);
                serial_print(" | ");
                itoa(second, dbg, 10); serial_print(dbg);
                serial_print(" | ");
                itoa(result, dbg, 10); serial_print(dbg);
                serial_print("\n");
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
                int32_t result;

                if(top > second) {
                    result = 1;
                } else {
                    result = 0;
                }
                
                proc->stack[proc->sp - 2] = result;
                proc->sp--;
                
                // Debug
                char dbg[64];
                serial_print("DEBUG LT: ");
                itoa(top, dbg, 10); serial_print(dbg);
                serial_print(" | ");
                itoa(second, dbg, 10); serial_print(dbg);
                serial_print(" | ");
                itoa(result, dbg, 10); serial_print(dbg);
                serial_print("\n");
            } else {
                serial_print("Stack underflow in LT\n");
                proc->exit_code = -1;
                proc->active = false;
                return false;
            }
            break;

        // Flow control:
        case 0x30: // JMP
            if(proc->ip + 1 < proc->size) {
                uint8_t low = proc->bytecode[proc->ip++];
                uint8_t high = proc->bytecode[proc->ip++];
                uint16_t addr = (uint16_t)((high << 8) | low);
                
                char dbg[64];
                serial_print("DEBUG JMP: raw_addr=");
                itoa(addr, dbg, 10);
                serial_print(dbg);
                serial_print(", current_ip=");
                itoa(proc->ip, dbg, 10);
                serial_print(dbg);
                serial_print("\n");
                
                if(addr >= 4 && addr < proc->size) {
                    proc->ip = addr;
                    
                    serial_print("DEBUG JMP to: ");
                    itoa(addr, dbg, 10);
                    serial_print(dbg);
                    serial_print("\n");
                } else {
                    serial_print("JMP: invalid address\n");
                    proc->exit_code = -1;
                    proc->active = false;
                    return false;
                }
            }
            break;
        case 0x31: // JZ
            if (proc->sp > 0) {
                int32_t value = proc->stack[--proc->sp]; // pop value
                if (proc->ip + 1 < proc->size) {
                    uint8_t low = proc->bytecode[proc->ip++];
                    uint8_t high = proc->bytecode[proc->ip++];
                    uint16_t addr = (uint16_t)((high << 8) | low);
                    
                    if (value == 0) {
                        if (addr >= 4 && addr < proc->size) {
                            proc->ip = addr;

                            // Debug
                            char dbg[64];
                            serial_print("DEBUG JZ to: ");
                            itoa(addr, dbg, 10);
                            serial_print(dbg);
                            serial_print("\n");
                        } else {
                            serial_print("JZ: invalid address\n");
                            proc->exit_code = -1;
                            proc->active = false;
                            return false;
                        }
                    }
                } else {
                    serial_print("JZ: not enough bytes for address\n");
                    proc->exit_code = -1;
                    proc->active = false;
                    return false;
                }
            } else {
                serial_print("JZ: stack underflow\n");
                proc->exit_code = -1;
                proc->active = false;
                return false;
            }
            break;

        case 0x32: // JNZ
            if (proc->sp > 0) {
                int32_t value = proc->stack[--proc->sp]; // pop before test
                if (proc->ip + 1 < proc->size) {
                    uint8_t low = proc->bytecode[proc->ip++];
                    uint8_t high = proc->bytecode[proc->ip++];
                    uint16_t addr = (uint16_t)((high << 8) | low);
                    
                    if (value != 0) {
                        if (addr >= 4 && addr < proc->size) {
                            proc->ip = addr;

                            // Debug
                            char dbg[64];
                            serial_print("DEBUG JNZ to: ");
                            itoa(addr, dbg, 10);
                            serial_print(dbg);
                            serial_print("\n");
                        } else {
                            serial_print("JNZ: invalid address\n");
                            proc->exit_code = -1;
                            proc->active = false;
                            return false;
                        }
                    }
                } else {
                    serial_print("JNZ: not enough bytes for address\n");
                    proc->exit_code = -1;
                    proc->active = false;
                    return false;
                }
            } else {
                serial_print("JNZ: stack underflow\n");
                proc->exit_code = -1;
                proc->active = false;
                return false;
            }
            break;

        case 0x33: // CALL
            if(proc->ip + 1 < proc->size) {
                uint8_t low = proc->bytecode[proc->ip++];
                uint8_t high = proc->bytecode[proc->ip++];
                uint16_t addr = (uint16_t)((high << 8) | low);
                
                // Instruction pointer out of range
                if(proc->sp < sizeof(proc->stack)/sizeof(proc->stack[0]) - 1) {
                    proc->stack[proc->sp++] = proc->ip;
                    
                    if(addr >= 4 && addr < proc->size) {
                        proc->ip = addr;
                        
                        // Debug
                        char dbg[64];
                        serial_print("DEBUG CALL to: ");
                        itoa(addr, dbg, 10);
                        serial_print(dbg);
                        serial_print(", return to: ");
                        itoa(proc->ip, dbg, 10);
                        serial_print(dbg);
                        serial_print("\n");
                    } else {
                        serial_print("CALL: invalid address\n");
                        proc->exit_code = -1;
                        proc->active = false;
                        return false;
                    }
                } else {
                    serial_print("CALL: stack overflow\n");
                    proc->exit_code = -1;
                    proc->active = false;
                    return false;
                }
            } else {
                serial_print("CALL: not enough bytes for address\n");
                proc->exit_code = -1;
                proc->active = false;
                return false;
            }
            break;

        case 0x34: // RET
            if(proc->sp > 0) {
                uint16_t return_addr = (uint16_t)proc->stack[--proc->sp]; // pop return address
                
                // Debug
                char dbg[64];
                serial_print("DEBUG RET: return_addr=");
                itoa(return_addr, dbg, 10);
                serial_print(dbg);
                serial_print(", sp=");
                itoa(proc->sp, dbg, 10);
                serial_print(dbg);
                serial_print("\n");
                
                if(return_addr >= 4 && return_addr < proc->size) {
                    proc->ip = return_addr;
                    
                    serial_print("DEBUG RET to: ");
                    itoa(return_addr, dbg, 10);
                    serial_print(dbg);
                    serial_print("\n");
                } else {
                    serial_print("RET: invalid return address ");
                    itoa(return_addr, dbg, 10);
                    serial_print(dbg);
                    serial_print("\n");
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
        case 0x40: // LOAD xx
            if(proc->ip < proc->size) {
                uint8_t var_index = proc->bytecode[proc->ip++];
                
                // Check bounds
                if(var_index < sizeof(proc->locals)/sizeof(proc->locals[0])) {
                    int32_t value = proc->locals[var_index];
                    
                    // Check stack overflow
                    if(proc->sp < sizeof(proc->stack)/sizeof(proc->stack[0])) {
                        proc->stack[proc->sp++] = value;
                        
                        // Debug
                        char dbg[64];
                        serial_print("DEBUG LOAD: locals[");
                        itoa(var_index, dbg, 10);
                        serial_print(dbg);
                        serial_print("] = ");
                        itoa(value, dbg, 10);
                        serial_print(dbg);
                        serial_print("\n");
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

        case 0x41: // STORE xx
            if(proc->ip < proc->size) {
                uint8_t var_index = proc->bytecode[proc->ip++];
                
                // Check bounds and that stack has a value
                if(var_index < sizeof(proc->locals)/sizeof(proc->locals[0]) && proc->sp > 0) {
                    int32_t value = proc->stack[--proc->sp]; // pop value
                    proc->locals[var_index] = value;
                    
                    // Debug
                    char dbg[64];
                    serial_print("DEBUG STORE: locals[");
                    itoa(var_index, dbg, 10);
                    serial_print(dbg);
                    serial_print("] = ");
                    itoa(value, dbg, 10);
                    serial_print(dbg);
                    serial_print("\n");
                } else {
                    serial_print("STORE: invalid index or stack underflow\n");
                    proc->exit_code = -1;
                    proc->active = false;
                    return false;
                }
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
