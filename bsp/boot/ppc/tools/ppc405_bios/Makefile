include $(SRCDIR)/mk/own.mk

TARGET:=	ppc405_rom.bin
OBJS:=		rom.o
LDSCRIPT:=	rom.ld

include $(SRCDIR)/mk/common.mk

$(TARGET): $(OBJS) $(LDSCRIPT)
	$(LD) $(LDFLAGS) $(OUTPUT_OPTION) $(OBJS)
	$(OBJCOPY) -O binary -R .note -R .comment $@
