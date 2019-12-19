MEMORY {
	rom (rx)  : ORIGIN = 0, LENGTH = 256K
	/* stack occupies the last 1K; see below */
	ram (w)   : ORIGIN = 0x20000000, LENGTH = 15K
}

SECTIONS {
	/DISCARD/ : {
		*(.ARM.exidx)
	}
	.rom : {
		/*
		 * this MUST be first ; it's the code to pull the CPU out of the reset state
		 * newlib doesn't seem to provide it, that's okay, I can compensate
		 */
		*(.vectors)
		/* these can be in whatever order */
		*(.text)
		*(.rodata*)
		*(COMMON)
		*(.init)
		*(.fini)
		*(.text.startup)
		*(.eh_frame)
		/* this MUST be last. if it is not last, you will get a linker error. */
		__data_start_rom = .;
	} >rom
	.data : AT (__data_start_rom) {
		/* this is expected to perfectly match __data_start_rom */
		__data_start__ = .;
		*(.data)
		__data_end__ = .;
	} >ram
	.ram : {
		/* newlib handles zeroing this */
		__bss_start__ = .;
		*(.bss)
		__bss_end__ = .;
		__heap_start__ = .;
	} >ram
	__heap_end__ = 0x20003C00;
	__stack_start__ = 0x20003C00;
	__stack_end__ = 0x20004000;
}

