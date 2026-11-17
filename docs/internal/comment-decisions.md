# Code Comment decisions {#comment_decisions}

- Use `/** ... */` in headers, directly above the declaration.
- `@brief` always. `@param` and `@return` only when not obvious.
- One `@defgroup` per subsystem (`hal`, `mm`. `sched`, `drivers`...), defined in a single place per module.
- Assembly routines are documented on their C prototype (include calling convention and clobbered registers), plus a header comment in the `.s` file.
- EVERYTHING is documented with Doxygen. The internal documentation is only created for the `<platform>-debug` preset, 
  and stripped away on the `<platform>-release` config. Only interfaces (everything under `include/` for now) reach the release.

## Workflow habits
- Docs change in the same commit as the code.
- Write an ADR (`docs/internal/decisions/NNNN-title.md`) whenever picking between real alternatives (memory map layout, allocator design, how the HAL splits, C vs C++ for a module).
- Run `docs` before milestones and fix warnings. Once clean, enable `WARN_AS_ERROR`.