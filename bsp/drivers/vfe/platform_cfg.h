/*
 * linux-4.9/drivers/media/platform/sunxi-vfe/platform_cfg.h
 *
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
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

/*
 ***************************************************************************************
 *
 * platform_cfg.h
 *
 * Hawkview ISP - platform_cfg.h module
 *
 * Copyright (c) 2014 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Version		  Author         Date		    Description
 *
 *   2.0		  Yang Feng	2014/07/24	      Second Version
 *
 ****************************************************************************************
 */

#ifndef __PLATFORM_CFG__H__
#define __PLATFORM_CFG__H__

/* #define FPGA_VER */

#ifdef FPGA_VER
#define FPGA_PIN
#else
#define VFE_CLK
#define VFE_GPIO
#define VFE_PMU
#endif

#include <linux/gpio.h>

#ifdef VFE_CLK
#include <linux/clk.h>
//#include <linux/clk/sunxi.h>
#include <sunxi-clk.h>
#include <linux/sh_clk.h>
#endif

#ifdef VFE_GPIO
#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/pinconf.h>
#endif

#ifdef VFE_PMU
#include <linux/regulator/consumer.h>
#endif

#include <sunxi-gpio.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>

#include <linux/slab.h>

#ifdef FPGA_VER
#define DPHY_CLK (48*1000*1000)
#else
#define DPHY_CLK (150*1000*1000)
#endif

#if defined CONFIG_ARCH_SUN3IW1P1
#include "platform/sun3iw1p1_vfe_cfg.h"
#define SUNXI_PLATFORM_ID ISP_PLATFORM_NUM
#elif defined CONFIG_ARCH_SUN50IW1P1
#include "platform/sun50iw1p1_vfe_cfg.h"
#define SUNXI_PLATFORM_ID ISP_PLATFORM_SUN50IW1P1
#elif defined CONFIG_ARCH_SUN8IW8P1
#include "platform/sun8iw8p1_vfe_cfg.h"
#define SUNXI_PLATFORM_ID ISP_PLATFORM_SUN8IW8P1
#elif defined CONFIG_ARCH_SUN8IW5P1
#include "platform/sun8iw5p1_vfe_cfg.h"
#define SUNXI_PLATFORM_ID ISP_PLATFORM_SUN8IW5P1
#elif defined CONFIG_ARCH_SUN8IW6P1
#include "platform/sun8iw6p1_vfe_cfg.h"
#define SUNXI_PLATFORM_ID ISP_PLATFORM_SUN8IW6P1
#elif defined CONFIG_ARCH_SUN8IW7P1
#include "platform/sun8iw7p1_vfe_cfg.h"
#define SUNXI_PLATFORM_ID ISP_PLATFORM_SUN8IW7P1
#elif defined CONFIG_ARCH_SUN8IW10P1
#include "platform/sun8iw10p1_vfe_cfg.h"
#define SUNXI_PLATFORM_ID ISP_PLATFORM_NUM
#elif defined CONFIG_ARCH_SUN8IW11
/* Not currently supported CH_OUTPUT_IN_DIFFERENT_VIDEO */
//#define CH_OUTPUT_IN_DIFFERENT_VIDEO
#include "platform/sun8iw11p1_vfe_cfg.h"
#define SUNXI_PLATFORM_ID ISP_PLATFORM_NUM
#elif defined CONFIG_ARCH_SUN50IW2P1
#include "platform/sun50iw2p1_vfe_cfg.h"
#define SUNXI_PLATFORM_ID ISP_PLATFORM_SUN50IW1P1
#else
#include "platform/sun8iw11p1_vfe_cfg.h"
#define SUNXI_PLATFORM_ID ISP_PLATFORM_NUM
#endif

#define ISP_LUT_MEM_OFS             0x0
#define ISP_LENS_MEM_OFS            (ISP_LUT_MEM_OFS + ISP_LUT_MEM_SIZE)
#define ISP_GAMMA_MEM_OFS           (ISP_LENS_MEM_OFS + ISP_LENS_MEM_SIZE)
#define ISP_LINEAR_MEM_OFS           (ISP_GAMMA_MEM_OFS + ISP_GAMMA_MEM_SIZE)

#define ISP_DRC_MEM_OFS            0x0
#define ISP_DISC_MEM_OFS          (ISP_DRC_MEM_OFS + ISP_DRC_MEM_SIZE)

#endif /* __PLATFORM_CFG__H__ */
