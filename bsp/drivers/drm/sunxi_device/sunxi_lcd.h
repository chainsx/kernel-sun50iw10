/*sunxi_lcd.h
 *
 * Copyright (C) 2022 Allwinnertech Co., Ltd.
 * Authors: zhengwanyu <zhengwanyu@allwinnertech.com>
 * Authors: hongyaobin <hongyaobin@allwinnertech.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef _SUNXI_LCD_H_
#define _SUNXI_LCD_H_

#include <linux/clk.h>
#include "de/include.h"
#include "sunxi_common.h"
#include "de/disp_lcd.h"
#include "lcd_panel/panels.h"
#include "de_dsi.h"

#define LCD_NUM_MAX      4
#define DRM_DEBUG 0
/* NOTE:
 * if sunxi lcd driver should support more than 3 lcd at the same time,
 * this enum should be extended
 */
enum sunxi_lcd_connector {
	SUNXI_LVDS0 = 0,
	SUNXI_LVDS1,

	SUNXI_RGB0,
	SUNXI_RGB1,

	SUNXI_CPU0,
	SUNXI_CPU1,

	SUNXI_DSI0,
	SUNXI_DSI1,

	SUNXI_VDPO0,
	SUNXI_VDPO1,

	SUNXI_LCD_CON_NUM,
};

/* RGB */
enum hv_if_type {
	PARALLEL_RGB = 0,
	SERIAL_RGB = 8,
	DUMMY_RGB = 10,
	RGB_DUMMY = 11,
	SERIAL_YUV = 12,/* CCIR656 */
};

/* CPU */
enum cpu_if_type {
	RGB666_18_1 = 0, /* 18bit/1cycle */
	RGB565_16_1 = 4,
	RGB666_18_3 = 6,
	RGB565_16_2 = 8,
	RGB666_9_1 = 10,
	RGB666_8_3 = 12,
	RGB565_8_2 = 14,
};

enum lvds_if_type {
	SINGLE_LINK = 0,
	DUAL_LINK = 1,
};

/* dsi working mode */
enum dsi_if_type {
	VIDEO_MODE = 0,
	COMMAND_MODE = 1,
	VIDEO_BURST_MODE = 2,
};

struct sunxi_lcd_resource {
	uintptr_t	reg_base[SUNXI_LCD_CON_NUM];
	struct clk	*mclk[SUNXI_LCD_CON_NUM];
	struct clk      *mclk_bus[SUNXI_LCD_CON_NUM];
	unsigned int	irq_no[SUNXI_LCD_CON_NUM];
	struct reset_control *rst_bus_lvds;

	/* think more about it is suitable to place this clk resource here */
#if IS_ENABLED(CONFIG_AW_DRM_LCD_DSI)
	struct clk *clk_mipi_dsi[CLK_DSI_NUM];
	struct clk *clk_bus_mipi_dsi[CLK_DSI_NUM];
	struct reset_control *rst_bus_mipi_dsi[DEVICE_DSI_NUM];
#endif
};

struct sunxi_lcd_funcs {
	unsigned int (*get_type)(unsigned int lcd_id);
	struct disp_panel_para *(*get_panel_para)(unsigned int lcd_id);
	bool (*is_use_irq)(unsigned int lcd_id);
	unsigned int (*get_irq_no)(unsigned int lcd_id);
	int (*enable)(unsigned int lcd_id);
	int (*sw_enable)(unsigned int lcd_id);
	void (*disable)(unsigned int lcd_id);
};

struct sunxi_lcd {
	struct platform_device *pdev;

	int			id;
	bool			is_used;
	bool			is_enabled;

	/* register base */
	uintptr_t		reg_base;

	enum disp_lcd_if	type;

	/*
	 * for example, typde_id = 2, type = LCD_IF_LVDS,
	 * it means that this sunxi_lcd is LVDS2
	 */
	unsigned int		type_id;

	/* clock resource */
	struct clk		*mclk;/* module clk */
	struct clk		*mclk_bus;/* module bus clk */
	struct clk		*parent_clk;

	/* interrupt resource */
	bool			irq_used;
	bool			irq_enabled;
	unsigned int		irq_no;

	struct disp_lcd_cfg	lcd_cfg;
	struct panel_extend_para extend_para;

	/* this member indicate the info of a lcd panel,
	NOT info of lcd connector */
	struct disp_panel_para	panel_para;
	struct __lcd_panel	*panel;

	/* indicate if this lcd use backlight */
	unsigned char		use_bl;

	struct sunxi_lcd_funcs *funcs;
};

struct sunxi_lcd_drv {
	/* linux system relative */
	struct drv_model_info		drv_model;

	/* collect all of lcd resources, including reg_base/irq/clk,
	* and then dispatch them to the relative lcd connectors
	* after lcd0/1/2/3 init.
	* we need to do that, because we only know there is lcd0/1/2/3,
	* but we don't know who is DSI, who is lvds, who is cpu,
	* before lcd0/1/2/3 init. So we collect all of lcd resources here and
	* dispatch them to the LCD connector they belongs to after lcd0/1/2/3
	* init
	*/
	struct sunxi_lcd_resource	res;

	unsigned int			lcd_cnt;
	unsigned int			lvds_cnt;
	unsigned int			rgb_cnt;
	unsigned int			cpu_cnt;
	unsigned int			dsi_cnt;
	unsigned int			edp_cnt;
	struct sunxi_lcd		hwlcd[LCD_NUM_MAX];
};

int sunxi_lcd_gpio_set_value(unsigned int lcd_id, unsigned int io_index,
					unsigned int data);
int sunxi_lcd_gpio_set_direction(struct sunxi_lcd *lcd, unsigned int io_index,
				unsigned int direction);
int sunxi_lcd_pin_cfg(unsigned int lcd_id, unsigned char enable);
int sunxi_lcd_power_enable(unsigned int lcd_id, u32 power_id);
int sunxi_lcd_power_disable(unsigned int lcd_id, u32 power_id);
unsigned int sunxi_lcd_get_tcon_id(unsigned int lcd_id);
unsigned int sunxi_lcd_get_type_id(unsigned int lcd_id);
struct panel_extend_para *sunxi_lcd_get_panel_ext_para(unsigned int lcd_id);


unsigned int sunxi_lcd_get_type(unsigned int lcd_id);
struct disp_panel_para *sunxi_lcd_get_panel_para(unsigned int lcd_id);


unsigned int sunxi_lcd_get_dclk(unsigned int lcd_id);
struct sunxi_lcd *sunxi_lcd_get_lcd(unsigned int lcd_id);
unsigned int sunxi_lcd_get_lcd_if(unsigned int lcd_id);
unsigned int sunxi_lcd_get_usec_per_line(unsigned int lcd_id);


bool sunxi_lcd_is_use_irq(unsigned int lcd_id);
unsigned int sunxi_lcd_get_irq_no(unsigned int lcd_id);

struct sunxi_lcd_funcs *sunxi_lcd_get_hw_funcs(int lcd_id);

int sunxi_lcd_module_init(void);
void sunxi_lcd_module_exit(void);

#ifndef CONFIG_AW_DRM_BACKLIGHT
static inline int sunxi_backlight_enable(unsigned int lcd_id) { return 0; }

static inline int sunxi_backlight_disable(unsigned int lcd_id) {	return 0; }
#endif

#endif
