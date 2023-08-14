/*sunxi_lcd.c
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

#include <linux/dma-mapping.h>
#include <linux/clk.h>
#include <drm/drm_print.h>

#include "sunxi_common.h"
#include "sunxi_tcon.h"
#include "sunxi_lcd.h"

#if defined(CONFIG_AW_DRM_BACKLIGHT)
#include "sunxi_backlight.h"
#endif

#define LCD_PIN_STATE_ACTIVE "active"
#define LCD_PIN_STATE_SLEEP "sleep"

static struct sunxi_lcd_drv *lcd_drv;
static struct device *g_lcd_dev; /* fix me */

unsigned char sunxi_lcd_get_count(void)
{
	return lcd_drv->lcd_cnt;
}

struct sunxi_dispdev_name sunxi_lcd_connector_type_name[] = {
	{LCD_IF_HV,	"RGB"},
	{LCD_IF_CPU,	"CPU"},
	{LCD_IF_LVDS,	"LVDS"},
	{LCD_IF_DSI,	"DSI"},
	{LCD_IF_EDP,	"EDP"},
	{LCD_IF_EXT_DSI, "EXT_DSI"},
	{LCD_IF_VDPO,	"VDPO"},
};

/* RGB */
struct sunxi_dispdev_name sunxi_hv_if_name[] = {
	{PARALLEL_RGB,	"PARALLEL_RGB"},
	{SERIAL_RGB,	"SERIAL_RGB"},
	{DUMMY_RGB,	"DUMMY_RGB"},
	{RGB_DUMMY,	"RGB_DUMMY"},
	{SERIAL_YUV,	"SERIAL_YUV"},
};

/* CPU */
struct sunxi_dispdev_name sunxi_cpu_if_name[] = {
	{RGB666_18_1,	"RGB666_18_1"},
	{RGB565_16_1,	"RGB565_16_1"},
	{RGB666_18_3,	"RGB666_18_3"},
	{RGB565_16_2,	"RGB565_16_2"},
	{RGB666_9_1,	"RGB666_9_1"},
	{RGB666_8_3,	"RGB666_8_3"},
	{RGB565_8_2,	"RGB565_8_2"},
};

struct sunxi_dispdev_name sunxi_lvds_if_name[] = {
	{SINGLE_LINK,	"SINGLE_LINK"},
	{DUAL_LINK,	"DUAL_LINK"},
};

struct sunxi_dispdev_name sunxi_dsi_if_name[] = {
	{VIDEO_MODE,		"VIDEO_MODE"},
	{COMMAND_MODE,		"COMMAND_MODE"},
	{VIDEO_BURST_MODE,	"VIDEO_BURST_MODE"},
};

struct __lcd_panel *sunxi_panel[] = {
#ifdef CONFIG_EINK_PANEL_USED
	&default_eink,
#endif
#ifdef CONFIG_LCD_SUPPORT_DEFAULT
	&default_panel,
#endif
#ifdef CONFIG_LCD_SUPPORT_LT070ME05000
	&lt070me05000_panel,
#endif
#ifdef CONFIG_LCD_SUPPORT_WTQ05027D01
	&wtq05027d01_panel,
#endif
#ifdef CONFIG_LCD_SUPPORT_T27P06
	&t27p06_panel,
#endif
#ifdef CONFIG_LCD_SUPPORT_DX0960BE40A1
	&dx0960be40a1_panel,
#endif
#ifdef CONFIG_LCD_SUPPORT_TFT720X1280
	&tft720x1280_panel,
#endif
#ifdef CONFIG_LCD_SUPPORT_S6D7AA0X01
	&S6D7AA0X01_panel,
#endif
#ifdef CONFIG_LCD_SUPPORT_GG1P4062UTSW
	&gg1p4062utsw_panel,
#endif
#ifdef CONFIG_LCD_SUPPORT_LS029B3SX02
	&ls029b3sx02_panel,
#endif
#ifdef CONFIG_LCD_SUPPORT_HE0801A068
	&he0801a068_panel,
#endif
#ifdef CONFIG_LCD_SUPPORT_INET_DSI_PANEL
	&inet_dsi_panel,
#endif
#ifdef CONFIG_LCD_SUPPORT_LQ101R1SX03
	&lq101r1sx03_panel,
#endif
	/* add new panel below */
#ifdef CONFIG_LCD_SUPPORT_WILLIAMLCD
	&WilliamLcd_panel,
#endif
#ifdef CONFIG_LCD_SUPPORT_KD101N51
	&kd101n51_panel,
#endif
#ifdef CONFIG_LCD_SUPPORT_BP101WX1
	&bp101wx1_panel,
#endif
#ifdef CONFIG_LCD_SUPPORT_JC0WS0081CM
	&jc0ws008lcm_panel,
#endif

};

static char *sunxi_lcd_get_connector_type_name(int type)
{
	int i, cnt;

	cnt  = sizeof(sunxi_lcd_connector_type_name)
			/ sizeof(struct sunxi_dispdev_name);

	for (i = 0; i < cnt; i++)
		if (sunxi_lcd_connector_type_name[i].device == type)
			return sunxi_lcd_connector_type_name[i].name;

	return NULL;
}

/* RGB */
static char *sunxi_lcd_get_hv_if_name(int type)
{
	int i, cnt;

	cnt  = sizeof(sunxi_hv_if_name)
			/ sizeof(struct sunxi_dispdev_name);

	for (i = 0; i < cnt; i++)
		if (sunxi_hv_if_name[i].device == type)
			return sunxi_hv_if_name[i].name;

	return NULL;
}

static char *sunxi_lcd_get_cpu_if_name(int type)
{
	int i, cnt;

	cnt  = sizeof(sunxi_cpu_if_name)
			/ sizeof(struct sunxi_dispdev_name);

	for (i = 0; i < cnt; i++)
		if (sunxi_cpu_if_name[i].device == type)
			return sunxi_cpu_if_name[i].name;

	return NULL;
}

static char *sunxi_lcd_get_lvds_if_name(int type)
{
	int i, cnt;

	cnt  = sizeof(sunxi_lvds_if_name)
			/ sizeof(struct sunxi_dispdev_name);

	for (i = 0; i < cnt; i++)
		if (sunxi_lvds_if_name[i].device == type)
			return sunxi_lvds_if_name[i].name;

	return NULL;
}

static char *sunxi_lcd_get_dsi_if_name(int type)
{
	int i, cnt;

	cnt  = sizeof(sunxi_dsi_if_name)
			/ sizeof(struct sunxi_dispdev_name);

	for (i = 0; i < cnt; i++)
		if (sunxi_dsi_if_name[i].device == type)
			return sunxi_dsi_if_name[i].name;

	return NULL;
}

static struct __lcd_panel *sunxi_lcd_get_panel(char *name)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sunxi_panel); i++)
		if (sunxi_panel[i] && (!strcmp(name, sunxi_panel[i]->name)))
			return sunxi_panel[i];

	return NULL;
}

/**
 * @name       :sunxi_dsi_io_cfg
 * @brief      :open dsi io
 * @param[IN]  :type_id:dsi index
 * @param[OUT] :enable:1->en, 0->disable
 * @return     :always 0
 */
static int sunxi_dsi_io_cfg(int lcd_id, unsigned int enable)
{
	int type_id = -1;
	struct disp_panel_para *panel = NULL;

	type_id = sunxi_lcd_get_type_id(lcd_id);
	panel = sunxi_lcd_get_panel_para(lcd_id);

	if (!panel || type_id < 0) {
		DRM_ERROR("NULL pointer!\n");
		return -1;
	}

	if (enable == 1) {
#if defined(SUPPORT_DSI)
		dsi_io_open(type_id, panel);
		if (panel->lcd_tcon_mode == DISP_TCON_DUAL_DSI &&
		    type_id + 1 < DEVICE_DSI_NUM)
			dsi_io_open(type_id + 1, panel);
#endif
	} else {
#if defined(SUPPORT_DSI)
		dsi_io_close(type_id);
		if (panel->lcd_tcon_mode == DISP_TCON_DUAL_DSI &&
		    type_id + 1 < DEVICE_DSI_NUM)
			dsi_io_close(type_id + 1);
#endif
	}
	return 0;
}

/**
 * @name       :sunxi_lcd_get_lcd
 * @brief      :get lcd structure of specified lcd id
 * @param[IN]  :lcd_id: id of lcd
 * @return     :a pointer of sunxi_lcd or NULL if fail to get one
 */
struct sunxi_lcd *sunxi_lcd_get_lcd(unsigned int lcd_id)
{
	struct sunxi_lcd *lcd = NULL;

	if (lcd_id < LCD_NUM_MAX)
		lcd = &lcd_drv->hwlcd[lcd_id];

	return lcd;
}

struct sunxi_lcd_funcs *sunxi_lcd_get_hw_funcs(int lcd_id)
{
	struct sunxi_lcd *lcd;

	lcd = sunxi_lcd_get_lcd(lcd_id);

	return lcd->funcs;
}


/* pinctrl and pin power */
/*
 * pin power
 * pin set state: Active or Sleep
 * explain: pin_cfg belongs to pinctrl that configured int DTS.
 * pin_power: means that power conresponse to the pinctrl configured int DTS.
 */
int sunxi_lcd_pin_cfg(unsigned int lcd_id, unsigned char enable)
{
	int i, ret = 0;
	struct disp_lcd_cfg *cfg;
	struct sunxi_lcd *lcd;
	char dev_name[25];

	lcd = sunxi_lcd_get_lcd(lcd_id);
	if (!lcd) {
		DRM_ERROR("Null lcd pointer!\n");
		return -1;
	}
	cfg = &lcd->lcd_cfg;

	/* set pin power enable */
	if (enable) {
		for (i = 0; i < LCD_GPIO_REGU_NUM; i++) {
			if (cfg->lcd_power_used[i]) {
				ret = sunxi_drm_sys_power_enable(g_lcd_dev,
					cfg->lcd_pin_power[i]);
				if (ret) {
					DRM_ERROR("sunxi_lcd_pin_cfg"
					" sunxi_drm_sys_power_enable failed");
				}
			}
		}
	}

	sprintf(dev_name, "lcd%d", lcd_id);
	ret = sunxi_drm_sys_pin_set_state(dev_name, enable ?
				LCD_PIN_STATE_ACTIVE : LCD_PIN_STATE_SLEEP);
	if (ret) {
		DRM_ERROR("sunxi_drm_sys_pin_set_state failed\n");
		return ret;
	}

	if (lcd->type == LCD_IF_DSI)
		sunxi_dsi_io_cfg(lcd_id, enable);


	/* set pin power disable */
	if (!enable) {
		for (i = 0; i < LCD_GPIO_REGU_NUM; i++) {
			if (cfg->lcd_power_used[i]) {
				ret = sunxi_drm_sys_power_disable(g_lcd_dev,
					cfg->lcd_pin_power[i]);
				if (ret) {
					DRM_ERROR("sunxi_lcd_pin_cfg"
					" sunxi_drm_sys_power_disable failed");
					return -1;
				}
			}
		}
	}

	return 0;
}


