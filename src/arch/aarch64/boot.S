// AArch64 mode

// To keep this in the first portion of the binary.
.section ".text.boot"

// Make _start global.
.global _start

// Setup for the EL1 drop (clobbers x0)
.macro prepare_el1
    mov     x0, #(1 << 31)          // HCR_EL2.RW = 1 (EL1 is AArch64)
    msr     hcr_el2, x0
    mov     x0, #3                  // EL1 can use physical counter/timer
    msr     cnthctl_el2, x0
    msr     cntvoff_el2, xzr
    mov     x0, #0x33ff             // CPTR_EL2: don't trap FP/SIMD to EL2
    msr     cptr_el2, x0
    mov     x0, #0x0800             // SCTLR_EL1: RES1 bits, MMU/caches off
    movk    x0, #0x30d0, lsl #16
    msr     sctlr_el1, x0
.endm


_start:
    // Check processor ID
    mrs     x1, mpidr_el1
    and     x1, x1, #3 
    cbz     x1, master // If CPU ID is 0, jump to master
    b proc_hang // Otherwise, hang

proc_hang:  
    wfe // Wait for event (low power)
    b proc_hang

master: 
    mrs x0, CurrentEL
    lsr x0, x0, #2
    cmp x0, #3
    b.eq from_el3 // We're in EL3 (QEMU)
    cmp x0, #2
    b.eq from_el2 // We're in EL2 (physical rpi4)
    b in_el1 // already EL1

from_el3:
    mov     x0, #((1 << 10) | (1 << 0) | (3 << 4))  // SCR_EL3: RW, NS, RES1
    msr     scr_el3, x0
    msr     cptr_el3, xzr // don't trap FP/SIMD to EL3
    prepare_el1
    mov     x0, #0x3c5 // SPSR: EL1h, DAIF masked
    msr     spsr_el3, x0
    adr     x0, in_el1
    msr     elr_el3, x0
    eret // EL3 -> EL1 directly

from_el2:
    prepare_el1
    mov     x0, #0x3c5
    msr     spsr_el2, x0
    adr     x0, in_el1
    msr     elr_el2, x0
    eret                            // EL2 -> EL1

in_el1:
    mov     x0, #(3 << 20)          // CPACR_EL1.FPEN = 0b11 (EL1 and EL0)
    msr     cpacr_el1, x0
    isb                             // make it take effect before any FP use

    // SP_EL1 is unknown after the eret, so set the stack here
    adr     x0, __stack_top
    mov     sp, x0

    // Clear BSS (Uninitialized global variables)
    adr x0, __bss_start
    adr x1, __bss_end
    sub x1, x1, x0 // Size in bytes
    lsr x1, x1, #3 // Divide the size in bytes by 8
    cbz x1, run_main

clear_bss:
    str xzr, [x0], #8 // Store zero, post increment
    sub x1, x1, #1
    cbnz x1, clear_bss

run_main:
    // Jump to C code, should not return
    bl kernel_main

    // In case it does return, halt the master core too
    b proc_hang