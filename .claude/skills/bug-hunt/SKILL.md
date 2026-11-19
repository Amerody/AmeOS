---
name: bug-hunt
description: Check the whole AmeOS codebase (or a directory the user names) for bugs, not just a diff. Reviews every C and assembly source plus documentation that contradicts the code, runs the concurrency-reviewer agent in parallel, builds and boots in QEMU, re-verifies existing @bug comments, adds new Doxygen @bug comments (comments only, never code) and ends with a full report. Use when the user says "bug hunt", "bug check", asks to look for bugs across the code base, or wants an audit of everything. For uncommitted changes only, use kernel-review instead.
---

# Bug hunt across AmeOS

The user writes this kernel by hand to learn. Your job is to **find and explain bugs, not fix them**:

- **You never change code.** The only edits allowed are `@bug` comments (and fixes to the text of existing `@bug` comments). The user checks every change with `git diff`.
- **You never commit.**
- Explain why each finding is wrong and **where to read about it**: `docs/bcm2711-peripherals.pdf` (section/table + register), the Arm ARM (section + register/instruction), the GIC-400 TRM / GICv2 spec, or `docs/internal/style.md`.
- If you aren't sure about a register, bit field, errata or QEMU behaviour, say so and make it a question. Don't guess, and don't add a `@bug` for it.

## 1. Scope and defaults

These are the user's standing choices. Don't ask about them again; ask only if `$ARGUMENTS` or the user's message changes them.

| Topic | Default |
|---|---|
| Target | All `.c`, `.h`, `.S` under `src/` and `include/`. If `$ARGUMENTS` names files or folders, only those (but still read callers and headers outside them). |
| Also in scope | Documentation that contradicts the code: Doxygen comments, `docs/internal/*.md`, decision records. |
| Out of scope | Linker script, `config.txt`, CMake/toolchain, `tools/`. Read them when needed to judge a source file (e.g. `rpi4.ld` for stack and BSS alignment). Problems there go in the report's "noticed in passing" section only. |
| `@bug` placement | Doxygen `@bug` in the doc block of the affected item, like the existing ones (see section 5). |
| Existing `@bug` | Re-verify each one. Fix or remove ones that are wrong or already fixed, and list all of them in the report with a status. |
| Build/run | Allowed: build and boot headless with `tools/run.sh`. Never use the `debug` mode (it waits for GDB). |

## 2. Before starting: ask everything at once

1. Run `git status --porcelain --untracked-files=no`. If a tracked file is modified or staged, the final diff would mix with the user's work: ask them to commit or stash first. Untracked files don't matter.
2. Collect every question you need answered (scope changes, an unclear choice, a dirty tree) and ask them **all in one message**, before any review work. If there are none, start without asking.
3. Don't come back mid-way with more questions. Anything uncertain you find while reviewing goes in the report's Questions section.

## 3. Start the parallel work

Do these in the same message, right after the questions are settled:

1. **Launch the `concurrency-reviewer` agent in the background** on the same target. It reviews a diff by default, so tell it explicitly to review the whole target, not `git diff`. Point it at:
   - exception entry/exit (`vectors.S`, `exceptions.c`, `boot.S`): trap frame, SP alignment, DAIF;
   - the IRQ path (GIC, `hal_irq`, handler tables) and every interrupt handler;
   - ring buffers and other state shared between handlers and normal code;
   - the timer, the console/`kprintf` reentrancy, the mailbox/framebuffer (barriers, DMA/cache);
   - anything the current `boot.S` implies (check whether the MMU and caches are on: SCTLR_EL1), and whether secondary cores are parked.
   Ask for `file:line`, severity (bug / likely bug / question), a concrete failure scenario and the document to read.
2. **Build and boot**: `bash tools/run.sh -t 8` (call it through `bash`, since the execute bit may be missing on Linux). Save the UART output: it is the baseline you compare against at the end.
3. **Collect warnings**: run `cmake --build --preset rpi4-debug --clean-first --target kernel` and keep every warning line. Only build files go into `build/`; nothing in the source tree changes.

## 4. Review the code yourself

While the agent runs, read **every** file in the target in full. Don't sample. A practical order is boot/arch, then platform drivers, then HAL headers, drivers, kernel libs, `lib/`, `gfx/`, `main.c`.

Check each file against [../kernel-review/checklist.md](../kernel-review/checklist.md) (all sections apply here, not only those touched by a diff), plus:

