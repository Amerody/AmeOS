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

/** @} */
