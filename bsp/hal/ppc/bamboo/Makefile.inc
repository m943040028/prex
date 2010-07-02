
SRCS:=		ppc/arch/locore.S \
		ppc/arch/cpufunc.S \
		ppc/arch/trap.c \
		ppc/arch/context.c \
		ppc/arch/cpu.c \
		ppc/arch/clock.c \
		ppc/bamboo/io.c \
		ppc/bamboo/machdep.c \
		ppc/bamboo/interrupt.c

ifeq ($(CONFIG_MMU),y)
SRCS+=		ppc/arch/mmu_oea.c
endif
ifeq ($(DEBUG),1)
SRCS+=		ppc/bamboo/diag.c
endif
