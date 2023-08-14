/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2019 Allwinner.
 *
 * Author:zhengwanyu  <zhengwanyu@allwinnertech.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include "sunxi_dsi.h"

/**
 * dsi module mode switch
 * @id         :dsi id
 * @param[IN]  :cmd_en: command mode enable
 * @param[IN]  :lp_en: lower power mode enable
 * @return     :0 if success
 */
s32 sunxi_dsi_mode_switch(struct sunxi_dsi *dsi, u32 cmd_en, u32 lp_en)
{
	s32 ret = -1;

	if (dsi->tcon_mode == DISP_TCON_SLAVE_MODE)
		goto OUT;

	ret = dsi_mode_switch(dsi->id, cmd_en, lp_en);
	if (dsi->tcon_mode == DISP_TCON_DUAL_DSI
		&& (dsi->id + 1) < DEVICE_DSI_NUM)
		ret = dsi_mode_switch(dsi->id + 1, cmd_en, lp_en);
	else if (dsi->tcon_mode != DISP_TCON_NORMAL_MODE
			&& dsi->tcon_mode != DISP_TCON_DUAL_DSI)
		ret = dsi_mode_switch(dsi->slave_tcon_num, cmd_en, lp_en);

OUT:
	return ret;
}


static s32 sunxi_dsi_clk_en(struct sunxi_dsi *dsi, u32 en)
{
	s32 ret = -1;


	if (dsi->tcon_mode == DISP_TCON_SLAVE_MODE)
		goto OUT;

	ret = dsi_clk_enable(dsi->id, en);
	if (dsi->tcon_mode == DISP_TCON_DUAL_DSI &&
	    dsi->id + 1 < DEVICE_DSI_NUM)
		ret = dsi_clk_enable(dsi->id + 1, en);
	else if (dsi->tcon_mode != DISP_TCON_NORMAL_MODE &&
		 dsi->tcon_mode != DISP_TCON_DUAL_DSI)
		ret = dsi_clk_enable(dsi->slave_tcon_num, en);
OUT:
	return ret;
}

s32 sunxi_dsi_clk_enable(struct sunxi_dsi *dsi)
{
	return sunxi_dsi_clk_en(dsi, 1);
}

s32 sunxi_dsi_clk_disable(struct sunxi_dsi *dsi)
{
	return sunxi_dsi_clk_en(dsi, 0);
}

static s32 sunxi_dsi_dcs_wr(struct sunxi_dsi *dsi, u8 command, u8 *para,
						u32 para_num)
{
	s32 ret = -1;


	if (dsi->tcon_mode == DISP_TCON_SLAVE_MODE)
		goto OUT;

	ret = dsi_dcs_wr(dsi->id, command, para, para_num);
	if (dsi->tcon_mode == DISP_TCON_DUAL_DSI
		&& (dsi->id + 1) < DEVICE_DSI_NUM
		&& dsi->port_num == DISP_LCD_DSI_SINGLE_PORT)
		ret = dsi_dcs_wr(dsi->id + 1, command, para, para_num);
	else if (dsi->tcon_mode != DISP_TCON_NORMAL_MODE &&
		 dsi->tcon_mode != DISP_TCON_DUAL_DSI)
		ret = dsi_dcs_wr(dsi->slave_tcon_num, command, para,
				 para_num);
OUT:
	return ret;
}

/**
 * sunxi_dsi_dcs_wr - write command and para to mipi panel.
 * @dsi: The index of screen.
 * @command: Command to be transfer.
 */
s32 sunxi_dsi_dcs_write_0para(struct sunxi_dsi *dsi, u8 command)
{
	u8 tmp[5];

	sunxi_dsi_dcs_wr(dsi, command, tmp, 0);

	return -1;
}

/**
 * sunxi_dsi_dcs_write_1para - write command and para to mipi panel.
 * @dsi: The index of screen.
 * @command: Command to be transfer.
 * @paran: Para to be transfer.
 */
s32 sunxi_dsi_dcs_write_1para(struct sunxi_dsi *dsi, u8 command, u8 para1)
{
	u8 tmp[5];

	tmp[0] = para1;
	sunxi_dsi_dcs_wr(dsi, command, tmp, 1);

	return -1;
}

