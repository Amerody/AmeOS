// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

/**
 * @file hal_irq.h
 * @brief Interrupt-controller contract: per-source enable, handlers, dispatch.
 *
 * Implemented by src/platform/<PLATFORM>/ (GIC-400 on rpi4).
 *
 * This is the controller side only. Whether the CPU accepts interrupts at all
 * is a separate switch, see arch/irq.h. An interrupt is delivered only when the
 * peripheral raises it, the source is enabled here, and the CPU has IRQs
 * unmasked.
 *
 * IRQ numbers are platform-defined identifiers (GIC interrupt IDs on rpi4).
 * Portable code never hardcodes them. Their callers are platform code for its
 * own devices and arch code for CPU-local sources the architecture fixes (for
 * example the aarch64 generic timer, which uses the number the architecture
 * assigns to it).
 *
 * See platform.h for the shared conventions (errors, contexts, init order).
 */
#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

	/**
	 * @brief Handler for one interrupt source.
	 *
	 * Runs in interrupt context with CPU IRQs masked. It must be short and must
	 * not block or sleep. Acknowledging the end of the interrupt at the controller
	 * is done by hal_irq_dispatch(), not by the handler. The handler only has to
	 * quiet the device that raised it.
	 *
	 * @param arg The pointer given to hal_irq_register().
	 */
	typedef void (*hal_irq_handler_t)(void* arg);

	/**
	 * @brief Initializes the handling of interrupts
	 *
	 * In rpi4 this implies enabling the GIC-400
	 */
	void hal_irq_init();

	/**
	 * @brief Attach a handler to an interrupt source.
	 *
	 * Does not enable the source: register it first, then call hal_irq_enable(), so
	 * an interrupt can never arrive without a handler.
	 *
	 * @param irq      Platform IRQ number.
	 * @param handler  Function to call; must not be NULL.
	 * @param arg      Passed back to the handler unchanged; may be NULL.
	 * @return 0 on success, -ERROR_INVALID_ARG if @p irq does not exist on this platform,
	 *         -ERROR_BUSY if the source already has a handler.
	 */
	int hal_irq_register(unsigned irq, hal_irq_handler_t handler, void* arg);

	/**
	 * @brief Enable a source at the interrupt controller (and route it to this CPU).
	 *
	 * Does not unmask the CPU (arch_irq_enable()). A source without a registered
	 * handler must not be enabled.
	 *
	 * @param irq Platform IRQ number. Out-of-range numbers are ignored.
	 *
	 * @bug rpi4: nothing is routed "to this CPU": only GICD_ISENABLERn is written, so an
	 *      SPI keeps the target gic_init() gave it (core 0, GICD_ITARGETSRn). Latent: only
	 *      core 0 runs today; a secondary core enabling an SPI would never receive it
	 *      (GICv2 spec, 4.3.12 GICD_ITARGETSRn).
	 */
	void hal_irq_enable(unsigned irq);

	/**
	 * @brief Disable a source at the interrupt controller.
	 *
	 * An interrupt already pending in the controller may still be delivered once
	 * after this returns if it was taken before the call completed.
	 *
	 * @param irq Platform IRQ number. Out-of-range numbers are ignored.
	 */
	void hal_irq_disable(unsigned irq);

	/**
	 * @brief Handle one IRQ exception. Called by the arch exception entry only.
	 *
	 * Core and drivers never call this. For every IRQ exception the arch vectors
	 * call it, with CPU IRQs masked, and it does the whole controller dance:
	 * acknowledge the interrupt, run the registered handler, signal end of
	 * interrupt. This keeps controller-specific steps (the GIC's ack/EOI pair, or
	 * whatever another controller needs) out of core.
	 *
	 * Spurious interrupts (reported by the controller, or arriving for a source
	 * with no handler) are dropped without calling anything and, where the
	 * controller requires it, still completed.
	 *
	 * Valid after platform_init().
	 */
	void hal_irq_dispatch();

#ifdef __cplusplus
}
#endif
