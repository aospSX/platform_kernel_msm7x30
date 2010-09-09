/*
 * DVB USB Linux driver for Afatech AF9015 DVB-T USB2.0 receiver
 *
 * Copyright (C) 2007 Antti Palosaari <crope@iki.fi>
 *
 * Thanks to Afatech who kindly provided information.
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef _DVB_USB_AF9015_H_
#define _DVB_USB_AF9015_H_

#define DVB_USB_LOG_PREFIX "af9015"
#include "dvb-usb.h"

#define deb_info(args...) dprintk(dvb_usb_af9015_debug, 0x01, args)
#define deb_rc(args...)   dprintk(dvb_usb_af9015_debug, 0x02, args)
#define deb_xfer(args...) dprintk(dvb_usb_af9015_debug, 0x04, args)
#define deb_reg(args...)  dprintk(dvb_usb_af9015_debug, 0x08, args)
#define deb_i2c(args...)  dprintk(dvb_usb_af9015_debug, 0x10, args)
#define deb_fw(args...)   dprintk(dvb_usb_af9015_debug, 0x20, args)

#define AF9015_I2C_EEPROM  0xa0
#define AF9015_I2C_DEMOD   0x38
#define AF9015_USB_TIMEOUT 2000

/* EEPROM locations */
#define AF9015_EEPROM_IR_MODE        0x18
#define AF9015_EEPROM_IR_REMOTE_TYPE 0x34
#define AF9015_EEPROM_TS_MODE        0x31
#define AF9015_EEPROM_DEMOD2_I2C     0x32

#define AF9015_EEPROM_SAW_BW1        0x35
#define AF9015_EEPROM_XTAL_TYPE1     0x36
#define AF9015_EEPROM_SPEC_INV1      0x37
#define AF9015_EEPROM_IF1L           0x38
#define AF9015_EEPROM_IF1H           0x39
#define AF9015_EEPROM_MT2060_IF1L    0x3a
#define AF9015_EEPROM_MT2060_IF1H    0x3b
#define AF9015_EEPROM_TUNER_ID1      0x3c

#define AF9015_EEPROM_SAW_BW2        0x45
#define AF9015_EEPROM_XTAL_TYPE2     0x46
#define AF9015_EEPROM_SPEC_INV2      0x47
#define AF9015_EEPROM_IF2L           0x48
#define AF9015_EEPROM_IF2H           0x49
#define AF9015_EEPROM_MT2060_IF2L    0x4a
#define AF9015_EEPROM_MT2060_IF2H    0x4b
#define AF9015_EEPROM_TUNER_ID2      0x4c

#define AF9015_EEPROM_OFFSET (AF9015_EEPROM_SAW_BW2 - AF9015_EEPROM_SAW_BW1)

struct req_t {
	u8  cmd;       /* [0] */
	/*  seq */     /* [1] */
	u8  i2c_addr;  /* [2] */
	u16 addr;      /* [3|4] */
	u8  mbox;      /* [5] */
	u8  addr_len;  /* [6] */
	u8  data_len;  /* [7] */
	u8  *data;
};

enum af9015_cmd {
	GET_CONFIG           = 0x10,
	DOWNLOAD_FIRMWARE    = 0x11,
	BOOT                 = 0x13,
	READ_MEMORY          = 0x20,
	WRITE_MEMORY         = 0x21,
	READ_WRITE_I2C       = 0x22,
	COPY_FIRMWARE        = 0x23,
	RECONNECT_USB        = 0x5a,
	WRITE_VIRTUAL_MEMORY = 0x26,
	GET_IR_CODE          = 0x27,
	READ_I2C,
	WRITE_I2C,
};

enum af9015_ir_mode {
	AF9015_IR_MODE_DISABLED = 0,
	AF9015_IR_MODE_HID,
	AF9015_IR_MODE_RLC,
	AF9015_IR_MODE_RC6,
	AF9015_IR_MODE_POLLING, /* just guess */
};

struct af9015_state {
	struct i2c_adapter i2c_adap; /* I2C adapter for 2nd FE */
};

struct af9015_config {
	u8  dual_mode:1;
	u16 mt2060_if1[2];
	u16 firmware_size;
	u16 firmware_checksum;
	u32 eeprom_sum;
	u8  *ir_table;
	u16 ir_table_size;
};

enum af9015_remote {
	AF9015_REMOTE_NONE                    = 0,
/* 1 */	AF9015_REMOTE_A_LINK_DTU_M,
	AF9015_REMOTE_MSI_DIGIVOX_MINI_II_V3,
	AF9015_REMOTE_MYGICTV_U718,
	AF9015_REMOTE_DIGITTRADE_DVB_T,
/* 5 */	AF9015_REMOTE_AVERMEDIA_KS,
};

/* LeadTek - Y04G0051 */
/* Leadtek WinFast DTV Dongle Gold */
static struct ir_scancode ir_codes_af9015_table_leadtek[] = {
	{ 0x001e, KEY_1 },
	{ 0x001f, KEY_2 },
	{ 0x0020, KEY_3 },
	{ 0x0021, KEY_4 },
	{ 0x0022, KEY_5 },
	{ 0x0023, KEY_6 },
	{ 0x0024, KEY_7 },
	{ 0x0025, KEY_8 },
	{ 0x0026, KEY_9 },
	{ 0x0027, KEY_0 },
	{ 0x0028, KEY_OK },
	{ 0x004f, KEY_RIGHT },
	{ 0x0050, KEY_LEFT },
	{ 0x0051, KEY_DOWN },
	{ 0x0052, KEY_UP },
	{ 0x011a, KEY_POWER2 },
	{ 0x04b4, KEY_TV },
	{ 0x04b3, KEY_RED },
	{ 0x04b2, KEY_GREEN },
	{ 0x04b1, KEY_YELLOW },
	{ 0x04b0, KEY_BLUE },
	{ 0x003d, KEY_TEXT },
	{ 0x0113, KEY_SLEEP },
	{ 0x0010, KEY_MUTE },
	{ 0x0105, KEY_ESC },
	{ 0x0009, KEY_SCREEN },
	{ 0x010f, KEY_MENU },
	{ 0x003f, KEY_CHANNEL },
	{ 0x0013, KEY_REWIND },
	{ 0x0012, KEY_PLAY },
	{ 0x0011, KEY_FASTFORWARD },
	{ 0x0005, KEY_PREVIOUS },
	{ 0x0029, KEY_STOP },
	{ 0x002b, KEY_NEXT },
	{ 0x0041, KEY_EPG },
	{ 0x0019, KEY_VIDEO },
	{ 0x0016, KEY_AUDIO },
	{ 0x0037, KEY_DOT },
	{ 0x002a, KEY_AGAIN },
	{ 0x002c, KEY_CAMERA },
	{ 0x003c, KEY_NEW },
	{ 0x0115, KEY_RECORD },
	{ 0x010b, KEY_TIME },
	{ 0x0043, KEY_VOLUMEUP },
	{ 0x0042, KEY_VOLUMEDOWN },
	{ 0x004b, KEY_CHANNELUP },
	{ 0x004e, KEY_CHANNELDOWN },
};