/* lcd power */
int sunxi_lcd_all_power_enable(unsigned int lcd_id)
{
	int i, ret;
	struct sunxi_lcd *lcd;
	char *power;

	lcd = sunxi_lcd_get_lcd(lcd_id);
	if (!lcd) {
		DRM_ERROR("Null lcd pointer!\n");
		return -1;
	}

	power = lcd->lcd_cfg.lcd_bl_en_power;
	if (strcmp(power, "")) {
		ret = sunxi_drm_sys_power_enable(g_lcd_dev, power);
		if (ret) {
			DRM_ERROR("disp_lcd_power_enable:%s failed\n", power);
			return ret;
		}
	}

	for (i = 0; i < LCD_POWER_NUM; i++) {
		if (!lcd->lcd_cfg.lcd_power_used[i])
			continue;
		power = lcd->lcd_cfg.lcd_power[i];
		ret = sunxi_drm_sys_power_enable(g_lcd_dev, power);
		if (ret) {
			DRM_ERROR("disp_lcd_power_enable:%s failed\n", power);
			return ret;
		}
	}

	for (i = 0; i < LCD_POWER_NUM; i++) {
		if (!lcd->lcd_cfg.lcd_fix_power_used[i])
			continue;
		power = lcd->lcd_cfg.lcd_fix_power[i];
		ret = sunxi_drm_sys_power_enable(g_lcd_dev, power);
		if (ret) {
			DRM_ERROR("disp_lcd_power_enable:%s failed\n", power);
			return ret;
		}
	}

	for (i = 0; i < LCD_GPIO_REGU_NUM; i++) {
		if (!lcd->lcd_cfg.lcd_power_used[i])
			continue;
		power = lcd->lcd_cfg.lcd_gpio_power[i];
		ret = sunxi_drm_sys_power_enable(g_lcd_dev, power);
		if (ret) {
			DRM_ERROR("disp_lcd_power_enable:%s failed\n", power);
			return ret;
		}
	}

	for (i = 0; i < LCD_GPIO_REGU_NUM; i++) {
		if (!lcd->lcd_cfg.lcd_power_used[i])
			continue;
		power = lcd->lcd_cfg.lcd_pin_power[i];
		ret = sunxi_drm_sys_power_enable(g_lcd_dev, power);
		if (ret) {
			DRM_ERROR("disp_lcd_power_enable:%s failed\n", power);
			return ret;
		}
	}

	return 0;
}

/*
 * The power effect on the whole lcd
 */
int sunxi_lcd_power_enable(unsigned int lcd_id, u32 power_id)
{
	int ret = 0;
	struct sunxi_lcd *lcd;

	lcd = sunxi_lcd_get_lcd(lcd_id);
	if (!lcd) {
		DRM_ERROR("Null lcd pointer!\n");
		return -1;
	}

	if (lcd->lcd_cfg.lcd_power_used[power_id]) {
		/* regulator type */
		ret = sunxi_drm_sys_power_enable(g_lcd_dev, lcd->lcd_cfg.lcd_power[power_id]);
		if (ret) {
			DRM_ERROR("disp_lcd_power_enable"
				" sunxi_drm_sys_power_disable failed");
			return -1;
		}
	}

	return 0;
}

int sunxi_lcd_power_disable(unsigned int lcd_id, u32 power_id)
{
	int ret = 0;
	struct sunxi_lcd *lcd;

	lcd = sunxi_lcd_get_lcd(lcd_id);
	if (!lcd) {
		DRM_ERROR("Null lcd pointer!\n");
		return -1;
	}
	if (lcd->lcd_cfg.lcd_power_used[power_id]) {
		/* regulator type */
		ret = sunxi_drm_sys_power_disable(g_lcd_dev, lcd->lcd_cfg.lcd_power[power_id]);
		if (ret) {
			DRM_ERROR("disp_lcd_power_enable"
				" sunxi_drm_sys_power_disable failed");
			return -1;
		}
	}


	return 0;
}

/* lcd power */

int sunxi_lcd_fix_power_enable(struct sunxi_lcd *lcd)
{
	int ret = 0, i;

	for (i = 0; i < LCD_POWER_NUM; i++) {
		if (lcd->lcd_cfg.lcd_fix_power_used[i]) {
			/* regulator type */
			ret = sunxi_drm_sys_power_enable(g_lcd_dev, lcd->lcd_cfg.lcd_fix_power[i]);
			if (ret) {
				DRM_ERROR("disp_lcd_power_enable"
					" sunxi_drm_sys_power_disable failed");
				return -1;
			}
		}
	}

	return 0;
}

int sunxi_lcd_fix_power_disable(struct sunxi_lcd *lcd)
{
	int ret = 0, i;

	for (i = 0; i < LCD_POWER_NUM; i++) {
		if (lcd->lcd_cfg.lcd_fix_power_used[i]) {
			/* regulator type */
			ret = sunxi_drm_sys_power_disable(g_lcd_dev, lcd->lcd_cfg.lcd_fix_power[i]);
			if (ret) {
				DRM_ERROR("disp_lcd_power_enable"
					" sunxi_drm_sys_power_disable failed");
				return -1;
			}
		}
	}


	return 0;
}


/* lcd_gpio and lcd_gpio_power */
/*
 *lcd_gpio: the general gpio that used in whole lcd
 *lcd_gpio_power: the coresponsible power of lcd_gpio
 */
static int sunxi_lcd_gpio_power_enable(struct sunxi_lcd *lcd, u32 power_id)
{
	int ret = 0;

	if (lcd->lcd_cfg.lcd_power_used[power_id]) {
		/* regulator type */
		ret = sunxi_drm_sys_power_enable(g_lcd_dev, lcd->lcd_cfg.lcd_gpio_power[power_id]);
		if (ret) {
			DRM_ERROR("disp_lcd_power_enable"
				" sunxi_drm_sys_power_disable failed");
			return -1;
		}
	}

	return 0;
}

static int sunxi_lcd_gpio_power_disable(struct sunxi_lcd *lcd, u32 power_id)
{
	int ret = 0;

	if (lcd->lcd_cfg.lcd_power_used[power_id]) {
		/* regulator type */
		ret = sunxi_drm_sys_power_disable(g_lcd_dev, lcd->lcd_cfg.lcd_gpio_power[power_id]);
		if (ret) {
			DRM_ERROR("disp_lcd_power_enable"
				" sunxi_drm_sys_power_disable failed");
			return -1;
		}
	}


	return 0;
}

/**
 * enable lcd_gpio power and set it as sys_config set
 * lcd_gpio: the gpio effect on whole lcd, has no speciffic function, like bl_en
 * gpio
 */
int sunxi_lcd_gpio_init(struct sunxi_lcd *lcd)
{
	int i;
	struct disp_lcd_cfg *lcd_cfg = &lcd->lcd_cfg;

	for (i = 0; i < LCD_GPIO_REGU_NUM; i++) {
		if (sunxi_lcd_gpio_power_enable(lcd, i) < 0) {
			DRM_ERROR("sunxi_lcd_power_enable failed\n");
		}
	}

	for (i = 0; i < LCD_GPIO_NUM; i++) {
		lcd_cfg->gpio_hdl[i] = 0;
		if (lcd_cfg->lcd_gpio_used[i]) {
			lcd_cfg->gpio_hdl[i]
				= sunxi_drm_sys_gpio_request((struct disp_gpio_info *)&lcd_cfg->lcd_gpio[i]);
			if (lcd_cfg->gpio_hdl[i] < 0) {
				DRM_ERROR("%s"
				" sunxi_drm_sys_gpio_request failed", __func__);
			}
		}
	}

	return 0;
}

int sunxi_lcd_gpio_exit(struct sunxi_lcd *lcd)
{
	int i;
	struct disp_gpio_info gpio_info;
	struct disp_lcd_cfg *lcd_cfg = &lcd->lcd_cfg;

	for (i = 0; i < LCD_GPIO_NUM; i++) {
		if (lcd_cfg->gpio_hdl[i]) {
			sunxi_drm_sys_gpio_release(lcd_cfg->gpio_hdl[i]);

			memcpy(&gpio_info, &(lcd_cfg->lcd_gpio[i]),
			       sizeof(gpio_info));

			lcd_cfg->gpio_hdl[i] = sunxi_drm_sys_gpio_request(&gpio_info);
			if (lcd_cfg->gpio_hdl[i] < 0) {
				DRM_ERROR("sunxi_drm_sys_gpio_request failed\n");
			}
			sunxi_drm_sys_gpio_release(lcd_cfg->gpio_hdl[i]);
			lcd_cfg->gpio_hdl[i] = 0;
		}
	}

	/* io-pad */
	for (i = LCD_GPIO_REGU_NUM - 1; i >= 0; i--) {
		if (sunxi_lcd_gpio_power_disable(lcd, i) < 0) {
			DRM_ERROR("sunxi_lcd_power_disable failed\n");
		}
	}

	return 0;
}

/* direction: input(0), output(1) */
int sunxi_lcd_gpio_set_direction(struct sunxi_lcd *lcd, unsigned int io_index,
				unsigned int direction)
{
	char gpio_name[20];

	sprintf(gpio_name, "lcd_gpio_%d", io_index);
	return sunxi_drm_sys_gpio_set_direction(lcd->lcd_cfg.gpio_hdl[io_index],
					   direction, gpio_name);
}

int sunxi_lcd_gpio_set_value(unsigned int lcd_id, unsigned int io_index,
					unsigned int data)
{
	char gpio_name[20];
	struct sunxi_lcd *lcd;

	lcd = sunxi_lcd_get_lcd(lcd_id);
	if (!lcd) {
		DRM_ERROR("Null lcd pointer!\n");
		return -1;
	}

	if (io_index >= LCD_GPIO_NUM) {
		DRM_ERROR("gpio num out of range\n");
		return -1;
	}

	sprintf(gpio_name, "lcd_gpio_%d", io_index);

	return sunxi_drm_sys_gpio_set_value(lcd->lcd_cfg.lcd_gpio[io_index].gpio/* lcd->lcd_cfg.gpio_hdl[io_index] */,
							data, gpio_name);
}

unsigned int sunxi_lcd_get_tcon_id(unsigned int lcd_id)
{
	int tcon_id = sunxi_tcon_lcd_get_tcon_id(lcd_id);

	if (tcon_id < 0) {
		DRM_ERROR("\n");
		return -1;
	}

	return tcon_id;
}

unsigned int sunxi_lcd_get_type_id(unsigned int lcd_id)
{
	struct sunxi_lcd *lcd;

	lcd = sunxi_lcd_get_lcd(lcd_id);
	if (!lcd) {
		DRM_ERROR("Null lcd pointer!\n");
		return 0;
	}

	return lcd->type_id;
}

unsigned int sunxi_lcd_get_type(unsigned int lcd_id)
{
	struct sunxi_lcd *lcd;

	lcd = sunxi_lcd_get_lcd(lcd_id);
	if (!lcd) {
		DRM_ERROR("Null lcd pointer!\n");
		return 0;
	}

	return lcd->type;
}

