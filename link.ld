/* Entry point */
ENTRY(_start)

/* Библиотека Limine */
PROVIDE(hhdm = 0xffff800000000000);

SECTIONS
{
    /* Ядро загружается по адресу в высшей половине */
    . = 0xffffffff80000000;

    /* Мультизагрузочный заголовок (не нужен для Limine, но оставляем) */
    .multiboot : {
        *(.multiboot)
    }

    .text : {
        *(.text)
        *(.text.*)
    }

    .rodata : ALIGN(0x1000) {
        *(.rodata)
        *(.rodata.*)
    }

    .data : ALIGN(0x1000) {
        *(.data)
        *(.data.*)
    }

    .bss : ALIGN(0x1000) {
        *(COMMON)
        *(.bss)
        *(.bss.*)
    }

    /DISCARD/ : {
        *(.comment)
        *(.note.*)
    }
}