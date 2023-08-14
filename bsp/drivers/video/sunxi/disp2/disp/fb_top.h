/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2022 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __FB_TOP_H__
#define __FB_TOP_H__

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/fb.h>
#include <linux/memblock.h>
#include <linux/decompress/unlzma.h>
#include <linux/dma-buf.h>
#include <linux/dma-mapping.h>
#include <video/sunxi_display2.h>

#include "dev_disp.h"
#include "de/disp_edp.h"
#include "fb_g2d_rot.h"

#define MAX_DEVICE	8
extern int fb_debug_val;

#define fb_debug_inf(fmt, arg...)\
	do {\
		if (unlikely(fb_debug_val)) {\
			if (fb_debug_val > 1) \
				printk("%s: " fmt, "[DISP FB]", ## arg);\
			else\
				printk(KERN_ERR "%s: " fmt, "[DISP FB]", ## arg);\
		} \
	} while (0)

struct fb_map {
	u32 hw_display;
	u32 hw_channel;
	u32 hw_layer;
	u32 hw_zorder;
};

struct fb_init_info_each {
	u32 width;
	u32 height;
	struct fb_map map;
	bool logo_display;
	bool g2d_rot_used;
	int rot_degree;
};

struct logo_info_fb {
	unsigned long long phy_addr;
	unsigned long width;
	unsigned long height;
	unsigned long bpp;
	unsigned long stride;
	unsigned long crop_l;
	unsigned long crop_t;
	unsigned long crop_r;
	unsigned long crop_b;
};

struct logo_info_file {
	unsigned long long phy_addr;
};

enum fb_logo_type {
	LOGO_FILE = 0,
	LOGO_FB,
};

union logo_info {
	struct logo_info_fb logo_fb;
	struct logo_info_file logo_file;
};

struct fb_info_share {
	int fb_num;
	struct device *disp_dev;
	enum fb_logo_type logo_type;
	union logo_info logo;
	enum disp_pixel_format format;
	bool smooth_display;
	int smooth_display_hw_id;
	enum disp_output_type output_type[MAX_DEVICE];
	unsigned int output_mode[MAX_DEVICE];
};

struct fb_create_info {
	/* size fb_share.fb_num */
	struct fb_init_info_each *fb_each;
	struct fb_info_share fb_share;
};
#endif
