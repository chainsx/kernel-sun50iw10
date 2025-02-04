/*
 * linux-4.9/drivers/media/platform/sunxi-vfe/vfe_sundev.c
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
#include <linux/device.h>
#include <linux/module.h>

#include "vfe.h"
#include "vfe_os.h"
#include "vfe_subdev.h"
#include "platform_cfg.h"
#include "csi/sunxi_csi.h"


/*
 * called by subdev in power on/off sequency
 * must be called after update_ccm_info
 */
#ifdef VFE_PMU
static int iovdd_on_off_cnt;
#endif

#define CLK_OUT_CTRL_REG	0xf1c000f0

/* enable/disable pmic channel */
int vfe_set_pmu_channel(struct v4l2_subdev *sd, enum pmic_channel pmic_ch, enum on_off on_off)
{
#ifdef VFE_PMU
	struct vfe_dev *dev = (struct vfe_dev *)dev_get_drvdata(sd->v4l2_dev->dev);
	struct regulator *pmic;
	int ret;

	switch (pmic_ch) {
	case IOVDD:
		pmic = dev->power->iovdd;
		if (pmic) {
			ret = regulator_set_voltage(pmic, dev->power->iovdd_vol, 3300000);
			vfe_dbg(0, "set regulator iovdd = %d, return %x\n", dev->power->iovdd_vol, ret);
		}
		break;
	case DVDD:
		pmic = dev->power->dvdd;
		if (pmic) {
			ret = regulator_set_voltage(pmic, dev->power->dvdd_vol, 3300000);
			vfe_dbg(0, "set regulator dvdd = %d, return %x\n", dev->power->dvdd_vol, ret);
		}
		break;
	case AVDD:
		pmic = dev->power->avdd;
		if (pmic) {
			ret = regulator_set_voltage(pmic, dev->power->avdd_vol, 3300000);
			vfe_dbg(0, "set regulator avdd = %d, return %x\n", dev->power->avdd_vol, ret);
		}
		break;
	case AFVDD:
		pmic = dev->power->afvdd;
		if (pmic) {
			ret = regulator_set_voltage(pmic, dev->power->afvdd_vol, 3300000);
			vfe_dbg(0, "set regulator afvdd = %d, return %x\n", dev->power->afvdd_vol, ret);
		}
		break;
	case FLVDD:
		pmic = dev->power->flvdd;
		if (pmic) {
			ret = regulator_set_voltage(pmic, dev->power->flvdd_vol, 3300000);
			vfe_dbg(0, "set regulator flvdd = %d, return %x\n", dev->power->flvdd_vol, ret);
		}
		break;
	default:
		pmic = NULL;
	}
	if (on_off == OFF) {
		if (pmic) {
			if (!regulator_is_enabled(pmic))
				vfe_dbg(0, "regulator_is already disabled\n");
			if (pmic_ch == IOVDD) {
				iovdd_on_off_cnt--;
				vfe_dbg(0, "iovdd_on_off_cnt = %d! return\n", iovdd_on_off_cnt);
			}
			return regulator_disable(pmic);
		}
	} else {
		if (pmic) {
			if (regulator_is_enabled(pmic))
				vfe_dbg(0, "regulator_is already enabled\n");
			if (pmic_ch == IOVDD) {
				iovdd_on_off_cnt++;
				vfe_dbg(0, "iovdd_on_off_cnt = %d!\n", iovdd_on_off_cnt);
			}
			return regulator_enable(pmic);
		}
	}

#endif
	return 0;
}
EXPORT_SYMBOL_GPL(vfe_set_pmu_channel);

/* enable/disable master clock */
int vfe_set_mclk(struct v4l2_subdev *sd, enum on_off on_off)
{
#ifdef VFE_CLK
	struct vfe_dev *dev = (struct vfe_dev *)dev_get_drvdata(sd->v4l2_dev->dev);
	struct csi_dev *csi = v4l2_get_subdevdata(dev->csi_sd);
	char pin_name[20] = "";
	struct pinctrl *mclk_pin;

	switch (on_off) {
	case ON:
		sprintf(pin_name, "mclk%d-default", csi->id);
		break;
	case OFF:
		sprintf(pin_name, "mclk%d-sleep", csi->id);
		break;
	default:
		return -1;
	}

	mclk_pin = devm_pinctrl_get_select(&dev->pdev->dev, pin_name);
	if (IS_ERR_OR_NULL(mclk_pin)) {
		vfe_err("mclk%d request pin handle failed!\n", csi->id);
		return -EINVAL;
	}

	switch (on_off) {
	case ON:
		vfe_print("mclk on\n");
#if defined CONFIG_ARCH_SUN8IW8P1
		dev->gpio[MCLK_PIN].mul_sel = 3; /*set mclk PIN to MCLK func.*/
#elif defined CONFIG_ARCH_SUN8IW10P1
		dev->gpio[MCLK_PIN].mul_sel = 4; /*set mclk PIN to MCLK func.*/
#elif defined CONFIG_ARCH_SUN8IW11
		dev->gpio[MCLK_PIN].mul_sel = 3; /*set mclk PIN to MCLK func.*/
#else
		dev->gpio[MCLK_PIN].mul_sel = 2; /*set mclk PIN to MCLK func.*/
#endif
		os_gpio_set(&dev->gpio[MCLK_PIN], 1);
		usleep_range(10000, 12000);
		if (csi->clock[CSI_MASTER_CLK]) {
			if (clk_prepare_enable(csi->clock[CSI_MASTER_CLK])) {
				vfe_err("csi%d master clock enable error\n", csi->id);
				return -1;
			}
#ifdef CONFIG_ARCH_SUN3IW1P1
			/*CLK_OUT enable, CLK_OUT_SRC=OSC24M, DIV=0*/
			writel(0x82000000, CLK_OUT_CTRL_REG);
#endif
		} else {
			vfe_err("csi%d master clock is null\n", csi->id);
			return -1;
		}
		break;
	case OFF:
		vfe_print("mclk off\n");
		if (csi->clock[CSI_MASTER_CLK]) {
#ifdef CONFIG_ARCH_SUN3IW1P1
			/*CLK_OUT disable, CLK_OUT_SRC=LOSC, DIV=0*/
			writel(0x00000000, CLK_OUT_CTRL_REG);
#endif
			clk_disable_unprepare(csi->clock[CSI_MASTER_CLK]);
		} else {
			vfe_err("csi%d master clock is null\n", csi->id);
			return -1;
		}
		usleep_range(10000, 12000);
		dev->gpio[MCLK_PIN].mul_sel = GPIO_OUTPUT; /* set mclk PIN to output. */
		os_gpio_set(&dev->gpio[MCLK_PIN], 1);
		vfe_gpio_write(sd, MCLK_PIN, 0);
		break;
	default:
		return -1;
	}
#endif
	return 0;
}
EXPORT_SYMBOL_GPL(vfe_set_mclk);

