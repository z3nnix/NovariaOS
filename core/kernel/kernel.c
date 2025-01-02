#include <string.h>
#include <stdint.h>

#include <core/kernel/idt.h>
#include <core/kernel/kstd.h>

extern void acpi_off();
void kmain() {
    kprint("NovariaOS. 0.0.3 indev build. TG: ", 15);
    kprint("@NovariaOS\n", 9);

    acpi_off();
}