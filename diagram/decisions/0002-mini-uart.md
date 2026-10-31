# 0002: Mini UART

- Date: 2026-10-31
- Status: accepted

## Context
The `mini_uart (UART1)` uses the `AUX_*` registers while the rest of the `UART[02345]` use their respective `UART` registers.

## Decision
Expose all UARTs (UART0-5) through one public API, `uart.h`, taking a `uart_t`.
Implement it with four modules:
- `uart.h/c`: public API; dispatches on `uart_t`.
- `aux.h/c`: owns shared AUX registers (`AUX_ENABLES`, `AUX_IRQ`); also used
  later by SPI1/SPI2.
- `uart_mini.h/c`: internal; UART1 via the `AUX_MU_*` registers. Depends on `aux`, `gpio`.
- `uart_pl011.h/c`: internal; UART0, 2-5 via the PL011 registers. Depends on `gpio`.

Register blocks follow decision `0001`.

## Alternatives considered
**UART1 code inside `uart.c`**:
- Mixes two register models
- baud formulas and flag polarities in one file
- PL011 growth (IRQ, DMA) would bury it

**Separate public `mini_uart` module**:
- Callers would need to know `UART1` is special

**UART1 enable handled inside `uart_mini.c`**:
- Shared `AUX_ENABLES` would have several writers once SPI1/2 exist

## Consequences
- Two extra header/source pairs and a thin dispatch layer in `uart.c`.
- Differences (flag polarity, baud formula, clock source) are hidden behind the API.
- UART1 is the only UART that depends on `aux`.