/*
 * linux-4.9/drivers/media/platform/sunxi-vfe/vfe_os.c
 *
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include "vfe_os.h"

unsigned int vfe_dbg_en;
unsigned int vfe_dbg_lv = 1;
EXPORT_SYMBOL_GPL(vfe_dbg_en);
EXPORT_SYMBOL_GPL(vfe_dbg_lv);

int os_gpio_request(struct vfe_gpio_cfg *gpio_list, __u32 group_count_max)
{
#ifdef VFE_GPIO
	int ret = 0;
	struct gpio_config pin_cfg;

	if (gpio_list == NULL)
		return -1;

	if (gpio_list->gpio == GPIO_INDEX_INVALID)
		return -1;

	pin_cfg.gpio = gpio_list->gpio;
	pin_cfg.mul_sel = gpio_list->mul_sel;
	pin_cfg.pull = gpio_list->pull;
	pin_cfg.drv_level = gpio_list->drv_level;
	pin_cfg.data = gpio_list->data;
	ret = gpio_request(pin_cfg.gpio, NULL);
	if (ret != 0) {
		vfe_warn("os_gpio_request failed, gpio=%d, ret=0x%x, %d\n", gpio_list->gpio, ret, ret);
		return -1;
	}
	return 0;
#else
	return 0;
#endif
}
EXPORT_SYMBOL_GPL(os_gpio_request);

int os_gpio_set(struct vfe_gpio_cfg *gpio_list, __u32 group_count_max)
{
/* #ifdef VFE_GPIO */
#if 0
	struct gpio_config pin_cfg;
	char pin_name[32];
	__u32 config;

	if (gpio_list == NULL)
		return -1;
	if (gpio_list->gpio == GPIO_INDEX_INVALID)
		return -1;

	pin_cfg.gpio = gpio_list->gpio;
	pin_cfg.mul_sel = gpio_list->mul_sel;
	pin_cfg.pull = gpio_list->pull;
	pin_cfg.drv_level = gpio_list->drv_level;
	pin_cfg.data = gpio_list->data;

	if (!IS_AXP_PIN(pin_cfg.gpio)) {
		/* valid pin of sunxi-pinctrl,
		 * config pin attributes individually.
		 */
		sunxi_gpio_to_name(pin_cfg.gpio, pin_name);
		config = PIN_CONF_PACKED(SUNXI_PINCFG_TYPE_FUNC, pin_cfg.mul_sel);
		pin_config_set(SUNXI_PINCTRL, pin_name, config);
		if (pin_cfg.pull != GPIO_PULL_DEFAULT) {
			config = PIN_CONF_PACKED(SUNXI_PINCFG_TYPE_PUD, pin_cfg.pull);
			pin_config_set(SUNXI_PINCTRL, pin_name, config);
		}
		if (pin_cfg.drv_level != GPIO_DRVLVL_DEFAULT) {
			config = PIN_CONF_PACKED(SUNXI_PINCFG_TYPE_DRV, pin_cfg.drv_level);
			pin_config_set(SUNXI_PINCTRL, pin_name, config);
		}
		if (pin_cfg.data != GPIO_DATA_DEFAULT) {
			config = PIN_CONF_PACKED(SUNXI_PINCFG_TYPE_DAT, pin_cfg.data);
			pin_config_set(SUNXI_PINCTRL, pin_name, config);
		}
	} else if (IS_AXP_PIN(pin_cfg.gpio)) {
		/* valid pin of axp-pinctrl,
		 * config pin attributes individually.
		 */
		sunxi_gpio_to_name(pin_cfg.gpio, pin_name);
		config = PIN_CONF_PACKED(SUNXI_PINCFG_TYPE_FUNC, pin_cfg.mul_sel);
		pin_config_set(AXP_PINCTRL, pin_name, config);
		if (pin_cfg.data != GPIO_DATA_DEFAULT) {
			config = PIN_CONF_PACKED(SUNXI_PINCFG_TYPE_DAT, pin_cfg.data);
			pin_config_set(AXP_PINCTRL, pin_name, config);
		}
	} else {
		vfe_warn("invalid pin [%d] from sys-config\n", pin_cfg.gpio);
		return -1;
	}
	return 0;
#else
	return 0;
#endif
}
EXPORT_SYMBOL_GPL(os_gpio_set);

int os_gpio_release(u32 p_handler, __s32 if_release_to_default_status)
{
#ifdef VFE_GPIO
	if (p_handler != GPIO_INDEX_INVALID)
		gpio_free(p_handler);
	else
		vfe_dbg(0, "os_gpio_release, hdl is INVALID\n");
#endif
	return 0;
}
EXPORT_SYMBOL_GPL(os_gpio_release);

int os_gpio_write(u32 p_handler, __u32 value_to_gpio, const char *gpio_name, int force_value_flag)
{
#ifdef VFE_GPIO
	if (force_value_flag == 1) {
		if (p_handler != GPIO_INDEX_INVALID)
			__gpio_set_value(p_handler, value_to_gpio);
		else
			vfe_dbg(0, "os_gpio_write, hdl is INVALID\n");
	} else {
		if (p_handler != GPIO_INDEX_INVALID) {
			if (value_to_gpio == 0) {
				os_gpio_set_status(p_handler, 1, gpio_name);
				__gpio_set_value(p_handler, value_to_gpio);
			} else
				os_gpio_set_status(p_handler, 0, gpio_name);
		} else
			vfe_dbg(0, "os_gpio_write, hdl is INVALID\n");
	}
#endif
	return 0;
}
EXPORT_SYMBOL_GPL(os_gpio_write);


int os_gpio_set_status(u32 p_handler, __u32 if_set_to_output_status, const char *gpio_name)
{
	int ret = 0;
#ifdef VFE_GPIO
	if (p_handler != GPIO_INDEX_INVALID) {
		if (if_set_to_output_status) {
			ret = gpio_direction_output(p_handler, 0);
			if (ret != 0)
				vfe_warn("gpio_direction_output fail!\n");
		} else {
			ret = gpio_direction_input(p_handler);
			if (ret != 0)
				vfe_warn("gpio_direction_input fail!\n");
		}
	} else {
		vfe_warn("os_gpio_set_status, hdl is INVALID\n");
		ret = -1;
	}
#endif
	return ret;
}
EXPORT_SYMBOL_GPL(os_gpio_set_status);

int os_mem_alloc(struct device *dev, struct vfe_mm *mem_man)
{
	mem_man->vir_addr = dma_alloc_coherent(dev, (size_t)mem_man->size,
			(dma_addr_t *)&mem_man->phy_addr, GFP_KERNEL);
	if (!mem_man->vir_addr) {
		vfe_err("dma_alloc_coherent memory alloc size %d failed\n", mem_man->size);
		return -ENOMEM;
	}
	mem_man->dma_addr = mem_man->phy_addr + HW_DMA_OFFSET-CPU_DRAM_PADDR_ORG;

	return 0;
}
EXPORT_SYMBOL_GPL(os_mem_alloc);

void os_mem_free(struct device *dev, struct vfe_mm *mem_man)
{
	if (mem_man->vir_addr)
		dma_free_coherent(dev, mem_man->size, mem_man->vir_addr, (dma_addr_t)mem_man->phy_addr);

	mem_man->phy_addr = NULL;
	mem_man->dma_addr = NULL;
	mem_man->vir_addr = NULL;
}
EXPORT_SYMBOL_GPL(os_mem_free);

MODULE_AUTHOR("raymonxiu");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Video front end OSAL for sunxi");
