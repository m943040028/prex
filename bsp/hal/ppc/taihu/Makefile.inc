
SRCS:=		ppc/arch/locore_40x.S \
		ppc/arch/cpufunc.S \
		ppc/arch/trap.c \
		ppc/arch/context.c \
		ppc/arch/cpu.c \
		ppc/arch/40x/clock.c \
		ppc/arch/40x/mmu.c \
		ppc/arch/dcr-low.S \
		ppc/platform/40x/interrupt.c \
		ppc/platform/40x/io.c \
		ppc/taihu/machdep.c

ifeq ($(DEBUG),1)
SRCS+=		ppc/platform/40x/diag.c
endif
