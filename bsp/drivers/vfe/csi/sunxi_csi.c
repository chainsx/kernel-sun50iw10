/*
 * linux-4.9/drivers/media/platform/sunxi-vfe/csi/sunxi_csi.c
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
 ***************************************************************************************
 *
 * sunxi_csi.c
 *
 * Hawkview ISP - sunxi_csi.c module
 *
 * Copyright (c) 2015 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Version		  Author         Date		    Description
 *
 *   3.0		  Yang Feng	2015/02/27	ISP Tuning Tools Support
 *
 ****************************************************************************************
 */
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>

#include <media/v4l2-device.h>
#include <media/v4l2-mediabus.h>
#include <media/v4l2-subdev.h>
#include <media/sunxi_camera.h>

#include "bsp_csi.h"
#include "sunxi_csi.h"
#include "../vfe_os.h"
#include "../platform_cfg.h"

#define CSI_MODULE_NAME "vfe_csi"

#define IS_FLAG(x, y) (((x)&(y)) == y)

static LIST_HEAD(csi_drv_list);

#ifndef CONFIG_ARCH_SUN3IW1P1
#ifdef VFE_CLK
static char *clk_name[CSI_CLK_NUM] = {
	"csi_core_clk",
	"csi_master_clk",
	"csi_core_clk_src",
	"csi_master_clk_pll_src",
	"csi_master_clk_24M_src",
};
#endif
#else
static char *clk_name[CSI_CLK_NUM] = {
	"csi_master_clk",
	"csi_master_clk_pll_src",
	"csi_master_clk_24M_src",
};
#endif

static int csi_clk_get(struct csi_dev *dev)
{
#ifdef VFE_CLK
//	int i;
//	struct device_node *np = dev->pdev->dev.of_node;
	struct device *device;
	char clk_name[20];

	device = &dev->pdev->dev;

	dev->clock[CSI_CORE_CLK] = devm_clk_get(device, "csi_sclk");
	if (IS_ERR(dev->clock[CSI_CORE_CLK])) {
		vfe_err("get csi_sclk clk fail\n");
		return PTR_ERR(dev->clock[CSI_CORE_CLK]);
	}

	sprintf(clk_name, "csi_mclk%d", dev->id);
	dev->clock[CSI_MASTER_CLK] = devm_clk_get(device, clk_name);
	if (IS_ERR(dev->clock[CSI_MASTER_CLK])) {
		vfe_err("get csi_mclk0 clk fail\n");
		return PTR_ERR(dev->clock[CSI_MASTER_CLK]);
	}

	dev->clock[CSI_CORE_CLK_SRC] = devm_clk_get(device, "csi_sclk_src");
	if (IS_ERR(dev->clock[CSI_CORE_CLK_SRC])) {
		vfe_err("get csi_sclk_src clk fail\n");
		return PTR_ERR(dev->clock[CSI_CORE_CLK_SRC]);
	}

	dev->clock[CSI_MASTER_CLK_24M_SRC] = devm_clk_get(device, "csi_mclk_24m");
	if (IS_ERR(dev->clock[CSI_MASTER_CLK_24M_SRC])) {
		vfe_err("get csi_mclk_24m clk fail\n");
		return PTR_ERR(dev->clock[CSI_MASTER_CLK_24M_SRC]);
	}

	dev->clock[CSI_MASTER_CLK_PLL_SRC] = devm_clk_get(device, "csi_mclk_pll");
	if (IS_ERR(dev->clock[CSI_MASTER_CLK_PLL_SRC])) {
		vfe_err("get csi_mclk_pll clk fail\n");
		return PTR_ERR(dev->clock[CSI_MASTER_CLK_PLL_SRC]);
	}

	/*get bus clk*/
	sprintf(clk_name, "csi%d_bus", dev->id);
	dev->bus_clk[VIN_CSI_BUS_CLK] = devm_clk_get(device, clk_name);
	if (IS_ERR_OR_NULL(dev->bus_clk[VIN_CSI_BUS_CLK])) {
		vfe_err("Get csi bus clk failed!\n");
		return PTR_ERR(dev->bus_clk[VIN_CSI_BUS_CLK]);
	}

	/*get dram clk gating*/
	sprintf(clk_name, "csi%d_dclk_gating", dev->id);
	dev->bus_clk[VIN_CSI_MBUS_CLK] = devm_clk_get(device, clk_name);
	if (IS_ERR_OR_NULL(dev->bus_clk[VIN_CSI_MBUS_CLK])) {
		vfe_err("Get csi dram clk gating failed!\n");
		return PTR_ERR(dev->bus_clk[VIN_CSI_MBUS_CLK]);
	}

	/*get csi reset*/
	sprintf(clk_name, "csi%d_ret", dev->id);
	dev->clk_reset[VIN_CSI_RET] = devm_reset_control_get(device, clk_name);
	if (IS_ERR(dev->clk_reset[VIN_CSI_RET])) {
		vfe_err("Get csi reset control fail\n");
		return PTR_ERR(dev->clk_reset[VIN_CSI_RET]);
	}

#ifndef CONFIG_ARCH_SUN3IW1P1
	if (dev->clock[CSI_CORE_CLK] && dev->clock[CSI_CORE_CLK_SRC]) {
		if (clk_set_parent(dev->clock[CSI_CORE_CLK], dev->clock[CSI_CORE_CLK_SRC])) {
			vfe_err("csi core clock set parent failed\n");
			return -1;
		}
		if (clk_set_rate(dev->clock[CSI_CORE_CLK], CSI_CORE_CLK_RATE)) {
			vfe_err("set core clock rate error\n");
			return -1;
		}
	} else {
		vfe_err("csi core clock is null\n");
		return -1;
	}
	vfe_dbg(0, "csi core clk = %ld\n", clk_get_rate(dev->clock[CSI_CORE_CLK]));
#endif
#endif
	return 0;
}

