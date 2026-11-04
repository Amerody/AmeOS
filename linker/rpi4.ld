ENTRY(_start)

SECTIONS
{
    . = 0x80000; /* Kernel load address for RPi 4 (64-bit) */

    /* .text : { KEEP(*(.text.boot)) *(.text .text.*) } */

    .text : {
        KEEP(*(.text.boot))
        . = ALIGN(0x800);
        KEEP(*(.text.vectors))
        *(.text .text.*)
    }

    .rodata : { *(.rodata .rodata.*) }
    .data : { *(.data .data.*) }

    .bss (NOLOAD) : {
        . = ALIGN(16);
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(16);
        __bss_end = .;
    }
    
    . = ALIGN(16);
    . += 0x4000; /* 16KB Stack */
    __stack_top = .;
}