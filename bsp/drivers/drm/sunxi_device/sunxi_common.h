/*
 * Copyright (C) 2016 Allwinnertech Co.Ltd
 * Authors: zhengwanyu
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#ifndef _SUNXI_COMMON_H_
#define _SUNXI_COMMON_H_
#include "disp_sys_intf.h"

struct sunxi_connector_work_mode {
	unsigned int color_fmt;
	unsigned int color_depth;
};

struct sunxi_dispdev_name {
	int device;
	char name[20];
};

/* linux driver model information */
struct drv_model_info {
	dev_t devid;
	struct cdev *cdev;
	struct device *dev;
	struct class *sysclass;
};

enum {
	COLOR_FMT_RGB444,
	COLOR_FMT_YUV444,
	COLOR_FMT_RGB422,
	COLOR_FMT_RGB420,
	COLOR_FMT_YUV422,
	COLOR_FMT_YUV420,
};

void sunxi_disp_bsp_init_para_init(void);
struct disp_bsp_init_para *sunxi_disp_get_bsp_init_para(void);

struct device_node *sunxi_drm_get_name_node(char *device_name);

int sunxi_drm_get_sys_item_gpio(struct device_node *node, char *sub_name,
					struct disp_gpio_info *gpio_info);
int sunxi_drm_get_sys_item_char(struct device_node *node, char *sub_name,
							    char *value);
int sunxi_drm_get_sys_item_int(struct device_node *node, char *sub_name,
							    int *value);

int sunxi_drm_sys_pin_set_state(char *dev_name, char *name);
int sunxi_drm_sys_gpio_set_direction(u32 p_handler, u32 direction,
				const char *gpio_name);
int sunxi_drm_sys_gpio_release(int p_handler);
int sunxi_drm_sys_gpio_request(struct disp_gpio_info *gpio_info);
int sunxi_drm_sys_gpio_set_value(u32 p_handler, u32 value_to_gpio,
					   const char *gpio_name);


int sunxi_drm_sys_power_disable(struct device *dev, const char *name); /* fix me */
int sunxi_drm_sys_power_enable(struct device *dev, const char *name); /* fix me */

void sunxi_drm_delayed_ms(unsigned int ms);
int bsp_disp_get_print_level(void);
void bsp_disp_set_print_level(unsigned char level);

int disp_delay_us(u32 us);
s32 disp_delay_ms(u32 ms);
int disp_sys_script_get_item(char *main_name,
	char *sub_name, int value[], int type);

#endif
