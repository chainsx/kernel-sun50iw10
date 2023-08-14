/*
 * Copyright (C) 2016 Allwinnertech Co.Ltd
 * Authors: Jet Cui
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#include <linux/clk-provider.h>
#include <linux/gpio.h>
#include "sunxi-gpio.h"
#include <linux/pinctrl/consumer.h>
#include <linux/pwm.h>
#include <linux/regulator/consumer.h>
#include <drm/drm_print.h>

#include "de/include.h"
#include "disp_sys_intf.h"

/* it is the structure that connect to sunxi display lowlevel */
static struct disp_bsp_init_para disp_init_para;

/* For log printing level of sunxi disp lowlevel */
static unsigned char disp_log_level;

void sunxi_disp_bsp_init_para_init(void)
{
	memset(&disp_init_para, 0, sizeof(disp_init_para));
}

struct disp_bsp_init_para *sunxi_disp_get_bsp_init_para(void)
{
	return &disp_init_para;
}

void sunxi_drm_delayed_ms(unsigned int ms)
{
	unsigned int timeout = msecs_to_jiffies(ms);

	set_current_state(TASK_UNINTERRUPTIBLE);
	schedule_timeout(timeout);
}

int sunxi_drm_sys_power_enable(struct device *dev, const char *name) /* fix me */
{
	int ret = 0;

//#ifdef CONFIG_AW_AXP
	struct regulator *regu = NULL;

	regu = regulator_get(dev, name);

	if (IS_ERR(regu)) {
		DRM_ERROR("fail to get regulator %s\n", name);
		goto exit;
	}

	ret = regulator_enable(regu);
	if (ret) {
		DRM_ERROR("fail to enable regulator %s!\n", name);
		goto exit1;
	} /* else
		DRM_INFO("suceess to enable regulator %s!\n", name);
	   */
	return ret;

exit1:
	regulator_put(regu);
exit:
//#endif
	return ret;
}

int sunxi_drm_sys_power_disable(struct device *dev, const char *name) /* fix me */
{
	int ret = 0;
//#ifdef CONFIG_AW_AXP
	struct regulator *regu = NULL;

	regu = regulator_get(dev, name);

	if (IS_ERR(regu)) {
		DRM_ERROR("fail to get regulator %s\n", name);
		goto exit;
	}

	ret = regulator_disable(regu);
	if (ret) {
		DRM_ERROR("fail to disable regulator %s!\n", name);
		goto exit1;
	} else
		DRM_INFO("suceess to disable regulator %s!\n", name);


exit1:
	regulator_put(regu);
exit:
//#endif
	return ret;
}

int sunxi_drm_sys_gpio_request(struct disp_gpio_info *gpio_info)
{
	int ret = 0;

	if (!gpio_info) {
		pr_err("%s: gpio_info is null\n", __func__);
		return -1;
	}

	if (!strlen(gpio_info->name))
		return 0;

	if (!gpio_is_valid(gpio_info->gpio)) {
		pr_err("%s: gpio (%d) is invalid\n", __func__, gpio_info->gpio);
		return -1;
	}

	ret = gpio_direction_output(gpio_info->gpio, gpio_info->value);
	if (ret) {
		pr_err("%s failed, gpio_name=%s, gpio=%d, value=%d, ret=%d\n", __func__,
				gpio_info->name, gpio_info->gpio, gpio_info->value, ret);
		return -1;
	}

	DRM_INFO("%s, gpio_name=%s, gpio=%d, value=%d, ret=%d\n", __func__,
			gpio_info->name, gpio_info->gpio, gpio_info->value, ret);

	return ret;
}

/* direction: 0:input, 1:output */
int sunxi_drm_sys_gpio_set_direction(u32 p_handler, u32 direction,
				const char *gpio_name)
{
	int ret = -1;

	if (p_handler) {
		if (direction) {
			s32 value;

			value = __gpio_get_value(p_handler);
			ret = gpio_direction_output(p_handler, value);
			if (ret != 0)
				__wrn("gpio_direction_output fail!\n");
		} else {
			ret = gpio_direction_input(p_handler);
			if (ret != 0)
				__wrn("gpio_direction_input fail!\n");
		}
	} else {
		__wrn("OSAL_GPIO_DevSetONEPIN_IO_STATUS, hdl is NULL\n");
		ret = -1;
	}
	return ret;
}

int sunxi_drm_sys_gpio_set_value(u32 p_handler, u32 value_to_gpio,
	const char *gpio_name)
{
	DRM_DEBUG_DRIVER("%s  gpio_name:%s\n", __func__, gpio_name);

	if (p_handler)
		__gpio_set_value(p_handler, value_to_gpio);
	else
		__wrn("OSAL_GPIO_DevWRITE_ONEPIN_DATA, hdl is NULL\n");

	return 0;
}