static u8 af9015_ir_table_leadtek[] = {
	0x03, 0xfc, 0x00, 0xff, 0x1a, 0x01, 0x00, /* KEY_POWER2 */
	0x03, 0xfc, 0x56, 0xa9, 0xb4, 0x04, 0x00, /* KEY_TV */
	0x03, 0xfc, 0x4b, 0xb4, 0xb3, 0x04, 0x00, /* KEY_RED */
	0x03, 0xfc, 0x4c, 0xb3, 0xb2, 0x04, 0x00, /* KEY_GREEN */
	0x03, 0xfc, 0x4d, 0xb2, 0xb1, 0x04, 0x00, /* KEY_YELLOW */
	0x03, 0xfc, 0x4e, 0xb1, 0xb0, 0x04, 0x00, /* KEY_BLUE */
	0x03, 0xfc, 0x1f, 0xe0, 0x3d, 0x00, 0x00, /* KEY_TEXT */
	0x03, 0xfc, 0x40, 0xbf, 0x13, 0x01, 0x00, /* KEY_SLEEP */
	0x03, 0xfc, 0x14, 0xeb, 0x10, 0x00, 0x00, /* KEY_MUTE */
	0x03, 0xfc, 0x49, 0xb6, 0x05, 0x01, 0x00, /* KEY_ESC */
	0x03, 0xfc, 0x50, 0xaf, 0x29, 0x00, 0x00, /* KEY_STOP (1)*/
	0x03, 0xfc, 0x0c, 0xf3, 0x52, 0x00, 0x00, /* KEY_UP */
	0x03, 0xfc, 0x03, 0xfc, 0x09, 0x00, 0x00, /* KEY_SCREEN */
	0x03, 0xfc, 0x08, 0xf7, 0x50, 0x00, 0x00, /* KEY_LEFT */
	0x03, 0xfc, 0x13, 0xec, 0x28, 0x00, 0x00, /* KEY_OK (1) */
	0x03, 0xfc, 0x04, 0xfb, 0x4f, 0x00, 0x00, /* KEY_RIGHT */
	0x03, 0xfc, 0x4f, 0xb0, 0x0f, 0x01, 0x00, /* KEY_MENU */
	0x03, 0xfc, 0x10, 0xef, 0x51, 0x00, 0x00, /* KEY_DOWN */
	0x03, 0xfc, 0x51, 0xae, 0x3f, 0x00, 0x00, /* KEY_CHANNEL */
	0x03, 0xfc, 0x42, 0xbd, 0x13, 0x00, 0x00, /* KEY_REWIND */
	0x03, 0xfc, 0x43, 0xbc, 0x12, 0x00, 0x00, /* KEY_PLAY */
	0x03, 0xfc, 0x44, 0xbb, 0x11, 0x00, 0x00, /* KEY_FASTFORWARD */
	0x03, 0xfc, 0x52, 0xad, 0x19, 0x00, 0x00, /* KEY_VIDEO (1) */
	0x03, 0xfc, 0x54, 0xab, 0x05, 0x00, 0x00, /* KEY_PREVIOUS */
	0x03, 0xfc, 0x46, 0xb9, 0x29, 0x00, 0x00, /* KEY_STOP (2) */
	0x03, 0xfc, 0x55, 0xaa, 0x2b, 0x00, 0x00, /* KEY_NEXT */
	0x03, 0xfc, 0x53, 0xac, 0x41, 0x00, 0x00, /* KEY_EPG */
	0x03, 0xfc, 0x05, 0xfa, 0x1e, 0x00, 0x00, /* KEY_1 */
	0x03, 0xfc, 0x06, 0xf9, 0x1f, 0x00, 0x00, /* KEY_2 */
	0x03, 0xfc, 0x07, 0xf8, 0x20, 0x00, 0x00, /* KEY_3 */
	0x03, 0xfc, 0x1e, 0xe1, 0x19, 0x00, 0x00, /* KEY_VIDEO (2) */
	0x03, 0xfc, 0x09, 0xf6, 0x21, 0x00, 0x00, /* KEY_4 */
	0x03, 0xfc, 0x0a, 0xf5, 0x22, 0x00, 0x00, /* KEY_5 */
	0x03, 0xfc, 0x0b, 0xf4, 0x23, 0x00, 0x00, /* KEY_6 */
	0x03, 0xfc, 0x1b, 0xe4, 0x16, 0x00, 0x00, /* KEY_AUDIO */
	0x03, 0xfc, 0x0d, 0xf2, 0x24, 0x00, 0x00, /* KEY_7 */
	0x03, 0xfc, 0x0e, 0xf1, 0x25, 0x00, 0x00, /* KEY_8 */
	0x03, 0xfc, 0x0f, 0xf0, 0x26, 0x00, 0x00, /* KEY_9 */
	0x03, 0xfc, 0x16, 0xe9, 0x28, 0x00, 0x00, /* KEY_OK (2) */
	0x03, 0xfc, 0x41, 0xbe, 0x37, 0x00, 0x00, /* KEY_DOT */
	0x03, 0xfc, 0x12, 0xed, 0x27, 0x00, 0x00, /* KEY_0 */
	0x03, 0xfc, 0x11, 0xee, 0x2a, 0x00, 0x00, /* KEY_AGAIN */
	0x03, 0xfc, 0x48, 0xb7, 0x2c, 0x00, 0x00, /* KEY_CAMERA */
	0x03, 0xfc, 0x4a, 0xb5, 0x3c, 0x00, 0x00, /* KEY_NEW */
	0x03, 0xfc, 0x47, 0xb8, 0x15, 0x01, 0x00, /* KEY_RECORD */
	0x03, 0xfc, 0x45, 0xba, 0x0b, 0x01, 0x00, /* KEY_TIME */
	0x03, 0xfc, 0x5e, 0xa1, 0x43, 0x00, 0x00, /* KEY_VOLUMEUP */
	0x03, 0xfc, 0x5a, 0xa5, 0x42, 0x00, 0x00, /* KEY_VOLUMEDOWN */
	0x03, 0xfc, 0x5b, 0xa4, 0x4b, 0x00, 0x00, /* KEY_CHANNELUP */
	0x03, 0xfc, 0x5f, 0xa0, 0x4e, 0x00, 0x00, /* KEY_CHANNELDOWN */
};

/* TwinHan AzureWave AD-TU700(704J) */
static struct ir_scancode ir_codes_af9015_table_twinhan[] = {
	{ 0x053f, KEY_POWER },
	{ 0x0019, KEY_FAVORITES },    /* Favorite List */
	{ 0x0004, KEY_TEXT },         /* Teletext */
	{ 0x000e, KEY_POWER },
	{ 0x000e, KEY_INFO },         /* Preview */
	{ 0x0008, KEY_EPG },          /* Info/EPG */
	{ 0x000f, KEY_LIST },         /* Record List */
	{ 0x001e, KEY_1 },
	{ 0x001f, KEY_2 },
	{ 0x0020, KEY_3 },
	{ 0x0021, KEY_4 },
	{ 0x0022, KEY_5 },
	{ 0x0023, KEY_6 },
	{ 0x0024, KEY_7 },
	{ 0x0025, KEY_8 },
	{ 0x0026, KEY_9 },
	{ 0x0027, KEY_0 },
	{ 0x0029, KEY_CANCEL },       /* Cancel */
	{ 0x004c, KEY_CLEAR },        /* Clear */
	{ 0x002a, KEY_BACK },         /* Back */
	{ 0x002b, KEY_TAB },          /* Tab */
	{ 0x0052, KEY_UP },           /* up arrow */
	{ 0x0051, KEY_DOWN },         /* down arrow */
	{ 0x004f, KEY_RIGHT },        /* right arrow */
	{ 0x0050, KEY_LEFT },         /* left arrow */
	{ 0x0028, KEY_ENTER },        /* Enter / ok */
	{ 0x0252, KEY_VOLUMEUP },
	{ 0x0251, KEY_VOLUMEDOWN },
	{ 0x004e, KEY_CHANNELDOWN },
	{ 0x004b, KEY_CHANNELUP },
	{ 0x004a, KEY_RECORD },
	{ 0x0111, KEY_PLAY },
	{ 0x0017, KEY_PAUSE },
	{ 0x000c, KEY_REWIND },       /* FR << */
	{ 0x0011, KEY_FASTFORWARD },  /* FF >> */
	{ 0x0115, KEY_PREVIOUS },     /* Replay */
	{ 0x010e, KEY_NEXT },         /* Skip */
	{ 0x0013, KEY_CAMERA },       /* Capture */
	{ 0x010f, KEY_LANGUAGE },     /* SAP */
	{ 0x0113, KEY_TV2 },          /* PIP */
	{ 0x001d, KEY_ZOOM },         /* Full Screen */
	{ 0x0117, KEY_SUBTITLE },     /* Subtitle / CC */
	{ 0x0010, KEY_MUTE },
	{ 0x0119, KEY_AUDIO },        /* L/R */ /* TODO better event */
	{ 0x0116, KEY_SLEEP },        /* Hibernate */
	{ 0x0116, KEY_SWITCHVIDEOMODE },
					  /* A/V */ /* TODO does not work */
	{ 0x0006, KEY_AGAIN },        /* Recall */
	{ 0x0116, KEY_KPPLUS },       /* Zoom+ */ /* TODO does not work */
	{ 0x0116, KEY_KPMINUS },      /* Zoom- */ /* TODO does not work */
	{ 0x0215, KEY_RED },
	{ 0x020a, KEY_GREEN },
	{ 0x021c, KEY_YELLOW },
	{ 0x0205, KEY_BLUE },
};

