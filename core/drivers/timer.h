#ifndef _VGA_H_
#define _VGA_H_

#include <stdint.h>
#include <core/kernel/kstd.h>

extern void pit_init();
extern void outb(uint16_t port, uint8_t val);
extern void nvm_scheduler_tick();

#endif