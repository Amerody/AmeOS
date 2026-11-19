---
name: decision
description: Create a new AmeOS architecture decision record (ADR) in docs/internal/decisions/ from the project template, interviewing the user for the context, the choice, the rejected alternatives and the consequences. Use when the user says "decision", wants to record or document a design decision, or has just chosen between real alternatives (memory map, allocator design, HAL split, C vs C++ for a module, ...). Also use to mark an older decision as superseded.
---

# Write an AmeOS decision record

Decision records live in `docs/internal/decisions/` and are the user's own notes on *why* the kernel is built the way it is. They're written in the user's voice. Your job is to interview, organise and write them down, **not to make the decision**.

## 1. Gather context

1. List `docs/internal/decisions/` and read `0000-template.md`.
2. Read the existing records related to the topic (`$ARGUMENTS` if given), so the new record can reference them and doesn't contradict them silently. If it does conflict with an accepted record, say so: this record may supersede it.
3. If the decision is about code that already exists, read the relevant files so your questions are concrete.

## 2. Interview the user

Ask in short rounds of 1-3 questions, not one long questionnaire. Skip anything the user has already told you. Cover:

- **Context:** what problem or constraint forced a choice? What depends on it?
- **Decision:** what was chosen, precisely (module names, API shape, files)?
- **Alternatives:** what else was considered, and why each one lost.
- **Consequences:** what does this commit them to, what gets harder later, what has to be revisited (e.g. on real hardware, with SMP, after the MMU)?
- **Sources:** any document or page that informed it (BCM2711 PDF section, Arm ARM section, a tutorial). Optional.
- **Status:** `accepted` if decided, `proposed` if still open.

If the user hasn't decided yet, help them think: lay out the trade-offs of each option, point to where to read more (BCM2711 PDF, Arm ARM, GIC spec, OSTEP chapter), and ask questions that expose the trade-offs. **Let them choose.** Record it as `proposed` if they want to write it before deciding.

You may suggest an extra alternative or consequence they didn't mention, but ask before including it. Never put reasons in the record that the user didn't give or approve. If you aren't sure about a hardware fact, say so instead of writing it in.

## 3. Draft

- **Number:** highest existing `NNNN` + 1, zero-padded to 4 digits.
- **File name:** `NNNN-short-title.md`, lowercase kebab-case, a few words. Never a Windows reserved name (`aux`, `con`, `nul`, `prn`, `com1`, ...) as a word in it.
- **Heading:** `# NNNN: Title {#decision_NNNN-short-title}`. The ID must be exactly `decision_` + the file name without `.md`; CMake generates the Doxygen index from the file names, and a mismatched ID breaks the link.
- **Date:** today, from `date +%F`.
- Follow the template's sections and the existing records' style: short, bullet points, **bold** alternative names with their reasons below, code identifiers in backticks, first person is fine. Reference other records as "decision `0001`". Put sources in a `Source:` line.

Show the full draft and the file name to the user and ask for changes. **Write the file only after they approve it.**

## 4. Write and finish

1. Create `docs/internal/decisions/NNNN-short-title.md`. No CMake change is needed: the docs index picks up new records automatically.
2. If it supersedes an older record, ask before changing it, then set that record's status to `superseded by NNNN` and leave the rest of it as history.
3. Don't touch source code. If the decision implies code changes, list them as a short to-do for the user instead.
4. Remind the user, in one line, that `style.md` asks for the record to go in the same commit as the code it describes (or the next one).
