OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)
SECTIONS
{
	. = 0x10000 + SIZEOF_HEADERS ;

	.text : {
		*(.text)
	} = 0x9090

	. = ALIGN(32);

	.rodata : {
		*(.rodata)
		*(.rodata.*)
	}

	. = ALIGN (0x1000);
	.data : {
		*(.data)
	}

	.bss . : {
		*(.bss)
		*(COMMON)
	}

	/DISCARD/ : { *(.comment .note) }
}