s32 sunxi_dsi_dcs_write_2para(struct sunxi_dsi *dsi, u8 command, u8 para1,
			u8 para2)
{
	u8 tmp[5];

	tmp[0] = para1;
	tmp[1] = para2;
	sunxi_dsi_dcs_wr(dsi, command, tmp, 2);

	return -1;
}

s32 sunxi_dsi_dcs_write_3para(struct sunxi_dsi *dsi, u8 command, u8 para1,
			u8 para2, u8 para3)
{
	u8 tmp[5];

	tmp[0] = para1;
	tmp[1] = para2;
	tmp[2] = para3;
	sunxi_dsi_dcs_wr(dsi, command, tmp, 3);

	return -1;
}

s32 sunxi_dsi_dcs_write_4para(struct sunxi_dsi *dsi, u8 command, u8 para1,
		u8 para2, u8 para3, u8 para4)
{
	u8 tmp[5];

	tmp[0] = para1;
	tmp[1] = para2;
	tmp[2] = para3;
	tmp[3] = para4;
	sunxi_dsi_dcs_wr(dsi, command, tmp, 4);

	return -1;
}

s32 sunxi_dsi_dcs_write_5para(struct sunxi_dsi *dsi, u8 command, u8 para1,
		u8 para2, u8 para3, u8 para4, u8 para5)
{
	u8 tmp[5];

	tmp[0] = para1;
	tmp[1] = para2;
	tmp[2] = para3;
	tmp[3] = para4;
	tmp[4] = para5;
	sunxi_dsi_dcs_wr(dsi, command, tmp, 5);

	return -1;
}

s32 sunxi_dsi_dcs_write_6para(struct sunxi_dsi *dsi, u8 command, u8 para1,
		u8 para2,  u8 para3, u8 para4, u8 para5, u8 para6)
{
	u8 tmp[6];

	tmp[0] = para1;
	tmp[1] = para2;
	tmp[2] = para3;
	tmp[3] = para4;
	tmp[4] = para5;
	tmp[5] = para6;
	sunxi_dsi_dcs_wr(dsi, command, tmp, 6);

	return -1;
}


s32 sunxi_dsi_gen_wr(struct sunxi_dsi *dsi,
			u8 command, u8 *para, u32 para_num)
{
	s32 ret = -1;

	if (dsi->tcon_mode == DISP_TCON_SLAVE_MODE)
		goto OUT;

	ret = dsi_gen_wr(dsi->id, command, para, para_num);
	if (dsi->tcon_mode == DISP_TCON_DUAL_DSI &&
		(dsi->id + 1) < DEVICE_DSI_NUM &&
			dsi->port_num == DISP_LCD_DSI_SINGLE_PORT)
		ret = dsi_gen_wr(dsi->id + 1, command, para, para_num);
	else if (dsi->tcon_mode != DISP_TCON_NORMAL_MODE &&
		 dsi->tcon_mode != DISP_TCON_DUAL_DSI)
		ret = dsi_gen_wr(dsi->slave_tcon_num, command, para,
				 para_num);
OUT:
	return ret;
}

s32 sunxi_dsi_gen_short_read(u32 id, u8 *para_p, u8 para_num,
				    u8 *result)
{
	s32 ret = -1;

	if (!result || !para_p || para_num > 2) {
		pr_err("[sunxi-dsi]error: wrong para\n");
		goto OUT;
	}

	ret = dsi_gen_short_rd(id, para_p, para_num, result);
OUT:
	return ret;
}

s32 sunxi_dsi_dcs_read(u32 id, u8 cmd, u8 *result, u32 *num_p)
{
	s32 ret = -1;

	if (!result || !num_p) {
		pr_err("[sunxi-dsi]error: wrong para\n");
		goto OUT;
	}
	ret = dsi_dcs_rd(id, cmd, result, num_p);
OUT:
	return ret;
}

s32 sunxi_set_max_ret_size(u32 id, u32 size)
{
	return dsi_set_max_ret_size(id, size);
}

s32 sunxi_dsi_turn_on_peripheral_command(struct sunxi_dsi *dsi)
{
#ifdef CONFIG_ARCH_SUN20IW1
	return 0;
#else
	//return dsi_turn_on_peripheral_command(dsi->id);
	return 0;
#endif
}
