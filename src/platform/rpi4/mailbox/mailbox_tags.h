/**
 * @file mailbox_tags.h
 * @brief Typed helpers for property tags: one value struct and one add function per tag.
 * @ingroup mailbox
 *
 * Each mailbox_request_*() function only appends its tag to a request with
 * mailbox_prop_add(), sized from its value struct; nothing is sent. Several tags can
 * be added to one request, which the caller then sends with mailbox_prop_send(). Each
 * function returns a pointer to the tag's value words as its value struct, so the
 * caller reads the firmware's answer by member name after checking it with
 * mailbox_prop_tag_ok().
 *
 * Every function returns NULL if the tag doesn't fit in the request buffer; the request
 * is then marked as overflowed and mailbox_prop_send() fails.
 *
 * Value layouts follow the "Mailbox property interface" page of the Raspberry Pi
 * firmware wiki: https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
 * Values are in the CPU's byte order (little-endian on the Pi 4).
 *
 * Context: same as mailbox_prop_add(); not for interrupt handlers.
 */
#pragma once

#include "mailbox.h"

/// @brief Value of #MBOX_TAG_GET_FIRM_VER
struct mbox_firmware_ver
{
	uint32_t version; ///< Firmware revision
};

/**
 * @brief Adds a "get firmware revision" tag to a request.
 *
 * @param p Request being built, from mailbox_prop_begin()
 * @return The tag's value, valid after mailbox_prop_send(); NULL if it doesn't fit
 */
static inline struct mbox_firmware_ver* mailbox_request_firmware_version(struct mailbox_prop* p)
{
	return (struct mbox_firmware_ver*) mailbox_prop_add(p, MBOX_TAG_GET_FIRM_VER, sizeof(struct mbox_firmware_ver));
}

/// @brief Value of #MBOX_TAG_GET_BOARD_MODEL
struct mbox_board_model
{
	uint32_t model; ///< Board model
};

/**
 * @brief Adds a "get board model" tag to a request.
 *
 * @param p Request being built, from mailbox_prop_begin()
 * @return The tag's value, valid after mailbox_prop_send(); NULL if it doesn't fit
 */
static inline struct mbox_board_model* mailbox_request_board_model(struct mailbox_prop* p)
{
	return (struct mbox_board_model*) mailbox_prop_add(p, MBOX_TAG_GET_BOARD_MODEL, sizeof(struct mbox_board_model));
}

/// @brief Value of #MBOX_TAG_GET_BOARD_REVISION
struct mbox_board_revision
{
	uint32_t revision; ///< Board revision code
};

/**
 * @brief Adds a "get board revision" tag to a request.
 *
 * @param p Request being built, from mailbox_prop_begin()
 * @return The tag's value, valid after mailbox_prop_send(); NULL if it doesn't fit
 */
static inline struct mbox_board_revision* mailbox_request_board_revision(struct mailbox_prop* p)
{
	return (struct mbox_board_revision*) mailbox_prop_add(p, MBOX_TAG_GET_BOARD_REVISION, sizeof(struct mbox_board_revision));
}

/**
 * @brief Value of #MBOX_TAG_GET_BOARD_SERIAL
 *
 * The firmware returns one u64 in the CPU's byte order. On a little-endian CPU its low
 * half comes first, hence the two words: the serial is (@c high << 32) | @c low.
 */
struct mbox_board_serial
{
	uint32_t low;  ///< Low 32 bits of the board serial
	uint32_t high; ///< High 32 bits of the board serial
};

/**
 * @brief Adds a "get board serial" tag to a request.
 *
 * @param p Request being built, from mailbox_prop_begin()
 * @return The tag's value, valid after mailbox_prop_send(); NULL if it doesn't fit
 */
static inline struct mbox_board_serial* mailbox_request_board_serial(struct mailbox_prop* p)
{
	return (struct mbox_board_serial*) mailbox_prop_add(p, MBOX_TAG_GET_BOARD_SERIAL, sizeof(struct mbox_board_serial));
}

/**
 * @brief Value of #MBOX_TAG_GET_ARM_MEMORY
 *
 * One region only. The firmware wiki notes that future formats may return several
 * base and size pairs.
 */
struct mbox_arm_memory
{
	uint32_t base; ///< Base address of the region
	uint32_t size; ///< Size of the region in bytes
};

/**
 * @brief Adds a "get ARM memory" tag to a request.
 *
 * @param p Request being built, from mailbox_prop_begin()
 * @return The tag's value, valid after mailbox_prop_send(); NULL if it doesn't fit
 */
static inline struct mbox_arm_memory* mailbox_request_arm_memory(struct mailbox_prop* p)
{
	return (struct mbox_arm_memory*) mailbox_prop_add(p, MBOX_TAG_GET_ARM_MEMORY, sizeof(struct mbox_arm_memory));
}

/// @brief Value of #MBOX_TAG_GET_VC_MEMORY
struct mbox_vc_memory
{
	uint32_t base; ///< Base address of the VideoCore's region
	uint32_t size; ///< Size of the region in bytes
};

/**
 * @brief Adds a "get VideoCore memory" tag to a request.
 *
 * @param p Request being built, from mailbox_prop_begin()
 * @return The tag's value, valid after mailbox_prop_send(); NULL if it doesn't fit
 */
static inline struct mbox_vc_memory* mailbox_request_vc_memory(struct mailbox_prop* p)
{
	return (struct mbox_vc_memory*) mailbox_prop_add(p, MBOX_TAG_GET_VC_MEMORY, sizeof(struct mbox_vc_memory));
}

// video ones (used in fb.c) not covered for now
