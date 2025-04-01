/* drivers/video/sunxi/disp2/disp/lcd/jc0ws008lcm.c
 *
 * Copyright (c) 2022 Allwinnertech Co., Ltd.
 * Author: hongyaobin <hongyaobin@allwinnertech.com>
 *
 * jc0ws008-lcm panel driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <drm/drm_print.h>

#include "../sunxi_tcon.h"
#include "../sunxi_lcd.h"
#include "../sunxi_backlight.h"
#include "../sunxi_dsi.h"
#include "jc0ws008lcm.h"

static void lcd_power_on(u32 sel);
static void lcd_power_off(u32 sel);
static void lcd_bl_open(u32 sel);
static void lcd_bl_close(u32 sel);

static void lcd_panel_init(u32 sel);
static void lcd_panel_exit(u32 sel);

#define panel_reset(sel, val) sunxi_lcd_gpio_set_value(sel, 0, val)

static void lcd_cfg_panel_info(struct panel_extend_para *info)
{
	u32 i = 0, j = 0;
	u32 items;
	u8 lcd_gamma_tbl[][2] = {
		{0, 0},
		{15, 15},
		{30, 30},
		{45, 45},
		{60, 60},
		{75, 75},
		{90, 90},
		{105, 105},
		{120, 120},
		{135, 135},
		{150, 150},
		{165, 165},
		{180, 180},
		{195, 195},
		{210, 210},
		{225, 225},
		{240, 240},
		{255, 255},
	};

	u32 lcd_cmap_tbl[2][3][4] = {
	{
		{LCD_CMAP_G0, LCD_CMAP_B1, LCD_CMAP_G2, LCD_CMAP_B3},
		{LCD_CMAP_B0, LCD_CMAP_R1, LCD_CMAP_B2, LCD_CMAP_R3},
		{LCD_CMAP_R0, LCD_CMAP_G1, LCD_CMAP_R2, LCD_CMAP_G3},
		},
		{
		{LCD_CMAP_B3, LCD_CMAP_G2, LCD_CMAP_B1, LCD_CMAP_G0},
		{LCD_CMAP_R3, LCD_CMAP_B2, LCD_CMAP_R1, LCD_CMAP_B0},
		{LCD_CMAP_G3, LCD_CMAP_R2, LCD_CMAP_G1, LCD_CMAP_R0},
		},
	};

	items = sizeof(lcd_gamma_tbl) / 2;
	for (i = 0; i < items - 1; i++) {
		u32 num = lcd_gamma_tbl[i+1][0] - lcd_gamma_tbl[i][0];

		for (j = 0; j < num; j++) {
			u32 value = 0;

			value = lcd_gamma_tbl[i][1] +
				((lcd_gamma_tbl[i+1][1] - lcd_gamma_tbl[i][1])
				* j) / num;
			info->lcd_gamma_tbl[lcd_gamma_tbl[i][0] + j] =
							(value << 16)
							+ (value << 8) + value;
		}
	}
	info->lcd_gamma_tbl[255] = (lcd_gamma_tbl[items-1][1] << 16) +
					(lcd_gamma_tbl[items-1][1] << 8)
					+ lcd_gamma_tbl[items-1][1];

	memcpy(info->lcd_cmap_tbl, lcd_cmap_tbl, sizeof(lcd_cmap_tbl));
}

static s32 lcd_open_flow(u32 sel)
{
	int id;

#if IS_ENABLED(CONFIG_ARCH_SUN8I) /* @TODO: fix this */
	/* set the PLL_MIPI clk rate & enable bit LDO1、LDO2 */
	void __iomem *ioaddr = ioremap(0x1c20040, 4);
	writel(0x80c00011, ioaddr);
	iounmap(ioaddr);
#endif

	lcd_power_on(sel);
	msleep(100);
	lcd_panel_init(sel);
	msleep(200);
	id = sunxi_lcd_get_tcon_id(sel);
	if (id < 0) {
		DRM_ERROR("get tcon id failed\n");
		return -1;
	}
	if (sunxi_tcon_lcd_enable(id, sel)) {
		DRM_ERROR("sunxi_tcon_lcd_enable failed\n");
		return -1;

	}
	msleep(50);
	lcd_bl_open(sel);

	return 0;
}

