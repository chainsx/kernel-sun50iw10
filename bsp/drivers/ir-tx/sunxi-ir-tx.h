/*
 *
 * Copyright (c) 2013-2020 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _SUNXI_IR_TX_H
#define _SUNXI_IR_TX_H

#include <asm/ioctl.h>

#define IR_TX_GLR		(0x00)
#define IR_TX_MCR		(0x04)
#define IR_TX_CR		(0x08)
#define IR_TX_IDC_H		(0x0c)
#define IR_TX_IDC_L		(0x10)
#define IR_TX_ICR_H		(0x14)
#define IR_TX_ICR_L		(0x18)
#define IR_TX_TELR		(0x20)
#define IR_TX_INTC		(0x24)
#define IR_TX_TACR		(0x28)
#define IR_TX_STAR		(0x2c)
#define IR_TX_TR		(0x30)
#define IR_TX_DMAC		(0x34)
#define IR_TX_FIFO_DR		(0x80)

#define IR_TX_GL_VALUE		(0xa3)
#define IR_TX_MC_VALUE		(0x86)
#define IR_TX_CLK_VALUE		(0x05 << 1)
#define IR_TX_IDC_H_VALUE	(0x04)
#define IR_TX_IDC_L_VALUE	(0x00)
#define IR_TX_TEL_VALUE		(0x96 -  1)
#define IR_TX_INT_C_VALUE	(0x01)
#define IR_TX_STA_VALUE		(0x03)
#define IR_TX_T_VALUE		(0x64)
#define IR_TX_CLK               12000000

#define SUNXI_IR_TX_DRIVER_NAME "sunxi-ir-tx"
#define SUNXI_IR_TX_DEVICE_NAME "sunxi-ir-tx"
#define RC_MAP_SUNXI "rc_map_sunxi"

#define IR_TX_FIFO_SIZE		(128)

#define	IR_TX_RAW_BUF_SIZE	(256)
#define IR_TX_CYCLE_TYPE	(0)	/* 1:cycle 0:non-cycle */
#define IR_TX_CLK_Ts		(1)

#define SUNXI_IR_TX_VERSION "v1.0.1"

enum {
	DEBUG_INIT    = 1U << 0,
	DEBUG_INFO    = 1U << 1,
	DEBUG_SUSPEND = 1U << 2,
};

struct cmd {
	unsigned char protocol, address, command;
};

#define IR_TX_IOCSEND _IOR(66, 1, struct cmd)

#endif /* _SUNXI_IR_TX_H */
