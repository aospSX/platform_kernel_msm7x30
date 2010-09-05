/*
 * Shared Atheros AR9170 Header
 *
 * Firmware command interface definitions
 *
 * Copyright 2008, Johannes Berg <johannes@sipsolutions.net>
 * Copyright 2009, 2010, Christian Lamparter <chunkeey@googlemail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, see
 * http://www.gnu.org/licenses/.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *    Copyright (c) 2007-2008 Atheros Communications, Inc.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef __CARL9170_SHARED_FWCMD_H
#define __CARL9170_SHARED_FWCMD_H

#define	CARL9170_MAX_CMD_LEN		64
#define	CARL9170_MAX_CMD_PAYLOAD_LEN	60

#define CARL9170FW_API_MIN_VER		1
#define CARL9170FW_API_MAX_VER		1

enum carl9170_cmd_oids {
	CARL9170_CMD_RREG		= 0x00,
	CARL9170_CMD_WREG		= 0x01,
	CARL9170_CMD_ECHO		= 0x02,
	CARL9170_CMD_SWRST		= 0x03,
	CARL9170_CMD_REBOOT		= 0x04,
	CARL9170_CMD_BCN_CTRL		= 0x05,
	CARL9170_CMD_READ_TSF		= 0x06,

	/* CAM */
	CARL9170_CMD_EKEY		= 0x10,
	CARL9170_CMD_DKEY		= 0x11,

	/* RF / PHY */
	CARL9170_CMD_FREQUENCY		= 0x20,
	CARL9170_CMD_RF_INIT		= 0x21,
	CARL9170_CMD_SYNTH		= 0x22,
	CARL9170_CMD_FREQ_START		= 0x23,
	CARL9170_CMD_PSM		= 0x24,

	/* Asychronous command flag */
	CARL9170_CMD_ASYNC_FLAG		= 0x40,
	CARL9170_CMD_WREG_ASYNC		= (CARL9170_CMD_WREG |
					   CARL9170_CMD_ASYNC_FLAG),
	CARL9170_CMD_REBOOT_ASYNC	= (CARL9170_CMD_REBOOT |
					   CARL9170_CMD_ASYNC_FLAG),
	CARL9170_CMD_BCN_CTRL_ASYNC	= (CARL9170_CMD_BCN_CTRL |
					   CARL9170_CMD_ASYNC_FLAG),
	CARL9170_CMD_PSM_ASYNC		= (CARL9170_CMD_PSM |
					   CARL9170_CMD_ASYNC_FLAG),

	/* responses and traps */
	CARL9170_RSP_FLAG		= 0xc0,
	CARL9170_RSP_PRETBTT		= 0xc0,
	CARL9170_RSP_TXCOMP		= 0xc1,
	CARL9170_RSP_BEACON_CONFIG	= 0xc2,
	CARL9170_RSP_ATIM		= 0xc3,
	CARL9170_RSP_WATCHDOG		= 0xc6,
	CARL9170_RSP_TEXT		= 0xca,
	CARL9170_RSP_HEXDUMP		= 0xcc,
	CARL9170_RSP_RADAR		= 0xcd,
	CARL9170_RSP_GPIO		= 0xce,
	CARL9170_RSP_BOOT		= 0xcf,
};

struct carl9170_set_key_cmd {
	__le16		user;
	__le16		keyId;
	__le16		type;
	u8		macAddr[6];
	u32		key[4];
} __packed;
#define CARL9170_SET_KEY_CMD_SIZE		28

struct carl9170_disable_key_cmd {
	__le16		user;
	__le16		padding;
} __packed;
#define CARL9170_DISABLE_KEY_CMD_SIZE		4

struct carl9170_u32_list {
	u32	vals[0];
} __packed;

struct carl9170_reg_list {
	__le32		regs[0];
} __packed;

struct carl9170_write_reg {
	struct {
		__le32		addr;
		__le32		val;
	} regs[0] __packed;
} __packed;

#define	CARL9170FW_PHY_HT_ENABLE		0x4
#define	CARL9170FW_PHY_HT_DYN2040		0x8
#define	CARL9170FW_PHY_HT_EXT_CHAN_OFF		0x3
#define	CARL9170FW_PHY_HT_EXT_CHAN_OFF_S	2

struct carl9170_rf_init {
	__le32		freq;
	u8		ht_settings;
	u8		padding2[3];
	__le32		delta_slope_coeff_exp;
	__le32		delta_slope_coeff_man;
	__le32		delta_slope_coeff_exp_shgi;
	__le32		delta_slope_coeff_man_shgi;
	__le32		finiteLoopCount;
} __packed;
#define CARL9170_RF_INIT_SIZE		28