/* set frequency of master clock */
int vfe_set_mclk_freq(struct v4l2_subdev *sd, unsigned long freq)
{
#ifdef VFE_CLK
	struct vfe_dev *dev = (struct vfe_dev *)dev_get_drvdata(sd->v4l2_dev->dev);
	struct csi_dev *csi = v4l2_get_subdevdata(dev->csi_sd);
	struct clk *master_clk_src;

	if (freq == 24000000 || freq == 12000000 || freq == 6000000) {
			if (csi->clock[CSI_MASTER_CLK_24M_SRC]) {
				master_clk_src = csi->clock[CSI_MASTER_CLK_24M_SRC];
			} else {
				vfe_err("csi master clock 24M source is null\n");
				return -1;
			}
	} else {
		if (csi->clock[CSI_MASTER_CLK_PLL_SRC]) {
			master_clk_src = csi->clock[CSI_MASTER_CLK_PLL_SRC];
		} else {
			vfe_err("csi master clock pll source is null\n");
			return -1;
		}
	}

	if (csi->clock[CSI_MASTER_CLK]) {
		if (clk_set_parent(csi->clock[CSI_MASTER_CLK], master_clk_src)) {
			vfe_err("set vfe master clock source failed!!!\n");
			return -1;
		}
	} else {
		vfe_err("csi master clock is null\n");
		return -1;
	}

	if (csi->clock[CSI_MASTER_CLK]) {
		if (clk_set_rate(csi->clock[CSI_MASTER_CLK], freq)) {
			vfe_err("set csi%d master clock error\n", csi->id);
			return -1;
		}
	} else {
		vfe_err("csi master clock is null\n");
		return -1;
	}
	vfe_print("mclk set rate %ld, get rate %ld\n", freq, clk_get_rate(csi->clock[CSI_MASTER_CLK]));

#endif
	return 0;
}
EXPORT_SYMBOL_GPL(vfe_set_mclk_freq);

/* set the gpio io status */
int vfe_gpio_write(struct v4l2_subdev *sd, enum gpio_type gpio_type, unsigned int status)
{
#ifdef VFE_GPIO
	int force_value_flag = 1;
	struct vfe_dev *dev = (struct vfe_dev *)dev_get_drvdata(sd->v4l2_dev->dev);

	u32 gpio = dev->gpio[gpio_type].gpio;
#ifndef CONFIG_ARCH_SUN3IW1P1
	if ((gpio_type == PWDN) || (gpio_type == RESET))
		force_value_flag = 0;
#endif
	return os_gpio_write(gpio, status, NULL, force_value_flag);
#else
	return 0;
#endif
}
EXPORT_SYMBOL_GPL(vfe_gpio_write);

/* set the gpio io status */
int vfe_gpio_set_status(struct v4l2_subdev *sd, enum gpio_type gpio_type, unsigned int status)
{
#ifdef VFE_GPIO
	struct vfe_dev *dev = (struct vfe_dev *)dev_get_drvdata(sd->v4l2_dev->dev);
	u32 gpio = dev->gpio[gpio_type].gpio;

	return os_gpio_set_status(gpio, status, NULL);
#else
	return 0;
#endif
}
EXPORT_SYMBOL_GPL(vfe_gpio_set_status);

/* get standby mode */
void vfe_get_standby_mode(struct v4l2_subdev *sd, enum standby_mode *stby_mode)
{
	struct vfe_dev *dev = (struct vfe_dev *)dev_get_drvdata(sd->v4l2_dev->dev);

	*stby_mode = dev->power->stby_mode;
}
EXPORT_SYMBOL_GPL(vfe_get_standby_mode);

MODULE_AUTHOR("raymonxiu");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Video front end subdev for sunxi");
