#pragma once

/**
 * @addtogroup mmio
 * @{
 */

/**
 * @file mmio.h
 *
 * @brief Memory Mapped I/O Read/Write operations
 */

#include <stdint.h>

/// @brief Memory mapped 32-bit register
typedef volatile uint32_t* mmio_register32_t;

/// @brief Memory mapped 16-bit register
typedef volatile uint16_t* mmio_register16_t;

/// @brief Memory mapped 8-bit register
typedef volatile uint8_t* mmio_register8_t;

/// @brief Macro to calculate the offset to write on a register definition, use addresses aligned to 4 Bytes.
/// @code uint32_t RESERVED_90_E0[CALC_OFFSET(0x90, 0xE0)]; ///< 84 Bytes gap between GPAFEN1 and GPIO_PUP_PDN_CNTRL_REG0 @endcode
#define CALC_OFFSET(start, end) ((((end) - (start)) / 4) + 1)

/**
 * @brief Reads a whole Memory Mapped I/O register
 *
 * @param reg Register to read from
 * @return Value read from the register
 */
static inline uint32_t mmio_read(mmio_register32_t reg) { return *reg; }

/**
 * @brief Writes a whole Memory Mapped I/O register
 *
 * Does not read the register first, so it is safe for registers with
 * write-1-to-clear or clear-on-read bits. Every bit is overwritten,
 * including reserved ones: build the value according to the datasheet
 *
 * @param reg Register to write to
 * @param value Value to write
 */
static inline void mmio_write(mmio_register32_t reg, uint32_t value) { *reg = value; }

/**
 * @brief Reads a field of a Memory Mapped I/O register
 *
 * Performs a single read of the register. Be careful with registers
 * that have clear-on-read side effects.
 *
 * @param reg   Register to read from
 * @param mask  Field mask, unshifted (e.g. 0x7 for a 3-bit field)
 * @param shift Bit position of the field's lowest bit
 * @return Field value, moved down to bit 0 (same form as mmio_write_field()'s
 *         @p value)
 */
static inline uint32_t mmio_read_field(mmio_register32_t reg, uint32_t mask, uint32_t shift)
{
	return (mmio_read(reg) >> shift) & mask;
}

/**
 * @brief Writes a field of a Memory Mapped I/O register, preserving all other bits
 *
 * Performs a NON-ATOMIC read-modify-write. An ISR or another core
 * modifying the same register in between will have its change lost.
 * Do not use on registers with write-1-to-clear or clear-on-read bits;
 * use mmio_write instead.
 *
 * @param reg   Register to modify
 * @param value Field value, starting at bit 0 (not shifted into place).
 *              Truncated to the field width
 * @param mask  Field mask, unshifted (e.g. 0x7 for a 3-bit field)
 * @param shift Bit position of the field's lowest bit
 */
static inline void mmio_write_field(mmio_register32_t reg, uint32_t value, uint32_t mask, uint32_t shift)
{
	uint32_t current_value = mmio_read(reg);
	current_value &= ~(mask << shift);
	current_value |= (value & mask) << shift;
	mmio_write(reg, current_value);
}

/**
 * @brief Writes a whole 8-bit Memory Mapped I/O register
 *
 * Does not read the register first, so it is safe for registers with
 * write-1-to-clear or clear-on-read bits. Every bit is overwritten,
 * including reserved ones: build the value according to the datasheet
 *
 * @param reg Register to write to
 * @param value Value to write
 */
static inline void mmio_write8(mmio_register8_t reg, uint8_t value) { *reg = value; }

/**
 * @brief Reads a whole 8-bit Memory Mapped I/O register
 *
 * @param reg Register to read from
 * @return Value read from the register
 */
static inline uint8_t mmio_read8(mmio_register8_t reg) { return *reg; }

/** @} */
