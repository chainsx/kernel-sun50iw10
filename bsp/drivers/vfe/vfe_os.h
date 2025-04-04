/*
 * linux-4.9/drivers/media/platform/sunxi-vfe/vfe_os.h
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


/*
 * sunxi operation system resource
 * Author:raymonxiu
 */
#ifndef __VFE__OS__H__
#define __VFE__OS__H__

#include <linux/device.h>
#include <linux/clk.h>
//#include <linux/clk/sunxi.h>
#include <linux/interrupt.h>
/* #include <linux/gpio.h> */
#include "platform_cfg.h"
#include <linux/dma-mapping.h>  /* just include"PAGE_SIZE" macro */


extern unsigned int vfe_dbg_en;
extern unsigned int vfe_dbg_lv;

#define VFE_NOT_ADDR      -1

/* for internal driver debug */
#define vfe_dbg(l, x, arg...) ({if (vfe_dbg_en && (l <= vfe_dbg_lv)) pr_debug("[VFE_DEBUG]"x, ##arg); })
/* print when error happens */
#define vfe_err(x, arg...) pr_err("[VFE_ERR]"x, ##arg)
#define vfe_warn(x, arg...) pr_warn("[VFE_WARN]"x, ##arg)
/* print unconditional, for important info */
#define vfe_print(x, arg...) pr_info("[VFE]"x, ##arg)

struct vfe_mm {
	size_t size;
	void *phy_addr;
	void *vir_addr;
	void *dma_addr;
	struct ion_client *client;
	struct ion_handle *handle;
};

struct vfe_gpio_cfg {
	u32 gpio;
	u32 mul_sel;
	u32 pull;
	u32 drv_level;
	u32 data;
};

extern int os_gpio_request(struct vfe_gpio_cfg *gpio_list, __u32 group_count_max);
extern int os_gpio_set(struct vfe_gpio_cfg *gpio_list, __u32 group_count_max);
extern int os_gpio_release(u32 p_handler, __s32 if_release_to_default_status);
extern int os_gpio_write(u32 p_handler, __u32 value_to_gpio, const char *gpio_name, int force_value_flag);
extern int os_gpio_set_status(u32 p_handler, __u32 if_set_to_output_status, const char *gpio_name);
extern int os_mem_alloc(struct device *dev, struct vfe_mm *mem_man);
extern void os_mem_free(struct device *dev, struct vfe_mm *mem_man);

#endif /* __VFE__OS__H__ */
