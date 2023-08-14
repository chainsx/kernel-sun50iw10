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
#ifndef _SUNXI_HDMI20_H_
#define _SUNXI_HDMI20_H_

#include <linux/delay.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/dma-mapping.h>
#include <drm/drmP.h>
#include <linux/clk.h>

#include <video/sunxi_display2.h>

#include "sunxi_common.h"
#include "sunxi_hdmi.h"

#include "hdmi_core/api/hdmitx_dev.h"
#include "hdmi_core/api/access.h"

enum HDMI_VIC {
	HDMI_VIC_640x480P60 = 1,
	HDMI_VIC_720x480P60_4_3,
	HDMI_VIC_720x480P60_16_9,
	HDMI_VIC_1280x720P60,
	HDMI_VIC_1920x1080I60,
	HDMI_VIC_720x480I_4_3,
	HDMI_VIC_720x480I_16_9,
	HDMI_VIC_720x240P_4_3,
	HDMI_VIC_720x240P_16_9,
	HDMI_VIC_1920x1080P60 = 16,
	HDMI_VIC_720x576P_4_3,
	HDMI_VIC_720x576P_16_9,
	HDMI_VIC_1280x720P50,
	HDMI_VIC_1920x1080I50,
	HDMI_VIC_720x576I_4_3,
	HDMI_VIC_720x576I_16_9,
	HDMI_VIC_1920x1080P50 = 31,
	HDMI_VIC_1920x1080P24,
	HDMI_VIC_1920x1080P25,
	HDMI_VIC_1920x1080P30,
	HDMI_VIC_1280x720P24 = 60,
	HDMI_VIC_1280x720P25,
	HDMI_VIC_1280x720P30,
	HDMI_VIC_3840x2160P24 = 93,
	HDMI_VIC_3840x2160P25,
	HDMI_VIC_3840x2160P30,
	HDMI_VIC_3840x2160P50,
	HDMI_VIC_3840x2160P60,
	HDMI_VIC_4096x2160P24,
	HDMI_VIC_4096x2160P25,
	HDMI_VIC_4096x2160P30,
	HDMI_VIC_4096x2160P50,
	HDMI_VIC_4096x2160P60,

	HDMI_VIC_2560x1440P60 = 0x201,
	HDMI_VIC_1440x2560P70 = 0x202,
	HDMI_VIC_1080x1920P60 = 0x203,
};

#define HDMI1080P_24_3D_FP  (HDMI_VIC_1920x1080P24 + 0x80)
#define HDMI720P_50_3D_FP   (HDMI_VIC_1280x720P50 + 0x80)
#define HDMI720P_60_3D_FP   (HDMI_VIC_1280x720P60 + 0x80)

struct cec_params {
	unsigned int support;
	unsigned int support_super_standby;
};

#define POWER_CNT	4
#define POWER_NAME	20
/*
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

	/* hdmi hdcp clk in ccmu */
	struct clk *hdcp_clk;

	/* hdmi cec clk in ccmu */
	struct clk *cec_clk;

	/* name of axp power */
	char		power[POWER_CNT][POWER_NAME];
	u32		power_count;

	hdmi_tx_dev_t hdmi_dev;
	struct hdmi_dev_func dev_funcs;

	struct sunxi_hdmi_work_mode init_params;

	sink_edid_t sink_caps;

	videoParams_t video;
	audioParams_t audio;
	productParams_t product;
	hdcpParams_t hdcp;
	struct cec_params cec;

	struct disp_video_timings *video_info;

	const struct sunxi_hdmi_funcs *funcs;
};

int __init sunxi_hdmi20_module_init(void);
void __exit sunxi_hdmi20_module_exit(void);

extern unsigned int __clk_get_enable_count(struct clk *clk);

extern void hdmitx_api_init(hdmi_tx_dev_t *dev,
	videoParams_t *video,
	audioParams_t *audio,
	hdcpParams_t *hdcp);
extern void hdmitx_api_exit(void);

#define HDMI_INFO(fmt, arg...)   DRM_INFO("[SUNXI-HDMI]"fmt, ##arg)
#define HDMI_ERR(fmt, arg...)   DRM_ERROR("%s()%d - "fmt, __func__, __LINE__, ##arg)

#endif
