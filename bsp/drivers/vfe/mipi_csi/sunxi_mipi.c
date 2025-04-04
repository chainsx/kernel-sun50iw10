/*
 * linux-4.9/drivers/media/platform/sunxi-vfe/mipi_csi/sunxi_mipi.c
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
 * sunxi_mipi.c
 *
 * Hawkview ISP - sunxi_mipi.c module
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
#include <media/v4l2-device.h>
#include <media/v4l2-mediabus.h>
#include <media/v4l2-subdev.h>
#include "bsp_mipi_csi.h"
#include "sunxi_mipi.h"
#include "../vfe_os.h"
#include "../platform_cfg.h"
#define MIPI_MODULE_NAME "vfe_mipi"

#define IS_FLAG(x, y) (((x)&(y)) == y)

static LIST_HEAD(mipi_drv_list);

static char *clk_name[MIPI_CLK_NUM] = {
	"mipi_csi_clk",
	"mipi_dphy_clk",
	"mipi_csi_clk_src",
	"mipi_dphy_clk_src",
};

static int mipi_clk_get(struct mipi_dev *dev)
{
#ifdef VFE_CLK
	int i;
	int clk_index[MIPI_CLK_NUM];
	struct device_node *np = dev->pdev->dev.of_node;

	of_property_read_u32_array(np, "clocks-index", clk_index, MIPI_CLK_NUM);
	for (i = 0; i < MIPI_CLK_NUM; i++) {
		if (clk_index[i] != NOCLK) {
			dev->clock[i] = of_clk_get(np, clk_index[i]);
			if (IS_ERR_OR_NULL(dev->clock[i]))
				vfe_warn("Get clk Index:%d , Name:%s is NULL!\n", (int)clk_index[i], clk_name[i]);
		}
	}
#endif
	return 0;
}

static int mipi_dphy_clk_set(struct mipi_dev *dev, unsigned long freq)
{
#ifdef VFE_CLK
	if (dev->clock[MIPI_DPHY_CLK] && dev->clock[MIPI_DPHY_CLK_SRC]) {
		if (clk_set_parent(dev->clock[MIPI_DPHY_CLK], dev->clock[MIPI_DPHY_CLK_SRC])) {
			vfe_err("set vfe dphy clock source failed\n");
			return -1;
		}
		if (clk_set_rate(dev->clock[MIPI_DPHY_CLK], freq)) {
			vfe_err("set mipi%d dphy clock error\n", dev->id);
			return -1;
		}
	} else {
		vfe_warn("vfe dphy clock is null\n");
		return -1;
	}
#endif
  return 0;
}

static int mipi_clk_enable(struct mipi_dev *dev)
{
	int ret = 0;
#ifdef VFE_CLK
	int i;

	for (i = 0; i < MIPI_CSI_CLK_SRC; i++) {
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
#endif
	return ret;
}

static void mipi_clk_disable(struct mipi_dev *dev)
{
#ifdef VFE_CLK
	int i;

	for (i = 0; i < MIPI_CSI_CLK_SRC; i++) {
		if (dev->clock[i])
			clk_disable_unprepare(dev->clock[i]);
		else
			vfe_dbg(0, "%s is null\n", clk_name[i]);
	}
#endif
}

static void mipi_clk_release(struct mipi_dev *dev)
{
#ifdef VFE_CLK
	int i;

	for (i = 0; i < MIPI_CLK_NUM; i++) {
		if (dev->clock[i])
			clk_put(dev->clock[i]);
		else
			vfe_dbg(0, "%s is null\n", clk_name[i]);
	}
#endif
}

static enum pkt_fmt get_pkt_fmt(enum bus_pixelcode bus_pix_code)
{
	switch (bus_pix_code) {
	case BUS_FMT_RGB565_16X1:
		return MIPI_RGB565;
	case BUS_FMT_UYVY8_16X1:
		return MIPI_YUV422;
	case BUS_FMT_UYVY10_20X1:
		return MIPI_YUV422_10;
	case BUS_FMT_SBGGR8_1X8:
	case BUS_FMT_SGBRG8_1X8:
	case BUS_FMT_SGRBG8_1X8:
	case BUS_FMT_SRGGB8_1X8:
		return MIPI_RAW8;
	case BUS_FMT_SBGGR10_1X10:
	case BUS_FMT_SGBRG10_1X10:
	case BUS_FMT_SGRBG10_1X10:
	case BUS_FMT_SRGGB10_1X10:
		return MIPI_RAW10;
	case BUS_FMT_SBGGR12_1X12:
	case BUS_FMT_SGBRG12_1X12:
	case BUS_FMT_SGRBG12_1X12:
	case BUS_FMT_SRGGB12_1X12:
		return MIPI_RAW12;
	case BUS_FMT_YY8_UYVY8_12X1:
		return MIPI_YUV420;
	case BUS_FMT_YY10_UYVY10_15X1:
		return MIPI_YUV420_10;
	default:
		return MIPI_RAW8;
	}
}

static int sunxi_mipi_subdev_s_power(struct v4l2_subdev *sd, int enable)
{
	struct mipi_dev *mipi = v4l2_get_subdevdata(sd);

	if (enable) {
		mipi_dphy_clk_set(mipi, DPHY_CLK);
		mipi_clk_enable(mipi);
	} else {
		mipi_clk_disable(mipi);
	}
	return 0;
}
static int sunxi_mipi_subdev_s_stream(struct v4l2_subdev *sd, int enable)
{
	return 0;
}

static int sunxi_mipi_enum_mbus_code(struct v4l2_subdev *sd, struct v4l2_subdev_pad_config *cfg,
		      struct v4l2_subdev_mbus_code_enum *code)
{
	return 0;
}

static int sunxi_mipi_subdev_get_fmt(struct v4l2_subdev *sd, struct v4l2_subdev_pad_config *cfg,
				   struct v4l2_subdev_format *fmt)
{
	return 0;
}

static int sunxi_mipi_subdev_set_fmt(struct v4l2_subdev *sd, struct v4l2_subdev_pad_config *cfg,
				   struct v4l2_subdev_format *fmt)
{
	struct mipi_dev *mipi = v4l2_get_subdevdata(sd);
	int i;

	mipi->mipi_para.bps = fmt->reserved[0];
	mipi->mipi_para.auto_check_bps = 0;	/* TODO */
	mipi->mipi_para.dphy_freq = DPHY_CLK; /* TODO */

	for (i = 0; i < mipi->mipi_para.total_rx_ch; i++) {/* TODO */
		mipi->mipi_fmt.packet_fmt[i] = get_pkt_fmt((enum bus_pixelcode)fmt->format.code);
		mipi->mipi_fmt.field[i] = field_fmt_v4l2_to_common(fmt->format.field);
		mipi->mipi_fmt.vc[i] = i;
	}
	bsp_mipi_csi_dphy_init(mipi->id);
	bsp_mipi_csi_set_para(mipi->id, &mipi->mipi_para);
	bsp_mipi_csi_set_fmt(mipi->id, mipi->mipi_para.total_rx_ch, &mipi->mipi_fmt);

	/* for dphy clock async */
	bsp_mipi_csi_dphy_disable(mipi->id);

	return 0;
}