struct disp_panel_para *sunxi_lcd_get_panel_para(unsigned int lcd_id)
{
	struct sunxi_lcd *lcd;

	lcd = sunxi_lcd_get_lcd(lcd_id);
	if (!lcd) {
		DRM_ERROR("Null lcd pointer!\n");
		return 0;
	}

	return &lcd->panel_para;
}

struct panel_extend_para *sunxi_lcd_get_panel_ext_para(unsigned int lcd_id)
{
	struct sunxi_lcd *lcd;

	lcd = sunxi_lcd_get_lcd(lcd_id);
	if (!lcd) {
		DRM_ERROR("Null lcd pointer!\n");
		return NULL;
	}

	return &lcd->extend_para;
}

struct disp_lcd_panel_fun *sunxi_lcd_get_panel_func(unsigned int lcd_id)
{
	struct sunxi_lcd *lcd;

	lcd = sunxi_lcd_get_lcd(lcd_id);
	if (!lcd) {
		DRM_ERROR("Null lcd pointer!\n");
		return NULL;
	}
	if (!lcd->panel) {
		DRM_ERROR("lcd%d has NO panel\n", lcd_id);
		return NULL;
	}

	return &lcd->panel->func;
}

/*
 * @return: MHz
 */
unsigned int sunxi_lcd_get_dclk(unsigned int lcd_id)
{
	struct disp_panel_para *para = sunxi_lcd_get_panel_para(lcd_id);

	return para->lcd_dclk_freq;
}

unsigned int sunxi_lcd_get_lcd_if(unsigned int lcd_id)
{
	struct disp_panel_para *para = sunxi_lcd_get_panel_para(lcd_id);

	return para->lcd_if;
}

unsigned int sunxi_lcd_get_usec_per_line(unsigned int lcd_id)
{
	unsigned int usec_per_line;
	struct disp_panel_para *para = sunxi_lcd_get_panel_para(lcd_id);

	usec_per_line = para->lcd_ht / para->lcd_dclk_freq;

	return usec_per_line;
}

/**
 * @name       :sunxi_lcd_clk_config
 * @brief      :set lcd clk rate
 * @param[IN]  :lcd_id:lcd index
 * @return     :0 if success, -1 else
 * @TODO       :set dual dsi clk
 */
static int sunxi_lcd_clk_config(unsigned int lcd_id)
{
	int ret = -1;
	struct lcd_clk_info clk_info;
	struct disp_panel_para *p_panel = NULL;
	unsigned long dclk_rate = 33000000;
	unsigned long pll_rate = 297000000, lcd_rate = 33000000;
	unsigned long dsi_rate = 0;
	unsigned long dsi_rate_set = 0, pll_rate_set = 0;
	struct sunxi_lcd *lcd = NULL;
	struct clk *parent_clk = NULL;

	p_panel = sunxi_lcd_get_panel_para(lcd_id);

	lcd = sunxi_lcd_get_lcd(lcd_id);

	if (!lcd || !p_panel) {
		DRM_ERROR("Null lcd or panel pointer!\n");
		return -1;
	}

	/* no need to set clk rate */
	if (p_panel->lcd_if != LCD_IF_DSI)
		return 0;


	memset(&clk_info, 0, sizeof(clk_info));
	ret = sunxi_tcon_get_clk_info(lcd_id, &clk_info);
	if (ret) {
		DRM_WARN("Get clk_info fail!\n");
		return ret;
	}
	dclk_rate = ((unsigned long long)sunxi_lcd_get_dclk(lcd_id)) * 1000000;

	lcd_rate = dclk_rate * clk_info.dsi_div;
	pll_rate = lcd_rate * clk_info.lcd_div;
	dsi_rate = pll_rate / clk_info.dsi_div;
	pll_rate_set = pll_rate;
	parent_clk = clk_get_parent(lcd->mclk);
	if (parent_clk)
		pll_rate_set = clk_get_rate(parent_clk);

	if (p_panel->lcd_if == LCD_IF_DSI) {
		if (p_panel->lcd_dsi_if == LCD_DSI_IF_COMMAND_MODE)
			dsi_rate_set = pll_rate_set;
		else
			dsi_rate_set = pll_rate_set / clk_info.dsi_div;

		dsi_rate_set =
			(clk_info.dsi_rate == 0) ? dsi_rate_set : clk_info.dsi_rate;
		clk_set_rate(lcd->mclk, dsi_rate_set);
		dsi_rate_set = clk_get_rate(lcd->mclk);
		if (dsi_rate_set != dsi_rate)
			DRM_WARN("Dsi rate to be set:%lu, real clk rate:%lu\n", dsi_rate,
				 dsi_rate_set);
	}

	return ret;
}

bool sunxi_lcd_is_use_irq(unsigned int lcd_id)
{
	struct sunxi_lcd *lcd;

	lcd = sunxi_lcd_get_lcd(lcd_id);
	if (!lcd) {
		DRM_ERROR("Null lcd pointer!\n");
		return false;
	}

	return lcd->irq_used;
}

unsigned int sunxi_lcd_get_irq_no(unsigned int lcd_id)
{
	struct sunxi_lcd *lcd;

	lcd = sunxi_lcd_get_lcd(lcd_id);
	if (!lcd) {
		DRM_ERROR("Null lcd pointer!\n");
		return 0;
	}

	return lcd->irq_no;
}

static int sunxi_lcd_prepare(unsigned int lcd_id)
{
	int ret = 0, i = 0;
	struct sunxi_lcd *lcd;

	lcd = sunxi_lcd_get_lcd(lcd_id);
	if (!lcd) {
		DRM_ERROR("Null lcd pointer!\n");
		return -1;
	}

	if (lcd_drv->res.rst_bus_lvds) {
		ret = reset_control_deassert(lcd_drv->res.rst_bus_lvds);
		if (ret) {
			DRM_ERROR("reset_control_deassert for rst_bus_lvds failed!\n");
			return ret;
		}
	}

#if IS_ENABLED(CONFIG_AW_DRM_LCD_DSI)
	for (i = 0; i < CLK_DSI_NUM; i++) {
		ret = reset_control_deassert(lcd_drv->res.rst_bus_mipi_dsi[i]);
		if (ret) {
			DRM_ERROR("%s: reset_control_deassert for rst_bus_mipi_dsi[%d] failed\n", __func__, i);
			return ret;
		}

		ret = clk_prepare_enable(lcd_drv->res.clk_mipi_dsi[i]);
		if (ret) {
			DRM_ERROR("%s: clk_prepare_enable for clk_mipi_dsi[%d] failed\n", __func__, i);
			return ret;
		}

		ret = clk_prepare_enable(lcd_drv->res.clk_bus_mipi_dsi[i]);
		if (ret) {
			DRM_ERROR("%s: clk_prepare_enable for clk_bus_mipi_dsi[%d] failed\n", __func__, i);
			return ret;
		}
	}
#endif

	sunxi_lcd_clk_config(lcd_id);
	if (lcd->mclk) {
		ret = clk_prepare_enable(lcd->mclk);
		if (ret) {
			DRM_ERROR("clk_prepare_enable failed\n");
			return -1;
		}
	}

	if (lcd->mclk_bus) {
		ret = clk_prepare_enable(lcd->mclk_bus);
		if (ret) {
			DRM_ERROR("clk_prepare_enable failed\n");
			return -1;
		}
	}

	ret = sunxi_lcd_fix_power_enable(lcd);
	if (ret) {
		DRM_ERROR("sunxi_lcd_fix_power_enable failed\n");
		return -1;
	}

	ret = sunxi_lcd_gpio_init(lcd);
	if (ret) {
		DRM_ERROR("sunxi_lcd_gpio_init failed\n");
		return -1;
	}

	return 0;
}

static int sunxi_lcd_sw_prepare(unsigned int lcd_id)
{
	int ret = 0;
	struct sunxi_lcd *lcd;

	lcd = sunxi_lcd_get_lcd(lcd_id);
	if (!lcd) {
		DRM_ERROR("Null lcd pointer!\n");
		return -1;
	}

	if (lcd->mclk) {
		ret = clk_prepare_enable(lcd->mclk);
		if (ret) {
			DRM_ERROR("clk_prepare_enable failed\n");
			return -1;
		}
	}

	ret = sunxi_lcd_fix_power_enable(lcd);
	if (ret) {
		DRM_ERROR("sunxi_lcd_fix_power_enable failed\n");
		return -1;
	}

	ret = sunxi_lcd_gpio_init(lcd);
	if (ret) {
		DRM_ERROR("sunxi_lcd_gpio_init failed\n");
		return -1;
	}

	return 0;
}


static void sunxi_lcd_unprepare(unsigned int lcd_id)
{
	int ret = 0;
	struct sunxi_lcd *lcd;

	lcd = sunxi_lcd_get_lcd(lcd_id);
	if (!lcd) {
		DRM_ERROR("Null lcd pointer!\n");
		return;
	}

	ret = sunxi_lcd_gpio_exit(lcd);
	if (ret) {
		DRM_ERROR("sunxi_lcd_gpio_exit failed\n");
		return;
	}

	ret = sunxi_lcd_fix_power_disable(lcd);
	if (ret) {
		DRM_ERROR("sunxi_lcd_fix_power_disable failed\n");
		return;
	}

	if (lcd->mclk)
		clk_disable_unprepare(lcd->mclk);

}

int sunxi_lcd_sw_enable(unsigned int lcd_id)
{
	struct disp_lcd_panel_fun *panel_func = NULL;

	panel_func = sunxi_lcd_get_panel_func(lcd_id);

	sunxi_lcd_sw_prepare(lcd_id);

	if (!panel_func->cfg_open_flow) {
		DRM_ERROR("cfg_sw_open_flow hdl is NULL\n");
		return -1;
	}
	panel_func->cfg_open_flow(lcd_id);

	return 0;
}

static int sunxi_lcd_dsi_enable(unsigned int lcd_id)
{
	int ret;
	struct disp_panel_para *panel_para;
	struct disp_lcd_panel_fun *panel_func = NULL;
	struct sunxi_lcd *lcd = sunxi_lcd_get_lcd(lcd_id);

	ret = sunxi_lcd_prepare(lcd_id);
	if (ret < 0) {
		DRM_ERROR("sunxi_lcd:%d prepare fialed\n", lcd_id);
		return -1;
	}

	panel_para = sunxi_lcd_get_panel_para(lcd_id);
	panel_func = sunxi_lcd_get_panel_func(lcd_id);
	if (!panel_para) {
		DRM_ERROR("lcd_id:%d panel_para is NULL\n", lcd_id);
		return -1;
	}
#if defined(SUPPORT_DSI)
	if (dsi_cfg(lcd->type_id, panel_para) != 0) {
		DRM_ERROR("dsi_cfg failed\n");
		return -1;
	}
#endif
	if (!panel_func) {
		DRM_ERROR("GET panal_func Failed!\n");
		return -1;
	}

	if (!panel_func->cfg_open_flow) {
		DRM_ERROR("cfg_open_flow hdl is NULL\n");
		return -1;
	}
	panel_func->cfg_open_flow(lcd_id);

	return 0;
}

