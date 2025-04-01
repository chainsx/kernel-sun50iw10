/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#ifndef _SUNXI_DSI_H_
#define _SUNXI_DSI_H_
#include "de/include.h"
#include "de_dsi.h"

struct sunxi_dsi {
	u32 id;

	/* Just for abnormal dsi use */
	u32 tcon_mode;
	u32 slave_tcon_num;
	u32 port_num;
};

s32 sunxi_dsi_mode_switch(struct sunxi_dsi *dsi, u32 cmd_en, u32 lp_en);
s32 sunxi_dsi_clk_enable(struct sunxi_dsi *dsi);
s32 sunxi_dsi_clk_disable(struct sunxi_dsi *dsi);

s32 sunxi_dsi_dcs_write_0para(struct sunxi_dsi *dsi, u8 command);
s32 sunxi_dsi_dcs_write_1para(struct sunxi_dsi *dsi, u8 command, u8 para1);
s32 sunxi_dsi_dcs_write_2para(struct sunxi_dsi *dsi, u8 command, u8 para1,
			u8 para2);
s32 sunxi_dsi_dcs_write_3para(struct sunxi_dsi *dsi, u8 command, u8 para1,
			u8 para2, u8 para3);
s32 sunxi_dsi_dcs_write_4para(struct sunxi_dsi *dsi, u8 command, u8 para1,
			u8 para2, u8 para3, u8 para4);
s32 sunxi_dsi_dcs_write_5para(struct sunxi_dsi *dsi, u8 command, u8 para1,
			u8 para2, u8 para3, u8 para4, u8 para5);
s32 sunxi_dsi_dcs_write_6para(struct sunxi_dsi *dsi, u8 command, u8 para1,
			u8 para2, u8 para3, u8 para4, u8 para5, u8 para6);

s32 sunxi_dsi_gen_wr(struct sunxi_dsi *dsi, u8 command, u8 *para, u32 para_num);
s32 sunxi_dsi_gen_short_read(u32 id, u8 *para_p, u8 para_num, u8 *result);
s32 sunxi_dsi_dcs_read(u32 id, u8 cmd, u8 *result, u32 *num_p);
s32 sunxi_set_max_ret_size(u32 id, u32 size);
s32 sunxi_dsi_turn_on_peripheral_command(struct sunxi_dsi *dsi);
#endif