int sunxi_drm_sys_gpio_release(int p_handler)
{
	if (p_handler)
		gpio_free(p_handler);
	else
		DRM_INFO("OSAL_GPIO_Release, hdl is NULL\n");

	return 0;
}

int sunxi_drm_sys_pin_set_state(char *dev_name, char *name)
{
	char compat[32];
	u32 len = 0;
	struct device_node *node;
	struct platform_device *pdev;
	struct pinctrl *pctl;
	struct pinctrl_state *state;
	int ret = -1;

	len = sprintf(compat, "allwinner,sunxi-%s", dev_name);
	if (len > 32)
		DRM_ERROR("size of mian_name is out of range\n");

	node = of_find_compatible_node(NULL, NULL, compat);
	if (!node) {
		DRM_ERROR("of_find_compatible_node %s fail\n", compat);
		goto exit;
	}

	pdev = of_find_device_by_node(node);
	if (!node) {
		DRM_ERROR("of_find_device_by_node for %s fail\n", compat);
		goto exit;
	}
	pctl = pinctrl_get(&pdev->dev);
	if (IS_ERR(pctl)) {
		DRM_INFO("[WARN]can NOT get pinctrl for %s \n", compat);
		ret = 0;
		goto exit;
	}

	state = pinctrl_lookup_state(pctl, name);
	if (IS_ERR(state)) {
		DRM_ERROR("pinctrl_lookup_state for %s fail\n", compat);
		ret = PTR_ERR(state);
		goto exit;
	}

	ret = pinctrl_select_state(pctl, state);
	if (ret < 0) {
		DRM_ERROR("pinctrl_select_state(%s) for %s fail\n",
			name, compat);
		goto exit;
	}
	ret = 0;

exit:
	return ret;
}

int sunxi_drm_get_sys_item_int(struct device_node *node,
	char *sub_name, int *value)
{

	return of_property_read_u32_array(node, sub_name, value, 1);
}

int sunxi_drm_get_sys_item_char(struct device_node *node,
	char *sub_name, char *value)
{
	const char *str;

	if (of_property_read_string(node, sub_name, &str)) {
		DRM_DEBUG_DRIVER("failed to get [%s] string.\n", sub_name);
		return  -EINVAL;
	}

	memcpy((void *)value, str, strlen(str)+1);

	return 0;
}

int sunxi_drm_get_sys_item_gpio(struct device_node *node,
	char *sub_name, struct disp_gpio_info *gpio_info)
{
	int gpio;
	enum of_gpio_flags flags;

	gpio = of_get_named_gpio_flags(node, sub_name, 0, &flags);
	if (!gpio_is_valid(gpio)) {
		DRM_DEBUG_DRIVER("There is NO gpio[%s] in DTS, "
			"failed to get it.\n", sub_name);
		return -EINVAL;
	}

	gpio_info->gpio = gpio;
	gpio_info->value = (flags == OF_GPIO_ACTIVE_LOW) ? 0 : 1;
	memcpy(gpio_info->name, sub_name, strlen(sub_name) + 1);

	return 0;
}

struct device_node *sunxi_drm_get_name_node(char *device_name)
{
	struct device_node *node = NULL;
	char compat[32];
	u32 len = 0;

	len = sprintf(compat, "allwinner,%s", device_name);
	if (len > 32)
		DRM_INFO("size of mian_name is out of range\n");

	node = of_find_compatible_node(NULL, NULL, compat);
	if (!node) {
		DRM_ERROR("There is NO dts node %s fail\n", compat);
		return NULL;
	}

	return node;
}

int bsp_disp_get_print_level(void)
{
	return disp_log_level;
}

void bsp_disp_set_print_level(unsigned char level)
{
	disp_log_level = level;
}

int disp_delay_us(u32 us)
{
	udelay(us);
	return 0;
}

s32 disp_delay_ms(u32 ms)
{
	sunxi_drm_delayed_ms(ms);
	return 0;
}

int disp_sys_script_get_item(char *main_name,
	char *sub_name, int value[], int type)
{
	struct device_node *node;
	char compat[32];
	u32 len = 0;

	len = sprintf(compat, "sunxi-%s", main_name);
	node = sunxi_drm_get_name_node(compat);
	if (!node) {
		DRM_ERROR("get [%s] item err.\n", main_name);
		return -EINVAL;
	}
	switch (type) {
	case 1:
		if (sunxi_drm_get_sys_item_int(node, sub_name, value))
			return 0;
		return type;
	case 2:
		if (sunxi_drm_get_sys_item_char(node, sub_name, (char *)value))
			return 0;
		return type;
	case 3:
		if (sunxi_drm_get_sys_item_gpio(node, sub_name,
			(struct disp_gpio_info *)value))
			return 0;
		return type;
	default:
		return 0;
	}
}

