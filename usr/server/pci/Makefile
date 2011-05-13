include $(SRCDIR)/mk/own.mk

DEFS+=		_STANDALONE

PROG=		pci
OBJS=		main.o

#DISASM= 	pow.lst
#SYMBOL= 	pow.sym

ifeq ($(CONFIG_PPC_40x), y)
OBJS+=		plat/40x/ppc4xx_pci.o
endif
LIBS=		$(LIBSDIR)/libserver.a \
		$(LIBSDIR)/libuio.a

include $(SRCDIR)/mk/prog.mk