struct carl9170_rf_init_result {
	__le32		ret;		/* AR9170_PHY_REG_AGC_CONTROL */
} __packed;
#define	CARL9170_RF_INIT_RESULT_SIZE	4

#define	CARL9170_PSM_SLEEP		0x1000
#define	CARL9170_PSM_SOFTWARE		0
#define	CARL9170_PSM_WAKE		0 /* internally used. */
#define	CARL9170_PSM_COUNTER		0xfff
#define	CARL9170_PSM_COUNTER_S		0

struct carl9170_psm {
	__le32		state;
} __packed;
#define CARL9170_PSM_SIZE		4

struct carl9170_bcn_ctrl_cmd {
	__le32		vif_id;
	__le32		mode;
	__le32		bcn_addr;
	__le32		bcn_len;
} __packed;
#define CARL9170_BCN_CTRL_CMD_SIZE	16

#define CARL9170_BCN_CTRL_DRAIN	0
#define CARL9170_BCN_CTRL_CAB_TRIGGER	1

struct carl9170_cmd_head {
	union {
		struct {
			u8	len;
			u8	cmd;
			u8	seq;
			u8	ext;
		} __packed;

		u32 hdr_data;
	} __packed;
} __packed;

struct carl9170_cmd {
	struct carl9170_cmd_head hdr;
	union {
		struct carl9170_set_key_cmd	setkey;
		struct carl9170_disable_key_cmd	disablekey;
		struct carl9170_u32_list	echo;
		struct carl9170_reg_list	rreg;
		struct carl9170_write_reg	wreg;
		struct carl9170_rf_init		rf_init;
		struct carl9170_psm		psm;
		struct carl9170_bcn_ctrl_cmd	bcn_ctrl;
		u8 data[CARL9170_MAX_CMD_PAYLOAD_LEN];
	} __packed;
} __packed;

#define	CARL9170_TX_STATUS_QUEUE	3
#define	CARL9170_TX_STATUS_QUEUE_S	0
#define	CARL9170_TX_STATUS_RIX_S	2
#define	CARL9170_TX_STATUS_RIX		(3 << CARL9170_TX_STATUS_RIX_S)
#define	CARL9170_TX_STATUS_TRIES_S	4
#define	CARL9170_TX_STATUS_TRIES	(7 << CARL9170_TX_STATUS_TRIES_S)
#define	CARL9170_TX_STATUS_SUCCESS	0x80

/*
 * NOTE:
 * Both structs [carl9170_tx_status and _carl9170_tx_status]
 * need to be "bit for bit" in sync.
 */
struct carl9170_tx_status {
	/*
	 * Beware of compiler bugs in all gcc pre 4.4!
	 */

	u8 cookie;
	u8 queue:2;
	u8 rix:2;
	u8 tries:3;
	u8 success:1;
} __packed;
struct _carl9170_tx_status {
	/*
	 * This version should be immune to all alignment bugs.
	 */

	u8 cookie;
	u8 info;
} __packed;
#define CARL9170_TX_STATUS_SIZE		2

#define	CARL9170_RSP_TX_STATUS_NUM	(CARL9170_MAX_CMD_PAYLOAD_LEN /	\
					 sizeof(struct _carl9170_tx_status))

#define	CARL9170_TX_MAX_RATE_TRIES	7

#define	CARL9170_TX_MAX_RATES		4
#define	CARL9170_TX_MAX_RETRY_RATES	(CARL9170_TX_MAX_RATES - 1)
#define	CARL9170_ERR_MAGIC		"ERR:"
#define	CARL9170_BUG_MAGIC		"BUG:"

struct carl9170_gpio {
	__le32 gpio;
} __packed;
#define CARL9170_GPIO_SIZE		4

struct carl9170_tsf_rsp {
	union {
		__le32 tsf[2];
		__le64 tsf_64;
	} __packed;
} __packed;
#define CARL9170_TSF_RSP_SIZE		8

struct carl9170_rsp {
	struct carl9170_cmd_head hdr;

	union {
		struct carl9170_rf_init_result	rf_init_res;
		struct carl9170_u32_list	rreg_res;
		struct carl9170_u32_list	echo;
		struct carl9170_tx_status	tx_status[0];
		struct _carl9170_tx_status	_tx_status[0];
		struct carl9170_gpio		gpio;
		struct carl9170_tsf_rsp		tsf;
		struct carl9170_psm		psm;
		u8 data[CARL9170_MAX_CMD_PAYLOAD_LEN];
	} __packed;
} __packed;

#endif /* __CARL9170_SHARED_FWCMD_H */
