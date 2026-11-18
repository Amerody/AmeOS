---
name: document
description: Write or fix Doxygen documentation for AmeOS source code (.c, .h, .S) in a directory or file the user points to. Finds undocumented or badly documented functions, types, macros, globals and files, writes the documentation following docs/internal/style.md, removes duplicated Doxygen comments from source files, and leaves all changes uncommitted for the user to review. Use when the user says "document", asks to document a directory or file, or wants to find what is still undocumented. Never changes code, only comments.
---

# Document AmeOS source code

AmeOS kernel code is written by the user by hand. Documentation may be written by Claude, and the user reviews every change with `git diff` before committing. Your job is to make the documentation complete and accurate, and to make the review easy. **You never change code, only comments.** You never commit.

## 1. Gather context

0. Run `git status`. If there are unstaged changes, stop and ask the user to stage (`git add`) or commit them first, so that `git diff` afterwards shows only documentation changes.
1. Read `docs/internal/style.md`, especially section 7 (documentation comments) and section 4 (assembly). It is the authority on format.
2. Read 2-3 well-documented files near the target and match their style, including whether they use `/** */` or `///` blocks and `///<` for members. If `style.md` and the existing files disagree, ask the user which to follow before writing anything.
3. Find the existing Doxygen groups: `grep -rn "@defgroup" include src docs`. Note each group's name and where it's defined.
4. Check the Doxygen configuration (the Doxyfile or its CMake template) for `WARN_IF_UNDOCUMENTED`, `EXTRACT_ALL`, `EXTRACT_STATIC`, `INPUT` and `WARN_LOGFILE`, so you know which warnings to expect and where they're written.

## 2. Find what needs work

The target is `$ARGUMENTS` (a directory or file). If it's empty, ask the user for one.

Find, in every `.c`, `.h` and `.S` file in the target, recursively:
- **Missing documentation:** files without a `@file` header, functions (public and `static`), structs, unions, enums and their members, typedefs, macros, register struct members, and global or file-scope variables.
- **Wrong or incomplete documentation:** comments that contradict the code, wrong `@param` names, missing `@param` or `@retval` entries, error values not listed, missing context (IRQ-safe, may block, preconditions, ownership).
- **Duplicates:** public functions documented with Doxygen both in their header and in the source file.

Doxygen only documents a file's global functions, types and macros if the file itself is documented, so files without `@file` produce fewer warnings than they should. Use your own scan as the main source, and the Doxygen warnings as a cross-check after step 5.

## 3. Ask questions once, up front

Before writing anything, collect every question you need answered and ask them all together, in one message. Typical questions:
- Which `@defgroup` a file belongs to, when no existing group clearly fits. **Never create a new group without asking.**
- Style questions that `style.md` and the existing files don't settle.

If there are no questions, start writing without asking. Don't ask about individual items whose purpose is unclear; handle those as described in step 4.

## 4. Write the documentation

### Rules for content
- **Document what the code does and why it exists**, not a line-by-line restatement.
- `@brief` always. `@param` and `@retval` (preferred over `@return`) when not obvious. Every error value a function can return gets a `@retval`.
- State context: IRQ-safe or not, may block or not, required exception level, preconditions and init order (e.g. "call after `console_init()`"), ownership of pointers, side effects on hardware or global state.
- Units must match the code and `style.md` conventions: `size` is bytes, `_ms`, `_us`, `_hz`, `_pa`, `_va`.
- **Don't invent reasons.** If you can't tell why something is done a certain way (a magic value, a barrier, a delay, an unusual init order), document only what it does, and list it in the final summary for the user.
- **Hardware references:** you may cite a section or register of `docs/bcm2711-peripherals.pdf` only after checking it in that PDF. Don't cite the Arm ARM or GIC spec from memory. If a comment would benefit from a reference you can't verify, leave it out and list it in the summary.

### Where documentation goes
- **Public functions, types and macros:** Doxygen in the header, directly above the declaration.
- **`static` functions, file-local types and register structs:** Doxygen in the `.c` file, above the definition, following the register struct style in `style.md`. Reserved gaps get a `///<` comment stating the gap size, as in the examples.
- **File headers:** every `.c` and `.h` file gets a `@file` block with a `@brief` and `@ingroup <group>`, using the groups found in step 1.
- **Assembly (`.S`):** a plain block comment at the top stating the file's purpose and register conventions (section 4 of `style.md`). Routines are documented on their C prototypes, not in the `.S` file; if a prototype is missing its documentation, document it there.

### Duplicated documentation (header and source)
When a public function is documented with Doxygen in both its header and its source file:
1. The header comment is the one that stays.
2. If the source comment contains information about the **interface** that the header lacks (a parameter detail, an error value, a precondition), add it to the header comment.
3. If the source comment contains **implementation details** the header doesn't have, turn them into plain `//` comments above the definition in the source file.
4. Remove the Doxygen comment from the source file.

### Existing documentation
You may rewrite existing documentation that is wrong or incomplete. **Every such change must be listed in the final summary with a one-line reason**, so the user can check it. Don't rewrite comments only to change their wording.

### Formatting
- Keep comment lines within 100 columns and match the surrounding indentation (4 spaces, no tabs).
- Don't run `clang-format` on whole files, because it could change code formatting and pollute the diff.

## 5. Verify

1. Review your own `git diff` (unstaged changes only). Every changed line must be a comment line or a blank line next to a comment. If any code changed, revert that change.
2. Build with `tools/run.sh` to confirm nothing broke (e.g. a stray `*/` closing a comment early).
3. Build the documentation and read the Doxygen warnings (from `WARN_LOGFILE` if set, otherwise the build output). Fix any warnings in the target that your changes should have covered. Warnings outside the target: mention their count, don't fix them.

## 6. Summary for the user

Leave all changes uncommitted. Then give the user a review guide:

- **Files changed**, with the number of items documented in each.
- **Existing documentation rewritten**, with file, item and a one-line reason for each. These need the closest review.
- **Duplicates resolved**: which source comments were removed, and what was moved to the header or turned into `//` comments.
- **Unclear items**: things documented only by what they do because the reason wasn't clear, and hardware references you couldn't verify.
- **Possible bugs** you noticed. Report them; don't fix them.
- **Remaining Doxygen warnings** in the target, if any.
- A suggested commit message following `style.md` (e.g. `docs: document gic and console`).

If more than about 10 existing comments in a file were rewritten, group them by reason (e.g. "added missing @retval for error codes: 7 functions") instead of listing each one.