static void sunxi_lcd_dsi_disable(unsigned int lcd_id)
{
	struct disp_lcd_panel_fun *panel_func = NULL;

	panel_func = sunxi_lcd_get_panel_func(lcd_id);
	if (!panel_func) {
		DRM_ERROR("GET panal_func Failed!\n");
		return;
	}

	if (!panel_func->cfg_close_flow) {
		DRM_ERROR("cfg_open_close hdl is NULL\n");
		return;
	}
	panel_func->cfg_close_flow(lcd_id);
#if defined(SUPPORT_DSI)
	if (dsi_exit(lcd_id) != 0) {
		DRM_ERROR("dsi_exit failed\n");
		return;
	}
#endif
	sunxi_lcd_unprepare(lcd_id);

}

static int sunxi_lcd_hv_enable(unsigned int lcd_id)
{
	int ret;
	struct disp_lcd_panel_fun *panel_func = NULL;

	/* set lcd output */
	ret = sunxi_lcd_prepare(lcd_id);
	if (ret < 0) {
		DRM_ERROR("sunxi_lcd:%d prepare fialed\n", lcd_id);
		return -1;
	}

	panel_func = sunxi_lcd_get_panel_func(lcd_id);
	if (!panel_func) {
		DRM_ERROR("GET panal_func Failed!\n");
		return -1;
	}

	if (!panel_func->cfg_open_flow) {
		DRM_ERROR("cfg_open_flow hdl is NULL\n");
		return -1;
	}

	panel_func->cfg_open_flow(lcd_id);

	return 0;

}

static void sunxi_lcd_hv_disable(unsigned int lcd_id)
{
	struct disp_lcd_panel_fun *panel_func = NULL;

	panel_func = sunxi_lcd_get_panel_func(lcd_id);
	if (!panel_func) {
		DRM_ERROR("GET panal_func Failed!\n");
		return;
	}

	if (!panel_func->cfg_close_flow) {
		DRM_ERROR("cfg_open_close hdl is NULL\n");
		return;
	}
	panel_func->cfg_close_flow(lcd_id);

	sunxi_lcd_unprepare(lcd_id);
}

static int sunxi_lcd_lvds_enable(unsigned int lcd_id)
{
	int ret;
	struct disp_lcd_panel_fun *panel_func = NULL;

	/* set lcd output */
	ret = sunxi_lcd_prepare(lcd_id);
	if (ret < 0) {
		DRM_ERROR("sunxi_lcd:%d prepare fialed\n", lcd_id);
		return -1;
	}

	panel_func = sunxi_lcd_get_panel_func(lcd_id);
	if (!panel_func) {
		DRM_ERROR("GET panal_func Failed!\n");
		return -1;
	}

	if (!panel_func->cfg_open_flow) {
		DRM_ERROR("cfg_open_flow hdl is NULL\n");
		return -1;
	}

	panel_func->cfg_open_flow(lcd_id);

	return 0;

}

static void sunxi_lcd_lvds_disable(unsigned int lcd_id)
{
	struct disp_lcd_panel_fun *panel_func = NULL;

	panel_func = sunxi_lcd_get_panel_func(lcd_id);
	if (!panel_func) {
		DRM_ERROR("GET panal_func Failed!\n");
		return;
	}

	if (!panel_func->cfg_close_flow) {
		DRM_ERROR("cfg_open_close hdl is NULL\n");
		return;
	}
	panel_func->cfg_close_flow(lcd_id);

	sunxi_lcd_unprepare(lcd_id);
}

static ssize_t backlight_info_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
#if defined(CONFIG_AW_DRM_BACKLIGHT)
	return sunxi_backlight_info(buf);
#else
	DRM_WARN("You must select CONFIG_AW_DRM_BACKLIGHT in menuconfig!\n");
	return 0;
#endif
}

static ssize_t backlight_info_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	return count;
}

static DEVICE_ATTR(backlight_info, 0660, backlight_info_show,
					backlight_info_store);

static ssize_t sunxi_lcd_cfg_dump(char *buf, struct disp_lcd_cfg *cfg)
{
	ssize_t n = 0;
	unsigned int i;

	if (cfg->lcd_bl_en_used) {
		n += sprintf(buf + n, "lcd_bl_en gpio_name:%s  gpio:%d\n",
		cfg->lcd_bl_en.name, cfg->lcd_bl_en.gpio);
		n += sprintf(buf + n, "lcd_bl_en_power:%s\n",
						cfg->lcd_bl_en_power);
	}

	for (i = 0; i < LCD_POWER_NUM; i++) {
		if (cfg->lcd_power_used[i])
			n += sprintf(buf + n, "lcd_power%d:%s\n",
				i, cfg->lcd_power[i]);
	}

	for (i = 0; i < LCD_POWER_NUM; i++) {
		if (cfg->lcd_fix_power_used[i])
			n += sprintf(buf + n, "lcd_fix_power%d:%s\n",
				i, cfg->lcd_fix_power[i]);
	}

	for (i = 0; i < LCD_GPIO_NUM; i++) {
		if (cfg->lcd_gpio_used[i])
			n += sprintf(buf + n, "lcd_gpio%d name:%s  gpio:%d\n",
					i, cfg->lcd_gpio[i].name,
					cfg->lcd_gpio[i].gpio);
	}

	for (i = 0; i < LCD_GPIO_REGU_NUM; i++) {
		if (cfg->lcd_power_used[i])
			n += sprintf(buf + n, "lcd_gpio_power%d:%s\n",
						i, cfg->lcd_gpio_power[i]);
	}

	for (i = 0; i < LCD_GPIO_REGU_NUM; i++) {
		if (cfg->lcd_power_used[i])
			n += sprintf(buf + n, "lcd_pin_power%d:%s\n",
						i, cfg->lcd_pin_power[i]);
	}

	n += sprintf(buf + n, "backlight_bright:%u\n", cfg->backlight_bright);
	n += sprintf(buf + n, "backlight_dimming:%u\n", cfg->backlight_dimming);
	n += sprintf(buf + n, "lcd_bright:%u\n", cfg->lcd_bright);
	n += sprintf(buf + n, "lcd_contrast:%u\n", cfg->lcd_contrast);
	n += sprintf(buf + n, "lcd_saturation:%u\n", cfg->lcd_saturation);
	n += sprintf(buf + n, "lcd_hue:%u\n", cfg->lcd_hue);

	return n;
}

static ssize_t sunxi_lcd_exten_para_dump(char *buf,
				struct panel_extend_para *para)
{
	ssize_t n = 0;

	n += sprintf(buf + n, "lcd_gamma_en:%u\n", para->lcd_gamma_en);
	n += sprintf(buf + n, "lcd_cmap_en:%u\n", para->lcd_cmap_en);

	return n;
}

static ssize_t sunxi_lcd_panel_para_dump(char *buf, struct disp_panel_para *para)
{
	ssize_t n = 0;

	n += sprintf(buf + n, "lcd_if:%s\n",
			sunxi_lcd_get_connector_type_name(para->lcd_if));
	n += sprintf(buf + n, "lcd_width:%u\n", para->lcd_width);
	n += sprintf(buf + n, "lcd_height:%u\n", para->lcd_height);
	n += sprintf(buf + n, "lcd_dclk_freq:%u\n", para->lcd_dclk_freq);
	n += sprintf(buf + n, "lcd_frm:%u\n", para->lcd_frm);
	n += sprintf(buf + n, "lcd_rb_swap:%u\n", para->lcd_rb_swap);
	n += sprintf(buf + n, "lcd_gamma_en:%u\n", para->lcd_gamma_en);
	n += sprintf(buf + n, "lcd_cmap_en:%u\n", para->lcd_cmap_en);
	n += sprintf(buf + n, "lcd_xtal_freq:%u\n", para->lcd_xtal_freq);
	n += sprintf(buf + n, "lcd_size:%s\n", para->lcd_size);
	n += sprintf(buf + n, "lcd_model_name:%s\n", para->lcd_model_name);
	if (para->lcd_pwm_used) {
		n += sprintf(buf + n, "lcd_pwm_ch:%u\n", para->lcd_pwm_ch);
		n += sprintf(buf + n, "lcd_pwm_freq:%u\n", para->lcd_pwm_freq);
		n += sprintf(buf + n, "lcd_pwm_pol:%u\n", para->lcd_pwm_pol);
	} else {
		n += sprintf(buf + n, "%s\n", "NOT use PWM");
	}

	if (para->lcd_if == LCD_IF_LVDS) {
		n += sprintf(buf + n, "lcd_lvds_if:%s\n",
			sunxi_lcd_get_lvds_if_name(para->lcd_hv_if));
		n += sprintf(buf + n, "lcd_lvds_mode:%u\n",
						para->lcd_lvds_mode);
		n += sprintf(buf + n, "lcd_lvds_colordepth:%u\n",
						para->lcd_lvds_colordepth);
		n += sprintf(buf + n, "lcd_lvds_io_polarity:%u\n",
						para->lcd_lvds_io_polarity);
	} else if (para->lcd_if == LCD_IF_HV) { /* RGB */
		n += sprintf(buf + n, "lcd_hv_if:%s\n",
			sunxi_lcd_get_hv_if_name(para->lcd_hv_if));
		n += sprintf(buf + n, "lcd_hv_clk_phase:%u\n",
						para->lcd_hv_clk_phase);
		n += sprintf(buf + n, "lcd_hv_sync_polarity:%u\n",
						para->lcd_hv_sync_polarity);
		n += sprintf(buf + n, "lcd_hv_srgb_seq:%u\n",
						para->lcd_hv_srgb_seq);
		n += sprintf(buf + n, "lcd_hv_syuv_seq:%u\n",
						para->lcd_hv_syuv_seq);
		n += sprintf(buf + n, "lcd_hv_syuv_fdly:%u\n",
						para->lcd_hv_syuv_fdly);
	} else if (para->lcd_if == LCD_IF_CPU) {
		n += sprintf(buf + n, "lcd_cpu_if:%s\n",
			sunxi_lcd_get_cpu_if_name(para->lcd_cpu_if));
		n += sprintf(buf + n, "lcd_cpu_te:%u\n", para->lcd_cpu_te);
	} else if (para->lcd_if == LCD_IF_DSI) {
		n += sprintf(buf + n, "lcd_dsi_if:%s\n",
			sunxi_lcd_get_dsi_if_name(para->lcd_dsi_if));
		n += sprintf(buf + n, "lcd_dsi_lane:%u\n", para->lcd_dsi_lane);
		n += sprintf(buf + n, "lcd_dsi_format:%u\n",
							para->lcd_dsi_format);
		n += sprintf(buf + n, "lcd_dsi_eotp:%u\n", para->lcd_dsi_eotp);
		n += sprintf(buf + n, "lcd_dsi_te:%u\n", para->lcd_dsi_te);
	} else if (para->lcd_if == LCD_IF_EDP) {
		/* n += sprintf(buf + n, "lcd_edp_rate:%u\n", para->lcd_edp_rate);
		n += sprintf(buf + n, "lcd_edp_lane:%u\n", para->lcd_edp_lane);
		n += sprintf(buf + n, "lcd_edp_colordepth:%u\n",
						para->lcd_edp_colordepth);
		n += sprintf(buf + n, "lcd_edp_fps:%u\n", para->lcd_edp_fps); */
	}

	return n;
}

