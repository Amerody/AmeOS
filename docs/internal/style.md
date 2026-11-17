# Naming and style conventions {#style}

These rules apply to the whole project (C, C++, assembly, build files, docs). The goal is
consistency: when a choice isn't covered here, follow the closest existing example, then
**add the rule to this file in the same commit**.

Where a rule is a real design decision (not just taste), record it as an ADR in
`docs/decisions/`.

## 1. Guiding principles

- **One style across C and C++.** Everything is `snake_case` except constants and macros,
  so code can migrate from C to C++ file by file without renaming.
- **Names say what and where.** Public functions start with their subsystem prefix.
- **Units and address kinds go in the name** when they aren't obvious from the type.
- **Prefer clarity over brevity**, except for universally known hardware terms
  (`irq`, `mmu`, `tlb`, `gic`, `el1`).

## 2. Files and directories

| Kind | Rule | Example |
|---|---|---|
| Directories | lowercase, `snake_case` if needed | `src/arch/aarch64/` |
| C source | `.c` | `gic.c` |
| C++ source | `.cpp` | `page_allocator.cpp` |
| Assembly | `.S` (capital: preprocessed by the compiler) | `boot.S`, `vectors.S` |
| Headers used from C **and** C++ | `.h`, with `extern "C"` guards | `uart.h` |
| C++-only headers | `.hpp` | `bitmap.hpp` |
| Include guards | `#pragma once` | |
| Docs | lowercase kebab-case `.md` | `hardware-notes.md` |
| Decision records | `NNNN-short-title.md` (zero-padded, kebab-case) | `0001-platform-layer.md` |

Source layout:

```
include/                      public headers, Doxygen comments live here
src/kernel/                   platform-independent kernel code
src/arch/aarch64/             CPU-specific code (boot, vectors, context switch)
src/platform/<platform>/      board-specific code; private headers stay here
linker/<platform>.ld          one linker script per platform
docs/                         style.md, hardware-notes.md, decisions/, mainpage.md
```

A header and its source share a base name (`pmm.h` / `pmm.c`). One subsystem per header.

## 3. C identifiers

| Kind | Rule | Example |
|---|---|---|
| Functions | `subsystem_verb_noun` | `pmm_alloc_page`, `uart_putc` |
| File-local (`static`) functions | `snake_case`, no prefix required | `find_free_run` |
| Variables, parameters, members | `snake_case` | `free_count` |
| Structs, enums, unions | `snake_case`, tag only | `struct page_frame` |
| Typedefs | `snake_case_t`, only for fixed-width/opaque types | `paddr_t`, `vaddr_t` |
| Enum constants | `UPPER_SNAKE`, prefixed by the enum | `IRQ_TYPE_TIMER` |
| Macros and constants | `UPPER_SNAKE`, prefixed by the subsystem | `PAGE_SIZE`, `UART_BASE` |
| Non-`static` global variables | `g_` prefix | `g_boot_dtb` |
| File-`static` variables | `s_` prefix | `s_next_pid` |
| Booleans | `is_`, `has_`, `can_`, `should_` | `is_mapped`, `has_fpu` |
| Boolean-returning functions | same prefix after the subsystem | `vmm_is_mapped()` |

Function verbs, used consistently: `init`, `alloc`/`free`, `map`/`unmap`, `read`/`write`,
`get`/`set`, `enable`/`disable`, `register`/`unregister`.
`init` sets a subsystem up once; `create`/`destroy` handle objects of which there can be many.

### Constants and macros

- Prefer `enum`, `static const`, or `constexpr` over `#define` for typed constants.
- Hardware registers: `<DEVICE>-><REGISTER>` for offsets/addresses and
  `<DEVICE>_<REGISTER>_<FIELD>` for bits and masks if needed.
```c
#define UART_DR            0x00
#define UART_FR            0x18
#define UART_FR_TXFF       (1u << 5)   /* transmit FIFO full */
```

- The hardware registers are defined with structs the following way inside its corresponding `.c` file:
```c
/// @brief All the GICC Registers
/// @warning @p RESERVED_* are not to be used
typedef struct
{
	volatile uint32_t CTLR;   ///< CPU Interface Control Register
	volatile uint32_t PMR;    ///< Interrupt Priority Mask Register
	// ...
	volatile uint32_t AHPPIR; ///< Aliased Highest Priority Pending Interrupt Register

	uint32_t RESERVED_02C_0CC[CALC_OFFSET(0x02C, 0x0CC)]; ///< 168 Bytes gap between AHPPIR and APR0

	volatile uint32_t APR0; ///< Active Priority Register

	uint32_t RESERVED_0D4_0DC[CALC_OFFSET(0x0D4, 0x0DC)]; ///< 12 Bytes gap between APR0 and NSAPR0

  // if there is a gap bigger than 4 bytes use
  // uint32_t RESERVED_<start_addr>_<end_addr>[CALC_OFFSET(<start_addr>_<end_addr>)] (every address is aligned by word)

  // ...
} GICC_Registers;

#define GICC_BASE (PERIPHERAL_BASE + 0x1842000) ///< GICC base address
#define GICC ((GICC_Registers*) GICC_BASE)      ///< GICC access
```