static u8 af9015_ir_table_twinhan[] = {
	0x00, 0xff, 0x16, 0xe9, 0x3f, 0x05, 0x00,
	0x00, 0xff, 0x07, 0xf8, 0x16, 0x01, 0x00,
	0x00, 0xff, 0x14, 0xeb, 0x11, 0x01, 0x00,
	0x00, 0xff, 0x1a, 0xe5, 0x4d, 0x00, 0x00,
	0x00, 0xff, 0x4c, 0xb3, 0x17, 0x00, 0x00,
	0x00, 0xff, 0x12, 0xed, 0x11, 0x00, 0x00,
	0x00, 0xff, 0x40, 0xbf, 0x0c, 0x00, 0x00,
	0x00, 0xff, 0x11, 0xee, 0x4a, 0x00, 0x00,
	0x00, 0xff, 0x54, 0xab, 0x13, 0x00, 0x00,
	0x00, 0xff, 0x41, 0xbe, 0x15, 0x01, 0x00,
	0x00, 0xff, 0x42, 0xbd, 0x0e, 0x01, 0x00,
	0x00, 0xff, 0x43, 0xbc, 0x17, 0x01, 0x00,
	0x00, 0xff, 0x50, 0xaf, 0x0f, 0x01, 0x00,
	0x00, 0xff, 0x4d, 0xb2, 0x1d, 0x00, 0x00,
	0x00, 0xff, 0x47, 0xb8, 0x13, 0x01, 0x00,
	0x00, 0xff, 0x05, 0xfa, 0x4b, 0x00, 0x00,
	0x00, 0xff, 0x02, 0xfd, 0x4e, 0x00, 0x00,
	0x00, 0xff, 0x0e, 0xf1, 0x06, 0x00, 0x00,
	0x00, 0xff, 0x1e, 0xe1, 0x52, 0x02, 0x00,
	0x00, 0xff, 0x0a, 0xf5, 0x51, 0x02, 0x00,
	0x00, 0xff, 0x10, 0xef, 0x10, 0x00, 0x00,
	0x00, 0xff, 0x49, 0xb6, 0x19, 0x01, 0x00,
	0x00, 0xff, 0x15, 0xea, 0x27, 0x00, 0x00,
	0x00, 0xff, 0x03, 0xfc, 0x1e, 0x00, 0x00,
	0x00, 0xff, 0x01, 0xfe, 0x1f, 0x00, 0x00,
	0x00, 0xff, 0x06, 0xf9, 0x20, 0x00, 0x00,
	0x00, 0xff, 0x09, 0xf6, 0x21, 0x00, 0x00,
	0x00, 0xff, 0x1d, 0xe2, 0x22, 0x00, 0x00,
	0x00, 0xff, 0x1f, 0xe0, 0x23, 0x00, 0x00,
	0x00, 0xff, 0x0d, 0xf2, 0x24, 0x00, 0x00,
	0x00, 0xff, 0x19, 0xe6, 0x25, 0x00, 0x00,
	0x00, 0xff, 0x1b, 0xe4, 0x26, 0x00, 0x00,
	0x00, 0xff, 0x00, 0xff, 0x2b, 0x00, 0x00,
	0x00, 0xff, 0x4a, 0xb5, 0x4c, 0x00, 0x00,
	0x00, 0xff, 0x4b, 0xb4, 0x52, 0x00, 0x00,
	0x00, 0xff, 0x51, 0xae, 0x51, 0x00, 0x00,
	0x00, 0xff, 0x52, 0xad, 0x4f, 0x00, 0x00,
	0x00, 0xff, 0x4e, 0xb1, 0x50, 0x00, 0x00,
	0x00, 0xff, 0x0c, 0xf3, 0x29, 0x00, 0x00,
	0x00, 0xff, 0x4f, 0xb0, 0x28, 0x00, 0x00,
	0x00, 0xff, 0x13, 0xec, 0x2a, 0x00, 0x00,
	0x00, 0xff, 0x17, 0xe8, 0x19, 0x00, 0x00,
	0x00, 0xff, 0x04, 0xfb, 0x0f, 0x00, 0x00,
	0x00, 0xff, 0x48, 0xb7, 0x0e, 0x00, 0x00,
	0x00, 0xff, 0x0f, 0xf0, 0x04, 0x00, 0x00,
	0x00, 0xff, 0x1c, 0xe3, 0x08, 0x00, 0x00,
	0x00, 0xff, 0x18, 0xe7, 0x15, 0x02, 0x00,
	0x00, 0xff, 0x53, 0xac, 0x0a, 0x02, 0x00,
	0x00, 0xff, 0x5e, 0xa1, 0x1c, 0x02, 0x00,
	0x00, 0xff, 0x5f, 0xa0, 0x05, 0x02, 0x00,
};

/* A-Link DTU(m) */
static struct ir_scancode ir_codes_af9015_table_a_link[] = {
	{ 0x001e, KEY_1 },
	{ 0x001f, KEY_2 },
	{ 0x0020, KEY_3 },
	{ 0x0021, KEY_4 },
	{ 0x0022, KEY_5 },
	{ 0x0023, KEY_6 },
	{ 0x0024, KEY_7 },
	{ 0x0025, KEY_8 },
	{ 0x0026, KEY_9 },
	{ 0x0027, KEY_0 },
	{ 0x002e, KEY_CHANNELUP },
	{ 0x002d, KEY_CHANNELDOWN },
	{ 0x0428, KEY_ZOOM },
	{ 0x0041, KEY_MUTE },
	{ 0x0042, KEY_VOLUMEDOWN },
	{ 0x0043, KEY_VOLUMEUP },
	{ 0x0044, KEY_GOTO },         /* jump */
	{ 0x0545, KEY_POWER },
};

static u8 af9015_ir_table_a_link[] = {
	0x08, 0xf7, 0x12, 0xed, 0x45, 0x05, 0x00, /* power */
	0x08, 0xf7, 0x1a, 0xe5, 0x41, 0x00, 0x00, /* mute */
	0x08, 0xf7, 0x01, 0xfe, 0x1e, 0x00, 0x00, /* 1 */
	0x08, 0xf7, 0x1c, 0xe3, 0x21, 0x00, 0x00, /* 4 */
	0x08, 0xf7, 0x03, 0xfc, 0x24, 0x00, 0x00, /* 7 */
	0x08, 0xf7, 0x05, 0xfa, 0x28, 0x04, 0x00, /* zoom */
	0x08, 0xf7, 0x00, 0xff, 0x43, 0x00, 0x00, /* volume up */
	0x08, 0xf7, 0x16, 0xe9, 0x42, 0x00, 0x00, /* volume down */
	0x08, 0xf7, 0x0f, 0xf0, 0x1f, 0x00, 0x00, /* 2 */
	0x08, 0xf7, 0x0d, 0xf2, 0x22, 0x00, 0x00, /* 5 */
	0x08, 0xf7, 0x1b, 0xe4, 0x25, 0x00, 0x00, /* 8 */
	0x08, 0xf7, 0x06, 0xf9, 0x27, 0x00, 0x00, /* 0 */
	0x08, 0xf7, 0x14, 0xeb, 0x2e, 0x00, 0x00, /* channel up */
	0x08, 0xf7, 0x1d, 0xe2, 0x2d, 0x00, 0x00, /* channel down */
	0x08, 0xf7, 0x02, 0xfd, 0x20, 0x00, 0x00, /* 3 */
	0x08, 0xf7, 0x18, 0xe7, 0x23, 0x00, 0x00, /* 6 */
	0x08, 0xf7, 0x04, 0xfb, 0x26, 0x00, 0x00, /* 9 */
	0x08, 0xf7, 0x07, 0xf8, 0x44, 0x00, 0x00, /* jump */
};