static ssize_t lcd_info_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;
	unsigned int i;
	struct sunxi_lcd *lcd;

	n += sprintf(buf + n, "%s\n", "sunxi LCD info:");
	n += sprintf(buf + n, "lcd count:%u\n", lcd_drv->lcd_cnt);
	if (lcd_drv->lvds_cnt)
		n += sprintf(buf + n, "lvds count:%u\n", lcd_drv->lvds_cnt);
	if (lcd_drv->rgb_cnt)
		n += sprintf(buf + n, "rgb count:%u\n", lcd_drv->rgb_cnt);
	if (lcd_drv->dsi_cnt)
		n += sprintf(buf + n, "dsi count:%u\n", lcd_drv->dsi_cnt);
	if (lcd_drv->edp_cnt)
		n += sprintf(buf + n, "edp count:%u\n", lcd_drv->edp_cnt);
	n += sprintf(buf + n, "\n");

	for (i = 0; i < lcd_drv->lcd_cnt; i++) {
		lcd = sunxi_lcd_get_lcd(i);
		if (!lcd) {
			DRM_ERROR("Null lcd pointer!\n");
			return 0;
		}
		n += sprintf(buf + n, "id:%d\n", lcd->id);
		if (!lcd->is_used) {
			n += sprintf(buf + n, "%s\n\n", "NOT used");
			continue;
		}

		n += sprintf(buf + n, "type:%s%d\n",
			sunxi_lcd_get_connector_type_name(lcd->type),
					lcd->type_id);

		if (!lcd->reg_base)
			n += sprintf(buf + n, "%s\n", "Do NOT have reg_base"
				", rely on TCON");
		else
			n += sprintf(buf + n, "reg_base:%lu\n", lcd->reg_base);

		if (lcd->mclk) {
			n += sprintf(buf + n, "clk_name:%s clk_rate: %lu"
						"  enable count:%d\n",
					__clk_get_name(lcd->mclk),
					clk_get_rate(lcd->mclk),
					__clk_get_enable_count(lcd->mclk));
		} else {
			n += sprintf(buf + n, "%s\n", "ERROR: NOT have CLK");
		}

		if (lcd->irq_used)
			n += sprintf(buf + n, "irq_no:%u\n", lcd->irq_no);
		else
			n += sprintf(buf + n, "%s\n", "NOT use irq and "
				"do NOT have irq_no ");

		if (lcd->use_bl)
			n += sprintf(buf + n, "Use Backlight\n");
		else
			n += sprintf(buf + n, "NOT use Backlight\n");

		n += sunxi_lcd_cfg_dump(buf + n, &lcd->lcd_cfg);
		n += sunxi_lcd_exten_para_dump(buf + n, &lcd->extend_para);
		n += sunxi_lcd_panel_para_dump(buf + n, &lcd->panel_para);

		n += sprintf(buf + n, "\n");
	}



	return n;
}

static ssize_t lcd_info_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	return count;
}

static DEVICE_ATTR(lcd_info, 0660, lcd_info_show, lcd_info_store);

static ssize_t lcd_reg_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	return 0;
}

static ssize_t lcd_reg_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	return count;
}

static DEVICE_ATTR(reg, 0660, lcd_reg_show, lcd_reg_store);


static struct attribute *lcd_attributes[] = {
	&dev_attr_lcd_info.attr,
	&dev_attr_reg.attr,
	&dev_attr_backlight_info.attr,
	NULL
};

static struct attribute_group lcd_attribute_group = {
	.name = "attr",
	.attrs = lcd_attributes,
};

static int sunxi_lcd_dts_parse_lcd_core(struct platform_device *pdev,
				struct sunxi_lcd_resource *res)
{
	unsigned int count = 0, i;
	char id[32];
	struct device_node *node = pdev->dev.of_node;

#ifdef SUPPORT_LVDS
#if (!defined CONFIG_ARCH_SUN50IW9) && (!defined CONFIG_ARCH_SUN8I)
	res->mclk[SUNXI_LVDS0] = of_clk_get(node, count);
	if (IS_ERR(res->mclk[SUNXI_LVDS0])) {
		DRM_ERROR("fail to get clk for lvds\n");
		return -EINVAL;
	}
	count++;
#endif
	res->rst_bus_lvds = devm_reset_control_get_shared(&pdev->dev, "rst_bus_lvds");
	if (IS_ERR(res->rst_bus_lvds)) {
		DRM_ERROR("fail to get reset clk for rst_bus_lvds\n");
		return -EINVAL;
	}
#endif
#if IS_ENABLED(CONFIG_AW_DRM_LCD_DSI)
	for (i = 0; i < CLK_DSI_NUM; i++) {
		sprintf(id, "clk_mipi_dsi%d", i);
		res->clk_mipi_dsi[i] = devm_clk_get(&pdev->dev, id);
		if (IS_ERR(res->clk_mipi_dsi[i])) {
			res->clk_mipi_dsi[i] = NULL;
			dev_err(&pdev->dev, "failed to get clk for %s\n", id);
			return -EINVAL;
		}

		sprintf(id, "clk_bus_mipi_dsi%d", i);
		res->clk_bus_mipi_dsi[i] = devm_clk_get(&pdev->dev, id);
		if (IS_ERR(res->clk_bus_mipi_dsi[i])) {
			res->clk_bus_mipi_dsi[i] = NULL;
			dev_err(&pdev->dev, "failed to get clk for %s\n", id);
			return -EINVAL;
		}

		sprintf(id, "rst_bus_mipi_dsi%d", i);
		res->rst_bus_mipi_dsi[i] = devm_reset_control_get(&pdev->dev, id);
		if (IS_ERR(res->rst_bus_mipi_dsi[i])) {
			res->rst_bus_mipi_dsi[i] = NULL;
			dev_err(&pdev->dev, "failed to get reset for %s\n", id);
			return -EINVAL;
		}
	}

	for (i = 0; i < 1/*DRM_DSI_NUM*/; ++i) { /* @TODO: fix this */
		res->reg_base[SUNXI_DSI0 + i] = (uintptr_t __force)
			of_iomap(node, 0);
		if (!res->reg_base[SUNXI_DSI0 + i]) {
			dev_err(&pdev->dev, "unable to map dsi registers\n");
			return -EINVAL;
		}
		res->irq_no[SUNXI_DSI0 + i] = irq_of_parse_and_map(node, 0);
	}
#endif

	return 0;
}

static int sunxi_lcd_init_al(struct sunxi_lcd_resource *res)
{
#if defined(SUPPORT_DSI)
	int i;
	for (i = 0; i < 1/* DRM_DSI_NUM */; ++i) /* @TODO: fix this */
		dsi_set_reg_base(i, res->reg_base[SUNXI_DSI0 + i]);
#endif
	return 0;
}

/*
 * parse lcd gpio info
 * lcd_bl_en: backlight enable gpio
 * lcd_gpio_xxx: lcd general gpio
 * lcd_gpio_sda/scl
 */
static int sunxi_lcd_dts_parse_gpio_info(struct platform_device *pdev,
					struct disp_lcd_cfg  *lcd_cfg)
{
	struct disp_gpio_info *gpio_info;
	char sub_name[25];
	int ret, i;
	struct device_node *node = pdev->dev.of_node;

	/* lcd_bl_en gpio
	 * gpio for enable LCD backlight
	 * NOTE: it is NOT PWM
	 */
	lcd_cfg->lcd_bl_en_used = 0;
	gpio_info = (struct disp_gpio_info *)&(lcd_cfg->lcd_bl_en);
	ret = sunxi_drm_get_sys_item_gpio(node, "lcd_bl_en", gpio_info);
	if (ret == 0) {
		lcd_cfg->lcd_bl_en_used = 1;
		DRM_INFO("[SUNXI-LCD]get gpio:lcd_bl_en: ");
		DRM_INFO("[SUNXI-LCD]name:%s gpio:%d\n",
				gpio_info->name, gpio_info->gpio);
	}

	/* lcd_gpio_0/1/2
	 * LCD common gpio, such as LCD-RST/ so on
	 */
	for (i = 0; i < LCD_GPIO_NUM; i++) {
		sprintf(sub_name, "lcd_gpio_%d", i);
		gpio_info = (struct disp_gpio_info *)&(lcd_cfg->lcd_gpio[i]);
		ret = sunxi_drm_get_sys_item_gpio(node, sub_name, gpio_info);
		if (ret == 0) {
			lcd_cfg->lcd_gpio_used[i] = 1;
			DRM_INFO("[SUNXI-LCD]get gpio:%s: ", sub_name);
			DRM_INFO("[SUNXI-LCD]name:%s gpio:%d\n",
				gpio_info->name, gpio_info->gpio);
		}
	}

	/* lcd_gpio_scl,lcd_gpio_sda */
	gpio_info = (struct disp_gpio_info *)&(lcd_cfg->lcd_gpio[LCD_GPIO_SCL]);
	ret = sunxi_drm_get_sys_item_gpio(node, "lcd_gpio_scl", gpio_info);
	if (ret == 0) {
		lcd_cfg->lcd_gpio_used[LCD_GPIO_SCL] = 1;
		DRM_INFO("[SUNXI-LCD]get lcd_gpio_scl: ");
		DRM_INFO("[SUNXI-LCD]name:%s gpio:%d\n",
				gpio_info->name, gpio_info->gpio);
	}

	gpio_info = (struct disp_gpio_info *)&(lcd_cfg->lcd_gpio[LCD_GPIO_SDA]);
	ret = sunxi_drm_get_sys_item_gpio(node, "lcd_gpio_sda", gpio_info);
	if (ret == 0) {
		lcd_cfg->lcd_gpio_used[LCD_GPIO_SDA] = 1;
		DRM_INFO("[SUNXI-LCD]get lcd_gpio_sda: ");
		DRM_INFO("[SUNXI-LCD]name:%s gpio:%d\n",
				gpio_info->name, gpio_info->gpio);
	}

	return 0;
}

/**
 * parse LCD power info
 * cd_bl_en_power
 * cd_fix_power
 * cd_power/lcd_power0/1/2
 * cd_gpio_power
 * cd_pin_power
 */
static int sunxi_lcd_dts_parse_power_info(struct platform_device *pdev,
				struct disp_lcd_cfg  *lcd_cfg)
{
	char sub_name[25];
	struct device_node *node = pdev->dev.of_node;
	int ret, i;
	g_lcd_dev = &pdev->dev;
	/* lcd_bl_en_power */
	sprintf(sub_name, "lcd_bl_en_power");
	ret = sunxi_drm_get_sys_item_char(node, sub_name,
		lcd_cfg->lcd_bl_en_power);
	if (ret == 0)
		DRM_INFO("[SUNXI-LCD]get %s(%s)\n", sub_name,
				lcd_cfg->lcd_bl_en_power);

