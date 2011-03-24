#!/bin/sh

qemu-system-ppcemb -nographic -M taihu \
-bios bsp/boot/ppc/tools/ppc405_bios/ppc405_rom.bin \
-net user -net nic,model=e1000,macaddr=52:54:00:12:34:56 \
-redir tcp:8080::8080 -kernel prexos
