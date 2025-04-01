 /*
 * Copyright (C) 2019 Allwinnertech Co.Ltd
 * Authors: zhengwanyu
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */
#include "../sunxi_drm_connector.h"
#ifndef _SUNXI_BACKLIGHT_H_
#define _SUNXI_BACKLIGHT_H_

#include <linux/pwm.h>
#include <linux/backlight.h>

#define	BL_NUM_MAX	2

struct pwm_config {
	unsigned int lcd_pwm_ch;
	unsigned int lcd_pwm_freq;
	unsigned int lcd_pwm_pol;
};

struct backlight_config {
	unsigned char use_bl_en;
	struct disp_gpio_info bl_en_gpio;
	char bl_en_power[32];

	unsigned char use_pwm;
	unsigned long long bright;
	struct pwm_config pwm;
};

struct pwm_info {
#if IS_ENABLED(CONFIG_AW_PWM) || IS_ENABLED(CONFIG_PWM_SUNXI_NEW) || IS_ENABLED(CONFIG_PWM_SUNXI_GROUP)
	struct pwm_device *pwm_dev;
#endif
	u32 channel;
	u32 polarity;
	u32 period_ns;
	u32 duty_ns;
	u32 enabled;
};

struct sunxi_lcd_backlight {
	struct device *dev;
	/* CONFIG_BACKLIGHT_CLASS_DEVICE */
	struct backlight_device *bl_dev;
	int lcd_id;

	unsigned char use_bl_en;
	struct disp_gpio_info bl_en_gpio;
	int bl_gpio_hdl;
	char bl_en_power[32];
	/* indicate if this backlight is enable,
	 * and how many time it enable
	 */
	unsigned char bl_en_count;

	unsigned char use_pwm;
	unsigned int bright_curve_tbl[256];
	unsigned long long bright;
	struct pwm_info pwm_info;

	unsigned int dimming;

	/* backlight registers and fields in struct intel_panel */
	struct mutex backlight_lock;
};

ssize_t sunxi_backlight_info(char *buf);

int sunxi_backlight_init(int lcd_id, struct backlight_config *config,
					unsigned int *bl_curve_tbl);
void sunxi_backlight_remove(int lcd_id);

int sunxi_backlight_drv_init(void);
int sunxi_backlight_drv_exit(void);

int sunxi_backlight_pwm_enable(unsigned int lcd_id);
int sunxi_backlight_pwm_disable(unsigned int lcd_id);
int sunxi_backlight_enable(unsigned int lcd_id);
int sunxi_backlight_disable(unsigned int lcd_id);

int sunxi_backlight_device_register(struct device *device,
		unsigned int lcd_id);
void sunxi_backlight_device_unregister(unsigned int lcd_id);
#endif