	/* lcd fix power
	 * lcd connector type specific power
	 * often used for vcc-dsi/vcc-lvds
	 */
	for (i = 0; i < LCD_POWER_NUM; i++) {
		if (i == 0)
			sprintf(sub_name, "lcd_fix_power");
		else
			sprintf(sub_name, "lcd_fix_power%d", i);

		lcd_cfg->lcd_fix_power_used[i] = 0;
		ret = sunxi_drm_get_sys_item_char(node, sub_name,
			lcd_cfg->lcd_fix_power[i]);
		if (ret == 0) {
			lcd_cfg->lcd_fix_power_used[i] = 1;
			DRM_INFO("[SUNXI-LCD]get %s(%s)\n", sub_name,
					lcd_cfg->lcd_fix_power[i]);
		}
	}

	/* lcd_power lcd common power
	 * such as: vcc-lcd
	 */
	for (i = 0; i < LCD_POWER_NUM; i++) {
		if (i == 0)
			sprintf(sub_name, "lcd_power");
		else
			sprintf(sub_name, "lcd_power%d", i);

		lcd_cfg->lcd_power_used[i] = 0;
		ret = sunxi_drm_get_sys_item_char(node, sub_name,
			lcd_cfg->lcd_power[i]);
		if (ret == 0) {
			lcd_cfg->lcd_power_used[i] = 1;
			DRM_INFO("[SUNXI-LCD]get %s(%s)\n", sub_name,
					lcd_cfg->lcd_power[i]);
		}
	}

	for (i = 0; i < LCD_GPIO_REGU_NUM; i++) {
		sprintf(sub_name, "lcd_gpio_power%d", i);
		ret = sunxi_drm_get_sys_item_char(node, sub_name,
			lcd_cfg->lcd_gpio_power[i]);
		if (ret == 0) {
			lcd_cfg->lcd_power_used[i] = 1;
			DRM_INFO("[SUNXI-LCD]get %s(%s)\n", sub_name,
					lcd_cfg->lcd_gpio_power[i]);
		}
	}

	/* lcd power for pin, such as: vcc-pd */
	for (i = 0; i < LCD_GPIO_REGU_NUM; i++) {
		if (0 == i)
			sprintf(sub_name, "lcd_pin_power");
		else
			sprintf(sub_name, "lcd_pin_power%d", i);

		ret = sunxi_drm_get_sys_item_char(node, sub_name,
			lcd_cfg->lcd_pin_power[i]);
		if (ret == 0) {
			lcd_cfg->lcd_power_used[i] = 1;
			DRM_INFO("[SUNXI-LCD]get %s(%s)\n", sub_name,
					lcd_cfg->lcd_pin_power[i]);
		}
	}

	return 0;
}

/* lcd backlight
 * NOTE: is often PWM, different from backlight enable(lcd_bl_en)
 */
static int sunxi_lcd_dts_parse_backlight_info(struct platform_device *pdev,
					struct disp_lcd_cfg  *lcd_cfg)
{
	 struct device_node *node = pdev->dev.of_node;
	 int ret, i, value = 1;
	 char sub_name[25];

	/* backlight adjust */
	for (i = 0; i < 101; i++) {
		sprintf(sub_name, "lcd_bl_%d_percent", i);
		lcd_cfg->backlight_curve_adjust[i] = 0;

		if (i == 100) {
			lcd_cfg->backlight_curve_adjust[i] = 255;
		}

		ret = of_property_read_u32(node, sub_name, &value);
		if (ret == 0) {
			value = (value > 100) ? 100:value;
			value = value * 255 / 100;
			lcd_cfg->backlight_curve_adjust[i] = value;
		}
	}

	sprintf(sub_name, "lcd_backlight");
	ret = of_property_read_u32(node, sub_name, &value);
	if (ret == 0) {
		value = (value > 256) ? 256:value;
		lcd_cfg->backlight_bright = value;
	} else {
		lcd_cfg->backlight_bright = 197;
	}
	lcd_cfg->lcd_bright = lcd_cfg->backlight_bright;

	return 0;
}

static int sunxi_lcd_dts_parse_pannel_timing(struct platform_device *pdev,
						struct disp_panel_para *info)
{
	struct device_node *node = pdev->dev.of_node;
	int ret, value = 1;

	/* display mode */
	ret = of_property_read_u32(node, "lcd_x", &value);
	if (ret != 0) {
		DRM_ERROR("parse lcd_x failed\n");
		return -1;
	}
	info->lcd_x = value;

	ret = of_property_read_u32(node, "lcd_y", &value);
	if (ret != 0) {
		DRM_ERROR("parse lcd_y failed\n");
		return -1;
	}
	info->lcd_y = value;

	ret = of_property_read_u32(node, "lcd_hbp", &value);
	if (ret == 0)
		info->lcd_hbp = value;

	ret = of_property_read_u32(node, "lcd_ht", &value);
	if (ret == 0)
		info->lcd_ht = value;

	ret = of_property_read_u32(node, "lcd_vbp", &value);
	if (ret == 0)
		info->lcd_vbp = value;

	ret = of_property_read_u32(node, "lcd_vt", &value);
	if (ret == 0)
		info->lcd_vt = value;

	ret = of_property_read_u32(node, "lcd_vspw", &value);
	if (ret == 0)
		info->lcd_vspw = value;

	ret = of_property_read_u32(node, "lcd_hspw", &value);
	if (ret == 0)
		info->lcd_hspw = value;

	return 0;
}

static int sunxi_lcd_dts_parse_pwm(struct platform_device *pdev,
					struct disp_panel_para *info)
{
	struct device_node *node = pdev->dev.of_node;
	int ret, value = 1;

	ret = of_property_read_u32(node, "lcd_pwm_ch", &value);
	if (ret != 0) {
		DRM_ERROR("parse lcd_pwm_ch failed\n");
		return -1;
	}
	info->lcd_pwm_ch = value;

	ret = of_property_read_u32(node, "lcd_pwm_freq", &value);
	if (ret != 0) {
		DRM_ERROR("parse lcd_pwm_freq failed\n");
		return -1;
	}
	info->lcd_pwm_freq = value;

	ret = of_property_read_u32(node, "lcd_pwm_pol", &value);
	if (ret != 0) {
		DRM_ERROR("parse lcd_pwm_pol failed\n");
		return -1;
	}
	info->lcd_pwm_pol = value;


	return 0;
}

/* RGB */
static int sunxi_lcd_dts_parse_hv(struct platform_device *pdev,
					struct disp_panel_para *info)
{
	struct device_node *node = pdev->dev.of_node;
	int ret, value = 1;

	ret = of_property_read_u32(node, "lcd_hv_if", &value);
	if (ret != 0) {
		DRM_ERROR("parse lcd_hv_if failed\n");
		return -1;
	}
	info->lcd_hv_if = value;

	ret = of_property_read_u32(node, "lcd_hv_clk_phase", &value);
	if (ret != 0) {
		DRM_ERROR("parse lcd_hv_clk_phase failed\n");
		return -1;
	}
	info->lcd_hv_clk_phase = value;

	ret = of_property_read_u32(node, "lcd_hv_sync_polarity", &value);
	if (ret != 0) {
		DRM_ERROR("parse lcd_hv_sync_polarity failed\n");
		return -1;
	}
	info->lcd_hv_sync_polarity = value;

	ret = of_property_read_u32(node, "lcd_hv_srgb_seq", &value);
	if (ret != 0) {
		DRM_ERROR("parse lcd_hv_srgb_seq failed\n");
		return -1;
	}
	info->lcd_hv_srgb_seq = value;

	ret = of_property_read_u32(node, "lcd_hv_syuv_seq", &value);
	if (ret != 0) {
		DRM_ERROR("parse lcd_hv_syuv_seq failed\n");
		return -1;
	}
	info->lcd_hv_syuv_seq = value;

	ret = of_property_read_u32(node, "lcd_hv_syuv_fdly", &value);
	if (ret != 0) {
		DRM_ERROR("parse lcd_hv_syuv_fdly failed\n");
		return -1;
	}
	info->lcd_hv_syuv_fdly = value;

	return 0;
}

static int sunxi_lcd_dts_parse_lvds(struct platform_device *pdev,
					struct disp_panel_para *info)
{
	struct device_node *node = pdev->dev.of_node;
	int ret, value = 1;

	ret = of_property_read_u32(node, "lcd_lvds_if", &value);
	if (ret != 0) {
		DRM_ERROR("parse lcd_lvds_if failed\n");
		return -1;
	}
	info->lcd_lvds_if = value;

	ret = of_property_read_u32(node, "lcd_lvds_mode", &value);
	if (ret != 0) {
		DRM_ERROR("parse lcd_lvds_mode failed\n");
		return -1;
	}
	info->lcd_lvds_mode = value;

	ret = of_property_read_u32(node, "lcd_lvds_colordepth",
		&value);
	if (ret != 0) {
		DRM_ERROR("parse lcd_lvds_colordepth failed\n");
		return -1;
	}
	info->lcd_lvds_colordepth = value;

	ret = of_property_read_u32(node, "lcd_lvds_io_polarity",
		&value);
	if (ret == 0)
		info->lcd_lvds_io_polarity = value;

	return 0;
}

static int sunxi_lcd_dts_parse_cpu(struct platform_device *pdev,
					struct disp_panel_para *info)
{
	struct device_node *node = pdev->dev.of_node;
	int ret, value = 1;

	ret = of_property_read_u32(node, "lcd_cpu_if", &value);
	if (ret != 0) {
		DRM_ERROR("parse lcd_cpu_if failed\n");
		return -1;
	}
	info->lcd_cpu_if = value;

	ret = of_property_read_u32(node, "lcd_cpu_te", &value);
	if (ret != 0) {
		DRM_ERROR("parse lcd_cpu_te failed\n");
		return -1;
	}
	info->lcd_cpu_te = value;

	return 0;
}

static int sunxi_lcd_dts_parse_dsi(struct platform_device *pdev,
					struct disp_panel_para *info)
{
	struct device_node *node = pdev->dev.of_node;
	int ret, value = 1;

	ret = of_property_read_u32(node, "lcd_dsi_if", &value);
	if (ret != 0) {
		DRM_ERROR("parse lcd_dsi_if failed\n");
		return -1;
	}
	info->lcd_dsi_if = value;

	ret = of_property_read_u32(node, "lcd_dsi_lane", &value);
	if (ret != 0) {
		DRM_ERROR("parse lcd_dsi_lane failed\n");
		return -1;
	}
	info->lcd_dsi_lane = value;

	ret = of_property_read_u32(node, "lcd_dsi_format",
		&value);
	if (ret != 0) {
		DRM_ERROR("parse lcd_dsi_format failed\n");
		return -1;
	}
	info->lcd_dsi_format = value;

	ret = of_property_read_u32(node, "lcd_dsi_eotp", &value);
	if (ret != 0) {
		DRM_ERROR("parse lcd_dsi_eotp failed\n");
		return -1;
	}
	info->lcd_dsi_eotp = value;

	ret = of_property_read_u32(node, "lcd_dsi_te", &value);
	if (ret != 0) {
		DRM_ERROR("parse lcd_dsi_te failed\n");
		return -1;
	}
	info->lcd_dsi_te = value;