static int csi_clk_enable(struct csi_dev *dev)
{
	int ret = 0;
#ifdef VFE_CLK
	int i;

	ret = clk_prepare_enable(dev->bus_clk[VIN_CSI_BUS_CLK]);
	if (ret) {
		vfe_err("csi bus clk prepare enable fail\n");
		return ret;
	}

	ret = clk_prepare_enable(dev->bus_clk[VIN_CSI_MBUS_CLK]);
	if (ret) {
		vfe_err("csi dram clk gating prepare enable fail\n");
		return ret;
	}

	for (i = 0; i < CSI_CORE_CLK_SRC; i++) {
		if (i != CSI_MASTER_CLK) {
			if (dev->clock[i]) {
				if (clk_prepare_enable(dev->clock[i])) {
					vfe_err("%s enable error\n", clk_name[i]);
					ret = -1;
				}
			} else {
				vfe_dbg(0, "%s is null\n", clk_name[i]);
				ret = -1;
			}
		}
	}
#else
	void __iomem *clk_base;

	clk_base = ioremap(0x01c20000, 0x200);
	if (!clk_base) {
		pr_info("clk_base directly write pin config EIO\n");
		return -EIO;
	}
	writel(0xffffffff, (clk_base+0x64));
	writel(0xffffffff, (clk_base+0x2c4));
	writel(0x0000000f, (clk_base+0x100));
	writel(0x80000000, (clk_base+0x130));/* open misc clk gate */
	writel(0x80018000, (clk_base+0x134));/* set sclk src pll_periph0 and mclk src clk_hosc */
#endif
	return ret;
}

static void csi_clk_disable(struct csi_dev *dev)
{
#ifdef VFE_CLK
	int i;

	for (i = 0; i < CSI_CORE_CLK_SRC; i++) {
		if (i != CSI_MASTER_CLK) {
			if (dev->clock[i])
				clk_disable_unprepare(dev->clock[i]);
			else
				vfe_dbg(0, "%s is null\n", clk_name[i]);
		}
	}

	clk_disable_unprepare(dev->bus_clk[VIN_CSI_BUS_CLK]);
	clk_disable_unprepare(dev->bus_clk[VIN_CSI_MBUS_CLK]);

#endif
}

