# NovariaOS Source:
```
  _   _                      _        ___  ____  
 | \ | | _____   ____ _ _ __(_) __ _ / _ \/ ___| 
 |  \| |/ _ \ \ / / _` | '__| |/ _` | | | \___ \ 
 | |\  | (_) \ V / (_| | |  | | (_| | |_| |___) |
 |_| \_|\___/ \_/ \__,_|_|  |_|\__,_|\___/|____/ 
```
A UNIX-like OS written from scratch.
Our mission is to create a complete operating system. Our operating system can be used as a subject of study.
Our source code is open to the public,
and anyone can check it for security risks.
We welcome such testing.
Follow us on social media! TG: [@novariaos](https://t.me/novariaos)

# Source-tree:
```
NovariaOS
├── LICENSE
├── README.md
├── apps
│   ├── adder.asm
│   ├── compartest.asm
│   ├── flowcontroltest.asm
│   ├── iotest.asm
│   └── memtest.asm
├── build
│   └── boot
│       ├── grub
│       │   └── grub.cfg
│       ├── initramfs
│       └── kernel.bin
├── chorus.build
├── core
│   ├── arch
│   │   ├── acpi.c
│   │   ├── boot.asm
│   │   ├── idt.h
│   │   ├── multiboot.h
│   │   ├── pause.c
│   │   └── pause.h
│   ├── drivers
│   │   ├── serial.c
│   │   ├── serial.h
│   │   ├── timer.c
│   │   ├── timer.h
│   │   ├── vga.c
│   │   └── vga.h
│   ├── fs
│   │   ├── initramfs.c
│   │   ├── initramfs.h
│   │   ├── ramfs.c
│   │   └── ramfs.h
│   └── kernel
│       ├── kernel.c
│       ├── kstd.c
│       ├── kstd.h
│       ├── mem.c
│       ├── mem.h
│       └── nvm
│           ├── caps.c
│           ├── caps.h
│           ├── nvm.c
│           ├── nvm.h
│           ├── syscall.h
│           └── syscalls.c
├── docs
│   ├── 1.0-What-is-Novaria.md
│   ├── 1.1-Our-goals.md
│   ├── 1.2-Build-from-source.md
│   ├── 2.0-What-is-NVM.md
│   ├── 2.1-Bytecode.md
│   ├── 2.2-Syscalls.md
│   ├── 2.3-Example-program.md
│   ├── 2.4-Caps.md
│   └── 3.0-What-is-RamFS.md
├── initramfs-rebuild.rb
├── lib
│   └── nc
│       ├── stdbool.h
│       ├── stdlib.h
│       └── time.h
├── link.ld
├── qemu.log
└── website
    ├── docs.html
    ├── docsStyles.css
    ├── index.html
    └── styles.css
```