int sunxi_mipi_addr_init(struct v4l2_subdev *sd, u32 val)
{
	return 0;
}

static int sunxi_mipi_s_mbus_config(struct v4l2_subdev *sd, unsigned int pad,
				struct v4l2_mbus_config *cfg)
{
	struct mipi_dev *mipi = v4l2_get_subdevdata(sd);

	if (cfg->type == V4L2_MBUS_CSI2_DPHY) {
		if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_4_LANE))
			mipi->mipi_para.lane_num = 4;
		else if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_3_LANE))
			mipi->mipi_para.lane_num = 3;
		else if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_2_LANE))
			mipi->mipi_para.lane_num = 2;
		else
			mipi->mipi_para.lane_num = 1;

		mipi->mipi_para.total_rx_ch = 0;
		if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_CHANNEL_0))
			mipi->mipi_para.total_rx_ch++;

		if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_CHANNEL_1))
			mipi->mipi_para.total_rx_ch++;

		if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_CHANNEL_2))
			mipi->mipi_para.total_rx_ch++;

		if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_CHANNEL_3))
			mipi->mipi_para.total_rx_ch++;

	}

	return 0;
}

static const struct v4l2_subdev_core_ops sunxi_mipi_core_ops = {
	.s_power = sunxi_mipi_subdev_s_power,
	.init = sunxi_mipi_addr_init,
};

static const struct v4l2_subdev_video_ops sunxi_mipi_subdev_video_ops = {
	.s_stream = sunxi_mipi_subdev_s_stream,
};

static const struct v4l2_subdev_pad_ops sunxi_mipi_subdev_pad_ops = {
	.enum_mbus_code = sunxi_mipi_enum_mbus_code,
	.get_fmt = sunxi_mipi_subdev_get_fmt,
	.set_fmt = sunxi_mipi_subdev_set_fmt,
	.set_mbus_config = sunxi_mipi_s_mbus_config,
};


