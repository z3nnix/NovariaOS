#pragma once

#include <stdint.h>
#include <core/kernel/kstd.h>

void pit_init();
extern void outb(uint16_t port, uint8_t val);
extern void nvm_scheduler_tick();
extern void pit_polling_loop();
