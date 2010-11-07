
SRCS:=		ppc/arch/locore_44x.S \
		ppc/arch/cpufunc.S \
		ppc/arch/trap.c \
		ppc/arch/context.c \
		ppc/arch/cpu.c \
		ppc/arch/clock_booke.c \
		ppc/bamboo/io.c \
		ppc/bamboo/machdep.c \
		ppc/bamboo/interrupt.c \
		ppc/arch/mmu_booke.c

ifeq ($(DEBUG),1)
SRCS+=		ppc/bamboo/diag.c
endif
