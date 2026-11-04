#include "platform/rpi4/auxiliary.h"

#include "aux_regs.h"
#include "platform/rpi4/mmio.h"

/**
 * @addtogroup aux
 * @{
 */

/**
 * @file auxiliary.c
 *
 * @brief Implementation of the AUX Peripherals Driver
 */

#include <stdint.h>

void aux_enable(aux_peripheral_t peripheral)
{
	mmio_write_field(&AUX->ENABLES, 1, 0x1, (uint32_t)peripheral);
}

void aux_disable(aux_peripheral_t peripheral)
{
	mmio_write_field(&AUX->ENABLES, 0, 0x1, (uint32_t)peripheral);
}

bool aux_is_enabled(aux_peripheral_t peripheral)
{
<<<<<<< HEAD
	return (bool)mmio_read_field(&AUX->AUX_ENABLES, 0x1, (uint32_t)peripheral);
=======
	return (bool)mmio_read_field(&AUX->ENABLES, 0x1, (uint32_t)peripheral);
>>>>>>> homeserver/Dev
}

/** @} */