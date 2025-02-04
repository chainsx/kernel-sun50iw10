/*
 * drivers/video/fbdev/sunxi/lcd_fb/dev_lcd_fb/dev_lcd_fb.c
 *
 * Copyright (c) 2007-2019 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
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
#include "include.h"
#include "./panels/panels.h"
#include "disp_lcd.h"
#include "disp_display.h"
#include "dev_fb.h"
#include "dev_lcd_fb.h"

struct dev_lcd_fb_t g_drv_info;


extern int lcd_init(void);



static struct attribute *lcd_fb_attributes[] = {
	NULL
};

static struct attribute_group lcd_fb_attribute_group = {
	.name = "attr",
	.attrs = lcd_fb_attributes
};

static void start_work(struct work_struct *work)
{
	int i = 0;
	struct lcd_fb_device *dispdev = NULL;

	for (i = 0; i < SUPPORT_MAX_LCD; ++i) {
		dispdev = lcd_fb_device_get(i);
		if (dispdev)
			dispdev->enable(dispdev);
	}

}

static s32 start_process(void)
{
	flush_work(&g_drv_info.start_work);
	schedule_work(&g_drv_info.start_work);
	return 0;
}

static int lcd_fb_probe(struct platform_device *pdev)
{
	int ret = 0;

	g_drv_info.device = &pdev->dev;
	lcd_fb_wrn("\n");
	ret = sysfs_create_group(&pdev->dev.kobj, &lcd_fb_attribute_group);
	INIT_WORK(&g_drv_info.start_work, start_work);

	disp_init_lcd(&g_drv_info);

	lcd_init();

	fb_init(&g_drv_info);

	start_process();
	return ret;
}

static void lcd_fb_shutdown(struct platform_device *pdev)
{
	int i = 0;
	struct lcd_fb_device *dispdev = NULL;

	for (i = 0; i < SUPPORT_MAX_LCD; ++i) {
		dispdev = lcd_fb_device_get(i);
		if (dispdev)
			dispdev->disable(dispdev);
	}
	lcd_fb_wrn("Finish\n");
}

static int lcd_fb_remove(struct platform_device *pdev)
{
	int ret = 0;

	lcd_fb_shutdown(pdev);
	fb_exit();
	disp_exit_lcd();
	sysfs_remove_group(&pdev->dev.kobj, &lcd_fb_attribute_group);
	platform_set_drvdata(pdev, NULL);
	return ret;
}


static const struct of_device_id lcd_fb_match[] = {
	{.compatible = "allwinner,sunxi-lcd_fb0",},
	{},
};

int lcd_fb_suspend(struct device *dev)
{
	int i = 0;
	int ret = 0;
	struct lcd_fb_device *dispdev = NULL;

	for (i = 0; i < SUPPORT_MAX_LCD; ++i) {
		dispdev = lcd_fb_device_get(i);
		if (dispdev)
			ret = dispdev->disable(dispdev);
	}
	return ret;
}

int lcd_fb_resume(struct device *dev)
{
	int i = 0;
	int ret = 0;
	struct lcd_fb_device *dispdev = NULL;

	for (i = 0; i < SUPPORT_MAX_LCD; ++i) {
		dispdev = lcd_fb_device_get(i);
		if (dispdev)
			ret = dispdev->enable(dispdev);
	}
	return ret;
}

static const struct dev_pm_ops lcd_fb_runtime_pm_ops = {
	.suspend = lcd_fb_suspend,
	.resume = lcd_fb_resume,
};

struct platform_driver lcd_fb_driver = {
	.probe = lcd_fb_probe,
	.remove = lcd_fb_remove,
	.shutdown = lcd_fb_shutdown,
	.driver = {
		.name = "lcd_fb",
		.owner = THIS_MODULE,
		.pm             = &lcd_fb_runtime_pm_ops,
		.of_match_table = lcd_fb_match,
	},
};

static int __init lcd_fb_init(void)
{
	s32 ret = -1;
	lcd_fb_wrn("\n");
	ret = platform_driver_register(&lcd_fb_driver);
	return ret;
}

static void __exit lcd_fb_exit(void)
{
	platform_driver_unregister(&lcd_fb_driver);
	lcd_fb_wrn("\n");
}

int sunxi_disp_get_source_ops(struct sunxi_disp_source_ops *src_ops)
{
	memset((void *)src_ops, 0, sizeof(*src_ops));

	src_ops->sunxi_lcd_set_panel_funs = bsp_disp_lcd_set_panel_funs;
	src_ops->sunxi_lcd_delay_ms = disp_delay_ms;
	src_ops->sunxi_lcd_delay_us = disp_delay_us;
	src_ops->sunxi_lcd_backlight_enable = bsp_disp_lcd_backlight_enable;
	src_ops->sunxi_lcd_backlight_disable = bsp_disp_lcd_backlight_disable;
	src_ops->sunxi_lcd_pwm_enable = bsp_disp_lcd_pwm_enable;
	src_ops->sunxi_lcd_pwm_disable = bsp_disp_lcd_pwm_disable;
	src_ops->sunxi_lcd_power_enable = bsp_disp_lcd_power_enable;
	src_ops->sunxi_lcd_power_disable = bsp_disp_lcd_power_disable;
	src_ops->sunxi_lcd_pin_cfg = bsp_disp_lcd_pin_cfg;
	src_ops->sunxi_lcd_gpio_set_value = bsp_disp_lcd_gpio_set_value;
	src_ops->sunxi_lcd_gpio_set_direction = bsp_disp_lcd_gpio_set_direction;
	src_ops->sunxi_lcd_cmd_write = bsp_disp_lcd_cmd_write;
	src_ops->sunxi_lcd_para_write = bsp_disp_lcd_para_write;
	src_ops->sunxi_lcd_cmd_read = bsp_disp_lcd_cmd_read;
	return 0;
}

module_init(lcd_fb_init);
module_exit(lcd_fb_exit);

MODULE_AUTHOR("te <zhengxiaobin@allwinnertech.com>");
MODULE_DESCRIPTION("lcd fb module");
MODULE_LICENSE("GPL");
