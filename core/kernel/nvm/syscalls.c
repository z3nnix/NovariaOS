// SPDX-License-Identifier: LGPL-3.0-or-later

#include <stddef.h>
#include <core/kernel/nvm/syscall.h>
#include <core/kernel/nvm/nvm.h>
#include <core/kernel/nvm/caps.h>
#include <core/drivers/serial.h>
#include <core/kernel/log.h>
#include <core/kernel/mem.h>
// VFS is now in userspace - kernel syscalls don't need it directly

extern uint8_t inb(uint16_t port);
extern void outb(uint16_t port, uint8_t val);

uint16_t recipient;
uint16_t port;
uint8_t value;

typedef struct {
    uint16_t recipient;
    uint16_t sender;
    uint8_t content;
} message_t;


#define MAX_MESSAGES 32
static message_t message_queue[MAX_MESSAGES];
static int message_count = 0;

// Syscalls
int32_t syscall_handler(uint8_t syscall_id, nvm_process_t* proc) {
    int32_t result = 0;
    int32_t arg1;
    char buffer[32];
    
    switch(syscall_id) {
        case SYS_EXIT:
            if(proc->sp >= 1) {
                proc->exit_code = proc->stack[proc->sp - 1];
                itoa(proc->exit_code, buffer, 10);
                LOG_DEBUG("Procces %d: exited with code: %s\n", proc->pid, buffer);
            } else {
                proc->exit_code = 0;
            }
            proc->active = false;
            // Delete argument from stack
            if(proc->sp > 0) proc->sp--;
            break;
        
        case SYS_EXEC:
            LOG_WARN("Procces %d: Exec syscall was called. Ignore");
            break;

        case SYS_MSG_SEND:
            if (proc->sp < 2) {
                LOG_WARN("Procces %d: Stack underflow for msg_send\n", proc->pid);
                result = -1;
                break;
            }

            recipient = proc->stack[proc->sp - 2] & 0xFFFF;
            value = proc->stack[proc->sp - 1] & 0xFF;

            if (message_count >= MAX_MESSAGES) {
                LOG_WARN("Procces %d: Message queue full\n", proc->pid);
                result = -1;
                break;
            }

            message_t msg;
            msg.recipient = recipient;
            msg.sender = proc->pid;
            msg.content = value;
            
            message_queue[message_count] = msg;
            message_count++;

            for (int i = 0; i < MAX_PROCESSES; i++) {
                if (processes[i].active && processes[i].pid == recipient && processes[i].blocked) {
                    processes[i].blocked = false; 
                    processes[i].wakeup_reason = 1;

                    LOG_DEBUG("Unblocked procces %d due to incoming message", buffer);
                    break;
                }
            }

            proc->sp -= 2;
            break;
            
        case SYS_MSG_RECEIVE:
            int found_index = -1;
            for (int i = 0; i < message_count; i++) {
                if (message_queue[i].recipient == proc->pid) {
                    found_index = i;
                    break;
                }
            }
            
            if (found_index == -1) {
                serial_print("No messages for process ");
                itoa(proc->pid, buffer, 10);
                serial_print(buffer);
                serial_print(" - blocking process\n");
                LOG_DEBUG("Procces %d: No messages for process - blocking process\n", proc->pid);
                proc->blocked = true;
                result = -1;
                break;
            }
            
            message_t received_msg = message_queue[found_index];
            
            for (int i = found_index; i < message_count - 1; i++) {
                message_queue[i] = message_queue[i + 1];
            }
            message_count--;

            if (proc->sp + 1 < 256) {
                proc->stack[proc->sp] = received_msg.sender;
                proc->stack[proc->sp + 1] = received_msg.content;
                proc->sp += 2;
            } else {
                LOG_DEBUG("Procces %d: Stack overflow in msg_receive\n", proc->pid);
                result = -1;
                break;
            }

            itoa(proc->pid, buffer, 10);
            LOG_DEBUG("Procces %d: Message received. sender=%s. Unblock procces\n", proc->pid, buffer);
            break;

        case SYS_PORT_IN_BYTE:
            if (!caps_has_capability(proc, CAP_DRV_ACCESS)) {
                LOG_WARN("Procces %d: Terminate procces - required caps not received.\n", proc->pid);
                result = -1;
                break;
            }
            
            if (proc->sp == 0) {
                LOG_WARN("Procces %d: Stack empty for inb\n", proc->pid);
                result = -1;
                break;
            }
            port = proc->stack[proc->sp - 1];
            value = inb(port);
            
            proc->stack[proc->sp - 1] = (int16_t)value;
            break;

        case SYS_PORT_OUT_BYTE:
            if (proc->sp < 2) {
                LOG_WARN("Procces %d: Stack underflow for outb\\n", proc->pid);
                result = -1;
                break;
            }
            
            port = proc->stack[proc->sp - 2] & 0xFFFF;
            value = proc->stack[proc->sp - 1] & 0xFF;

            outb(port, value);
            proc->sp -= 2;
            break;

        case SYS_PRINT:
            // Print a single character using kprint
            if (proc->sp < 1) {
                LOG_WARN("Procces %d: Stack underflow for print\\n", proc->pid);
                result = -1;
                break;
            }
            
            value = proc->stack[proc->sp - 1] & 0xFF;
            char print_char[2] = {(char)value, 0};
            kprint(print_char, 15);
            proc->sp -= 1;
            break;

        case SYS_CREATE:
            // Create a file: filename_addr, data_addr, size
            if (proc->sp < 3) {
                LOG_WARN("Procces %d: Stack underflow for create\\n", proc->pid);
                result = -1;
                break;
            }
            
            uint32_t filename_addr = (uint32_t)proc->stack[proc->sp - 3];
            uint32_t data_addr = (uint32_t)proc->stack[proc->sp - 2];
            uint32_t file_size = (uint32_t)proc->stack[proc->sp - 1];
            
            // Check if addresses are valid (simple validation)
            if (filename_addr < 0x100000 || data_addr < 0x100000) {
                LOG_WARN("Procces %d: Invalid address for create\\n", proc->pid);
                result = -1;
                proc->sp -= 3;
                break;
            }
            
            // VFS is now in userspace - syscalls should be handled there
            LOG_WARN("Procces %d: SYS_CREATE not implemented in kernel (VFS moved to userspace)\\n", proc->pid);
            result = -1;
            
            proc->sp -= 3;
            if (proc->sp < STACK_SIZE) {
                proc->stack[proc->sp++] = result;
            }
            break;

        case SYS_WRITE:
            // Write to existing file: filename_addr, data_addr, size
            if (proc->sp < 3) {
                LOG_WARN("Procces %d: Stack underflow for write\\n", proc->pid);
                result = -1;
                break;
            }
            
            filename_addr = (uint32_t)proc->stack[proc->sp - 3];
            data_addr = (uint32_t)proc->stack[proc->sp - 2];
            file_size = (uint32_t)proc->stack[proc->sp - 1];
            
            if (filename_addr < 0x100000 || data_addr < 0x100000) {
                LOG_WARN("Procces %d: Invalid address for write\\n", proc->pid);
                result = -1;
                proc->sp -= 3;
                break;
            }
            
            // VFS is now in userspace
            LOG_WARN("Procces %d: SYS_WRITE not implemented in kernel (VFS moved to userspace)\\n", proc->pid);
            result = -1;
            
            proc->sp -= 3;
            if (proc->sp < STACK_SIZE) {
                proc->stack[proc->sp++] = result;
            }
            break;

        case SYS_READ:
            // Read file: filename_addr, buffer_addr, maxsize
            if (proc->sp < 3) {
                LOG_WARN("Procces %d: Stack underflow for read\\n", proc->pid);
                result = -1;
                break;
            }
            
            filename_addr = (uint32_t)proc->stack[proc->sp - 3];
            uint32_t buffer_addr = (uint32_t)proc->stack[proc->sp - 2];
            uint32_t max_size = (uint32_t)proc->stack[proc->sp - 1];
            
            if (filename_addr < 0x100000 || buffer_addr < 0x100000) {
                LOG_WARN("Procces %d: Invalid address for read\\n", proc->pid);
                result = -1;
                proc->sp -= 3;
                break;
            }
            
            // VFS is now in userspace
            LOG_WARN("Procces %d: SYS_READ not implemented in kernel (VFS moved to userspace)\\n", proc->pid);
            result = -1;
            
            proc->sp -= 3;
            if (proc->sp < STACK_SIZE) {
                proc->stack[proc->sp++] = result;
            }
            break;

        case SYS_DELETE:
            // Delete file: filename_addr
            if (proc->sp < 1) {
                LOG_WARN("Procces %d: Stack underflow for delete\\n", proc->pid);
                result = -1;
                break;
            }
            
            filename_addr = (uint32_t)proc->stack[proc->sp - 1];
            
            if (filename_addr < 0x100000) {
                LOG_WARN("Procces %d: Invalid address for delete\\n", proc->pid);
                result = -1;
                proc->sp -= 1;
                break;
            }
            
            // VFS is now in userspace
            LOG_WARN("Procces %d: SYS_DELETE not implemented in kernel (VFS moved to userspace)\\n", proc->pid);
            result = -1;
            
            proc->sp -= 1;
            if (proc->sp < STACK_SIZE) {
                proc->stack[proc->sp++] = result;
            }
            break;

        default:
            itoa(syscall_id, buffer, 10);
            LOG_WARN("Procces %d: unknown syscall", proc->pid);
            proc->exit_code = -1;
            proc->active = false;
    }
    
    return result;
}