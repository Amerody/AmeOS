# 0001: Register Access {#decision_0001-register-access}

- Date: 2026-10-30
- Status: accepted

## Context
I had to decide the way to access the registers.

## Decision
Defining a private struct, so the way to reference memory mapped registers is via `<module>-><register>` e.g `GPIO->GPFSEL0`

## Alternatives considered and why not
**Using enums**: 
- The compiler computes each register’s address from the declared layout, 
  so there’s no `base + offset` arithmetic or cast at every access site.
- `volatile` is declared once per member, not repeated at each access.
- Manual offsets are confined to one place (the struct definition)

## Consequences
- Each register block is described by a struct, with reserved arrays for gaps.
  Unused registers still need either a declaration or a gap, and gaps must be
  resized as more registers start to be used (error-prone).
- Access is typed and `volatile` per member; no address arithmetic at call sites.