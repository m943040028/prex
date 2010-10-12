#!/bin/sh

qemu-system-ppcemb -nographic -M bamboo -kernel prexos -s -S&
<<<<<<< HEAD
powerpc-unknown-elf-gdb sys/prex ; pkill qemu-system-ppcemb
=======
powerpc-amcc-elf-gdb sys/prex ; pkill qemu-system-ppcemb
>>>>>>> 6bbc0f265d9db2270e7cc3f32a640a430690ac5e