static void csi_clk_release(struct csi_dev *dev)
{
#ifdef VFE_CLK
	int i;

	for (i = 0; i < CSI_CLK_NUM; i++) {
		if (dev->clock[i])
			clk_put(dev->clock[i]);
		else
			vfe_dbg(0, "%s is null\n", clk_name[i]);
	}
#endif
}

static void csi_reset_enable(struct csi_dev *dev)
{
#ifdef VFE_CLK
	int ret;

	ret = reset_control_assert(dev->clk_reset[VIN_CSI_RET]);
	if (ret)
		vfe_err("csi reset assert fail\n");

#endif
}

static void csi_reset_disable(struct csi_dev *dev)
{
#ifdef VFE_CLK
	int ret;

	ret = reset_control_deassert(dev->clk_reset[VIN_CSI_RET]);
	if (ret)
		vfe_err("csi reset deassert fail\n");

#endif
}

static int sunxi_csi_subdev_s_power(struct v4l2_subdev *sd, int enable)
{
	struct csi_dev *csi = v4l2_get_subdevdata(sd);

	if (enable) {
		csi_clk_enable(csi);
		csi_reset_disable(csi);
	} else {
		csi_clk_disable(csi);
		csi_reset_enable(csi);
	}

	return 0;
}

static int sunxi_csi_subdev_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct csi_dev *csi = v4l2_get_subdevdata(sd);

	if (enable)
		if (csi->capture_mode == V4L2_MODE_IMAGE)
			bsp_csi_cap_start(csi->id, csi->bus_info.ch_total_num, CSI_SCAP);
		else
			bsp_csi_cap_start(csi->id, csi->bus_info.ch_total_num, CSI_VCAP);
	else
		if (csi->capture_mode == V4L2_MODE_IMAGE)
			bsp_csi_cap_stop(csi->id, csi->bus_info.ch_total_num, CSI_SCAP);
		else
			bsp_csi_cap_stop(csi->id, csi->bus_info.ch_total_num, CSI_VCAP);
	return 0;
}

int sunxi_csi_subdev_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param)
{
	struct csi_dev *csi = v4l2_get_subdevdata(sd);

	csi->capture_mode = param->parm.capture.capturemode;

	return 0;
}

static int sunxi_csi_enum_mbus_code(struct v4l2_subdev *sd, struct v4l2_subdev_pad_config *cfg,
		      struct v4l2_subdev_mbus_code_enum *code)
{
	return 0;
}

static int sunxi_csi_subdev_get_fmt(struct v4l2_subdev *sd, struct v4l2_subdev_pad_config *cfg,
				   struct v4l2_subdev_format *fmt)
{
	return 0;
}

static int sunxi_csi_subdev_set_fmt(struct v4l2_subdev *sd, struct v4l2_subdev_pad_config *cfg,
				   struct v4l2_subdev_format *fmt)
{
	struct csi_dev *csi = v4l2_get_subdevdata(sd);
	int i, ret;

	for (i = 0; i < csi->bus_info.ch_total_num; i++)
		csi->bus_info.bus_ch_fmt[i] = (enum bus_pixelcode)fmt->format.code;

	for (i = 0; i < csi->bus_info.ch_total_num; i++) {
		csi->frame_info.pix_ch_fmt[i] = pix_fmt_v4l2_to_common(fmt->reserved[0]);
		csi->frame_info.ch_field[i] = field_fmt_v4l2_to_common(fmt->format.field);
		csi->frame_info.ch_size[i].width = fmt->format.width;
		csi->frame_info.ch_size[i].height = fmt->format.height;
		csi->frame_info.ch_offset[i].hoff = fmt->format.reserved[0];
		csi->frame_info.ch_offset[i].voff = fmt->format.reserved[1];
	}
	csi->frame_info.arrange = csi->arrange;

