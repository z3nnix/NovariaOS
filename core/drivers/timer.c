#include <core/drivers/timer.h>
#include <core/arch/idt.h>

void pit_init() {
    uint16_t divisor = 1193182 / 1000; // 1000 Hz
    outb(0x43, 0x36);
    outb(0x40, divisor & 0xFF);
    outb(0x40, divisor >> 8);
    kprint(":: PIT Setup\n", 7);
}

void pit_polling_loop() {
    static uint16_t last_count = 0xFFFF;
    while (1) {
        // Read current value PIT timer
        outb(0x43, 0x00);
        uint8_t low = inb(0x40);
        uint8_t high = inb(0x40);
        uint16_t count = (high << 8) | low;

        if (count > last_count) {
            nvm_scheduler_tick();
        }
        last_count = count;
    }
}
