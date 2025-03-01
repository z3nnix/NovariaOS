#include <core/drivers/timer.h>

void pit_init() {
    uint16_t divisor = 1193182 / 1000; // 1000 Hz
    outb(0x43, 0x36);
    outb(0x40, divisor & 0xFF);
    outb(0x40, divisor >> 8);

    kprint(":: PIT Setup\n", 7);
}