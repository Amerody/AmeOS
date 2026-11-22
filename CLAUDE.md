# CLAUDE.md

A from-scratch kernel (and eventually an OS) for the Raspberry Pi 4, developed and tested in QEMU. The main goal is **learning OS internals at a deep level**. The long-term target is a multitasking OS with a graphical UI and its own drawing library. A shell comes first.

## How to help me (read this first)

- **Explain and review. Don't write whole drivers or subsystems unless I explicitly ask.** I want to write the code myself. Prefer explaining the concept, pointing out bugs, reviewing my diffs and suggesting what to look at next. Small snippets that illustrate a point are fine.
- **Point me to the relevant documentation section** when hardware or architecture details matter: the BCM2711 ARM Peripherals PDF (section/table and register name), the Arm Architecture Reference Manual for A-profile (ARM ARM; section and register/instruction name), or the GIC-400 / GICv2 spec. Cite where to look instead of just giving me the magic values.
- If you're not sure about a register, offset, bit field or QEMU behaviour, say so. Don't guess.
- When I ask "where should this go?", answer using the layout rules below and the design stance. Don't invent new top-level folders without discussing it.
- When you review, check for: missing `volatile` / barriers on MMIO, wrong register offsets, alignment, exception-level assumptions, and code that bypasses the HAL from kernel core.

## Target

- Hardware: Raspberry Pi 4 (BCM2711, Cortex-A72, AArch64). I don't own a Pi yet so QEMU is the main target for now.
- Emulator: QEMU `raspi4b` machine (not `virt`).
- Boot: started from the OSDev wiki Pi 4 `boot.s`. The kernel runs Non-secure at **EL1** and relies on the armstub for GIC interrupt group setup.
- Although the DTB is passed to QEMU, for now its parsing is deferred until the memory allocator works.

## Toolchain

- Development is mainly on **Windows**, sometimes on a Linux laptop. Commands should work on both, or give both variants.
- Compiler: **clang from my LLVM install**, cross-compiling for AArch64 bare metal. Not GCC, not MSVC.
- Build: CMake with `CMakePresets.json`. Main configure/build preset: `rpi4-debug`. To avoid building documentation pass `KERNEL_BUILD_DOCS OFF`
- Editor: VS Code with CMake presets and clangd.
- Debugging: `aarch64-none-elf-gdb` attached to QEMU started with `-s -S`, set up in `.vscode/launch.json` and `tasks.json`.
- Docs: Doxygen, generated from CMake and enabled by default. Everything gets documentation, even if it's internal.

## Build and run

Shell on Windows is Git Bash. Build and test with `tools/run.sh` (headless, 10 s timeout,
UART output on stdout). Use `-t N` for longer runs. Never run the `debug` mode: it waits for GDB.
`kernel.elf` is for GDB/symbols; QEMU boots `kernel8.img`.

## Build system rules

- **Explicit source lists, no globbing.** When adding a `.c`/`.S` file, add it to the right list in `CMakeLists.txt`.
- One CMake OBJECT library per category (`kernel_core`, `platform`, `aarch64`, ...), all linked into `kernel.elf`.

## Layout and architecture

HAL-based design, so it can be ported later (other ARM boards, maybe x86_64). Modeled on a university project where `drv_leds` sits on `hal_gpio`, with one implementation per board.

- `include/hal/`: hardware abstraction interfaces (`hal_irq`, `hal_timer`, `hal_fb`, UART, ...).
- `include/arch/aarch64/`, `src/arch/aarch64/`: CPU-architecture code (exception vectors, IRQ enable/disable, generic timer).
- `include/platform/rpi4/`, `src/platform/rpi4/`: board/SoC-specific code (MMIO base and accessors, GPIO, AUX/mini UART, GIC-400, mailbox, framebuffer).
- `src/drivers/`: drivers built on the HAL.
- `include/kernel/`, `src/kernel/`: kernel core (`kmain`, `kprintf`, console).

Rules and design stance:
- **Kernel core goes through the drivers / kernel libs (e.g. `kprintf`)**, not straight to platform registers.
- **No modules that only wrap the HAL or arch code** (no `ktimer`/`kinterrupts` that just forward calls). A module needs to add real logic.
- `hal_irq` is a layer on top of the existing GIC-400 driver (handler table + dispatch), not a rewrite of it.
- Console: `kmain` calls `console_init()` (declared in `include/kernel/console.h`), then registers the sinks it wants (`console_uart_init`, later `console_fb_init`). `kprintf` uses `console_write()`. `struct console` has no ctx pointer.
- `platform_init()` currently only does interrupt bring-up (GIC).
- Header placement is still being made consistent. If you see a header in the wrong place, point it out, don't silently move it.
- Windows reserves the file name `aux`, which is why the header is `auxiliary.h`. Never name a file `aux.*`, `con.*`, `nul.*`, `prn.*`, `com1.*` and so on.

## Code conventions
- Style guide: `docs/internal/style.md`
- Language: C for now; moving to C++ later where it fits naturally. Small parts in C or assembly are fine.
- Pointer style: star attached to the type: `const char* path`. Unless it's a function: `void (*callback)(...)`.
- Freestanding: no libc. String/memory functions (`strlen`, `memcpy`, `memset`, `memmove`, `memcmp`, ...) are our own.

## Status and roadmap

<!-- Keep this section short and update it as things change -->
Done:
- Mini UART output with my own init (ALT5 pin setup, AUX enable, 8-bit mode, FIFO clear)
- GIC-400 driver, exception handlers (C handlers take `struct trap_frame*`), virtual timer (`cntv`) tick IRQ
- Framebuffer via the mailbox with `hal_fb` (`hal_fb_init`, `hal_fb_set_pixel`, `hal_fb_fill_rect`, `hal_fb_draw_line`, pixel coordinates)
- TTY: font, terminal abstraction (cursor cell vs pixel position), console changes
- Own string/memory functions
- Basic shell: UART RX via interrupt on the mini UART

In progress / next (in order):
- Memory allocator 
- MMU
- Kernel heap (kmalloc)
- C++ support
- Single core scheduler
- Filesystem (preferably custom)
- Multicore (+ possibly multicore scheduler)
- DMA and USB

## References

- BCM2711 ARM Peripherals: `docs/bcm2711-peripherals.pdf`
- Arm Architecture Reference Manual (A-profile)
- ARM GIC-400 TRM / GICv2 architecture spec
- *Operating Systems: Three Easy Pieces* (OSTEP), for concepts
