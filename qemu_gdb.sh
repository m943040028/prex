#!/bin/sh

qemu-system-ppcemb -nographic -M bamboo -L . -pflash bsp/boot/bootldr -s -S&
powerpc-amcc-elf-insight bsp/boot/bootldr.elf ; pkill qemu-system-ppcemb
