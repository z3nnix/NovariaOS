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
├── build
│   └── boot
│       └── grub
│           └── grub.cfg
├── build.sh
├── core
│   ├── arch
│   │   ├── acpi.c
│   │   ├── boot.asm
│   │   ├── idt.h
│   │   ├── multiboot.h
│   │   ├── pause.c
│   │   └── pause.h
│   ├── drivers
│   │   ├── keymap.h
│   │   ├── ps2.c
│   │   ├── serial.c
│   │   ├── serial.h
│   │   ├── vga.c
│   │   └── vga.h
│   └── kernel
│       ├── kernel.c
│       ├── kstd.c
│       ├── kstd.h
│       ├── mem.c
│       └── mem.h
├── docs
│   ├── Build-from-source.md
│   ├── Our-goals.md
│   └── What-is-Novaria.md
├── lib
│   └── nc
│       ├── stdbool.h
│       ├── stdlib.h
│       └── time.h
├── link.ld
└── website
    ├── docs.html
    ├── index.html
    └── styles.css
```