/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 Amerody
 * 
 * boot.S: kernel entry point (_start) for aarch64.
 *
 * Linked first in the image (section .text.boot), so the firmware jumps here.
 * In order:
 *   1. Only CPU 0 (MPIDR_EL1 Aff0 == 0) continues; the others wait forever
 *      in a wfe loop (proc_hang).
 *   2. Drops from EL2 to EL1h (QEMU and the Pi 4 firmware both start the
 *      kernel in EL2). prepare_el1 sets up the EL2 controls and SCTLR_EL1
 *      first. The eret enters EL1h with D, A, I and F masked (SPSR 0x3c5):
 *      no vector table is installed yet. When started directly in EL1,
 *      nothing is dropped and DAIF is left as it was.
 *   3. Enables FP/SIMD at EL1 and EL0 (CPACR_EL1), sets SP to __stack_top
 *      and zeroes .bss 8 bytes at a time (__bss_start and __bss_end from the
 *      linker script).
 *   4. Calls kmain(dtb). If kmain() returns, CPU 0 also waits in proc_hang.
 *
 * Register conventions:
 *   x0   On entry: the DTB pointer from the firmware. Passed back to kmain()
 *        as its first argument. Otherwise used as scratch.
 *   x1   Scratch.
 *   x19  Holds the DTB pointer from entry until kmain() is called.
 *
 * The vector table is not installed here: arch_init() does that from kmain().
 */

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
    mov x19, x0 // Save the DTB pointer in callee-saved register x19 (see kmain for explanation on why we need the DTB pointer)

    mrs x0, CurrentEL
    lsr x0, x0, #2
    cmp x0, #3
    b.eq proc_hang // We shouldn't start in EL3. If we do, hang
    cmp x0, #2
    b.eq from_el2 // We're in EL2
    b in_el1 // already EL1 (shouldn't happen)

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

    // The vector table is not installed here
    // arch_init() does so from kmain(), then unmasks SError.
    // IRQs, and FIQs are unmasked by arch_irq_enable()

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
    mov x0, x19 // Restore the DTB pointer into x0 as argument 1 for kmain (see kmain for explanation on why we need the DTB pointer)
    // Jump to C code, should not return
    bl kmain

    // In case it does return, halt the master core too
    b proc_hang