/* MSI DIGIVOX mini II V3.0 */
static struct ir_scancode ir_codes_af9015_table_msi[] = {
	{ 0x001e, KEY_1 },
	{ 0x001f, KEY_2 },
	{ 0x0020, KEY_3 },
	{ 0x0021, KEY_4 },
	{ 0x0022, KEY_5 },
	{ 0x0023, KEY_6 },
	{ 0x0024, KEY_7 },
	{ 0x0025, KEY_8 },
	{ 0x0026, KEY_9 },
	{ 0x0027, KEY_0 },
	{ 0x030f, KEY_CHANNELUP },
	{ 0x030e, KEY_CHANNELDOWN },
	{ 0x0042, KEY_VOLUMEDOWN },
	{ 0x0043, KEY_VOLUMEUP },
	{ 0x0545, KEY_POWER },
	{ 0x0052, KEY_UP },           /* up */
	{ 0x0051, KEY_DOWN },         /* down */
	{ 0x0028, KEY_ENTER },
};

static u8 af9015_ir_table_msi[] = {
	0x03, 0xfc, 0x17, 0xe8, 0x45, 0x05, 0x00, /* power */
	0x03, 0xfc, 0x0d, 0xf2, 0x51, 0x00, 0x00, /* down */
	0x03, 0xfc, 0x03, 0xfc, 0x52, 0x00, 0x00, /* up */
	0x03, 0xfc, 0x1a, 0xe5, 0x1e, 0x00, 0x00, /* 1 */
	0x03, 0xfc, 0x02, 0xfd, 0x1f, 0x00, 0x00, /* 2 */
	0x03, 0xfc, 0x04, 0xfb, 0x20, 0x00, 0x00, /* 3 */
	0x03, 0xfc, 0x1c, 0xe3, 0x21, 0x00, 0x00, /* 4 */
	0x03, 0xfc, 0x08, 0xf7, 0x22, 0x00, 0x00, /* 5 */
	0x03, 0xfc, 0x1d, 0xe2, 0x23, 0x00, 0x00, /* 6 */
	0x03, 0xfc, 0x11, 0xee, 0x24, 0x00, 0x00, /* 7 */
	0x03, 0xfc, 0x0b, 0xf4, 0x25, 0x00, 0x00, /* 8 */
	0x03, 0xfc, 0x10, 0xef, 0x26, 0x00, 0x00, /* 9 */
	0x03, 0xfc, 0x09, 0xf6, 0x27, 0x00, 0x00, /* 0 */
	0x03, 0xfc, 0x14, 0xeb, 0x43, 0x00, 0x00, /* volume up */
	0x03, 0xfc, 0x1f, 0xe0, 0x42, 0x00, 0x00, /* volume down */
	0x03, 0xfc, 0x15, 0xea, 0x0f, 0x03, 0x00, /* channel up */
	0x03, 0xfc, 0x05, 0xfa, 0x0e, 0x03, 0x00, /* channel down */
	0x03, 0xfc, 0x16, 0xe9, 0x28, 0x00, 0x00, /* enter */
};

/* MYGICTV U718 */
static struct ir_scancode ir_codes_af9015_table_mygictv[] = {
	{ 0x003d, KEY_SWITCHVIDEOMODE },
					  /* TV / AV */
	{ 0x0545, KEY_POWER },
	{ 0x001e, KEY_1 },
	{ 0x001f, KEY_2 },
	{ 0x0020, KEY_3 },
	{ 0x0021, KEY_4 },
	{ 0x0022, KEY_5 },
	{ 0x0023, KEY_6 },
	{ 0x0024, KEY_7 },
	{ 0x0025, KEY_8 },
	{ 0x0026, KEY_9 },
	{ 0x0027, KEY_0 },
	{ 0x0041, KEY_MUTE },
	{ 0x002a, KEY_ESC },          /* Esc */
	{ 0x002e, KEY_CHANNELUP },
	{ 0x002d, KEY_CHANNELDOWN },
	{ 0x0042, KEY_VOLUMEDOWN },
	{ 0x0043, KEY_VOLUMEUP },
	{ 0x0052, KEY_UP },           /* up arrow */
	{ 0x0051, KEY_DOWN },         /* down arrow */
	{ 0x004f, KEY_RIGHT },        /* right arrow */
	{ 0x0050, KEY_LEFT },         /* left arrow */
	{ 0x0028, KEY_ENTER },        /* ok */
	{ 0x0115, KEY_RECORD },
	{ 0x0313, KEY_PLAY },
	{ 0x0113, KEY_PAUSE },
	{ 0x0116, KEY_STOP },
	{ 0x0307, KEY_REWIND },       /* FR << */
	{ 0x0309, KEY_FASTFORWARD },  /* FF >> */
	{ 0x003b, KEY_TIME },         /* TimeShift */
	{ 0x003e, KEY_CAMERA },       /* Snapshot */
	{ 0x0316, KEY_CYCLEWINDOWS }, /* yellow, min / max */
	{ 0x0000, KEY_ZOOM },         /* 'select' (?) */
	{ 0x0316, KEY_SHUFFLE },      /* Shuffle */
	{ 0x0345, KEY_POWER },
};

static u8 af9015_ir_table_mygictv[] = {
	0x02, 0xbd, 0x0c, 0xf3, 0x3d, 0x00, 0x00, /* TV / AV */
	0x02, 0xbd, 0x14, 0xeb, 0x45, 0x05, 0x00, /* power */
	0x02, 0xbd, 0x00, 0xff, 0x1e, 0x00, 0x00, /* 1 */
	0x02, 0xbd, 0x01, 0xfe, 0x1f, 0x00, 0x00, /* 2 */
	0x02, 0xbd, 0x02, 0xfd, 0x20, 0x00, 0x00, /* 3 */
	0x02, 0xbd, 0x03, 0xfc, 0x21, 0x00, 0x00, /* 4 */
	0x02, 0xbd, 0x04, 0xfb, 0x22, 0x00, 0x00, /* 5 */
	0x02, 0xbd, 0x05, 0xfa, 0x23, 0x00, 0x00, /* 6 */
	0x02, 0xbd, 0x06, 0xf9, 0x24, 0x00, 0x00, /* 7 */
	0x02, 0xbd, 0x07, 0xf8, 0x25, 0x00, 0x00, /* 8 */
	0x02, 0xbd, 0x08, 0xf7, 0x26, 0x00, 0x00, /* 9 */
	0x02, 0xbd, 0x09, 0xf6, 0x27, 0x00, 0x00, /* 0 */
	0x02, 0xbd, 0x0a, 0xf5, 0x41, 0x00, 0x00, /* mute */
	0x02, 0xbd, 0x1c, 0xe3, 0x2a, 0x00, 0x00, /* esc */
	0x02, 0xbd, 0x1f, 0xe0, 0x43, 0x00, 0x00, /* volume up */
	0x02, 0xbd, 0x12, 0xed, 0x52, 0x00, 0x00, /* up arrow */
	0x02, 0xbd, 0x11, 0xee, 0x50, 0x00, 0x00, /* left arrow */
	0x02, 0xbd, 0x15, 0xea, 0x28, 0x00, 0x00, /* ok */
	0x02, 0xbd, 0x10, 0xef, 0x4f, 0x00, 0x00, /* right arrow */
	0x02, 0xbd, 0x13, 0xec, 0x51, 0x00, 0x00, /* down arrow */
	0x02, 0xbd, 0x0e, 0xf1, 0x42, 0x00, 0x00, /* volume down */
	0x02, 0xbd, 0x19, 0xe6, 0x15, 0x01, 0x00, /* record */
	0x02, 0xbd, 0x1e, 0xe1, 0x13, 0x03, 0x00, /* play */
	0x02, 0xbd, 0x16, 0xe9, 0x16, 0x01, 0x00, /* stop */
	0x02, 0xbd, 0x0b, 0xf4, 0x28, 0x04, 0x00, /* yellow, min / max */
	0x02, 0xbd, 0x0f, 0xf0, 0x3b, 0x00, 0x00, /* time shift */
	0x02, 0xbd, 0x18, 0xe7, 0x2e, 0x00, 0x00, /* channel up */
	0x02, 0xbd, 0x1a, 0xe5, 0x2d, 0x00, 0x00, /* channel down */
	0x02, 0xbd, 0x17, 0xe8, 0x3e, 0x00, 0x00, /* snapshot */
	0x02, 0xbd, 0x40, 0xbf, 0x13, 0x01, 0x00, /* pause */
	0x02, 0xbd, 0x41, 0xbe, 0x09, 0x03, 0x00, /* FF >> */
	0x02, 0xbd, 0x42, 0xbd, 0x07, 0x03, 0x00, /* FR << */
	0x02, 0xbd, 0x43, 0xbc, 0x00, 0x00, 0x00, /* 'select' (?) */
	0x02, 0xbd, 0x44, 0xbb, 0x16, 0x03, 0x00, /* shuffle */
	0x02, 0xbd, 0x45, 0xba, 0x45, 0x03, 0x00, /* power */
};

