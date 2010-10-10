#!/bin/sh

qemu-system-ppcemb -nographic -M bamboo -kernel prexos -s -S&
powerpc-unknown-elf-gdb sys/prex ; pkill qemu-system-ppcemb
