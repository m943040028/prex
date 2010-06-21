#!/bin/sh

qemu-system-ppcemb -nographic -M taihu -L . -pflash bsp/boot/bootldr -s -S