	ret = bsp_csi_set_fmt(csi->id, &csi->bus_info, &csi->frame_info);
	if (ret < 0) {
		vfe_err("bsp_csi_set_fmt error at %s!\n", __func__);
		return -1;
	}

	ret = bsp_csi_set_size(csi->id, &csi->bus_info, &csi->frame_info);
	if (ret < 0) {
		vfe_err("bsp_csi_set_size error at %s!\n", __func__);
		return -1;
	}

	return 0;
}

int sunxi_csi_addr_init(struct v4l2_subdev *sd, u32 val)
{
	return 0;
}

static int sunxi_csi_s_mbus_config(struct v4l2_subdev *sd, unsigned int pad,
				struct v4l2_mbus_config *cfg)
{
	struct csi_dev *csi = v4l2_get_subdevdata(sd);

	if (cfg->type == V4L2_MBUS_CSI2_DPHY) {
		csi->bus_info.bus_if = CSI2;
		csi->bus_info.ch_total_num = 0;
		if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_CHANNEL_0))
			csi->bus_info.ch_total_num++;
		if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_CHANNEL_1))
			csi->bus_info.ch_total_num++;
		if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_CHANNEL_2))
			csi->bus_info.ch_total_num++;
		if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_CHANNEL_3))
			csi->bus_info.ch_total_num++;
		vfe_print("csi->bus_info.ch_total_num = %d\n", csi->bus_info.ch_total_num);
	} else if (cfg->type == V4L2_MBUS_PARALLEL) {
		csi->bus_info.bus_if = PARALLEL;
		if (IS_FLAG(cfg->flags, V4L2_MBUS_MASTER)) {
			if (IS_FLAG(cfg->flags, V4L2_MBUS_HSYNC_ACTIVE_HIGH))
				csi->bus_info.bus_tmg.href_pol = ACTIVE_HIGH;
			else
				csi->bus_info.bus_tmg.href_pol = ACTIVE_LOW;
			if (IS_FLAG(cfg->flags, V4L2_MBUS_VSYNC_ACTIVE_HIGH))
				csi->bus_info.bus_tmg.vref_pol = ACTIVE_HIGH;
			else
				csi->bus_info.bus_tmg.vref_pol = ACTIVE_LOW;
			if (IS_FLAG(cfg->flags, V4L2_MBUS_PCLK_SAMPLE_RISING))
				csi->bus_info.bus_tmg.pclk_sample = RISING;
			else
				csi->bus_info.bus_tmg.pclk_sample = FALLING;
			if (IS_FLAG(cfg->flags, V4L2_MBUS_FIELD_EVEN_HIGH))
				csi->bus_info.bus_tmg.field_even_pol = ACTIVE_HIGH;
			else
				csi->bus_info.bus_tmg.field_even_pol = ACTIVE_LOW;
		} else {
			vfe_err("Do not support MBUS SLAVE!cfg->type = %d\n", cfg->type);
			return -1;
		}
	} else if (cfg->type == V4L2_MBUS_BT656) {
		csi->bus_info.bus_if = BT656;
#if defined CONFIG_ARCH_SUN8IW11
		csi->bus_info.ch_total_num = 0;
		if (IS_FLAG(cfg->flags, CSI_CH_0))
			csi->bus_info.ch_total_num++;
		if (IS_FLAG(cfg->flags, CSI_CH_1))
			csi->bus_info.ch_total_num++;
		if (IS_FLAG(cfg->flags, CSI_CH_2))
			csi->bus_info.ch_total_num++;
		if (IS_FLAG(cfg->flags, CSI_CH_3))
			csi->bus_info.ch_total_num++;
		if (csi->bus_info.ch_total_num == 4) {
			csi->arrange.column = 2;
			csi->arrange.row = 2;
		} else if (csi->bus_info.ch_total_num == 2) {
			csi->arrange.column = 2;
			csi->arrange.row = 1;
		} else {
			csi->bus_info.ch_total_num = 1;
			csi->arrange.column = 1;
			csi->arrange.row = 1;
		}
		vfe_print("ch_total_num = %d\n", csi->bus_info.ch_total_num);
		if (IS_FLAG(cfg->flags, V4L2_MBUS_PCLK_SAMPLE_RISING))
			csi->bus_info.bus_tmg.pclk_sample = RISING;
		else
			csi->bus_info.bus_tmg.pclk_sample = FALLING;
#endif
	}

	return 0;
}