/* KWorld PlusTV Dual DVB-T Stick (DVB-T 399U) */
static u8 af9015_ir_table_kworld[] = {
	0x86, 0x6b, 0x0c, 0xf3, 0x2e, 0x07, 0x00,
	0x86, 0x6b, 0x16, 0xe9, 0x2d, 0x07, 0x00,
	0x86, 0x6b, 0x1d, 0xe2, 0x37, 0x07, 0x00,
	0x86, 0x6b, 0x00, 0xff, 0x1e, 0x07, 0x00,
	0x86, 0x6b, 0x01, 0xfe, 0x1f, 0x07, 0x00,
	0x86, 0x6b, 0x02, 0xfd, 0x20, 0x07, 0x00,
	0x86, 0x6b, 0x03, 0xfc, 0x21, 0x07, 0x00,
	0x86, 0x6b, 0x04, 0xfb, 0x22, 0x07, 0x00,
	0x86, 0x6b, 0x05, 0xfa, 0x23, 0x07, 0x00,
	0x86, 0x6b, 0x06, 0xf9, 0x24, 0x07, 0x00,
	0x86, 0x6b, 0x07, 0xf8, 0x25, 0x07, 0x00,
	0x86, 0x6b, 0x08, 0xf7, 0x26, 0x07, 0x00,
	0x86, 0x6b, 0x09, 0xf6, 0x4d, 0x07, 0x00,
	0x86, 0x6b, 0x0a, 0xf5, 0x4e, 0x07, 0x00,
	0x86, 0x6b, 0x14, 0xeb, 0x4f, 0x07, 0x00,
	0x86, 0x6b, 0x1e, 0xe1, 0x50, 0x07, 0x00,
	0x86, 0x6b, 0x17, 0xe8, 0x52, 0x07, 0x00,
	0x86, 0x6b, 0x1f, 0xe0, 0x51, 0x07, 0x00,
	0x86, 0x6b, 0x0e, 0xf1, 0x0b, 0x07, 0x00,
	0x86, 0x6b, 0x20, 0xdf, 0x0c, 0x07, 0x00,
	0x86, 0x6b, 0x42, 0xbd, 0x0d, 0x07, 0x00,
	0x86, 0x6b, 0x0b, 0xf4, 0x0e, 0x07, 0x00,
	0x86, 0x6b, 0x43, 0xbc, 0x0f, 0x07, 0x00,
	0x86, 0x6b, 0x10, 0xef, 0x10, 0x07, 0x00,
	0x86, 0x6b, 0x21, 0xde, 0x11, 0x07, 0x00,
	0x86, 0x6b, 0x13, 0xec, 0x12, 0x07, 0x00,
	0x86, 0x6b, 0x11, 0xee, 0x13, 0x07, 0x00,
	0x86, 0x6b, 0x12, 0xed, 0x14, 0x07, 0x00,
	0x86, 0x6b, 0x19, 0xe6, 0x15, 0x07, 0x00,
	0x86, 0x6b, 0x1a, 0xe5, 0x16, 0x07, 0x00,
	0x86, 0x6b, 0x1b, 0xe4, 0x17, 0x07, 0x00,
	0x86, 0x6b, 0x4b, 0xb4, 0x18, 0x07, 0x00,
	0x86, 0x6b, 0x40, 0xbf, 0x19, 0x07, 0x00,
	0x86, 0x6b, 0x44, 0xbb, 0x1a, 0x07, 0x00,
	0x86, 0x6b, 0x41, 0xbe, 0x1b, 0x07, 0x00,
	0x86, 0x6b, 0x22, 0xdd, 0x1c, 0x07, 0x00,
	0x86, 0x6b, 0x15, 0xea, 0x1d, 0x07, 0x00,
	0x86, 0x6b, 0x0f, 0xf0, 0x3f, 0x07, 0x00,
	0x86, 0x6b, 0x1c, 0xe3, 0x40, 0x07, 0x00,
	0x86, 0x6b, 0x4a, 0xb5, 0x41, 0x07, 0x00,
	0x86, 0x6b, 0x48, 0xb7, 0x42, 0x07, 0x00,
	0x86, 0x6b, 0x49, 0xb6, 0x43, 0x07, 0x00,
	0x86, 0x6b, 0x18, 0xe7, 0x44, 0x07, 0x00,
	0x86, 0x6b, 0x23, 0xdc, 0x45, 0x07, 0x00,
};

/* AverMedia Volar X */
static struct ir_scancode ir_codes_af9015_table_avermedia[] = {
	{ 0x053d, KEY_PROG1 },       /* SOURCE */
	{ 0x0512, KEY_POWER },       /* POWER */
	{ 0x051e, KEY_1 },           /* 1 */
	{ 0x051f, KEY_2 },           /* 2 */
	{ 0x0520, KEY_3 },           /* 3 */
	{ 0x0521, KEY_4 },           /* 4 */
	{ 0x0522, KEY_5 },           /* 5 */
	{ 0x0523, KEY_6 },           /* 6 */
	{ 0x0524, KEY_7 },           /* 7 */
	{ 0x0525, KEY_8 },           /* 8 */
	{ 0x0526, KEY_9 },           /* 9 */
	{ 0x053f, KEY_LEFT },        /* L / DISPLAY */
	{ 0x0527, KEY_0 },           /* 0 */
	{ 0x050f, KEY_RIGHT },       /* R / CH RTN */
	{ 0x0518, KEY_PROG2 },       /* SNAP SHOT */
	{ 0x051c, KEY_PROG3 },       /* 16-CH PREV */
	{ 0x052d, KEY_VOLUMEDOWN },  /* VOL DOWN */
	{ 0x053e, KEY_ZOOM },        /* FULL SCREEN */
	{ 0x052e, KEY_VOLUMEUP },    /* VOL UP */
	{ 0x0510, KEY_MUTE },        /* MUTE */
	{ 0x0504, KEY_AUDIO },       /* AUDIO */
	{ 0x0515, KEY_RECORD },      /* RECORD */
	{ 0x0511, KEY_PLAY },        /* PLAY */
	{ 0x0516, KEY_STOP },        /* STOP */
	{ 0x050c, KEY_PLAYPAUSE },   /* TIMESHIFT / PAUSE */
	{ 0x0505, KEY_BACK },        /* << / RED */
	{ 0x0509, KEY_FORWARD },     /* >> / YELLOW */
	{ 0x0517, KEY_TEXT },        /* TELETEXT */
	{ 0x050a, KEY_EPG },         /* EPG */
	{ 0x0513, KEY_MENU },        /* MENU */

	{ 0x050e, KEY_CHANNELUP },   /* CH UP */
	{ 0x050d, KEY_CHANNELDOWN }, /* CH DOWN */
	{ 0x0519, KEY_FIRST },       /* |<< / GREEN */
	{ 0x0508, KEY_LAST },        /* >>| / BLUE */
};

