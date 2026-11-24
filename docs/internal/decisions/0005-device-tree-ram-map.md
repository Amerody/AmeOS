# 0005: Device tree parsing and the RAM map {#decision_0005-device-tree-ram-map}

- Date: 2026-11-25
- Status: accepted

## Context
The physical memory allocator needs to know which RAM exists and which parts of
it are reserved by the firmware. The firmware (or QEMU) passes a device tree
blob (DTB) in `x0`, which describes both. The allocator doesn't exist yet, so
whatever reads the DTB has to work without allocating memory.

## Decision
- A read-only parser in `lib/fdt` (`include/lib/fdt.h`, `src/lib/fdt.c`) that
  works in place on the blob and never allocates.
- One global blob: `fdt_init()` checks the header once and selects the blob;
  every other `fdt_*` function works on it. `struct fdt_node` is only an offset
  into the structure block.
- Layers:
  - `lib/fdt` knows only the DTB format and the properties the Devicetree
    specification defines (`#address-cells`, `#size-cells`, `reg`, `status`, `ranges`).
  - `platform/rpi4` builds the RAM map from the `/memory` nodes, inside `platform_init()`.
  - Kernel core only sees `struct mem_region` through `platform_mem_regions()`.
    It never includes `fdt.h`.
- Who removes what from the RAM map:
  - the platform removes the memory reservation block, the `reg` children of
    `/reserved-memory` and the DTB itself;
  - core removes the kernel image (linker symbols) before handing pages to the allocator.
- Fail safe: if any reservation can't be read, `platform_mem_regions()` returns
  no RAM at all, rather than a map that might contain reserved memory.

## Alternatives considered and why not
**Port libfdt**
- The point of the project is to learn; writing the parser myself is how I
  learn the format.
- It expects libc-style headers and functions that would need shims in a
  freestanding kernel.

**Unflatten the tree into structs (Linux-style)**
- Needs an allocator, and the allocator needs the RAM map from the DTB first,
  so the first parse has to work in place anyway.

**Pass the blob to every call (like libfdt)**
- The firmware passes exactly one DTB per boot, so the argument would always be the same.
- Shorter calls: callers don't carry the blob pointer around, and the header is
  checked once, in `fdt_init()`.
- Handles stay small: `struct fdt_node` is just an offset, valid for the selected blob.

Source: [Devicetree Specification, chapter 5 "Flattened Devicetree (DTB) Format"](https://devicetree-specification.readthedocs.io/en/stable/flattened-format.html)

## Consequences
- The DTB stays reserved for the whole uptime, because drivers will read it later.
- Once the MMU is on, the DTB's physical range has to be mapped for the parser to keep working.
- The RAM and reserved range tables are static arrays of 16 entries
  (`MEM_MAX_RAM_RANGES`, `MEM_MAX_RESERVED_RANGES`). On hardware with more regions,
  the extra RAM is dropped and an error is reported. Too many reservations make
  the RAM map empty (fail safe).
- Every lookup rescans the blob from the node (O(n)). That's fine at boot, but
  slow if drivers query the tree often; that would be the time to unflatten it.
