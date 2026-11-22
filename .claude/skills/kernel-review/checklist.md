# AmeOS review checklist

## Hardware and MMIO

- Register access goes through `volatile` (the register structs in the `.c` files, or the `mmio_*` helpers in `src/platform/common/mmio.h`). Flag any raw non-volatile pointer to a peripheral address.
- Register struct layouts: offsets match the datasheet, reserved gaps use `RESERVED_<start>_<end>[CALC_OFFSET(start, end)]`, and the field order matches the address order. Check that each gap size is right; a wrong `CALC_OFFSET` silently shifts every register after it.
- Base addresses: `PERIPHERAL_BASE` + the offset from the datasheet. Be careful with the BCM2711 "legacy master" (0x7E...) addresses in the PDF versus the ARM physical addresses the kernel uses.
- Bit fields: the right shift and width, and `1u << n` (unsigned).
- `mmio_write_field` is a non-atomic read-modify-write. Flag it on:
  - registers with write-1-to-clear or clear-on-read bits (use `mmio_write`);
  - registers also changed from an interrupt handler or (later) another core.
- Memory barriers: the `mmio_*` helpers have none. Check whether ordering matters around the change, e.g. switching between different peripherals, the mailbox (writing a buffer in RAM, then ringing the mailbox), or anything DMA-related. If you think a `dsb`/`dmb` is needed, say which kind and why, and point to the Arm ARM section on memory barriers and the BCM2711 PDF's notes on peripheral access ordering. If unsure, raise it as a question.
- Busy-wait loops on status bits: is there a chance the loop never ends? Is the bit polled with a fresh volatile read each time?
- GPIO: function select (ALTn) and pull-up/down match what the peripheral needs for the pins used.

## Interrupts and concurrency

- Data shared between an IRQ handler and normal code is `volatile` or otherwise protected, and updates that need several steps happen with interrupts masked.
- Handlers acknowledge and end the interrupt correctly through `hal_irq` (GIC acknowledge/EOI), including on early-return paths.
- Interrupts are enabled only after the handler is registered and the device is set up.
- Nothing in an IRQ path blocks, busy-waits for long, or calls something that isn't IRQ-safe (check what `kprintf` does in that context).
- Ring buffers / FIFOs (e.g. UART RX): head and tail updates, full/empty conditions, and which side writes each index.

## AArch64 and exceptions

- Exception-level assumptions: the kernel runs at Non-secure **EL1**. Flag EL2/EL3 system registers or instructions used from EL1.
- System registers are accessed through `read_<reg>()` / `write_<reg>()` inline functions; an `isb` is present where a system register write must take effect before the next instruction.
- `struct trap_frame` layout matches the save/restore order in `vectors.S`.
- Assembly follows the AAPCS64: callee-saved registers (x19-x28, fp, lr) are preserved, the stack stays 16-byte aligned, and clobbered registers are documented on the C prototype.
- Alignment: structures and buffers shared with hardware (mailbox buffers: 16-byte aligned) or used for stacks are aligned as required.
- Integer widths and signedness: no truncation from 64 to 32 bits for addresses; `uint32_t` vs `uintptr_t` used correctly.

## Architecture and layout (from CLAUDE.md)

- Kernel core (`src/kernel/`) goes through the HAL or kernel libs (`kprintf`, `console`), never directly to platform registers or platform headers.
- No new module that only forwards calls to the HAL or arch code; a module must add real logic.
- `hal_irq` stays a layer on top of the GIC-400 driver; GIC logic isn't duplicated.
- New files are in the right folder (`include/hal`, `include/arch`, `src/arch/aarch64`, `src/platform/rpi4`, `src/drivers`, `src/kernel`, `src/lib`, `src/gfx`). Platform-private headers stay in `src/platform/rpi4/`. If placement is unclear, ask instead of deciding.
- No new top-level folders without discussion.
- File names avoid Windows reserved names (`aux`, `con`, `nul`, `prn`, `com1`, ...).
- Every new source file is added to the right explicit list in the matching `CMakeLists.txt` (no globbing).
- No libc: no `#include <string.h>`, `<stdio.h>`, `<stdlib.h>`; use the project's own `lib/string.h` and `lib/memory.h`. Freestanding headers (`<stdint.h>`, `<stddef.h>`, `<stdbool.h>`) are fine.

## Style (see docs/internal/style.md)

- Naming: `subsystem_verb_noun` functions, `snake_case` everything, `UPPER_SNAKE` macros/constants with a subsystem prefix, `g_`/`s_` prefixes for globals and file statics, `is_`/`has_`/... booleans, `_t` only for fixed-width or opaque typedefs.
- Units in names (`timeout_ms`, `freq_hz`), `size` means bytes, `_count` / `_idx` suffixes, `_pa` / `_va` for raw addresses.
- Error handling: `0` / negative `ERROR_*` for `int`, or a `<system>_status_t` enum for hardware code; pointer-returning functions document their failure value.
- Pointer style `const char* path`, function pointers `void (*callback)(...)`.
- `#pragma once` include guards; a header and its source share a base name.
- Don't comment on formatting that `.clang-format` handles.

## Documentation

- Everything new or changed has Doxygen comments (`/** ... */` above declarations in headers): `@brief` always, `@param` / `@retval` (preferred) or `@return` when not obvious, plus context (IRQ-safe? may block?), preconditions and hardware references.
- Changed interfaces have updated Doxygen comments.
- Each `.S` file has a top block comment with purpose and register conventions; assembly routines are documented on their C prototype.
- A real design decision (choosing between alternatives) in the diff should get an ADR in `docs/internal/decisions/` (template `0000-template.md`). Suggest one if it's missing.
- If `style.md` and `comment-decisions.md` (or the actual layout) disagree on something relevant to the diff, point out the contradiction instead of choosing a side.
