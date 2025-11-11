#include <core/kernel/nvm/syscall.h>
#include <core/kernel/nvm/nvm.h>
#include <core/kernel/nvm/caps.h>
#include <core/drivers/serial.h>

extern uint8_t inb(uint16_t port);
extern void outb(uint16_t port, uint8_t val);

uint16_t port;
uint8_t value;

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

        case SYS_PORT_IN_BYTE:
            if (!caps_has_capability(proc, CAP_DRV_ACCESS)) {
                serial_print("required caps not received\n");
                result = -1;
                break;
            }

            serial_print("DEBUG syscall inb: sp=");
            itoa(proc->sp, buffer, 10);
            serial_print(buffer);
            serial_print(" stack contents: ");
            for(int i = 0; i < proc->sp; i++) {
                itoa(proc->stack[i], buffer, 16);
                serial_print("0x");
                serial_print(buffer);
                serial_print(" ");
            }
            serial_print("\n");
            
            if (proc->sp == 0) {
                serial_print("ERROR: Stack empty for inb\n");
                result = -1;
                break;
            }
            port = proc->stack[proc->sp - 1];
            
            serial_print("DEBUG: Reading from port 0x");
            itoa(port, buffer, 16);
            serial_print(buffer);
            serial_print("\n");
            
            value = inb(port);
            
            serial_print("DEBUG: inb returned: 0x");
            itoa(value, buffer, 16);
            serial_print(buffer);
            serial_print("\n");
            
            proc->stack[proc->sp - 1] = (int16_t)value;
            break;

        case SYS_PORT_OUT_BYTE:
            if (proc->sp < 2) {
                serial_print("Stack underflow for outb\n");
                result = -1;
                break;
            }
            
            port = proc->stack[proc->sp - 2] & 0xFFFF;
            value = proc->stack[proc->sp - 1] & 0xFF;
            
            serial_print("DEBUG outb: port=0x");
            itoa(port, buffer, 16);
            serial_print(buffer);
            serial_print(" value=0x");
            itoa(value, buffer, 16);
            serial_print(buffer);
            serial_print("\n");
            
            outb(port, value);
            proc->sp -= 2;
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

/*
EXAMPLE OF CHECK CAPS IN SYSCALL:

            if (!caps_has_capability(proc, CAP_ALL)) {
                serial_print("required caps not received\n");
                result = -1;
                break;
            }

*/