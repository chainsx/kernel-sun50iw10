/*
 * drivers/usb/sunxi_usb/include/sunxi_usb_config.h
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * javen, 2011-4-14, create this file
 *
 * usb config head file.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __SUNXI_USB_CONFIG_H__
#define __SUNXI_USB_CONFIG_H__

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/errno.h>

#include  "sunxi_usb_typedef.h"
#include  "sunxi_usb_debug.h"
#include  "sunxi_usb_bsp.h"

#include  "sunxi_usb_board.h"
#include  "sunxi_udc.h"
#include  <sunxi-gpio.h>
#include  <linux/gpio.h>

#if defined(CONFIG_USB_SUNXI_HCD0)
#include "sunxi_hcd.h"
#endif

#if defined(CONFIG_AW_FPGA_S4) || defined(CONFIG_AW_FPGA_V7)
#define SUNXI_USB_FPGA
#endif

#endif /* __SUNXI_USB_CONFIG_H__ */