- **Contracts vs implementation:** every `@retval`, error code, precondition and context claim ("safe from interrupt handlers", "never blocks") in a header must hold in the implementation. Return values that are documented but can never happen, and errors that are silently ignored, are findings.
- **Arithmetic edge cases:** division by zero, unsigned wrap (`a - b` with `b > a`, `x + w` overflowing), signed/unsigned mixing, truncation, off-by-one on ranges (pin counts, array sizes, IDs).
- **Failure paths:** what state is left behind when an init fails halfway (registered but not initialised, globals set before the request).
- **Latent bugs:** code with no callers yet still gets a finding; say that it is latent.
- **Leftovers:** test code, stale comments that describe code that no longer exists.

### Verify hardware facts in the PDF

Before calling a register, offset, bit or access type wrong, check it in the BCM2711 PDF. Extract the text once into the scratchpad and grep it:

```bash
pdftotext -layout docs/bcm2711-peripherals.pdf <scratchpad>/bcm.txt
grep -n "AUX_MU_IER_REG Register" <scratchpad>/bcm.txt
```

Never write the extracted text into the repository. If the code deliberately differs from the PDF because of a known erratum (e.g. the mini UART IER bit order), it is not a bug: report it as a docs note ("cite the erratum in the code") and, if real hardware might differ from QEMU, as a question.

## 5. Add the `@bug` comments

Add a `@bug` only for findings you have confirmed yourself, including the agent's. **Re-check each agent finding against the code before using it**: it can be wrong about details (e.g. which addresses a NULL-pointer write actually hits).

**What gets a `@bug`:** code defects, latent ones included, and places where the behaviour contradicts the documentation of that same item. **What stays report-only:** pure wording errors in docs, questions, uncertain hardware behaviour, nits, and anything out of scope.

**Where:**
- Public function, type or macro: in its doc block in the header, after the existing `@param`/`@retval` lines (or before `@param` if the block's existing `@bug` entries are placed there; follow the block).
- `static` function or file-local item: in its doc block in the `.c` file. Convert a one-line `///` comment by adding `/// @bug ...` lines below it.
- Assembly: on the C prototype or in the private header that documents the routine (e.g. `src/arch/aarch64/exceptions.h`), not in the `.S` file.
- When the defect is in a platform implementation of a HAL contract, put it on the HAL header item and start it with the platform name (`@bug rpi4: ...`), as `hal_fb.h` already does.

**Format:**
- One `@bug` per defect. Continuation lines aligned under the text (`@bug ` plus 5 spaces), as in the existing ones.
- Say what is wrong, the concrete consequence, and the reference (e.g. "BCM2711 ARM Peripherals, chapter 5, Table 70"). Say "Latent: ..." when nothing triggers it today.
- Match the surrounding indentation exactly, character for character (some headers indent with a tab inside `extern "C"` blocks). Keep every line within 100 columns.
- Don't run `clang-format`.

## 6. Verify your changes

1. The diff must be comments only. This prints any changed line that isn't a comment line, and should print nothing:
   ```bash
   git diff -U0 | grep -E "^[+-]" | grep -vE "^(\+\+\+|---)" | grep -vE "^[+-]\s*(\*|///|/\*\*)"
   ```
   If anything shows up, undo that change.
2. No line over 100 columns: `git diff | grep "^+" | awk '{ gsub(/\t/, "    "); if (length($0)-1 > 100) print }'`.
3. Rebuild and boot again with `bash tools/run.sh -t 8`. The UART output must match the baseline from step 3.2. A stray `*/` breaks the build here.
4. Read the agent's final report as soon as it arrives and fold it in; don't write the report before it has finished.

## 7. Report

Leave everything uncommitted. Give the report as your final message, in this order, leaving out empty sections:

1. **One-paragraph verdict:** is anything broken today? How many real / latent bugs, questions, docs mismatches? Confirm that the diff is comments only and the boot output is unchanged.
2. **Existing `@bug` comments:** each one with its status (still accurate / reworded / removed because fixed) and a short reason.
3. **New `@bug` comments:** a table with the file link and item, what is wrong, and severity (real bug / latent / low / question). Order by severity.
4. **Report only, no comment added:**
   - Questions (hardware, errata, intent), each with what to check and where;
   - Docs that contradict the code;
   - Nits.
5. **Noticed in passing (out of scope):** linker, CMake, tools.

Use `[file](path:line)` links for every location. Don't suggest fixes beyond a one-line hint; the user writes the code.