static u8 af9015_ir_table_avermedia[] = {
	0x02, 0xfd, 0x00, 0xff, 0x12, 0x05, 0x00,
	0x02, 0xfd, 0x01, 0xfe, 0x3d, 0x05, 0x00,
	0x02, 0xfd, 0x03, 0xfc, 0x17, 0x05, 0x00,
	0x02, 0xfd, 0x04, 0xfb, 0x0a, 0x05, 0x00,
	0x02, 0xfd, 0x05, 0xfa, 0x1e, 0x05, 0x00,
	0x02, 0xfd, 0x06, 0xf9, 0x1f, 0x05, 0x00,
	0x02, 0xfd, 0x07, 0xf8, 0x20, 0x05, 0x00,
	0x02, 0xfd, 0x09, 0xf6, 0x21, 0x05, 0x00,
	0x02, 0xfd, 0x0a, 0xf5, 0x22, 0x05, 0x00,
	0x02, 0xfd, 0x0b, 0xf4, 0x23, 0x05, 0x00,
	0x02, 0xfd, 0x0d, 0xf2, 0x24, 0x05, 0x00,
	0x02, 0xfd, 0x0e, 0xf1, 0x25, 0x05, 0x00,
	0x02, 0xfd, 0x0f, 0xf0, 0x26, 0x05, 0x00,
	0x02, 0xfd, 0x11, 0xee, 0x27, 0x05, 0x00,
	0x02, 0xfd, 0x08, 0xf7, 0x04, 0x05, 0x00,
	0x02, 0xfd, 0x0c, 0xf3, 0x3e, 0x05, 0x00,
	0x02, 0xfd, 0x10, 0xef, 0x1c, 0x05, 0x00,
	0x02, 0xfd, 0x12, 0xed, 0x3f, 0x05, 0x00,
	0x02, 0xfd, 0x13, 0xec, 0x0f, 0x05, 0x00,
	0x02, 0xfd, 0x14, 0xeb, 0x10, 0x05, 0x00,
	0x02, 0xfd, 0x15, 0xea, 0x13, 0x05, 0x00,
	0x02, 0xfd, 0x17, 0xe8, 0x18, 0x05, 0x00,
	0x02, 0xfd, 0x18, 0xe7, 0x11, 0x05, 0x00,
	0x02, 0xfd, 0x19, 0xe6, 0x15, 0x05, 0x00,
	0x02, 0xfd, 0x1a, 0xe5, 0x0c, 0x05, 0x00,
	0x02, 0xfd, 0x1b, 0xe4, 0x16, 0x05, 0x00,
	0x02, 0xfd, 0x1c, 0xe3, 0x09, 0x05, 0x00,
	0x02, 0xfd, 0x1d, 0xe2, 0x05, 0x05, 0x00,
	0x02, 0xfd, 0x1e, 0xe1, 0x2d, 0x05, 0x00,
	0x02, 0xfd, 0x1f, 0xe0, 0x2e, 0x05, 0x00,
	0x03, 0xfc, 0x00, 0xff, 0x08, 0x05, 0x00,
	0x03, 0xfc, 0x01, 0xfe, 0x19, 0x05, 0x00,
	0x03, 0xfc, 0x02, 0xfd, 0x0d, 0x05, 0x00,
	0x03, 0xfc, 0x03, 0xfc, 0x0e, 0x05, 0x00,
};

static u8 af9015_ir_table_avermedia_ks[] = {
	0x05, 0xfa, 0x01, 0xfe, 0x12, 0x05, 0x00,
	0x05, 0xfa, 0x02, 0xfd, 0x0e, 0x05, 0x00,
	0x05, 0xfa, 0x03, 0xfc, 0x0d, 0x05, 0x00,
	0x05, 0xfa, 0x04, 0xfb, 0x2e, 0x05, 0x00,
	0x05, 0xfa, 0x05, 0xfa, 0x2d, 0x05, 0x00,
	0x05, 0xfa, 0x06, 0xf9, 0x10, 0x05, 0x00,
	0x05, 0xfa, 0x07, 0xf8, 0x0f, 0x05, 0x00,
	0x05, 0xfa, 0x08, 0xf7, 0x3d, 0x05, 0x00,
	0x05, 0xfa, 0x09, 0xf6, 0x1e, 0x05, 0x00,
	0x05, 0xfa, 0x0a, 0xf5, 0x1f, 0x05, 0x00,
	0x05, 0xfa, 0x0b, 0xf4, 0x20, 0x05, 0x00,
	0x05, 0xfa, 0x0c, 0xf3, 0x21, 0x05, 0x00,
	0x05, 0xfa, 0x0d, 0xf2, 0x22, 0x05, 0x00,
	0x05, 0xfa, 0x0e, 0xf1, 0x23, 0x05, 0x00,
	0x05, 0xfa, 0x0f, 0xf0, 0x24, 0x05, 0x00,
	0x05, 0xfa, 0x10, 0xef, 0x25, 0x05, 0x00,
	0x05, 0xfa, 0x11, 0xee, 0x26, 0x05, 0x00,
	0x05, 0xfa, 0x12, 0xed, 0x27, 0x05, 0x00,
	0x05, 0xfa, 0x13, 0xec, 0x04, 0x05, 0x00,
	0x05, 0xfa, 0x15, 0xea, 0x0a, 0x05, 0x00,
	0x05, 0xfa, 0x16, 0xe9, 0x11, 0x05, 0x00,
	0x05, 0xfa, 0x17, 0xe8, 0x15, 0x05, 0x00,
	0x05, 0xfa, 0x18, 0xe7, 0x16, 0x05, 0x00,
	0x05, 0xfa, 0x1c, 0xe3, 0x05, 0x05, 0x00,
	0x05, 0xfa, 0x1d, 0xe2, 0x09, 0x05, 0x00,
	0x05, 0xfa, 0x4d, 0xb2, 0x3f, 0x05, 0x00,
	0x05, 0xfa, 0x56, 0xa9, 0x3e, 0x05, 0x00
};

/* Digittrade DVB-T USB Stick */
static struct ir_scancode ir_codes_af9015_table_digittrade[] = {
	{ 0x010f, KEY_LAST },	/* RETURN */
	{ 0x0517, KEY_TEXT },	/* TELETEXT */
	{ 0x0108, KEY_EPG },	/* EPG */
	{ 0x0513, KEY_POWER },	/* POWER */
	{ 0x0109, KEY_ZOOM },	/* FULLSCREEN */
	{ 0x0040, KEY_AUDIO },	/* DUAL SOUND */
	{ 0x002c, KEY_PRINT },	/* SNAPSHOT */
	{ 0x0516, KEY_SUBTITLE },	/* SUBTITLE */
	{ 0x0052, KEY_CHANNELUP },	/* CH Up */
	{ 0x0051, KEY_CHANNELDOWN },/* Ch Dn */
	{ 0x0057, KEY_VOLUMEUP },	/* Vol Up */
	{ 0x0056, KEY_VOLUMEDOWN },	/* Vol Dn */
	{ 0x0110, KEY_MUTE },	/* MUTE */
	{ 0x0027, KEY_0 },
	{ 0x001e, KEY_1 },
	{ 0x001f, KEY_2 },
	{ 0x0020, KEY_3 },
	{ 0x0021, KEY_4 },
	{ 0x0022, KEY_5 },
	{ 0x0023, KEY_6 },
	{ 0x0024, KEY_7 },
	{ 0x0025, KEY_8 },
	{ 0x0026, KEY_9 },
	{ 0x0117, KEY_PLAYPAUSE },	/* TIMESHIFT */
	{ 0x0115, KEY_RECORD },	/* RECORD */
	{ 0x0313, KEY_PLAY },	/* PLAY */
	{ 0x0116, KEY_STOP },	/* STOP */
	{ 0x0113, KEY_PAUSE },	/* PAUSE */
};

static u8 af9015_ir_table_digittrade[] = {
	0x00, 0xff, 0x06, 0xf9, 0x13, 0x05, 0x00,
	0x00, 0xff, 0x4d, 0xb2, 0x17, 0x01, 0x00,
	0x00, 0xff, 0x1f, 0xe0, 0x2c, 0x00, 0x00,
	0x00, 0xff, 0x0a, 0xf5, 0x15, 0x01, 0x00,
	0x00, 0xff, 0x0e, 0xf1, 0x16, 0x01, 0x00,
	0x00, 0xff, 0x09, 0xf6, 0x09, 0x01, 0x00,
	0x00, 0xff, 0x01, 0xfe, 0x08, 0x01, 0x00,
	0x00, 0xff, 0x05, 0xfa, 0x10, 0x01, 0x00,
	0x00, 0xff, 0x02, 0xfd, 0x56, 0x00, 0x00,
	0x00, 0xff, 0x40, 0xbf, 0x57, 0x00, 0x00,
	0x00, 0xff, 0x19, 0xe6, 0x52, 0x00, 0x00,
	0x00, 0xff, 0x17, 0xe8, 0x51, 0x00, 0x00,
	0x00, 0xff, 0x10, 0xef, 0x0f, 0x01, 0x00,
	0x00, 0xff, 0x54, 0xab, 0x27, 0x00, 0x00,
	0x00, 0xff, 0x1b, 0xe4, 0x1e, 0x00, 0x00,
	0x00, 0xff, 0x11, 0xee, 0x1f, 0x00, 0x00,
	0x00, 0xff, 0x15, 0xea, 0x20, 0x00, 0x00,
	0x00, 0xff, 0x12, 0xed, 0x21, 0x00, 0x00,
	0x00, 0xff, 0x16, 0xe9, 0x22, 0x00, 0x00,
	0x00, 0xff, 0x4c, 0xb3, 0x23, 0x00, 0x00,
	0x00, 0xff, 0x48, 0xb7, 0x24, 0x00, 0x00,
	0x00, 0xff, 0x04, 0xfb, 0x25, 0x00, 0x00,
	0x00, 0xff, 0x00, 0xff, 0x26, 0x00, 0x00,
	0x00, 0xff, 0x1e, 0xe1, 0x13, 0x03, 0x00,
	0x00, 0xff, 0x1a, 0xe5, 0x13, 0x01, 0x00,
	0x00, 0xff, 0x03, 0xfc, 0x17, 0x05, 0x00,
	0x00, 0xff, 0x0d, 0xf2, 0x16, 0x05, 0x00,
	0x00, 0xff, 0x1d, 0xe2, 0x40, 0x00, 0x00,
};

