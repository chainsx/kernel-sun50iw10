/*
 * drivers/media/platform/sunxi-tvd/tvd/tvd.h
 *
 * Copyright (c) 2007-2018 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
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
#ifndef __TVD__H__
#define __TVD__H__

#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/workqueue.h>
#include <linux/pm_runtime.h>

#include <media/sunxi_camera.h>
#include <media/videobuf2-core.h>
#include <media/v4l2-device.h>
#include <linux/media-bus-format.h>
#include <media/v4l2-ctrls.h>
#include "bsp_tvd.h"
#include <linux/clk-provider.h>

#define TVD_MAX 4

static LIST_HEAD(devlist);

enum hpd_status {
	STATUE_CLOSE = 0,
	STATUE_OPEN  = 1,
};

struct tvd_fmt {
	u8              name[32];
	u32           	fourcc;          /* v4l2 format id */
	TVD_FMT_T    	output_fmt;
	int   	        depth;
	enum v4l2_field field;
	int    bus_pix_code;
	unsigned char   planes_cnt;
};

enum tvd_interface {
	CVBS,
	YPBPRI,
	YPBPRP,
};

/* buffer for one video frame */
struct tvd_buffer {
	struct vb2_buffer	vb;
	struct list_head	list;
	struct tvd_fmt		*fmt;
	enum vb2_buffer_state	state;
	void *paddr;
};

struct vin_buffer {
	struct vb2_buffer vb;
	struct list_head list;
	void *paddr;
	enum vb2_buffer_state	state;
};


struct tvd_dmaqueue {
	struct list_head  active;
	/* Counters to control fps rate */
	int               frame;
	int               ini_jiffies;
};

struct writeback_addr {
	dma_addr_t	y;
	dma_addr_t	c;
};

struct tvd_status {
	int tvd_used;
	int tvd_opened;
	int tvd_if;
	int tvd_3d_used;
	int locked;
	int tvd_system;
	int tvd_clk_enabled;
};

struct tvd_3d_fliter {
	int used;
	int size;
	void *vir_address;
	void *phy_address;
};

struct tvd_dev {
	struct list_head       	devlist;
	struct v4l2_device      v4l2_dev;
	struct v4l2_subdev	*sd;
	struct platform_device	*pdev;
	int			sel;
	int			id;
	int			first_flag;
	spinlock_t              slock;

	/* various device info */
	struct video_device     *vfd;
	struct tvd_dmaqueue     vidq;

	/* Several counters */
	unsigned     		ms;
	unsigned long           jiffies;

	/* Input Number */
	int	                input;

	/* video capture */
	struct tvd_fmt          *fmt;
	unsigned int            width;
	unsigned int            height;
	unsigned int		frame_size;
	unsigned int            capture_mode;
	struct vb2_queue   	vb_vidq;

	unsigned int            interface; /* 0 cvbs,1 ypbprI,2 ypbprP */
	unsigned int            system;	/* 0 ntsc, 1 pal */
	unsigned int		row;
	unsigned int		column;

	unsigned int            locked;	/* signal is stable or not */
	unsigned int            format;
	unsigned int            channel_index[TVD_MAX];
	unsigned int            channel_offset_y[TVD_MAX];
	unsigned int            channel_offset_c[TVD_MAX];
	int			irq;
	char			name[10];

	/* working state */
	unsigned long 	        generating;
	unsigned long		opened;

	/* clock */
	struct clk		*parent;
	struct clk		*clk;
	struct clk		*clk_bus;
	struct reset_control *rst_tvd;

	struct clk		*clk_top;
	struct clk		*clk_mbus;
	struct reset_control *clk_top_rst;

	void __iomem		*regs_top;
	void __iomem		*regs_tvd;

	struct writeback_addr	wb_addr;

	/* tvd revelent para */
	unsigned int            luma;
	unsigned int            contrast;
	unsigned int            saturation;
	unsigned int            hue;

	unsigned int            uv_swap;
	struct v4l2_fract       fps;
	struct tvd_3d_fliter fliter;
	/* struct early_suspend early_suspend; */
	unsigned int 		para_from;
	struct v4l2_ctrl_handler  ctrl_handler;
	struct mutex              buf_lock;
	struct mutex              stream_lock;
	struct mutex		  opened_lock;
	struct tvd_dmaqueue       vidq_special;
	struct tvd_dmaqueue       done_special;
	int special_active;
	int mulit_channel_mode;
};

#define SUNXI_TVD_DEBUG_LEVEL 0

#define tvd_wrn(fmt, ...)                                                     \
	pr_warn("[tvd] %s:%d " fmt "", __func__, __LINE__, ##__VA_ARGS__)

#if SUNXI_TVD_DEBUG_LEVEL == 2
#define tvd_here pr_warn("[tvd] %s:%d\n", __func__, __LINE__)
#else
#define tvd_here
#endif /* endif EDP_DEBUG */

#if SUNXI_TVD_DEBUG_LEVEL >= 1
#define tvd_dbg(fmt, ...)                                                     \
	pr_warn("[tvd] %s:%d " fmt "", __func__, __LINE__, ##__VA_ARGS__)
#else
#define tvd_dbg(fmt, ...)
#endif /* endif EDP_DEBUG */

#endif /* __TVD__H__ */

