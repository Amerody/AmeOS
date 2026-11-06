ENTRY(_start)

SECTIONS
{
    . = 0x80000; /* Kernel load address for RPi 4 (64-bit) */

<<<<<<< HEAD
    .text : { KEEP(*(.text.boot)) *(.text .text.*) }
    .rodata : { *(.rodata .rodata.*) }
    .data : { *(.data .data.*) }
=======
    /* .text : { KEEP(*(.text.boot)) *(.text .text.*) } */

    .text : {
        KEEP(*(.text.boot))
        . = ALIGN(0x800);
        KEEP(*(.text.vectors))
        *(.text .text.*)
    }

    .rodata : { *(.rodata .rodata.*) }
    .data : { *(.data .data.*) }

>>>>>>> homeserver/Dev
    .bss (NOLOAD) : {
        . = ALIGN(16);
        __bss_start = .;
        *(.bss .bss.*)
<<<<<<< HEAD
        __bss_end = .;
    }
=======
        *(COMMON)
        . = ALIGN(16);
        __bss_end = .;
    }
    
>>>>>>> homeserver/Dev
    . = ALIGN(16);
    . += 0x4000; /* 16KB Stack */
    __stack_top = .;
}