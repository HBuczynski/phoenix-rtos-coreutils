/*
 * Phoenix-RTOS
 *
 * psd - Serial Download Protocol client
 *
 * Copyright 2019 Phoenix Systems
 * Author: Bartosz Ciesla, Pawel Pisarczyk, Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

//#include <usbclient.h>

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/platform.h>
#include <sys/reboot.h>

#include <phoenix/arch/imxrt.h>

#include "../common/hid.h"
#include "../common/sdp.h"


#define SET_OPEN_HAB(b) (b)[0]=3;(b)[1]=0x56;(b)[2]=0x78;(b)[3]=0x78;(b)[4]=0x56;
#define SET_CLOSED_HAB(b) (b)[0]=3;(b)[1]=0x12;(b)[2]=0x34;(b)[3]=0x34;(b)[4]=0x12;
#define SET_COMPLETE(b) (b)[0]=4;(b)[1]=0x12;(b)[2]=0x8a;(b)[3]=0x8a;(b)[4]=0x12;
#define SET_FILE_COMPLETE(b) (b)[0]=4;(b)[1]=0x88;(b)[2]=0x88;(b)[3]=0x88;(b)[4]=0x88;
#define SET_HAB_ERROR(b, err) (b)[0]=4;(b)[1]=err;(b)[2]=0xaa;b[3]=0xaa;(b)[4]=0xaa;


#define HID_REPORT_1_SIZE (sizeof(sdp_cmd_t) + 1)
#define HID_REPORT_2_SIZE 1025
#define HID_REPORT_3_SIZE 5
#define HID_REPORT_4_SIZE 65


#define LOG(str, ...) do { if (1) fprintf(stderr, "psd: " str "\n", ##__VA_ARGS__); } while (0)
#define LOG_ERROR(str, ...) do { fprintf(stderr, __FILE__  ":%d error: " str "\n", __LINE__, ##__VA_ARGS__); } while (0)


struct {
	int run;
	char buff[SIZE_PAGE];

	int (*rf)(int, char *, unsigned int, char **);
	int (*sf)(int, const char *, unsigned int);

} psd;


const hid_dev_setup_t hid_setup = {
	 .ddev = {
		.len = sizeof(usbclient_desc_dev_t), .desc_type = USBCLIENT_DESC_TYPE_DEV, .bcd_usb = 0x200,
		.dev_class = 0, .dev_subclass = 0, .dev_prot = 0, .max_pkt_sz0 = 64,
		.vend_id = 0x1FC9, .prod_id = 0x0135, .bcd_dev = 0x0001,
		.man_str = 0, .prod_str = 0, .sn_str = 0,
		.num_conf = 1
	},
	.dstrman = {
		.len = 27 * 2 + 2,
		.desc_type = USBCLIENT_DESC_TYPE_STR,
		.string = { 'F', 0, 'r', 0, 'e', 0, 'e', 0, 's', 0, 'c', 0, 'a', 0, 'l', 0, 'e', 0, ' ', 0, 'S', 0, 'e', 0, 'm', 0, 'i', 0, 'C', 0, 'o', 0, 'n', 0, 'd', 0, 'u', 0, 'c', 0, 't', 0, 'o', 0, 'r', 0, ' ', 0, 'I', 0, 'n', 0, 'c', 0 }
	},
	.dstrprod = {
		.len = 11 * 2 + 2,
		.desc_type = USBCLIENT_DESC_TYPE_STR,
		.string = { 'S', 0, 'E', 0, ' ', 0, 'B', 0, 'l', 0, 'a', 0, 'n', 0, 'k', 0, ' ', 0, 'R', 0, 'T', 0 }
	}
};


int psd_hidResponse(int err, int type)
{
	int res;

	if (!err) {
		/* Report 3 device to host */
		SET_OPEN_HAB(psd.buff);
		if ((res = psd.sf(psd.buff[0], psd.buff, HID_REPORT_3_SIZE)) < 0)
			err = -eReport3;

		/* Report 4 device to host */
		switch (type) {
		case SDP_WRITE_FILE :
			SET_FILE_COMPLETE(psd.buff);
			memset(psd.buff + 5, 0, HID_REPORT_4_SIZE - 5);
			if ((res = psd.sf(psd.buff[0], psd.buff, HID_REPORT_4_SIZE)) < 0)
				err = -eReport4;
			break;

		case SDP_WRITE_REGISTER :
			SET_COMPLETE(psd.buff);
			memset(psd.buff + 5, 0, HID_REPORT_4_SIZE - 5);
			if ((res = psd.sf(psd.buff[0], psd.buff, HID_REPORT_4_SIZE)) < 0)
				err = -eReport4;
			break;

		default :
			err = -eReport4;
		}
	}
	else {
		/* Report 3 device to host */
		SET_CLOSED_HAB(psd.buff);
		if ((res = psd.sf(psd.buff[0], psd.buff, HID_REPORT_3_SIZE)) < 0)
			err = -eReport3;

		/* Report 4 device to host */
		SET_HAB_ERROR(psd.buff, err);
		memset(psd.buff + 5, 0, HID_REPORT_4_SIZE - 5);
		if ((res = psd.sf(psd.buff[0], psd.buff, HID_REPORT_4_SIZE)) < 0)
			err = -eReport4;
	}

	return err;
}


int psd_writeRegister(sdp_cmd_t *cmd)
{
	int err = hidOK;
	int address = (int)cmd->address;
	uint32_t data = cmd->data;

	LOG("Register: %d; data: %d", address, data);
	err = psd_hidResponse(err, SDP_WRITE_REGISTER);

	return err;
}


int psd_writeFile(sdp_cmd_t *cmd)
{
	int err = hidOK;

	err = psd_hidResponse(err, SDP_WRITE_FILE);

	return err;
}


static void psd_enabelCache(unsigned char enable)
{
	platformctl_t pctl;

	pctl.action = pctl_set;
	pctl.type = pctl_devcache;
	pctl.devcache.state = !!enable;

	platformctl(&pctl);
}


int main(int argc, char **argv)
{
	int err;
	sdp_cmd_t *pcmd = NULL;
	char cmdBuff[HID_REPORT_1_SIZE];

	psd.run = 1;
	psd_enabelCache(0);

	LOG("Initializing USB transport.");
	if (hid_init(&psd.rf, &psd.sf, &hid_setup)) {
		LOG_ERROR("Couldn't initialize USB transport.");
		return -1;
	}

	while (psd.run)
	{
		psd.rf(0, (void *)cmdBuff, sizeof(*pcmd) + 1, (void **)&pcmd);

		switch (pcmd->type) {
			case SDP_WRITE_REGISTER:

				if ((err = psd_writeRegister(pcmd)) != hidOK) {
					LOG_ERROR("Error during sdp write register, err: %d.", err);
					return err;
				}
				break;
			case SDP_WRITE_FILE:
				if ((err = psd_writeFile(pcmd)) != hidOK) {
					LOG_ERROR("Error during sdp write file, err: %d.", err);
					return err;
				}
				break;
			default:
				LOG_ERROR("Unrecognized command (%#x)", pcmd->type);
				break;
		}

		usleep(200);
	}

	psd_enabelCache(1);
	LOG("Closing PSD. Device is rebooting.");

	return EOK;
}
