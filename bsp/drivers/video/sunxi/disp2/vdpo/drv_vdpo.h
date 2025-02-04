/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2017 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#ifndef _DRV_VDPO_H
#define _DRV_VDPO_H

#include "drv_vdpo_common.h"
#include "drv_vdpo_lowlevel.h"
#include "drv_vdpo_timing.h"

u32 bsp_vdpo_get_print_level(void);
void bsp_vdpo_set_print_level(u32 print_level);

/**
 * save some info here for every vdpo module
 * protocol: (0:bt1120, 1:bt656)
 * separate_sync: (0:external sync enable, 1:embedded sync)
 * output_width: (0:8/10, 1:16/20)
 * output_mode:output resolution
 * data_seq_sel:yuv sequence order.(0:Cb-Y-Cr-Y, 1:Cr-Y-Cb-Y
 * 2:Y-Cb-Y-Cr,3:Y-Cr-Y-Cb)
 * dclk_invt:0:do not invert dclk, 1:invert dclk
 * dclk_dly_num:0:do not delay,otherwise:dly num of cycle of dclk
 * spl_type_u and spl_type_v:see table 1-3 in vdpo spec
 */
struct drv_vdpo_info_t {
	struct device *dev;
	enum disp_tv_mode output_mode;
	void __iomem *base_addr;
	struct clk *clk;
	struct clk *clk_parent;
	bool suspend;
	bool used;
	u32 enable;
	struct mutex mlock;
	u32 protocol;
	u32 separate_sync;
	u32 output_width;
	u32 data_seq_sel;
	u32 dclk_invt;
	u32 dclk_dly_num;
	u32 spl_type_u;
	u32 spl_type_v;
};

extern struct drv_vdpo_info_t g_vdpo_info[VDPO_NUM];
extern unsigned int disp_boot_para_parse(const char *name);
extern s32 disp_set_vdpo_func(struct disp_tv_func *func);

#endif /* End of file */