### Addresses, sizes, units

- Physical and virtual addresses use distinct types: `paddr_t`, `vaddr_t`.
  Variables holding raw addresses end in `_pa` / `_va` when no such type is used.
- `size` always means **bytes**. Anything else says so: `page_count`, `size_pages`, `len_chars`.
- Time and frequency carry a unit: `timeout_ms`, `delay_us`, `freq_hz`.
- Counts end in `_count`; indexes end in `_idx`.

### Error handling

- Functions that can fail return either `int`: `0` on success and a negative `ERR_*` code on failure or a `<system>_status_t` enum for hardware implementations.
```c
/// @brief GIC Status codes
typedef enum
{
	GIC_SUCCESS = 0, ///< Operation Success

	GIC_ERROR_INVALID_INTID = -1,    ///< An invalid interrupt id has been provided
	GIC_ERROR_INTID_NOT_SPI = -2,    ///< An interrupt id NOT corresponding to an SPI has been provided where it shouldn't be
	GIC_ERROR_INTID_NOT_SGI = -3,    ///< An interrupt id NOT corresponding to an SGI has been provided where it shouldn't be
	GIC_ERROR_INTID_IS_SGI = -4,     ///< An interrupt id corresponding to an SGI has been provided where it shouldn't be
	GIC_ERROR_INVALID_CPU_MASK = -5, ///< An invalid CPU mask has been provided

	GIC_ERROR_INVALID_ARG = -999, ///< An invalid argument has been provided (shouldn't happen)

} gic_status_t;
```
- Error constants are `<SYSTEM>_ERR_<NAME>` (`GIC_ERROR_INVALID_INTID`).
- Functions that return a pointer or address return `NULL` / `0` on failure, and say so in
  their Doxygen `@return` or `@retval`.

## 4. Assembly

- Global symbols use the same `snake_case` names that C sees (`_start`, `exception_vectors`).
- Local labels: numeric (`1:` with `1b` / `1f`) for short loops; `.L<name>` for longer ones.
- Registers lowercase (`x0`, `sp`), instructions lowercase, one instruction per line.
- A block comment at the top of every `.S` file states its purpose and register conventions.
  Each routine is documented on its C prototype.
- System-register accessors are C inline functions named `read_<reg>()` / `write_<reg>()`
  with the register in lowercase: `read_esr_el1()`, `write_vbar_el1()`.

## 5. Linker script symbols

- Symbols exported by `linker/*.ld` use a double-underscore prefix and `snake_case`:
  `__kernel_start`, `__kernel_end`, `__bss_start`, `__bss_end`, `__stack_top`.
- Declare them in C as arrays so you take their address correctly:
  `extern char __bss_start[];`
- Every platform's linker script must define the same set of symbols.

## 6. C++ (once code migrates)

- Same `snake_case` for types, functions and variables (STL-like); no PascalCase classes.
- Namespaces lowercase, matching the subsystem: `namespace pmm { ... }`.
- Member variables: `m_` prefix. Static class members: `s_`.
- Template parameters: `PascalCase` (`T`, `Allocator`). Concepts: `snake_case`.
- Compile-time constants: `constexpr`, `UPPER_SNAKE` (same as the C macros they replace).
- Anything crossing the C boundary is declared `extern "C"` and uses the C naming scheme.

## 7. Documentation comments

- Doxygen `/** ... */` in headers, directly above the declaration.
- `@brief` always; `@param` / `@return` or `@retval` when not obvious. Prefer `@retval`.
- State context (IRQ-safe? may block?), preconditions, ownership, and hardware references.
- One `@defgroup` per subsystem, with lowercase names: `hal`, `mm`, `sched`, `drivers`.
- EVERYTHING is documented with Doxygen. The internal documentation is only created for the `<platform>-debug` preset, 
  and stripped away on the `<platform>-release` config. Only interfaces (everything under `include/` for now) reach the release.

## 8. Formatting

Enforced by `.clang-format` (K&R braces, 4 spaces, no tabs, 100 columns). Don't hand-format against it.

## 9. Commits and docs

- Commit subject: `subsystem: imperative summary` (e.g. `pmm: add free-list allocator`),
  at most about 72 characters.
- A commit that changes an interface also updates its Doxygen comment.
- A design decision gets an ADR in the same commit or the next one.
