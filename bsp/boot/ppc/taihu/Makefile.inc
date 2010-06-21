
SRCS:=		ppc/taihu/head.S \
		ppc/taihu/resetvec.S \
		ppc/taihu/startup.c \
		ppc/taihu/debug.c

LDSCRIPT:=	ppc/taihu/qemu_bootrom.ld