/* TREKSTOR DVB-T USB Stick */
static struct ir_scancode ir_codes_af9015_table_trekstor[] = {
	{ 0x0704, KEY_AGAIN },		/* Home */
	{ 0x0705, KEY_MUTE },		/* Mute */
	{ 0x0706, KEY_UP },			/* Up */
	{ 0x0707, KEY_DOWN },		/* Down */
	{ 0x0709, KEY_RIGHT },		/* Right */
	{ 0x070a, KEY_ENTER },		/* OK */
	{ 0x070b, KEY_FASTFORWARD },	/* Fast forward */
	{ 0x070c, KEY_REWIND },		/* Rewind */
	{ 0x070d, KEY_PLAY },		/* Play/Pause */
	{ 0x070e, KEY_VOLUMEUP },		/* Volume + */
	{ 0x070f, KEY_VOLUMEDOWN },		/* Volume - */
	{ 0x0710, KEY_RECORD },		/* Record */
	{ 0x0711, KEY_STOP },		/* Stop */
	{ 0x0712, KEY_ZOOM },		/* TV */
	{ 0x0713, KEY_EPG },		/* Info/EPG */
	{ 0x0714, KEY_CHANNELDOWN },	/* Channel - */
	{ 0x0715, KEY_CHANNELUP },		/* Channel + */
	{ 0x071e, KEY_1 },
	{ 0x071f, KEY_2 },
	{ 0x0720, KEY_3 },
	{ 0x0721, KEY_4 },
	{ 0x0722, KEY_5 },
	{ 0x0723, KEY_6 },
	{ 0x0724, KEY_7 },
	{ 0x0725, KEY_8 },
	{ 0x0726, KEY_9 },
	{ 0x0708, KEY_LEFT },		/* LEFT */
	{ 0x0727, KEY_0 },
};

static u8 af9015_ir_table_trekstor[] = {
	0x00, 0xff, 0x86, 0x79, 0x04, 0x07, 0x00,
	0x00, 0xff, 0x85, 0x7a, 0x05, 0x07, 0x00,
	0x00, 0xff, 0x87, 0x78, 0x06, 0x07, 0x00,
	0x00, 0xff, 0x8c, 0x73, 0x07, 0x07, 0x00,
	0x00, 0xff, 0x89, 0x76, 0x09, 0x07, 0x00,
	0x00, 0xff, 0x88, 0x77, 0x0a, 0x07, 0x00,
	0x00, 0xff, 0x8a, 0x75, 0x0b, 0x07, 0x00,
	0x00, 0xff, 0x9e, 0x61, 0x0c, 0x07, 0x00,
	0x00, 0xff, 0x8d, 0x72, 0x0d, 0x07, 0x00,
	0x00, 0xff, 0x8b, 0x74, 0x0e, 0x07, 0x00,
	0x00, 0xff, 0x9b, 0x64, 0x0f, 0x07, 0x00,
	0x00, 0xff, 0x9d, 0x62, 0x10, 0x07, 0x00,
	0x00, 0xff, 0x8e, 0x71, 0x11, 0x07, 0x00,
	0x00, 0xff, 0x9c, 0x63, 0x12, 0x07, 0x00,
	0x00, 0xff, 0x8f, 0x70, 0x13, 0x07, 0x00,
	0x00, 0xff, 0x93, 0x6c, 0x14, 0x07, 0x00,
	0x00, 0xff, 0x97, 0x68, 0x15, 0x07, 0x00,
	0x00, 0xff, 0x92, 0x6d, 0x1e, 0x07, 0x00,
	0x00, 0xff, 0x96, 0x69, 0x1f, 0x07, 0x00,
	0x00, 0xff, 0x9a, 0x65, 0x20, 0x07, 0x00,
	0x00, 0xff, 0x91, 0x6e, 0x21, 0x07, 0x00,
	0x00, 0xff, 0x95, 0x6a, 0x22, 0x07, 0x00,
	0x00, 0xff, 0x99, 0x66, 0x23, 0x07, 0x00,
	0x00, 0xff, 0x90, 0x6f, 0x24, 0x07, 0x00,
	0x00, 0xff, 0x94, 0x6b, 0x25, 0x07, 0x00,
	0x00, 0xff, 0x98, 0x67, 0x26, 0x07, 0x00,
	0x00, 0xff, 0x9f, 0x60, 0x08, 0x07, 0x00,
	0x00, 0xff, 0x84, 0x7b, 0x27, 0x07, 0x00,
};

/* MSI DIGIVOX mini III */
static struct ir_scancode ir_codes_af9015_table_msi_digivox_iii[] = {
	{ 0x0713, KEY_POWER },       /* [red power button] */
	{ 0x073b, KEY_VIDEO },       /* Source */
	{ 0x073e, KEY_ZOOM },        /* Zoom */
	{ 0x070b, KEY_POWER2 },      /* ShutDown */
	{ 0x071e, KEY_1 },
	{ 0x071f, KEY_2 },
	{ 0x0720, KEY_3 },
	{ 0x0721, KEY_4 },
	{ 0x0722, KEY_5 },
	{ 0x0723, KEY_6 },
	{ 0x0724, KEY_7 },
	{ 0x0725, KEY_8 },
	{ 0x0726, KEY_9 },
	{ 0x0727, KEY_0 },
	{ 0x0752, KEY_CHANNELUP },   /* CH+ */
	{ 0x0751, KEY_CHANNELDOWN }, /* CH- */
	{ 0x0750, KEY_VOLUMEUP },    /* Vol+ */
	{ 0x074f, KEY_VOLUMEDOWN },  /* Vol- */
	{ 0x0705, KEY_ESC },         /* [back up arrow] */
	{ 0x0708, KEY_OK },          /* [enter arrow] */
	{ 0x073f, KEY_RECORD },      /* Rec */
	{ 0x0716, KEY_STOP },        /* Stop */
	{ 0x072a, KEY_PLAY },        /* Play */
	{ 0x073c, KEY_MUTE },        /* Mute */
	{ 0x0718, KEY_UP },
	{ 0x0707, KEY_DOWN },
	{ 0x070f, KEY_LEFT },
	{ 0x0715, KEY_RIGHT },
	{ 0x0736, KEY_RED },
	{ 0x0737, KEY_GREEN },
	{ 0x072d, KEY_YELLOW },
	{ 0x072e, KEY_BLUE },
};

