/* default_panel.c
 *
 * Copyright (C) 2022 Allwinnertech Co., Ltd.
 * Authors: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 * Authors: zhengwanyu <zhengwanyu@allwinnertech.com>
 * Authors: hongyaobin <hongyaobin@allwinnertech.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */
#include "../sunxi_tcon.h"
#include "../sunxi_lcd.h"
#include "../sunxi_backlight.h"
#include "default_panel.h"
#include <drm/drm_print.h>

static void lcd_power_on(u32 sel);
static void lcd_power_off(u32 sel);
static void lcd_bl_open(u32 sel);
static void lcd_bl_close(u32 sel);

static void lcd_panel_init(u32 sel);
static void lcd_panel_exit(u32 sel);

static void lcd_cfg_panel_info(struct panel_extend_para *info)
{
	u32 i = 0, j = 0;
	u32 items;
	u8 lcd_gamma_tbl[][2] = {
		/* {input value, corrected value} */
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
		u32 num = lcd_gamma_tbl[i + 1][0] - lcd_gamma_tbl[i][0];

		for (j = 0; j < num; j++) {
			u32 value = 0;

			value =
			    lcd_gamma_tbl[i][1] +
			    ((lcd_gamma_tbl[i + 1][1] -
			      lcd_gamma_tbl[i][1]) * j) / num;
			info->lcd_gamma_tbl[lcd_gamma_tbl[i][0] + j] =
			    (value << 16) + (value << 8) + value;
		}
	}
	info->lcd_gamma_tbl[255] =
	    (lcd_gamma_tbl[items - 1][1] << 16) +
	    (lcd_gamma_tbl[items - 1][1] << 8) + lcd_gamma_tbl[items - 1][1];

	memcpy(info->lcd_cmap_tbl, lcd_cmap_tbl, sizeof(lcd_cmap_tbl));

}

/*
* @sel: lcd id
*/
static s32 lcd_open_flow(u32 sel)
{
	int id;

	/* open lcd power, and delay 30ms */
	lcd_power_on(sel);
	msleep(30);

	lcd_panel_init(sel);
	msleep(50);

	/* open lcd controller, and delay 100ms */
	id = sunxi_lcd_get_tcon_id(sel);
	if (id < 0) {
		DRM_ERROR("get tcon id failed\n");
		return -1;
	}

	if (sunxi_tcon_lcd_enable(id, sel)) {
		DRM_ERROR("sunxi_tcon_lcd_enable failed\n");
		return -1;

	}

	msleep(100);
	/* open lcd backlight, and delay 0ms */
	lcd_bl_open(sel);

	return 0;
}

static s32 lcd_sw_open_flow(u32 sel)
{
	/* open lcd power, and delay 30ms */
	lcd_power_on(sel);

	lcd_panel_init(sel);

	/* open lcd backlight, and delay 0ms */
	lcd_bl_open(sel);

	return 0;
}

static s32 lcd_close_flow(u32 sel)
{
	int id;

	/* close lcd backlight, and delay 0ms */
	lcd_bl_close(sel);

	/* close lcd controller, and delay 0ms */
	id = sunxi_lcd_get_tcon_id(sel);
	if (id < 0) {
		DRM_ERROR("get tcon id failed\n");
		return -1;
	}

	if (sunxi_tcon_lcd_disable(id, sel) < 0) {
		DRM_ERROR("sunxi_tcon_lcd_disable failed\n");
		return -1;

	}

	lcd_panel_exit(sel);

	/* close lcd power, and delay 500ms */
	lcd_power_off(sel);

	return 0;
}

static void lcd_power_on(u32 sel)
{
	/* config lcd_power pin to open lcd power0 */
	sunxi_lcd_power_enable(sel, 0);
	sunxi_lcd_pin_cfg(sel, 1);
}

static void lcd_power_off(u32 sel)
{
	sunxi_lcd_pin_cfg(sel, 0);
	/* config lcd_power pin to close lcd power0 */
	sunxi_lcd_power_disable(sel, 0);
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
}

static void lcd_panel_exit(u32 sel)
{
}

/* sel: 0:lcd0; 1:lcd1 */
static s32 lcd_user_defined_func(u32 sel, u32 para1, u32 para2, u32 para3)
{
	return 0;
}

struct __lcd_panel default_panel = {
	/* panel driver name, must mach the lcd_drv_name in sys_config.fex */
	.name = "default_lcd",
	.func = {
		 .cfg_panel_info = lcd_cfg_panel_info,
		 .cfg_open_flow = lcd_open_flow,
		 .cfg_close_flow = lcd_close_flow,
		 .lcd_user_defined_func = lcd_user_defined_func,
	}
	,
};
