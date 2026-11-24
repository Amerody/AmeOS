# 0006: Peripheral base addresses from the device tree {#decision_0006-peripheral-base-from-dtb}

- Date: 2026-11-25
- Status: accepted

## Context
The drivers address their registers as offsets from two windows: the main
peripheral window (`PERIPHERAL_BASE`) and the ARM-local window (`ARM_LOCAL_BASE`,
used by the GIC-400). Until now the main base was the constant `0xFE000000`,
which is only right in low peripheral mode. With `arm_peri_high=1` in
`config.txt` the firmware moves the peripherals, and every driver would touch
the wrong addresses.

The DTB is parsed now anyway (decision `0005`), and `/soc`'s `ranges` says where
the CPU sees each window.

## Decision
- `soc.h` keeps the bus addresses of the two windows (`SOC_PERIPHERAL_BUS_BASE`
  = `0x7E000000`, `SOC_ARM_LOCAL_BUS_BASE` = `0x40000000`), the addresses the
  device tree and the BCM2711 document use.
- `platform_init()` translates both through `/soc`'s `ranges` with
  `fdt_translate_address()` and stores the CPU addresses in `g_peripheral_base`
  and `g_arm_local_base`. `PERIPHERAL_BASE` and `ARM_LOCAL_BASE` now expand to
  these globals, so the drivers keep their datasheet offsets unchanged.
- Fallback: if the DTB is invalid, has no `/soc`, or its `ranges` doesn't cover
  a window, that base keeps its low peripheral mode default
  (`SOC_PERIPHERAL_BASE_DEFAULT`, `SOC_ARM_LOCAL_BASE_DEFAULT`) and the problem
  is reported through `platform_get_init_errors()`. Booting with the defaults
  and a visible error is better than halting.

## Alternatives considered and why not
**Keep the compile-time constants**
- `0xFE000000` is wrong in high peripheral mode, which would break every driver.
- A build option for high mode would tie the kernel image to one `config.txt` setting.

**Each driver looks up its own node and translates its own `reg`**
- More work per driver: every driver would need its own node lookup instead of
  a datasheet offset from a window base.

Source: BCM2711 ARM Peripherals, section 1.2 "Address map"

## Consequences
- `PERIPHERAL_BASE` and `ARM_LOCAL_BASE` aren't constants anymore. Every
  register access first loads a global, and the macros can't be used in static
  initializers or assembly.
- Init order: `platform_init()` must set both bases (so `fdt_init()` must succeed
  first) before any driver touches a register, including the GIC in `hal_irq_init()`.
