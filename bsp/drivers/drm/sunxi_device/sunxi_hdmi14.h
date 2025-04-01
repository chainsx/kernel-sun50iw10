/*
 * Copyright (c) 2007-2019 Allwinnertech Co., Ltd.
 * Author: zhengwanyu <zhengwanyu@allwinnertech.com>
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
#ifndef _SUNXI_HDMI14_H_
#define _SUNXI_HDMI14_H_

#include <linux/delay.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/dma-mapping.h>
#include <drm/drmP.h>
#include <linux/clk.h>

#include <video/sunxi_display2.h>

#include "hdmi_bsp.h"
#include "sunxi_common.h"
#include "sunxi_hdmi.h"

#define Abort_Current_Operation             0
#define Special_Offset_Address_Read         1
#define Explicit_Offset_Address_Write       2
#define Implicit_Offset_Address_Write       3
#define Explicit_Offset_Address_Read        4
#define Implicit_Offset_Address_Read        5
#define Explicit_Offset_Address_E_DDC_Read  6
#define Implicit_Offset_Address_E_DDC_Read  7

#define HDMI1440_480I 6
#define HDMI1440_576I 21
#define HDMI480P 2
#define HDMI576P 17
#define HDMI720P_50 19
#define HDMI720P_60 4
#define HDMI1080I_50 20
#define HDMI1080I_60 5
#define HDMI1080P_50 31
#define HDMI1080P_60 16
#define HDMI1080P_24 32
#define HDMI1080P_25 33
#define HDMI1080P_30 34
#define HDMI1080P_24_3D_FP (HDMI1080P_24 + 0x80)
#define HDMI720P_50_3D_FP (HDMI720P_50 + 0x80)
#define HDMI720P_60_3D_FP (HDMI720P_60 + 0x80)
#define HDMI3840_2160P_30 (0x01 + 0x100)
#define HDMI3840_2160P_25 (0x02 + 0x100)
#define HDMI3840_2160P_24 (0x03 + 0x100)
#define HDMI4096_2160P_24 (0x04 + 0x100)

/**
 * sunxi_hdmi device class
 */
struct sunxi_hdmi {
	struct platform_device *pdev;

	/* sunxi hdmi virtual address */
	void __iomem *reg_base;

	/* hdmi clk in ccmu */
	struct clk *mclk;
	/* hdmi's parent clk in ccmu */
	struct clk *mclk_parent;

	/* hdmi ddc clk in ccmu */
	struct clk *ddc_clk;

	/* hdmi cec clk in ccmu */
	struct clk *cec_clk;

	/* name of axp power */
	const char *power;

	struct audio_para audio;
	struct video_para video;

	struct disp_video_timings *video_info;

	const struct sunxi_hdmi_funcs *funcs;
};

int __init sunxi_hdmi_module_init(void);
void __exit sunxi_hdmi_module_exit(void);

struct sunxi_hdmi_funcs *sunxi_hdmi_get_funcs(void);
extern unsigned int __clk_get_enable_count(struct clk *clk);
#define HDMI_INFO(fmt, arg...)   DRM_INFO("[SUNXI-HDMI]"fmt, ##arg)
#define HDMI_ERR(fmt, arg...)   DRM_ERROR("%s()%d - "fmt, __func__, __LINE__, ##arg)
#endif