static struct v4l2_subdev_ops sunxi_mipi_subdev_ops = {
	.core = &sunxi_mipi_core_ops,
	.video = &sunxi_mipi_subdev_video_ops,
	.pad = &sunxi_mipi_subdev_pad_ops,
};

static int sunxi_mipi_subdev_init(struct mipi_dev *mipi)
{
	struct v4l2_subdev *sd = &mipi->subdev;

	v4l2_subdev_init(sd, &sunxi_mipi_subdev_ops);
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	snprintf(sd->name, sizeof(sd->name), "sunxi_mipi.%u", mipi->id);

	v4l2_set_subdevdata(sd, mipi);
	return 0;
}

static int mipi_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;
	struct mipi_dev *mipi = NULL;
	int ret = 0;

	if (np == NULL) {
		vfe_err("MIPI failed to get of node\n");
		return -ENODEV;
	}

	mipi = kzalloc(sizeof(struct mipi_dev), GFP_KERNEL);
	if (!mipi) {
		ret = -ENOMEM;
		goto ekzalloc;
	}

	pdev->id = of_alias_get_id(np, "mipi");
	if (pdev->id < 0) {
		vfe_err("MIPI failed to get alias id\n");
		ret = -EINVAL;
		goto freedev;
	}

	mipi->base = of_iomap(np, 0);
	if (!mipi->base) {
		ret = -EIO;
		goto freedev;
	}
	mipi->id = pdev->id;
	mipi->pdev = pdev;

	spin_lock_init(&mipi->slock);
	init_waitqueue_head(&mipi->wait);

	ret = bsp_mipi_csi_set_base_addr(mipi->id, (unsigned long)mipi->base);
	ret = bsp_mipi_dphy_set_base_addr(mipi->id, (unsigned long)mipi->base + 0x1000);
	if (ret < 0)
		goto ehwinit;

	if (mipi_clk_get(mipi)) {
		vfe_err("mipi clock get failed!\n");
		return -ENXIO;
	}
/* mipi_clk_enable(mipi); */

	list_add_tail(&mipi->mipi_list, &mipi_drv_list);
	sunxi_mipi_subdev_init(mipi);

	platform_set_drvdata(pdev, mipi);
	vfe_print("mipi %d probe end!\n", mipi->id);
	return 0;

ehwinit:
	iounmap(mipi->base);
freedev:
	kfree(mipi);
ekzalloc:
	vfe_print("mipi probe err!\n");
	return ret;
}


static int mipi_remove(struct platform_device *pdev)
{
	struct mipi_dev *mipi = platform_get_drvdata(pdev);

	platform_set_drvdata(pdev, NULL);
	if (mipi->base)
		iounmap(mipi->base);
	mipi_clk_release(mipi);
	kfree(mipi);
	return 0;
}

static const struct of_device_id sunxi_mipi_match[] = {
	{ .compatible = "allwinner,sunxi-mipi", },
	{},
};

static struct platform_driver mipi_platform_driver = {
	.probe    = mipi_probe,
	.remove   = mipi_remove,
	.driver = {
		.name   = MIPI_MODULE_NAME,
		.owner  = THIS_MODULE,
		.of_match_table = sunxi_mipi_match,
	}
};

int sunxi_mipi_register_subdev(struct v4l2_device *v4l2_dev, struct v4l2_subdev *sd)
{
	if (sd == NULL)
		return -ENODEV;
	return v4l2_device_register_subdev(v4l2_dev, sd);
}

void sunxi_mipi_unregister_subdev(struct v4l2_subdev *sd)
{
	if (sd == NULL)
		return;
	v4l2_device_unregister_subdev(sd);
	v4l2_set_subdevdata(sd, NULL);
}

int sunxi_mipi_get_subdev(struct v4l2_subdev **sd, int sel)
{
	struct mipi_dev *mipi;

	list_for_each_entry(mipi, &mipi_drv_list, mipi_list) {
		if (mipi->id == sel) {
			*sd = &mipi->subdev;
			return 0;
		}
	}
	return -1;
}
int sunxi_mipi_put_subdev(struct v4l2_subdev **sd, int sel)
{
	*sd = NULL;
	return 0;
}

int sunxi_mipi_platform_register(void)
{
	int ret;

	ret = platform_driver_register(&mipi_platform_driver);
	if (ret) {
		vfe_err("platform driver register failed\n");
		return ret;
	}
	vfe_print("mipi_init end\n");
	return 0;
}

void sunxi_mipi_platform_unregister(void)
{
	vfe_print("mipi_exit start\n");
	platform_driver_unregister(&mipi_platform_driver);
	vfe_print("mipi_exit end\n");
}

