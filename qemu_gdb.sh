#!/bin/sh

qemu-system-ppcemb -nographic -M bamboo -kernel prexos -s -S&
powerpc-amcc-elf-gdb sys/prex ; pkill qemu-system-ppcemb
