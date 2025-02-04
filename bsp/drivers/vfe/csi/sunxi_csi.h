/*
 * linux-4.9/drivers/media/platform/sunxi-vfe/csi/sunxi_csi.h
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
 * linux-4.9/drivers/media/platform/sunxi-vfe/csi/sunxi_csi.h
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
 * sunxi_csi.h
 *
 * Hawkview ISP - sunxi_csi.h module
 *
 * Copyright (c) 2015 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Version		  Author         Date		    Description
 *
 *   3.0		  Yang Feng	2015/02/27	ISP Tuning Tools Support
 *
 ****************************************************************************************
 */

#ifndef _SUNXI_CSI_H_
#define _SUNXI_CSI_H_

#include "../platform_cfg.h"
#include <linux/reset.h>

#define VIDIOC_SUNXI_CSI_GET_FRM_SIZE			1
#define VIDIOC_SUNXI_CSI_SET_CORE_CLK			2
#define VIDIOC_SUNXI_CSI_SET_M_CLK			3

#define CSI_CORE_CLK_RATE (300*1000*1000)

#if defined(CONFIG_ARCH_SUN3IW1P1) || defined(CONFIG_ARCH_SUN8IW7P1) \
				|| defined(CONFIG_ARCH_SUN50IW1P1) \
				|| defined(CONFIG_ARCH_SUN8IW6P1) \
				|| defined(CONFIG_ARCH_SUN8IW8P1)
enum {
	CSI_CORE_CLK = 0,
	CSI_MASTER_CLK,
	CSI_MISC_CLK,
	CSI_CORE_CLK_SRC,
	CSI_MASTER_CLK_24M_SRC,
	CSI_MASTER_CLK_PLL_SRC,
	CSI_CLK_NUM,
};
#else
enum {
	CSI_CORE_CLK,
	CSI_MASTER_CLK,
	CSI_CORE_CLK_SRC,
	CSI_MASTER_CLK_PLL_SRC,
	CSI_MASTER_CLK_24M_SRC,
	CSI_CLK_NUM,
};
#endif

enum {
	VIN_CSI_RET = 0,
	VIN_ISP_RET,
	VIN_MAX_RET,
};

enum {
	VIN_CSI_BUS_CLK = 0,
	VIN_CSI_MBUS_CLK,
	VIN_ISP_BUS_CLK,
	VIN_ISP_MBUS_CLK,
	VIN_MAX_BUS_CLK,
};

#define NOCLK			0xff

struct csi_dev {
	int use_cnt;
	struct v4l2_subdev subdev;
	struct platform_device  *pdev;
	unsigned int id;
	spinlock_t slock;
	struct mutex subdev_lock;
	int irq;
	wait_queue_head_t   wait;
	void __iomem      *base;
	struct bus_info         bus_info;
	struct frame_info       frame_info;
	struct frame_arrange    arrange;
	unsigned int capture_mode;
	struct list_head csi_list;
	struct pinctrl *pctrl;
	struct clk *clock[CSI_CLK_NUM];
	struct reset_control *clk_reset[VIN_MAX_RET];
	struct clk *bus_clk[VIN_MAX_BUS_CLK];
};

int sunxi_csi_subdev_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param);
void sunxi_csi_dump_regs(struct v4l2_subdev *sd);
int sunxi_csi_get_subdev(struct v4l2_subdev **sd, int sel);
int sunxi_csi_put_subdev(struct v4l2_subdev **sd, int sel);
int sunxi_csi_register_subdev(struct v4l2_device *v4l2_dev, struct v4l2_subdev *sd);
void sunxi_csi_unregister_subdev(struct v4l2_subdev *sd);
int sunxi_csi_platform_register(void);
void sunxi_csi_platform_unregister(void);


#endif /*_SUNXI_CSI_H_*/
