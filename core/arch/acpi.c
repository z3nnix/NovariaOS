void acpi_off() {
    unsigned short port = 0x604;
    unsigned short value = 0x2000;
    asm volatile ("outw %0, %1" : : "a"(value), "Nd"(port));
    while (1) {
        asm volatile ("hlt");
    }
} // https://t.me/novariaos/11