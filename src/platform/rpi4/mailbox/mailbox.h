#pragma once

#include <stdint.h>

extern volatile uint32_t mailbox_buffer[36] __attribute__((aligned(16)));

#define MAILBOX_CH_PROP 8

#define MBOX_TAG_FB_ALLOCATE 0x40001
#define MBOX_TAG_FB_SET_PHYS_WH 0x48003
#define MBOX_TAG_FB_SET_VIRT_WH 0x48004
#define MBOX_TAG_FB_SET_DEPTH 0x48005
#define MBOX_TAG_FB_GET_PITCH 0x40008
#define MBOX_TAG_FB_SET_VIRT_OFF 0x48009

int mailbox_call(uint8_t channel);
