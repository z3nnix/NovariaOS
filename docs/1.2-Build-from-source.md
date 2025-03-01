# Build from source
To build NovariaOS, first you need to grab the sources from the NovariaOS Github repository:
```shell
[user@pc: ~] $ git clone https://github.com/z3nnix/novariaos
```

You will then need to install all dependencies:
- gcc
- nasm
- libc6-dev
- grub3
- gcc-multilib
- build-essential
- xorriso
- qemu-system-i386

When you have installed all the necessary dependencies, it's time to build the project:
```
[user@pc: ~/novariaos] $ sh build.sh
```