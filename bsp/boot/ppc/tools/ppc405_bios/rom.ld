OUTPUT_FORMAT("elf32-powerpc")
OUTPUT_ARCH(powerpc)

SECTIONS {
	. = 0xfffff000;
	.text . : {
		LONG(0);
	}
	. = 0xfffffffc;
	.boot . : {
		*(.boot)
	}
}
