#ifndef PANIC_H
#define PANIC_H

inline static void panic(const char* message) {
    asm volatile ("cli");
    kprint("KERNEL PANIC: ", 4);
    kprint(message, 4);
    kprint("\n", 4);

    for (;;) {
        asm volatile("hlt");
    }
}

#endif // PANIC_H