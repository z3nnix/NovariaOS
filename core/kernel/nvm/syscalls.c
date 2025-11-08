#include <core/kernel/nvm/nvm.h>
#include <core/drivers/serial.h>

#define SYS_EXIT 0x00

// Syscalls
int32_t syscall_handler(uint8_t syscall_id, nvm_process_t* proc) {
    int32_t result = 0;
    int32_t arg1;
    char buffer[32];
    
    switch(syscall_id) {
        case SYS_EXIT:
            serial_print("Stack before exit: ");
            for(int i = 0; i < proc->sp; i++) {
                itoa(proc->stack[i], buffer, 10);
                serial_print(buffer);
                serial_print(" ");
            }
            serial_print("\n");
            if(proc->sp >= 1) {
                proc->exit_code = proc->stack[proc->sp - 1];
                serial_print("Process exited with code: ");
                itoa(proc->exit_code, buffer, 10);
                serial_print(buffer);
                serial_print("\n");
            } else {
                proc->exit_code = 0;
            }
            proc->active = false;
            // Delete argument from stack
            if(proc->sp > 0) proc->sp--;
            break;
            
        default:
            serial_print("Unknown syscall: ");
            itoa(syscall_id, buffer, 10);
            serial_print(buffer);
            serial_print("\n");
            proc->exit_code = -1;
            proc->active = false;
    }
    
    return result;
}