static int sunxi_csi_get_frmsize(struct csi_dev *csi, unsigned int *arg)
{
#if defined(CH_OUTPUT_IN_DIFFERENT_VIDEO)
	*arg = csi->frame_info.frm_byte_size[0];
#else
	/* Not currently supported one video node outputs multiple channel data */
	/* *arg = csi->frame_info.frm_byte_size[0] * csi->bus_info.ch_total_num; */
	*arg = csi->frame_info.frm_byte_size[0];
#endif
	vfe_print("csi->frame_info.frm_byte_size = %d\n", *arg);
	return 0;
}

static long sunxi_csi_subdev_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct csi_dev *csi = v4l2_get_subdevdata(sd);
	int ret = 0;

	switch (cmd) {
	case VIDIOC_SUNXI_CSI_GET_FRM_SIZE:
		mutex_lock(&csi->subdev_lock);
		ret = sunxi_csi_get_frmsize(csi, arg);
		mutex_unlock(&csi->subdev_lock);
		break;
	case VIDIOC_SUNXI_CSI_SET_CORE_CLK:
		mutex_lock(&csi->subdev_lock);
#ifdef VFE_CLK
		ret = clk_set_rate(csi->clock[CSI_CORE_CLK], *(unsigned long *)arg);
		vfe_print("Set csi core clk = %ld, after Set csi core clk = %ld\n", *(unsigned long *)arg, clk_get_rate(csi->clock[CSI_CORE_CLK]));
#endif
		mutex_unlock(&csi->subdev_lock);
		break;
	case VIDIOC_SUNXI_CSI_SET_M_CLK:
		break;
	default:
		return -ENOIOCTLCMD;
	}

	return ret;
}

static const struct v4l2_subdev_core_ops sunxi_csi_core_ops = {
	.s_power = sunxi_csi_subdev_s_power,
	.init = sunxi_csi_addr_init,
	.ioctl = sunxi_csi_subdev_ioctl,
};

static const struct v4l2_subdev_video_ops sunxi_csi_subdev_video_ops = {
	.s_stream = sunxi_csi_subdev_s_stream,
};

static const struct v4l2_subdev_pad_ops sunxi_csi_subdev_pad_ops = {
	.enum_mbus_code = sunxi_csi_enum_mbus_code,
	.get_fmt = sunxi_csi_subdev_get_fmt,
	.set_fmt = sunxi_csi_subdev_set_fmt,
	.set_mbus_config = sunxi_csi_s_mbus_config,
};


static struct v4l2_subdev_ops sunxi_csi_subdev_ops = {
	.core = &sunxi_csi_core_ops,
	.video = &sunxi_csi_subdev_video_ops,
	.pad = &sunxi_csi_subdev_pad_ops,
};

static int sunxi_csi_subdev_init(struct csi_dev *csi)
{
	struct v4l2_subdev *sd = &csi->subdev;

	mutex_init(&csi->subdev_lock);
	csi->arrange.row = 1;
	csi->arrange.column = 1;
	csi->bus_info.ch_total_num = 1;
	v4l2_subdev_init(sd, &sunxi_csi_subdev_ops);
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	snprintf(sd->name, sizeof(sd->name), "sunxi_csi.%u", csi->id);

	v4l2_set_subdevdata(sd, csi);

	return 0;
}

