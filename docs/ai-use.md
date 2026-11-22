# AI Use {#ai-use}

AmeOS is primarily a learning project: the goal is to understand OS internals by building one. The kernel code is written by hand. AI tools, mainly [Claude Code](https://docs.claude.com/en/docs/claude-code/overview) by Anthropic, act as a tutor, reviewer and documentation writer rather than as the author of the kernel.

## How AI is used
* **Explaining concepts**: Working through how things like exception levels, interrupt controllers, and memory management work, before writing the code for them.
* **Pointing to documentation**: Hardware and architecture details are looked up in the primary sources (the BCM2711 ARM Peripherals document, the Arm Architecture Reference Manual, and the GIC-400/GICv2 specifications). The AI is asked to cite which section or register to read instead of supplying values directly.
* **Code review**: Reviewing changes for bugs such as missing `volatile` qualifiers or barriers on MMIO accesses, wrong register offsets, alignment problems, incorrect exception-level assumptions, and kernel code that bypasses the HAL.
* **Design discussion**: Deciding where new code belongs within the layered architecture (kernel core, drivers, HAL, platform, and architecture code).
* **Debugging**: Helping interpret QEMU output, exception information, and GDB sessions.
* **Documentation**: Written partly by Claude Code and partly by me. All of it is reviewed by hand.
* **Decision records**: Architecture decision records in `docs/internal/decisions/` are written by Claude Code from an interview with me. The decisions, alternatives, and reasoning are mine; the AI asks the questions and writes up the answers.

## Authorship
* **Kernel code**: All kernel code, including drivers, the HAL, and libraries, is written by me, maybe referencing other code (see [references.md](references.md)). AI may suggest small fixes during review, but the code is typed and understood by hand.
* **Tooling**: `tools/run.sh` (QEMU test runner) and `tools/bdf2inc.py` (font conversion) were written by Claude Code.
* **Documentation**: Written by both Claude Code and me, and of course reviewed by me. This includes both `.md` files and doxygen comments in the source.

## How correctness is checked
* Changes are built and run in QEMU (`raspi4b`) using `tools/run.sh` or using the VSCode task `qemu-debug` before being committed.
* Register addresses, offsets, and bit fields are verified against the official documentation, not taken from the AI's answers.
* Real-hardware testing on a Raspberry Pi 4 is planned. Until then, behavior that QEMU does not emulate accurately is untested.

## AI configuration in this repository
* `CLAUDE.md`: Project context and rules for Claude Code, including the instruction to explain and review rather than write kernel code.
* `.claude/skills/decision/`: A skill that interviews me and writes a decision record.
* `.claude/skills/document/`: A skill that writes Doxygen comments across the code base and reports to me the added comments for review.
* `.claude/skills/bug-hunt/`: A skill that checks the whole code base, or a given directory, for possible bugs, writes the corresponding `@bug` and reports back to me.
* `.claude/skills/kernel-review/`: A skill that checks the given directories or uncommitted changes for issues and reports them.
* `.claude/agents/concurrency-reviewer.md`: An agent that checks the given code for concurrency and memory-ordering bugs.
* `docs/internal/style.md`: The coding style guide.
