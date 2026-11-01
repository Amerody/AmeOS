# 0003: Timer

- Date: 2026-11-01
- Status: accepted

## Context
The kernel needs a time source for (a) a periodic or one-shot scheduler tick
and (b) a monotonic clock for timestamps and delays.

The Raspberry Pi 4 offers two candidates:
- **Legacy System Timer**: a memory-mapped, free-running 64-bit counter
  at 1 MHz, with 4 compare registers. Shared by all cores.
- **ARM Generic Timer**: the architectural timer built into each Cortex-A72
  core, accessed through system registers (e.g. `CNTPCT_EL0`, `CNTFRQ_EL0`).
  Per-core compare and control registers, running at a much higher
  frequency (54 MHz by default, verified by reading `CNTFRQ_EL0`).

## Decision
Use the ARM Generic Timer for both the scheduler tick and the monotonic clock.
The Legacy System Timer will not be used.

## Alternatives considered and why not
**Legacy System Timer**
- Resolution is 1 µs, versus roughly 18 ns for the Generic Timer.
- It is an MMIO peripheral, so every read is slower than reading a CPU
  system register.
- It is Broadcom-specific, so the code would not carry over to other ARMv8
  platforms.
- It is shared rather than per-core, which is awkward for future SMP support.

Source: [system timer guide](https://ohyaan.github.io/os-development/phase1-foundation/04._system_timer_-_precise_timing_for_your_os/#understanding-raspberry-pi-timers)

## Consequences
**Positive**
- Higher precision and cheaper reads.
- Per-core timers, which is the natural fit for multicore support later.
- Architectural standard, so the same driver works on other ARMv8 boards.

**Negative / costs**
- Interrupts go through the GIC, so a working GIC driver is mandatory before timer 
  interrupts function. The Legacy System Timer would have needed the interrupt controller 
  too, but this makes the GIC dependency explicit.
- Timer registers are only accessible at the right exception level, and
  EL2/EL3 setup affects which timer (physical vs. virtual) is able to be used.
