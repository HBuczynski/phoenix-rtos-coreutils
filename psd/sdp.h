/*
 * Phoenix-RTOS
 *
 * psd - Serial Download Protocol client
 *
 * Copyright 2019 Phoenix Systems
 * Author: Bartosz Ciesla, Pawel Pisarczyk
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _SDP_H_
#define _SDP_H_


enum {
	SDP_READ_REGISTER = 0x0101,
	SDP_WRITE_REGISTER = 0x0202,
	SDP_WRITE_FILE = 0x0404,
	SDP_ERROR_STATUS = 0x0505,
	SDP_DCD_WRITE = 0x0a0a,
	SDP_JUMP_ADDRESS = 0x0b0b,
	SDP_DCD_SKIP = 0x0c0c
};


typedef struct _sdp_cmd_t {
	u16 type;
	u32 address;
	u8 format;
	u32 datasz;
	u32 data;
	u8 reserved;
} __attribute__((packed)) sdp_cmd_t;


#endif
