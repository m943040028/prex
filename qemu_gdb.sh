#!/bin/sh

qemu-system-ppcemb -nographic -M taihu \
-bios bsp/boot/ppc/tools/ppc405_bios/ppc405_rom.bin \
-kernel prexos -s -S&
powerpc-amcc-elf-gdb sys/prex ; pkill qemu-system-ppcemb
