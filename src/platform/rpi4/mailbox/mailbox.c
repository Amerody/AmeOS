#include "mailbox.h"

#include <common/mmio.h>
#include <soc.h>

/// @brief All the relevant VideoCore Mailbox Registers
/// @note There is not a lot of publicly available documentation so this is only the required registers sourced from tutorials
/// @warning @p RESERVED_* are not to be used
typedef struct
{
	volatile uint32_t READ; ///< VideoCore Mailbox Read Register

	uint32_t RESERVED_04_0F[CALC_OFFSET(0x04, 0x0F)]; ///< 12 Bytes gap between READ and POLL

	volatile uint32_t POLL;   ///< VideoCore Mailbox Poll Register
	volatile uint32_t SENDER; ///< VideoCore Mailbox Sender Register
	volatile uint32_t STATUS; ///< VideoCore Mailbox Status Register
	volatile uint32_t CONFIG; ///< VideoCore Mailbox Config Register
	volatile uint32_t WRITE;  ///< VideoCore Mailbox Write Register
} VC_MBOX_Registers;

#define VC_MBOX_BASE (PERIPHERAL_BASE + 0x0000B880) ///< VideoCore Mailbox base address
#define VC_MBOX ((VC_MBOX_Registers*) VC_MBOX_BASE) ///< VideoCore Mailbox access

#define MBOX_RESPONSE 0x80000000
#define MBOX_FULL 0x80000000
#define MBOX_EMPTY 0x40000000

volatile uint32_t mailbox_buffer[36];

int mailbox_call(uint8_t channel)
{
	// 28-bit address (MSB) and 4-bit value (LSB)
	uint32_t r = ((uint32_t) ((uint64_t) &mailbox_buffer) & ~0xF) | (channel & 0xF);

	// Wait until we can write
	while (mmio_read(&VC_MBOX->STATUS) & MBOX_FULL)
		;

	// Write the address of our buffer to the mailbox with the channel appended
	mmio_write(&VC_MBOX->WRITE, r);

	while (1)
	{
		// Is there a reply?
		while (mmio_read(&VC_MBOX->STATUS) & MBOX_EMPTY)
			;

		// Is it a reply to our message?
		if (r == mmio_read(&VC_MBOX->READ))
			return (mailbox_buffer[1] == MBOX_RESPONSE) ? 0 : -1; // Is it successful?
	}
	return 0;
}