	return 0;
}

static int sunxi_lcd_dts_parse_edp(struct platform_device *pdev,
					struct disp_panel_para *info)
{
	struct device_node *node = pdev->dev.of_node;
	int ret, value = 1;

	ret = of_property_read_u32(node, "lcd_edp_rate", &value);
	if (ret != 0) {
		DRM_ERROR("parse lcd_edp_rate failed\n");
		return -1;
	}
	/* info->lcd_edp_rate = value; */

	ret = of_property_read_u32(node, "lcd_edp_lane", &value);
	if (ret != 0) {
		DRM_ERROR("parse lcd_edp_lane failed\n");
		return -1;
	}
	/* info->lcd_edp_lane = value; */

	ret = of_property_read_u32(node, "lcd_edp_colordepth", &value);
	if (ret != 0) {
		DRM_ERROR("parse lcd_edp_colordepth failed\n");
		return -1;
	}
	/* info->lcd_edp_colordepth = value; */

	ret = of_property_read_u32(node, "lcd_edp_fps", &value);
	if (ret != 0) {
		DRM_ERROR("parse lcd_edp_fps failed\n");
		return -1;
	}
	/* info->lcd_edp_fps = value; */

	return 0;
}

static int sunxi_lcd_dts_parse_panel_info(struct platform_device *pdev,
					struct disp_panel_para *info)
{
	struct device_node *node = pdev->dev.of_node;
	int ret, value = 1;

#ifndef CONFIG_ARCH_SUN8I
	ret = of_property_read_u32(node, "lcd_width", &value);
	if (ret < 0) {
		DRM_ERROR("get lcd_width failed\n");
		return -1;
	}
	info->lcd_width = value;

	ret = of_property_read_u32(node, "lcd_height", &value);
	if (ret < 0) {
		DRM_ERROR("get lcd_height failed\n");
		return -1;
	}
	info->lcd_height = value;
#endif

	ret = of_property_read_u32(node, "lcd_dclk_freq", &value);
	if (ret < 0) {
		DRM_ERROR("get lcd_dclk_freq failed\n");
		return -1;
	}
	info->lcd_dclk_freq = value;

	ret = sunxi_lcd_dts_parse_pannel_timing(pdev, info);
	if (ret < 0) {
		DRM_ERROR("sunxi_lcd_dts_parse_pannel_timing failed\n");
		return -1;
	}


	ret = of_property_read_u32(node, "lcd_pwm_used", &value);
	if (ret == 0) {
		info->lcd_pwm_used = value;
		if (sunxi_lcd_dts_parse_pwm(pdev, info)) {
			DRM_ERROR("sunxi_lcd_dts_parse_pwm failed\n");
			return -1;
		}
	} else {
		DRM_INFO("[SUNXI-LCD]NOT use PWM\n");
	}

	ret = of_property_read_u32(node, "lcd_if", &value);
	if (ret != 0) {
		DRM_ERROR("parse lcd_if failed\n");
		return -1;
	}
	info->lcd_if = value;

	if (info->lcd_if == LCD_IF_HV) { /* RGB */
		/* if (sunxi_lcd_dts_parse_hv(pdev, info)) {
			DRM_ERROR("sunxi_lcd_dts_parse_hv failed\n");
			return -1;
		} */
	} else if (info->lcd_if == LCD_IF_LVDS) {
		if (sunxi_lcd_dts_parse_lvds(pdev, info)) {
			DRM_ERROR("sunxi_lcd_dts_parse_lvds failed\n");
			return -1;
		}
	} else if (info->lcd_if == LCD_IF_CPU) {
		if (sunxi_lcd_dts_parse_cpu(pdev, info)) {
			DRM_ERROR("sunxi_lcd_dts_parse_lvds failed\n");
			return -1;
		}
	} else if (info->lcd_if == LCD_IF_DSI) {
		if (sunxi_lcd_dts_parse_dsi(pdev, info)) {
			DRM_ERROR("sunxi_lcd_dts_parse_dsi failed\n");
			return -1;
		}
	} else if (info->lcd_if == LCD_IF_EDP) {
		if (sunxi_lcd_dts_parse_edp(pdev, info)) {
			DRM_ERROR("sunxi_lcd_dts_parse_edp failed\n");
			return -1;
		}
	}

	ret = of_property_read_u32(node, "lcd_frm", &value);
	if (ret == 0)
		info->lcd_frm = value;

	ret = of_property_read_u32(node, "lcd_rb_swap", &value);
	if (ret == 0)
		info->lcd_rb_swap = value;

	ret = of_property_read_u32(node, "lcd_gamma_en", &value);
	if (ret == 0)
		info->lcd_gamma_en = value;

	ret = of_property_read_u32(node, "lcd_cmap_en", &value);
	if (ret == 0)
		info->lcd_cmap_en = value;

	ret = of_property_read_u32(node, "lcd_xtal_freq", &value);
	if (ret == 0)
		info->lcd_xtal_freq = value;

	ret = sunxi_drm_get_sys_item_char(node, "lcd_size",
		(void *)info->lcd_size);
	ret = sunxi_drm_get_sys_item_char(node, "lcd_model_name",
		(void *)info->lcd_model_name);
	if (ret) {
		ret = sunxi_drm_get_sys_item_char(node, "lcd_driver_name",
				(void *)info->lcd_model_name);
		if (ret) {
			DRM_ERROR("get lcd_driver_name failed\n");
			return -1;
		}
		DRM_INFO("[SUNXI-LCD]get lcd panel driver:%s\n",
					info->lcd_model_name);
	} else {
		DRM_INFO("[SUNXI-LCD]get lcd panel driver:%s\n",
					info->lcd_model_name);
	}

	return 0;
}

int sunxi_lcd_dts_parse(struct platform_device *pdev,
			struct sunxi_lcd *sunxi_lcd)
{
	struct disp_lcd_cfg  *lcd_cfg;
	struct disp_panel_para  *info;

	lcd_cfg = &sunxi_lcd->lcd_cfg;
	info = &sunxi_lcd->panel_para;

	if (sunxi_lcd_dts_parse_gpio_info(pdev, lcd_cfg) < 0) {
		DRM_ERROR("sunxi_lcd_dts_parse_gpio_info failed\n");
		return -1;
	}

	if (sunxi_lcd_dts_parse_power_info(pdev, lcd_cfg) < 0) {
		DRM_ERROR("sunxi_lcd_dts_parse_power_info failed\n");
		return -1;
	}

	if (sunxi_lcd_dts_parse_backlight_info(pdev, lcd_cfg) < 0) {
		DRM_ERROR("sunxi_lcd_dts_parse_backlight_info failed\n");
		return -1;
	}

	if (sunxi_lcd_dts_parse_panel_info(pdev, info) < 0) {
		DRM_ERROR("sunxi_lcd_dts_parse_panel_info failed\n");
		return -1;
	}

	return 0;
}

static int sunxi_lcd_cal_backlight_curve(struct sunxi_lcd *lcd)
{
	unsigned int i = 0, j = 0;
	unsigned int items = 0;
	unsigned int lcd_bright_curve_tbl[101][2];
	struct disp_lcd_cfg *cfg = &lcd->lcd_cfg;
	struct panel_extend_para *extend_para = &lcd->extend_para;

	for (i = 0; i < 101; i++) {
		if (cfg->backlight_curve_adjust[i] == 0) {
			if (i == 0) {
				lcd_bright_curve_tbl[items][0] = 0;
				lcd_bright_curve_tbl[items][1] = 0;
				items++;
			}
		} else {
			lcd_bright_curve_tbl[items][0] = 255 * i / 100;
			lcd_bright_curve_tbl[items][1] =
						cfg->backlight_curve_adjust[i];
			items++;
		}
	}

	for (i = 0; i < items - 1; i++) {
		u32 num = lcd_bright_curve_tbl[i + 1][0]
				- lcd_bright_curve_tbl[i][0];

		for (j = 0; j < num; j++) {
			u32 value = 0;

			value = lcd_bright_curve_tbl[i][1]
				+ ((lcd_bright_curve_tbl[i + 1][1]
				- lcd_bright_curve_tbl[i][1]) * j) / num;
			extend_para->lcd_bright_curve_tbl[
					lcd_bright_curve_tbl[i][0] + j] = value;
		}
	}

	extend_para->lcd_bright_curve_tbl[255] =
					lcd_bright_curve_tbl[items - 1][1];

	return 0;
}

#if defined(CONFIG_AW_DRM_BACKLIGHT)
static int sunxi_lcd_backlight_init(struct sunxi_lcd *lcd)
{
	struct backlight_config bl_cfg;
	struct disp_lcd_cfg     *lcd_cfg = &lcd->lcd_cfg;
	struct disp_panel_para  *panel_para = &lcd->panel_para;
	struct panel_extend_para *extend_para = &lcd->extend_para;
	int ret = 0;

	bl_cfg.use_bl_en = lcd_cfg->lcd_bl_en_used;
	memcpy(&bl_cfg.bl_en_gpio, &lcd_cfg->lcd_bl_en,
			sizeof(bl_cfg.bl_en_gpio));
	memcpy(bl_cfg.bl_en_power, lcd_cfg->lcd_bl_en_power, LCD_POWER_STR_LEN);

	if (sunxi_lcd_cal_backlight_curve(lcd) < 0) {
		DRM_ERROR("sunxi_lcd_cal_backlight_curve failed\n");
		return -1;
	}

	bl_cfg.bright = lcd_cfg->backlight_bright;
	bl_cfg.use_pwm = panel_para->lcd_pwm_used;
	if (bl_cfg.use_pwm) {
		bl_cfg.pwm.lcd_pwm_ch = panel_para->lcd_pwm_ch;
		bl_cfg.pwm.lcd_pwm_pol = panel_para->lcd_pwm_pol;
		bl_cfg.pwm.lcd_pwm_freq = panel_para->lcd_pwm_freq;
	}

	ret = sunxi_backlight_init(lcd->id, &bl_cfg,
			extend_para->lcd_bright_curve_tbl);
	if (ret < 0) {
		DRM_ERROR("sunxi_backlight_init failed\n");
		return -1;
	}

	return 0;
}

static void sunxi_lcd_backlight_remove(struct sunxi_lcd *lcd)
{
	sunxi_backlight_remove(lcd->id);
}
#endif

static struct sunxi_lcd_funcs sunxi_hv_funcs = {
	.get_type = sunxi_lcd_get_type,
	.get_panel_para = sunxi_lcd_get_panel_para,
	.is_use_irq = sunxi_lcd_is_use_irq,
	.get_irq_no = sunxi_lcd_get_irq_no,

	.enable = sunxi_lcd_hv_enable,
	.sw_enable = sunxi_lcd_sw_enable,
	.disable = sunxi_lcd_hv_disable,
};

static struct sunxi_lcd_funcs sunxi_lvds_funcs = {
	.get_type = sunxi_lcd_get_type,
	.get_panel_para = sunxi_lcd_get_panel_para,
	.is_use_irq = sunxi_lcd_is_use_irq,
	.get_irq_no = sunxi_lcd_get_irq_no,

