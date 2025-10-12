OUTPUT_FORMAT("elf64-x86-64")
ENTRY(kmain)

SECTIONS
{
    . = 0x20000;

    .text : {
        *(.text)
    }

    .rodata : {
        *(.rodata)
    }

    .data : {
        *(.data)
    }

    .bss : {
        *(.bss)
        *(COMMON)
    }
}