static int csi_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct csi_dev *csi = NULL;
	int ret = 0;

	if (np == NULL) {
		vfe_err("CSI failed to get of node\n");
		return -ENODEV;
	}
	csi = kzalloc(sizeof(struct csi_dev), GFP_KERNEL);
	if (!csi) {
		ret = -ENOMEM;
		goto ekzalloc;
	}

	pdev->id = of_alias_get_id(np, "csi_res");
	if (pdev->id < 0) {
		vfe_err("CSI failed to get alias id\n");
		ret = -EINVAL;
		goto freedev;
	}

	csi->base = of_iomap(np, 0);
	if (!csi->base) {
		ret = -EIO;
		goto freedev;
	}
	csi->id = pdev->id;
	csi->pdev = pdev;
	spin_lock_init(&csi->slock);
	init_waitqueue_head(&csi->wait);

	ret = bsp_csi_set_base_addr(csi->id, (unsigned long)csi->base);
	if (ret < 0)
		goto ehwinit;

	if (csi_clk_get(csi)) {
		vfe_err("csi clock get failed!\n");
		return -ENXIO;
	}
	/* csi_clk_enable(csi); */
	/* csi_reset_disable(csi); */

	list_add_tail(&csi->csi_list, &csi_drv_list);
	sunxi_csi_subdev_init(csi);

	platform_set_drvdata(pdev, csi);
	vfe_print("csi%d probe end!\n", csi->id);

	return 0;

ehwinit:
	iounmap(csi->base);
freedev:
	kfree(csi);
ekzalloc:
	vfe_print("csi probe err!\n");
	return ret;
}


static int csi_remove(struct platform_device *pdev)
{
	struct csi_dev *csi = platform_get_drvdata(pdev);

	platform_set_drvdata(pdev, NULL);
	csi_clk_release(csi);
	mutex_destroy(&csi->subdev_lock);
	if (csi->base)
		iounmap(csi->base);
	kfree(csi);

	return 0;
}

static const struct of_device_id sunxi_csi_match[] = {
	{.compatible = "allwinner,sunxi-csi",},
	{},
};

MODULE_DEVICE_TABLE(of, sunxi_csi_match);

static struct platform_driver csi_platform_driver = {
	.probe    = csi_probe,
	.remove   = csi_remove,
	.driver = {
		.name   = CSI_MODULE_NAME,
		.owner  = THIS_MODULE,
		.of_match_table = sunxi_csi_match,
	}
};

void sunxi_csi_dump_regs(struct v4l2_subdev *sd)
{
	struct csi_dev *csi = v4l2_get_subdevdata(sd);
	int i = 0;

	pr_info("Vfe dump CSI regs :\n");
	for (i = 0; i < 0xb0; i = i + 4) {
		if (i % 0x10 == 0)
			pr_info("0x%08x:    ", i);
		pr_info("0x%08x, ", readl(csi->base + i));
		if (i % 0x10 == 0xc)
			pr_info("\n");
	}
}

int sunxi_csi_register_subdev(struct v4l2_device *v4l2_dev, struct v4l2_subdev *sd)
{
	if (sd == NULL)
		return -ENODEV;

	return v4l2_device_register_subdev(v4l2_dev, sd);
}

void sunxi_csi_unregister_subdev(struct v4l2_subdev *sd)
{
	if (sd == NULL)
		return;
	v4l2_device_unregister_subdev(sd);
	v4l2_set_subdevdata(sd, NULL);
}

int sunxi_csi_get_subdev(struct v4l2_subdev **sd, int sel)
{
	struct csi_dev *csi;

	list_for_each_entry(csi, &csi_drv_list, csi_list) {
		if (csi->id == sel) {
			*sd = &csi->subdev;
			return 0;
		}
	}

	return -1;
}

int sunxi_csi_put_subdev(struct v4l2_subdev **sd, int sel)
{
	*sd = NULL;
	return 0;
}

int sunxi_csi_platform_register(void)
{
	int ret;

	ret = platform_driver_register(&csi_platform_driver);
	if (ret) {
		vfe_err("platform driver register failed\n");
		return ret;
	}
	vfe_print("csi_init end\n");
	return 0;
}

void sunxi_csi_platform_unregister(void)
{
	vfe_print("csi_exit start\n");
	platform_driver_unregister(&csi_platform_driver);
	vfe_print("csi_exit end\n");
}

