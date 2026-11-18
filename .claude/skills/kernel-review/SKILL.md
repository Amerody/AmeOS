---
name: kernel-review
description: Review the user's uncommitted AmeOS kernel changes (or given files / a commit range) against the project's hardware, architecture, style and documentation rules, and report problems without fixing them. Use when the user says "review", asks to check their changes or diff, or asks whether some kernel code is correct before committing.
---

# Review AmeOS changes

The user writes this kernel to learn. Your job is to **review, not rewrite**:

- **Never edit, create or delete files during a review.** Report only. If a fix is useful, show at most a few lines as an illustration.
- Explain *why* something is wrong and **point to where to read about it**: the BCM2711 ARM Peripherals PDF (`docs/bcm2711-peripherals.pdf`, section/table + register), the Arm ARM for A-profile (section + register/instruction), the GIC-400 TRM / GICv2 spec, or `docs/internal/style.md` (section number).
- If you aren't sure about a register, offset, bit field or QEMU behaviour, say so explicitly. Don't guess.

## 1. Work out the scope

- No arguments: review all uncommitted work. Use `git status --porcelain` and `git diff HEAD` (staged and unstaged together), and read any untracked `.c`, `.h`, `.S`, `.ld`, `CMakeLists.txt` or `.md` files in full.
- `$ARGUMENTS` given: if it names files or folders, review those files as they are now. If it looks like a commit range (`main..wip`, `HEAD~3`), review `git diff <range>`.
- Read enough surrounding code to judge each change: the header a function is declared in, the callers, and the register struct being used.

## 2. Check the changes

Go through every item in [checklist.md](checklist.md). Skip sections that clearly don't apply to the diff. The rules for naming and docs live in `docs/internal/style.md`; read it when the diff adds or renames identifiers, files or documentation.

## 3. Build and boot check

Run `tools/run.sh -t 5` (Git Bash). It builds only the `kernel` target, then boots headless for 5 seconds.

- Report compiler warnings and errors coming from the changed files.
- Report if the boot output looks broken (nothing printed, a panic, an exception dump, output stopping early).
- If the build fails because of the environment (QEMU or the toolchain not found), say so and continue with the review.

## 4. Report

Use this structure and leave out empty sections:

```
## Review: <scope in one line>

### Bugs
- `path/file.c:123`: what is wrong. Why it matters. **Read:** <document, section>

### Likely problems / risks
- ...

### Architecture and layout
- ...

### Style and docs
- ...

### Questions
- Things that might be intentional; ask instead of assuming.

### Build / boot
- Result of tools/run.sh in one or two lines.
```

- Order items by severity within each section, and always give `file:line`.
- Keep praise out of the list. If the diff is clean, say so in one line.
- End with **one** line suggesting what to look at or learn next, if there's a natural one.
