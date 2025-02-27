void pause() {
    asm volatile ("hlt");
}