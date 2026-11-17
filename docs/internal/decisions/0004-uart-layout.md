# 0004: UART Layout {#decision_0004-uart-layout}

- Date: 2026-11-17
- Status: accepted

## Context
Decision `0002` described the UART modules before the HAL existed, and the code has
moved on since:
- The UART is now reached through the HAL contract `include/hal/hal_uart.h`, not a
  public `uart.h` taking a `uart_t`.
- Windows reserves the file name `aux`.
- Receiving data needs interrupts, and the mini UART, SPI1 and SPI2 share one AUX
  interrupt line.

This record replaces decision `0002`.

## Decision
The UART contract is `include/hal/hal_uart.h`: `hal_uart_init(port, baud)` and
`hal_uart_putc(port, c)`, with an `unsigned port`. Ports use the hardware numbers:
port N is UARTN, so the mini UART is port 1.
The kernel mostly works with drivers (e.g. `console_uart`), which are built on the HAL.

The rpi4 implementation, in `src/platform/rpi4/`:
- `uart/uart.c`: implements `hal_uart.h`; dispatches on the port number.
- `uart/uart_mini.h/c`: internal; UART1 via the `AUX_MU_*` registers. Depends on
  `auxiliary`, `gpio`.
- `uart/uart_pl011.h/c`: internal; UART0, 2-5 via the PL011 registers. Depends on `gpio`.
- `auxiliary/auxiliary.h/c`: owns the shared AUX registers (`AUX_ENABLES`, `AUX_IRQ`)
  and the shared AUX interrupt line.
- `auxiliary/aux_regs.h`: the `AUX_Registers` struct, shared by `uart_mini.c` and the
  future SPI1/SPI2 drivers.

AUX interrupts: `aux_init()` registers one `hal_irq` handler on the AUX line. The
handler reads `AUX_IRQ` and calls the callback each peripheral set with
`aux_set_callback()`. `aux_init()` will be called by either `console_uart` or
`uart_mini_enable()` (not decided yet).

Register blocks follow decision `0001`.

## Alternatives considered and why not
**Public `uart.h` with a `uart_t` (decision `0002`)**
- The kernel should mostly work with the drivers, and the drivers are built on the HAL,
  so the UART interface belongs in the HAL contract.

**UART1 code inside `uart.c`**
- Mixes two register models
- baud formulas and flag polarities in one file
- PL011 growth (IRQ, DMA) would bury it

**Separate public `mini_uart` module**
- Callers would need to know `UART1` is special

**UART1 enable handled inside `uart_mini.c`**
- Shared `AUX_ENABLES` would have several writers once SPI1/2 exist

**`aux.h/c`**
- Windows reserves the file name `aux`.

**`AUX_Registers` inside `auxiliary.c`, as decision `0001` does for other blocks**
- `uart_mini.c` and the future SPI1/SPI2 drivers all need the struct.

**Each AUX driver registers its own `hal_irq` handler on the AUX line**
- `hal_irq_register()` allows one handler per IRQ and returns `-EBUSY` for a second one.

**`uart_mini` reads `AUX_IRQ` itself**
- The shared AUX registers would have several owners once SPI1/2 exist, the same
  reason `AUX_ENABLES` is owned by `auxiliary`.

## Consequences
- Two extra header/source pairs and a thin dispatch layer in `uart.c`.
- Differences (flag polarity, baud formula, clock source) are hidden behind the API.
- UART1 is the only UART that depends on `auxiliary`.
- `aux_regs.h` breaks the "register struct lives in the `.c`" rule: any file that
  includes it can touch every AUX register, so `auxiliary` owning `AUX_ENABLES` and
  `AUX_IRQ` is a convention, not enforced.
- Port numbers are platform-specific, so portable drivers can't assume one; `kmain`
  picks the console port (`console_uart_init(1)`).
- If `uart_mini_enable()` calls `aux_init()`, `aux_init()` must be safe to call more
  than once (reconfiguring the port, later the SPI drivers), since a second
  `hal_irq_register()` fails with `-EBUSY`.
- The RX contract (how received bytes reach the kernel) is not covered here; it gets
  its own record.