static s32 lcd_close_flow(u32 sel)
{
	int id;

	lcd_bl_close(sel);
	msleep(200);
	id = sunxi_lcd_get_tcon_id(sel);
	if (id < 0) {
		DRM_ERROR("get tcon id failed\n");
		return -1;
	}
	if (sunxi_tcon_lcd_disable(id, sel) < 0) {
		DRM_ERROR("sunxi_tcon_lcd_disable failed\n");
		return -1;

	}
	msleep(20);
	lcd_panel_exit(sel);
	msleep(10);
	lcd_power_off(sel);
	msleep(500);

	return 0;
}

static void lcd_power_on(u32 sel)
{
	sunxi_lcd_power_enable(sel, 0); //config lcd_power pin to open lcd power
	msleep(5);
	sunxi_lcd_power_enable(sel, 1); //config lcd_power pin to open lcd power1
	msleep(5);
	sunxi_lcd_power_enable(sel, 2); //config lcd_power pin to open lcd power2
	msleep(25);
	sunxi_lcd_gpio_set_value(sel, 0, 1);
	sunxi_lcd_gpio_set_value(sel, 1, 1);
	msleep(5);
	sunxi_lcd_pin_cfg(sel, 1);
}

static void lcd_power_off(u32 sel)
{
	sunxi_lcd_pin_cfg(sel, 0);
	msleep(20);
	sunxi_lcd_gpio_set_value(sel, 0, 0);
	sunxi_lcd_gpio_set_value(sel, 1, 0);
	msleep(5);
	sunxi_lcd_power_disable(sel, 2); //config lcd_power pin to close lcd power2
	msleep(5);
	sunxi_lcd_power_disable(sel, 1); //config lcd_power pin to close lcd power1
	msleep(5);
	sunxi_lcd_power_disable(sel, 0); //config lcd_power pin to close lcd power
}

static void lcd_bl_open(u32 sel)
{
	sunxi_backlight_enable(sel);
}

static void lcd_bl_close(u32 sel)
{
	sunxi_backlight_disable(sel);
}

static void lcd_panel_init(u32 sel)
{
	struct sunxi_dsi dsi;
	unsigned int tcon_id;

	tcon_id = sunxi_lcd_get_tcon_id(sel);
	if (tcon_id < 0) {
		DRM_ERROR("get tcon id failed\n");
		return;
	}

	dsi.id = sunxi_lcd_get_type_id(sel);
	dsi.tcon_mode = sunxi_tcon_mode(tcon_id);
	dsi.slave_tcon_num = sunxi_tcon_slave_num(tcon_id);
	dsi.port_num = sunxi_tcon_port_num(tcon_id);

	sunxi_dsi_clk_enable(&dsi);
	msleep(20);
	sunxi_dsi_dcs_write_0para(&dsi, DSI_DCS_SOFT_RESET);
	msleep(10);

	return;
}

static void lcd_panel_exit(u32 sel)
{
	unsigned int tcon_id;
	struct sunxi_dsi dsi;

	tcon_id = sunxi_lcd_get_tcon_id(sel);
	if (tcon_id < 0) {
		DRM_ERROR("get tcon id failed\n");
		return;
	}

	dsi.id = sunxi_lcd_get_type_id(sel);
	dsi.tcon_mode = sunxi_tcon_mode(tcon_id);
	dsi.slave_tcon_num = sunxi_tcon_slave_num(tcon_id);
	dsi.port_num = sunxi_tcon_port_num(tcon_id);

	sunxi_dsi_dcs_write_0para(&dsi, DSI_DCS_SET_DISPLAY_OFF);
	msleep(20);
	sunxi_dsi_dcs_write_0para(&dsi, DSI_DCS_ENTER_SLEEP_MODE);
	msleep(80);

	return ;
}

/*sel: 0:lcd0; 1:lcd1*/
static s32 lcd_user_defined_func(u32 sel, u32 para1, u32 para2, u32 para3)
{
	return 0;
}

struct __lcd_panel jc0ws008lcm_panel = {
	/* panel driver name, must mach the name of
	 * lcd_drv_name in sys_config.fex
	 */
	.name = "jc0ws008lcm",
	.func = {
		.cfg_panel_info = lcd_cfg_panel_info,
		.cfg_open_flow = lcd_open_flow,
		.cfg_close_flow = lcd_close_flow,
		.lcd_user_defined_func = lcd_user_defined_func,
	},
};

