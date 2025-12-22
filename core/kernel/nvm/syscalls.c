// SPDX-License-Identifier: LGPL-3.0-or-later

#include <stddef.h>
#include <core/kernel/nvm/syscall.h>
#include <core/kernel/nvm/nvm.h>
#include <core/kernel/nvm/caps.h>
#include <core/drivers/serial.h>
#include <core/kernel/log.h>
#include <core/kernel/mem.h>
#include <core/fs/vfs.h>

extern uint8_t inb(uint16_t port);
extern void outb(uint16_t port, uint8_t val);

int16_t recipient;
int16_t port;
int8_t value;

typedef struct {
    int16_t recipient;
    int16_t sender;
    int8_t content;
} message_t;

#define MAX_MESSAGES 32
static message_t message_queue[MAX_MESSAGES];
static int message_count = 0;

int32_t syscall_handler(int8_t syscall_id, nvm_process_t* proc) {
    int32_t result = 0;
    char buffer[32];
    
    switch(syscall_id) {
        case SYS_EXIT:
            if(proc->sp >= 1) {
                proc->exit_code = proc->stack[proc->sp - 1];
                itoa(proc->exit_code, buffer, 10);
                LOG_DEBUG("Process %d: exited with code: %s\n", proc->pid, buffer);
            } else {
                proc->exit_code = 0;
            }
            proc->active = false;

            if(proc->sp > 0) proc->sp--;
            break;
        
        case SYS_EXEC:
            LOG_WARN("Process %d: Exec syscall was called. Ignore\n", proc->pid);
            break;

        case SYS_OPEN: {
            if (!caps_has_capability(proc, CAP_FS_READ)) {
                result = -1;
                break;
            }

            if (proc->sp < 1) {
                result = -1;
                break;
            }

            int start_pos = proc->sp;
            int null_pos = -1;
            
            for (int i = proc->sp - 1; i >= 0; i--) {
                if ((proc->stack[i] & 0xFF) == 0) {
                    null_pos = i;
                    break;
                }
            }
            
            if (null_pos == -1) {
                result = -1;
                break;
            }
            
            char filename[MAX_FILENAME];
            int pos = 0;
       
            for (int i = null_pos + 1; i < start_pos && pos < MAX_FILENAME - 1; i++) {
                char ch = proc->stack[i] & 0xFF;
                filename[pos++] = ch;
            }
            
            filename[pos] = '\0';

            proc->sp = null_pos;
            
            LOG_TRACE("Process %d: Opening file '%s'\n", proc->pid, filename);
            
            int fd = vfs_open(filename, VFS_READ | VFS_WRITE);
            
            LOG_TRACE("     vfs_open returned: %d\n", fd);
            
            proc->stack[proc->sp] = fd;
            proc->sp++;
            
            result = 0;
            break;
        }

        case SYS_READ: {
            if (!caps_has_capability(proc, CAP_FS_READ)) {
                result = -1;
                break;
            }

            if (proc->sp < 1) {
                result = -1;
                break;
            }
            
            int32_t fd = proc->stack[proc->sp - 1];
            proc->sp--;
            
            if (fd < 0) {
                result = -1;
            } else {
                char buffer;
                size_t bytes = vfs_readfd(fd, &buffer, 1);
                
                if (bytes == 1) {
                    result = (unsigned char)buffer;
                } else if (bytes == 0) {
                    result = 0;  // EOF
                } else {
                    result = -1;
                }
            }

            proc->stack[proc->sp] = result;
            proc->sp++;
            break;
        }

        case SYS_WRITE: {
            if (!caps_has_capability(proc, CAP_FS_WRITE)) {
                result = -1;
                break;
            }

            if (proc->sp < 2) {
                result = -1;
                break;
            }
            
            int32_t fd = proc->stack[proc->sp - 2];
            int32_t byte_val = proc->stack[proc->sp - 1];
            proc->sp -= 2;
            
            if (fd < 0) {
                result = -1;
            } else if (fd == 1 || fd == 2) {
                char ch = (char)(byte_val & 0xFF);
                char str[2] = {ch, '\0'};
                kprint(str, 15);
                result = 1;
            } else {
                char ch = (char)(byte_val & 0xFF);
                result = vfs_writefd(fd, &ch, 1);
            }
            
            proc->stack[proc->sp] = result;
            proc->sp++;
            break;
        }

        case SYS_MSG_SEND:
                if (proc->sp < 2) {
                    LOG_WARN("Process %d: Stack underflow for msg_send\n", proc->pid);
                    result = -1;
                    break;
                }

                recipient = proc->stack[proc->sp - 2] & 0xFFFF;
                value = proc->stack[proc->sp - 1] & 0xFF;

                if (message_count >= MAX_MESSAGES) {
                    LOG_WARN("Process %d: Message queue full\n", proc->pid);
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
                        LOG_DEBUG("Unblocked process %d due to incoming message\n", recipient);
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
                LOG_DEBUG("Process %d: No messages - blocking\n", proc->pid);
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
                LOG_DEBUG("Process %d: Stack overflow in msg_receive\n", proc->pid);
                result = -1;
                break;
            }

            itoa(proc->pid, buffer, 10);
            LOG_DEBUG("Process %d: Message received\n", proc->pid);
            break;

        case SYS_PORT_IN_BYTE:
            if (!caps_has_capability(proc, CAP_DRV_ACCESS)) {
                LOG_WARN("Process %d: Terminate process - required caps not received\n", proc->pid);
                result = -1;
                break;
            }
            
            if (proc->sp == 0) {
                LOG_WARN("Process %d: Stack empty for inb\n", proc->pid);
                result = -1;
                break;
            }
            port = proc->stack[proc->sp - 1];
            value = inb(port);
            
            proc->stack[proc->sp - 1] = (int16_t)value;
            break;

        case SYS_PORT_OUT_BYTE:
            if (proc->sp < 2) {
                LOG_WARN("Process %d: Stack underflow for outb\n", proc->pid);
                result = -1;
                break;
            }
            
            port = proc->stack[proc->sp - 2] & 0xFFFF;
            value = proc->stack[proc->sp - 1] & 0xFF;

            outb(port, value);
            proc->sp -= 2;
            break;

        case SYS_PRINT:
            if (proc->sp < 1) {
                LOG_WARN("Process %d: Stack underflow for print\n", proc->pid);
                result = -1;
                break;
            }
            
            value = proc->stack[proc->sp - 1] & 0xFF;
            char print_char[2] = {(char)value, 0};
            kprint(print_char, 15);
            proc->sp -= 1;
            break;

        default:
            itoa(syscall_id, buffer, 16);
            LOG_WARN("Process %d: unknown syscall: 0x%s\n", proc->pid, buffer);
            proc->exit_code = -1;
            proc->active = false;
    }
    
    return result;
}