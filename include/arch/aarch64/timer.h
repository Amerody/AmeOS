#pragma once

/**
 * @addtogroup timer
 * @{
 */

/**
 * @file timer.h
 *
 * @brief Timer driver header
 */

#include <stdint.h>

/**
 * @brief Initializes the timer
 *
 */
void timer_init();

/**
 * @brief Gets the time in ticks
 *
 * @return Time in ticks
 */
uint64_t timer_get_ticks();

/**
 * @brief Sleeps for the specified amount of time
 *
 * @param ms Amount of time to sleep for in milliseconds
 */
void timer_sleep(uint32_t ms);

<<<<<<< HEAD
=======
/**
 * @brief Initializes the timer interrupts
 *
 * @param hz
 */
void timer_interrupt_init(uint32_t hz);

/**
 * @brief Enables the timer interrupts
 *
 */
void timer_interrupt_enable();

// /**
//  * @brief
//  *
//  */
// void timer_irq_handler();

>>>>>>> homeserver/Dev
/** @} */
