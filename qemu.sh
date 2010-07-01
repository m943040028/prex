#!/bin/sh

qemu-system-ppcemb -nographic -M bamboo -L . -bios bsp/boot/bootldr
