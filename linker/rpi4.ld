SECTIONS
{
    . = 0x80000; /* Kernel load address for RPi 4 (64-bit) */

    .text : { KEEP(*(.text.boot)) *(.text .text.*) }
    .rodata : { *(.rodata .rodata.*) }
    .data : { *(.data .data.*) }
    .bss (NOLOAD) : {
        . = ALIGN(16);
        __bss_start = .;
        *(.bss .bss.*)
        __bss_end = .;
    }
    . = ALIGN(16);
    . += 0x4000; /* 16KB Stack */
    __stack_top = .;
}