static u8 af9015_ir_table_msi_digivox_iii[] = {
	0x61, 0xd6, 0x43, 0xbc, 0x13, 0x07, 0x00, /* KEY_POWER */
	0x61, 0xd6, 0x01, 0xfe, 0x3b, 0x07, 0x00, /* KEY_VIDEO */
	0x61, 0xd6, 0x0b, 0xf4, 0x3e, 0x07, 0x00, /* KEY_ZOOM */
	0x61, 0xd6, 0x03, 0xfc, 0x0b, 0x07, 0x00, /* KEY_POWER2 */
	0x61, 0xd6, 0x04, 0xfb, 0x1e, 0x07, 0x00, /* KEY_1 */
	0x61, 0xd6, 0x08, 0xf7, 0x1f, 0x07, 0x00, /* KEY_2 */
	0x61, 0xd6, 0x02, 0xfd, 0x20, 0x07, 0x00, /* KEY_3 */
	0x61, 0xd6, 0x0f, 0xf0, 0x21, 0x07, 0x00, /* KEY_4 */
	0x61, 0xd6, 0x05, 0xfa, 0x22, 0x07, 0x00, /* KEY_5 */
	0x61, 0xd6, 0x06, 0xf9, 0x23, 0x07, 0x00, /* KEY_6 */
	0x61, 0xd6, 0x0c, 0xf3, 0x24, 0x07, 0x00, /* KEY_7 */
	0x61, 0xd6, 0x0d, 0xf2, 0x25, 0x07, 0x00, /* KEY_8 */
	0x61, 0xd6, 0x0a, 0xf5, 0x26, 0x07, 0x00, /* KEY_9 */
	0x61, 0xd6, 0x11, 0xee, 0x27, 0x07, 0x00, /* KEY_0 */
	0x61, 0xd6, 0x09, 0xf6, 0x52, 0x07, 0x00, /* KEY_CHANNELUP */
	0x61, 0xd6, 0x07, 0xf8, 0x51, 0x07, 0x00, /* KEY_CHANNELDOWN */
	0x61, 0xd6, 0x0e, 0xf1, 0x50, 0x07, 0x00, /* KEY_VOLUMEUP */
	0x61, 0xd6, 0x13, 0xec, 0x4f, 0x07, 0x00, /* KEY_VOLUMEDOWN */
	0x61, 0xd6, 0x10, 0xef, 0x05, 0x07, 0x00, /* KEY_ESC */
	0x61, 0xd6, 0x12, 0xed, 0x08, 0x07, 0x00, /* KEY_OK */
	0x61, 0xd6, 0x14, 0xeb, 0x3f, 0x07, 0x00, /* KEY_RECORD */
	0x61, 0xd6, 0x15, 0xea, 0x16, 0x07, 0x00, /* KEY_STOP */
	0x61, 0xd6, 0x16, 0xe9, 0x2a, 0x07, 0x00, /* KEY_PLAY */
	0x61, 0xd6, 0x17, 0xe8, 0x3c, 0x07, 0x00, /* KEY_MUTE */
	0x61, 0xd6, 0x18, 0xe7, 0x18, 0x07, 0x00, /* KEY_UP */
	0x61, 0xd6, 0x19, 0xe6, 0x07, 0x07, 0x00, /* KEY_DOWN */
	0x61, 0xd6, 0x1a, 0xe5, 0x0f, 0x07, 0x00, /* KEY_LEFT */
	0x61, 0xd6, 0x1b, 0xe4, 0x15, 0x07, 0x00, /* KEY_RIGHT */
	0x61, 0xd6, 0x1c, 0xe3, 0x36, 0x07, 0x00, /* KEY_RED */
	0x61, 0xd6, 0x1d, 0xe2, 0x37, 0x07, 0x00, /* KEY_GREEN */
	0x61, 0xd6, 0x1e, 0xe1, 0x2d, 0x07, 0x00, /* KEY_YELLOW */
	0x61, 0xd6, 0x1f, 0xe0, 0x2e, 0x07, 0x00, /* KEY_BLUE */
};

/* TerraTec - 4x7 slim remote */
static struct ir_scancode ir_codes_terratec[] = {
	{ 0x0010, KEY_MUTE },
	{ 0x0008, KEY_EPG },
	{ 0x0009, KEY_ZOOM },         /* symbol: PIP or zoom ? */
	{ 0x043d, KEY_POWER2 },       /* [red power button] */
	{ 0x001e, KEY_1 },
	{ 0x001f, KEY_2 },
	{ 0x0020, KEY_3 },
	{ 0x0021, KEY_4 },
	{ 0x0022, KEY_5 },
	{ 0x0023, KEY_6 },
	{ 0x0024, KEY_7 },
	{ 0x0025, KEY_8 },
	{ 0x0026, KEY_9 },
	{ 0x0027, KEY_0 },
	{ 0x0029, KEY_ESC },
	{ 0x0013, KEY_CAMERA },       /* snapshot */
	{ 0x0028, KEY_OK },
	{ 0x004f, KEY_RIGHT },
	{ 0x0050, KEY_LEFT },
	{ 0x0051, KEY_DOWN },
	{ 0x0052, KEY_UP },
	{ 0x004b, KEY_CHANNELUP },
	{ 0x004e, KEY_CHANNELDOWN },
	{ 0x0057, KEY_VOLUMEUP },
	{ 0x0056, KEY_VOLUMEDOWN },
	{ 0x0015, KEY_RECORD },
	{ 0x001b, KEY_STOP },
	{ 0x002c, KEY_PLAYPAUSE },
};

static u8 af9015_ir_terratec[] = {
	0x02, 0xbd, 0x0a, 0xf5, 0x10, 0x00, 0x00, /* KEY_MUTE */
	0x02, 0xbd, 0x0b, 0xf4, 0x09, 0x00, 0x00, /* KEY_ZOOM */
	0x02, 0xbd, 0x00, 0xff, 0x1e, 0x00, 0x00, /* KEY_1 */
	0x02, 0xbd, 0x01, 0xfe, 0x1f, 0x00, 0x00, /* KEY_2 */
	0x02, 0xbd, 0x02, 0xfd, 0x20, 0x00, 0x00, /* KEY_3 */
	0x02, 0xbd, 0x03, 0xfc, 0x21, 0x00, 0x00, /* KEY_4 */
	0x02, 0xbd, 0x04, 0xfb, 0x22, 0x00, 0x00, /* KEY_5 */
	0x02, 0xbd, 0x05, 0xfa, 0x23, 0x00, 0x00, /* KEY_6 */
	0x02, 0xbd, 0x06, 0xf9, 0x24, 0x00, 0x00, /* KEY_7 */
	0x02, 0xbd, 0x07, 0xf8, 0x25, 0x00, 0x00, /* KEY_8 */
	0x02, 0xbd, 0x08, 0xf7, 0x26, 0x00, 0x00, /* KEY_9 */
	0x02, 0xbd, 0x09, 0xf6, 0x27, 0x00, 0x00, /* KEY_0 */
	0x02, 0xbd, 0x12, 0xed, 0x52, 0x00, 0x00, /* KEY_UP */
	0x02, 0xbd, 0x17, 0xe8, 0x13, 0x00, 0x00, /* KEY_CAMERA */
	0x02, 0xbd, 0x11, 0xee, 0x50, 0x00, 0x00, /* KEY_LEFT */
	0x02, 0xbd, 0x15, 0xea, 0x28, 0x00, 0x00, /* KEY_OK */
	0x02, 0xbd, 0x10, 0xef, 0x4f, 0x00, 0x00, /* KEY_RIGHT */
	0x02, 0xbd, 0x18, 0xe7, 0x4b, 0x00, 0x00, /* KEY_CHANNELUP */
	0x02, 0xbd, 0x13, 0xec, 0x51, 0x00, 0x00, /* KEY_DOWN */
	0x02, 0xbd, 0x19, 0xe6, 0x15, 0x00, 0x00, /* KEY_RECORD */
	0x02, 0xbd, 0x1a, 0xe5, 0x4e, 0x00, 0x00, /* KEY_CHANNELDOWN */
	0x02, 0xbd, 0x0e, 0xf1, 0x56, 0x00, 0x00, /* KEY_VOLUMEDOWN */
	0x02, 0xbd, 0x16, 0xe9, 0x1b, 0x00, 0x00, /* KEY_STOP */
	0x02, 0xbd, 0x1c, 0xe3, 0x29, 0x00, 0x00, /* KEY_ESC */
	0x02, 0xbd, 0x1f, 0xe0, 0x57, 0x00, 0x00, /* KEY_VOLUMEUP */
	0x02, 0xbd, 0x44, 0xbb, 0x08, 0x00, 0x00, /* KEY_EPG */
	0x02, 0xbd, 0x45, 0xba, 0x3d, 0x04, 0x00, /* KEY_POWER2 */
	0x02, 0xbd, 0x0f, 0xf0, 0x2c, 0x00, 0x00, /* KEY_PLAYPAUSE */
};

#endif