	.enable = sunxi_lcd_lvds_enable,
	.sw_enable = sunxi_lcd_sw_enable,
	.disable = sunxi_lcd_lvds_disable,
};

static struct sunxi_lcd_funcs sunxi_dsi_funcs = {
	.get_type = sunxi_lcd_get_type,
	.get_panel_para = sunxi_lcd_get_panel_para,
	.is_use_irq = sunxi_lcd_is_use_irq,
	.get_irq_no = sunxi_lcd_get_irq_no,

	.enable = sunxi_lcd_dsi_enable,
	.sw_enable = sunxi_lcd_sw_enable,
	.disable = sunxi_lcd_dsi_disable,
};

static int sunxi_lcd_probe(struct platform_device *pdev)
{
	u32 lcd_used = 0, id;
	struct sunxi_lcd *lcd;
	struct sunxi_lcd_resource *res;

	res = &lcd_drv->res;

	DRM_INFO("pdev name:%s\n", pdev->name);

/* init LCD CORE */
	if ((!strcmp(pdev->name, "lcd-core"))
		|| strstr(pdev->name, "lcd-core")) {
		DRM_INFO("[SUNXI-LCD]: find sunxi-lcd\n");
		if (sunxi_lcd_dts_parse_lcd_core(pdev, res) < 0) {
			DRM_ERROR("sunxi_lcd_dts_parse_lcd_core failed");
			DRM_ERROR("probe failed\n");
			return -EINVAL;
		}

		if (sunxi_lcd_init_al(&lcd_drv->res) < 0) {
			DRM_ERROR("sunxi_lcd_init_al failed");
			DRM_ERROR("probe failed\n");
			return -EINVAL;
		}

#if defined(CONFIG_AW_DRM_BACKLIGHT)
		/* initial backlight driver */
		if (sunxi_backlight_drv_init() < 0) {
			DRM_ERROR("sunxi_backlight_drv_init failed");
			DRM_ERROR("probe failed\n");
			return -EINVAL;
		}
#endif


		DRM_INFO("[SUNXI-LCD]lcd-core probe end\n");
		return 0;
	}

/* init lcd0/1/2/3 */
	id = lcd_drv->lcd_cnt;

	lcd = &lcd_drv->hwlcd[id];
	lcd->id = id;
	lcd->pdev = pdev;

	if (of_property_read_u32(pdev->dev.of_node,
			"lcd_used", &lcd_used)) {
		DRM_ERROR("DTS parse lcd_used failed\n");
		goto lcd_err;
	}

	if (!lcd_used) {
		DRM_INFO("[SUNXI-LCD]Warn: lcd%d is NOT be used!\n", id);
		goto lcd_err;
	}

	lcd->is_used = true;

	if (sunxi_lcd_dts_parse(pdev, lcd) < 0) {
		DRM_ERROR("sunxi_lcd_dts_parse failed\n");
		goto lcd_err;
	}
	lcd->type = lcd->panel_para.lcd_if;

	lcd->extend_para.lcd_gamma_en =  lcd->panel_para.lcd_gamma_en;

/* LCD-CORE dispatch reg_base/irq_no/clk to sunxi_lcd */
	if (lcd->type == LCD_IF_DSI) { /* DSI */
#ifdef SUPPORT_DSI
		lcd->type_id = lcd_drv->dsi_cnt;
		lcd_drv->dsi_cnt++;
		lcd->reg_base = res->reg_base[SUNXI_DSI0 + lcd->type_id];

		lcd->mclk = res->mclk[SUNXI_DSI0 + lcd->type_id];
		lcd->mclk_bus = res->mclk_bus[SUNXI_DSI0 + lcd->type_id];
		lcd->parent_clk = clk_get_parent(lcd->mclk);
#ifdef DSI_VERSION_40
		lcd->irq_used = true;
#else
		lcd->irq_used = false;
#endif
		lcd->irq_no = res->irq_no[SUNXI_DSI0 + lcd->type_id];
#endif
	} else if (lcd->type == LCD_IF_LVDS) { /* LVDS */
		lcd->type_id = lcd_drv->lvds_cnt;
		lcd_drv->lvds_cnt++;
		lcd->mclk = res->mclk[SUNXI_LVDS0 + lcd->type_id];
		lcd->parent_clk = clk_get_parent(lcd->mclk);
		lcd->irq_used = false;
	} else if (lcd->type == LCD_IF_HV) { /* RGB */
		lcd->type_id = lcd_drv->rgb_cnt;
		lcd_drv->rgb_cnt++;
		lcd->mclk = res->mclk[SUNXI_RGB0 + lcd->type_id];
		lcd->parent_clk = clk_get_parent(lcd->mclk);
		lcd->irq_used = false;
	} else if (lcd->type == LCD_IF_CPU) { /* CPU */
		lcd->type_id = lcd_drv->cpu_cnt;
		lcd_drv->cpu_cnt++;
		lcd->mclk = res->mclk[SUNXI_CPU0 + lcd->type_id];
		lcd->parent_clk = clk_get_parent(lcd->mclk);
		lcd->irq_used = false;
	} else {
		DRM_ERROR("lcd type:%d haven't been Implement\n", lcd->type);
		goto lcd_err;
	}

/* sunxi_lcd get a backlight */

	if (lcd->lcd_cfg.lcd_bl_en_used || lcd->panel_para.lcd_pwm_used) {
		lcd->use_bl = 1;
#if defined(CONFIG_AW_DRM_BACKLIGHT)
		if (sunxi_lcd_backlight_init(lcd) < 0) {
			DRM_ERROR("sunxi_lcd_backlight_init failed\n");
			goto lcd_err;
		}
#endif
	}

/* initial lcd panel */
	lcd->panel = sunxi_lcd_get_panel(lcd->panel_para.lcd_model_name);
	if (!lcd->panel) {
		DRM_ERROR("get panel failed\n");
		goto lcd_err;
	}

	lcd->panel->func.cfg_panel_info(&lcd->extend_para);

	if (lcd->type == LCD_IF_DSI) {
		lcd->funcs = &sunxi_dsi_funcs;
	} else if (lcd->type == LCD_IF_LVDS) {
		lcd->funcs = &sunxi_lvds_funcs;
	}  else if (lcd->type == LCD_IF_HV) {
		lcd->funcs = &sunxi_hv_funcs;
	}  else if (lcd->type == LCD_IF_CPU) {
		/* lcd->funcs = &sunxi_cpu_funcs; */
	} else {
		DRM_ERROR("lcd type:%d haven't been Implement\n", lcd->type);
		goto lcd_err;
	}

/* increase lcd count */
	lcd_drv->lcd_cnt++;

	DRM_INFO("[SUNXI-LCD]lcd%d probe end\n", lcd->id);
	return 0;

lcd_err:
	/* lcd_drv->lcd_cnt++; */
	DRM_ERROR("probe failed\n");
	return -EINVAL;
}

static int sunxi_lcd_remove(struct platform_device *pdev)
{
#if defined(CONFIG_AW_DRM_BACKLIGHT)
	struct sunxi_lcd *lcd;
	int i = 0, lcd_cnt;

	lcd_cnt = sunxi_lcd_get_count();
	for (i = 0; i < lcd_cnt; i++) {
		lcd = sunxi_lcd_get_lcd(i);
		sunxi_lcd_backlight_remove(lcd);
	}

	sunxi_backlight_drv_exit();
#endif

	return 0;
}


static const struct of_device_id sunxi_lcd_match[] = {
	{ .compatible = "allwinner,sunxi-lcd", },
	{ .compatible = "allwinner,sunxi-lcd0", },
	{ .compatible = "allwinner,sunxi-lcd1", },

	/* generally, we at most use two lcd at the same time.
	 * but if you want to use more, and use these config below, and
	 * set lcd configs in sys_config.fex
	 */
	/*
	{ .compatible = "allwinner,sunxi-lcd2", },
	{ .compatible = "allwinner,sunxi-lcd3", },
	 */
	{},
};

struct platform_driver sunxi_lcd_platform_driver = {
	.probe = sunxi_lcd_probe,
	.remove = sunxi_lcd_remove,
	.driver = {
		   .name = "lcd",
		   .owner = THIS_MODULE,
		   .of_match_table = sunxi_lcd_match,
	},
};

int sunxi_lcd_module_init(void)
{
	int ret = 0, err;
	struct drv_model_info *drv_model;

	DRM_INFO("[SUNXI-LCD]sunxi_lcd_module_init\n");

	lcd_drv = kzalloc(sizeof(*lcd_drv), GFP_KERNEL);
	if (!lcd_drv) {
		DRM_ERROR("can NOT allocate memory for lcd_drv\n");
		goto lcd_err;
	}
	drv_model = &lcd_drv->drv_model;

	if (alloc_chrdev_region(&drv_model->devid, 0, 1, "lcd") < 0) {
		DRM_ERROR("alloc_chrdev_region failed\n");
		goto lcd_err;
	}

	drv_model->cdev = cdev_alloc();
	if (!drv_model->cdev) {
		DRM_ERROR("cdev_alloc failed\n");
		goto lcd_err;
	}

	cdev_init(drv_model->cdev, NULL);
	drv_model->cdev->owner = THIS_MODULE;
	err = cdev_add(drv_model->cdev, drv_model->devid, 1);
	if (err) {
		DRM_ERROR("cdev_add major number:%d failed\n",
						MAJOR(drv_model->devid));
		goto lcd_err;
	}

	drv_model->sysclass = class_create(THIS_MODULE, "lcd");
	if (IS_ERR(drv_model->sysclass)) {
		DRM_ERROR("create class error\n");
		goto lcd_err;
	}

	drv_model->dev = device_create(drv_model->sysclass, NULL,
					drv_model->devid, NULL, "lcd");
	if (!drv_model->dev) {
		DRM_ERROR("device_create failed\n");
		goto lcd_err;
	}

	ret = platform_driver_register(&sunxi_lcd_platform_driver);
	if (ret) {
		DRM_ERROR("platform_driver_register failed\n");
		goto lcd_err;
	}

	ret = sysfs_create_group(&drv_model->dev->kobj,
					&lcd_attribute_group);
	if (ret < 0) {
		DRM_ERROR("sysfs_create_file fail!\n");
		goto lcd_err;
	}

	DRM_INFO("[SUNXI-LCD]sunxi_lcd_module_init end\n\n");
	return 0;

lcd_err:
	DRM_ERROR("sunxi_lcd_module_init failed\n");
	kfree(lcd_drv);
	return -EINVAL;
}

void sunxi_lcd_module_exit(void)
{
	struct drv_model_info  *drv_model;

	DRM_INFO("[SUNXI-LCD]sunxi_lcd_module_exit\n");
	platform_driver_unregister(&sunxi_lcd_platform_driver);

	drv_model = &lcd_drv->drv_model;
	device_destroy(drv_model->sysclass, drv_model->devid);
	class_destroy(drv_model->sysclass);

	cdev_del(drv_model->cdev);
	if (lcd_drv)
		kfree(lcd_drv);
}

