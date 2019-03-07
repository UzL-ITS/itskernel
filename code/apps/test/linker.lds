
OUTPUT_FORMAT(elf64-x86-64)
ENTRY(main)

PAGE_SIZE  = 0x1000;

SECTIONS
{
	. = PAGE_SIZE;

	INIT_TEXT_START = .;
    .text : AT(0x1000)
    {
        *(.text)
        *(.rodata)
    }

    .data ALIGN(0x1000) :
    {
        INIT_DATA_START = .;
        *(.data)
    }

    .rodata ALIGN(0x1000) :
    {
        INIT_DATA_START = .;
        *(.rodata)
    }

    .bss ALIGN(0x1000) :
    {
        INIT_BSS_START = .;
        *(.bss)
        *(COMMON)
    }
    INIT_BSS_END = .;
}
