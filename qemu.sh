#!/bin/sh

qemu-system-ppcemb -nographic -M bamboo -L . -pflash bsp/boot/bootldr
