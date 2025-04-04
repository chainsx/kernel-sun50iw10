/*
 * vin_core.c for video manage
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zhao Wei <zhaowei@allwinnertech.com>
 *	Yang Feng <yangfeng@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/mutex.h>
#include <linux/videodev2.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/freezer.h>
#include <linux/debugfs.h>
//#include <linux/mpp.h>

#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/moduleparam.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-common.h>
#include <media/v4l2-mediabus.h>
#include <media/v4l2-subdev.h>
#include <media/videobuf2-dma-contig.h>

#include <linux/regulator/consumer.h>

#include "vin_core.h"
#include "../vin-cci/cci_helper.h"
#include "../utility/config.h"
#include "../modules/sensor/camera_cfg.h"
#include "../modules/sensor/sensor_helper.h"
#include "../utility/vin_io.h"
#include "../vin-csi/sunxi_csi.h"
#include "../vin-isp/sunxi_isp.h"
#include "../vin-vipp/sunxi_scaler.h"
#include "../vin-mipi/sunxi_mipi.h"
#include "../vin.h"
#include "dma_reg.h"

#define VIN_CORE_NAME "sunxi-vin-core"

struct vin_core *vin_core_gbl[VIN_MAX_DEV];

#define VIN_DEBUGFS_BUF_SIZE 768
struct vin_debugfs_buffer {
	size_t count;
	char data[VIN_DEBUGFS_BUF_SIZE*VIN_MAX_DEV];
};
struct dentry *vi_debugfs_root, *vi_node;
size_t vi_status_size[VIN_MAX_DEV];
size_t vi_status_size_sum;

uint ptn_frame_cnt;

#define DEVICE_ATTR_SHOW(name) \
static ssize_t name##_show(struct device *dev, \
			struct device_attribute *attr, \
			char *buf) \
{\
	return sprintf(buf, "%u\n", name); \
}

#define DEVICE_ATTR_STORE(name) \
static ssize_t name##_store(struct device *dev, \
			struct device_attribute *attr, \
			const char *buf,\
			size_t count) \
{\
	unsigned long val;\
	val = simple_strtoul(buf, NULL, 16);\
	name = val;\
	vin_print("Set val = 0x%lx\n", val);\
	return count;\
}

DEVICE_ATTR_SHOW(vin_log_mask)
DEVICE_ATTR_STORE(vin_log_mask)

static DEVICE_ATTR(vin_log_mask, S_IRUGO | S_IWUSR | S_IWGRP,
		   vin_log_mask_show, vin_log_mask_store);

static struct attribute *vin_attributes[] = {
	&dev_attr_vin_log_mask.attr,
	NULL
};

static struct attribute_group vin_attribute_group = {
	/*.name = "vin_attrs",*/
	.attrs = vin_attributes,
};

static struct vin_fmt vin_formats[] = {
	{
		.name		= "RGB565",
		.fourcc		= V4L2_PIX_FMT_RGB565,
		.depth		= { 16 },
		.color		= V4L2_COLORSPACE_SRGB,
		.memplanes	= 1,
		.colplanes	= 1,
		.flags		= VIN_FMT_RGB,
	}, {
		.name		= "RGB888",
		.fourcc 	= V4L2_PIX_FMT_RGB24,
		.depth		= { 24 },
		.color		= V4L2_COLORSPACE_SRGB,
		.memplanes	= 1,
		.colplanes	= 1,
		.mbus_code	= MEDIA_BUS_FMT_RGB888_1X24,
		.flags		= VIN_FMT_RGB | VIN_FMT_RAW,
	}, {
		.name		= "BGR888",
		.fourcc 	= V4L2_PIX_FMT_BGR24,
		.depth		= { 24 },
		.color		= V4L2_COLORSPACE_SRGB,
		.memplanes	= 1,
		.colplanes	= 1,
		.mbus_code	= MEDIA_BUS_FMT_BGR888_1X24,
		.flags		= VIN_FMT_RGB | VIN_FMT_RAW,
	}, {
		.name		= "ARGB1555, 16 bpp",
		.fourcc		= V4L2_PIX_FMT_RGB555,
		.depth		= { 16 },
		.color		= V4L2_COLORSPACE_SRGB,
		.memplanes	= 1,
		.colplanes	= 1,
		.flags		= VIN_FMT_RGB | VIN_FMT_OSD,
	}, {
		.name		= "ARGB4444, 16 bpp",
		.fourcc		= V4L2_PIX_FMT_RGB444,
		.depth		= { 16 },
		.color		= V4L2_COLORSPACE_SRGB,
		.memplanes	= 1,
		.colplanes	= 1,
		.flags		= VIN_FMT_RGB | VIN_FMT_OSD,
	}, {
		.name		= "ARGB8888, 32 bpp",
		.fourcc		= V4L2_PIX_FMT_RGB32,
		.depth		= { 32 },
		.color		= V4L2_COLORSPACE_SRGB,
		.memplanes	= 1,
		.colplanes	= 1,
		.flags		= VIN_FMT_RGB | VIN_FMT_OSD,
	}, {
		.name		= "YUV 4:2:2 planar, Y/Cb/Cr",
		.fourcc		= V4L2_PIX_FMT_YUV422P,
		.depth		= { 16 },
		.color		= V4L2_COLORSPACE_JPEG,
		.memplanes	= 1,
		.colplanes	= 3,
		.flags		= VIN_FMT_YUV | VIN_FMT_RAW,
	}, {
		.name		= "YUV 4:2:2 planar, Y/CbCr",
		.fourcc		= V4L2_PIX_FMT_NV16,
		.depth		= { 16 },
		.color		= V4L2_COLORSPACE_JPEG,
		.memplanes	= 1,
		.colplanes	= 2,
		.flags		= VIN_FMT_YUV | VIN_FMT_RAW,
	}, {
		.name		= "YUV 4:2:2 planar, Y/CbCr",
		.fourcc		= V4L2_PIX_FMT_NV16M,
		.depth		= { 8, 8 },
		.color		= V4L2_COLORSPACE_JPEG,
		.memplanes	= 2,
		.colplanes	= 2,
		.flags		= VIN_FMT_YUV | VIN_FMT_RAW,
	}, {
		.name		= "YUV 4:2:2 planar, Y/CrCb",
		.fourcc		= V4L2_PIX_FMT_NV61,
		.depth		= { 16 },
		.color		= V4L2_COLORSPACE_JPEG,
		.memplanes	= 1,
		.colplanes	= 2,
		.flags		= VIN_FMT_YUV | VIN_FMT_RAW,
	}, {
		.name		= "YUV 4:2:2 planar, Y/CrCb",
		.fourcc		= V4L2_PIX_FMT_NV61M,
		.depth		= { 8, 8 },
		.color		= V4L2_COLORSPACE_JPEG,
		.memplanes	= 2,
		.colplanes	= 2,
		.flags		= VIN_FMT_YUV | VIN_FMT_RAW,
	}, {
		.name		= "YUV 4:2:0 planar, YCbCr",
		.fourcc		= V4L2_PIX_FMT_YUV420,
		.depth		= { 12 },
		.color		= V4L2_COLORSPACE_JPEG,
		.memplanes	= 1,
		.colplanes	= 3,
		.flags		= VIN_FMT_YUV | VIN_FMT_RAW,
	}, {
		.name		= "YUV 4:2:0 non-contig. 3p, Y/Cb/Cr",
		.fourcc		= V4L2_PIX_FMT_YUV420M,
		.color		= V4L2_COLORSPACE_JPEG,
		.depth		= { 8, 2, 2 },
		.memplanes	= 3,
		.colplanes	= 3,
		.flags		= VIN_FMT_YUV | VIN_FMT_RAW,
	}, {
		.name		= "YUV 4:2:0 planar, YCrCb",
		.fourcc		= V4L2_PIX_FMT_YVU420,
		.depth		= { 12 },
		.color		= V4L2_COLORSPACE_JPEG,
		.memplanes	= 1,
		.colplanes	= 3,
		.flags		= VIN_FMT_YUV | VIN_FMT_RAW,
	}, {
		.name		= "YUV 4:2:0 non-contig. 3p, Y/Cr/Cb",
		.fourcc		= V4L2_PIX_FMT_YVU420M,
		.color		= V4L2_COLORSPACE_JPEG,
		.depth		= { 8, 2, 2 },
		.memplanes	= 3,
		.colplanes	= 3,
		.flags		= VIN_FMT_YUV | VIN_FMT_RAW,
	}, {
		.name		= "YUV 4:2:0 planar, Y/CbCr",
		.fourcc		= V4L2_PIX_FMT_NV12,
		.depth		= { 12 },
		.color		= V4L2_COLORSPACE_JPEG,
		.memplanes	= 1,
		.colplanes	= 2,
		.flags		= VIN_FMT_YUV | VIN_FMT_RAW,
	}, {
		.name		= "YUV 4:2:0 non-contig. 2p, Y/CbCr",
		.fourcc		= V4L2_PIX_FMT_NV12M,
		.color		= V4L2_COLORSPACE_JPEG,
		.depth		= { 8, 4 },
		.memplanes	= 2,
		.colplanes	= 2,
		.flags		= VIN_FMT_YUV | VIN_FMT_RAW,
	}, {
		.name		= "YUV 4:2:0 planar, Y/CrCb",
		.fourcc		= V4L2_PIX_FMT_NV21,
		.depth		= { 12 },
		.color		= V4L2_COLORSPACE_JPEG,
		.memplanes	= 1,
		.colplanes	= 2,
		.flags		= VIN_FMT_YUV | VIN_FMT_RAW,
	}, {
		.name		= "YUV 4:2:0 non-contig. 2p, Y/CrCb",
		.fourcc		= V4L2_PIX_FMT_NV21M,
		.color		= V4L2_COLORSPACE_JPEG,
		.depth		= { 8, 4 },
		.memplanes	= 2,
		.colplanes	= 2,
		.flags		= VIN_FMT_YUV | VIN_FMT_RAW,
	}, {
		.name		= "YUV 4:2:0 non-contig. 2p, tiled",
		.fourcc		= V4L2_PIX_FMT_NV12MT,
		.color		= V4L2_COLORSPACE_JPEG,
		.depth		= { 8, 4 },
		.memplanes	= 2,
		.colplanes	= 2,
		.flags		= VIN_FMT_YUV | VIN_FMT_RAW,
	}, {
		.name		= "YUV 4:0:0 planar, GREY",
		.fourcc		= V4L2_PIX_FMT_GREY,
		.depth		= { 8 },
		.color		= V4L2_COLORSPACE_JPEG,
		.memplanes	= 1,
		.colplanes	= 1,
		.flags		= VIN_FMT_YUV | VIN_FMT_RAW,
	}, {
		.name		= "YUV 4:2:0 non-contig. 2p, FBC",
		.fourcc		= V4L2_PIX_FMT_FBC,
		.color		= V4L2_COLORSPACE_JPEG,
		.depth		= { 12 },
		.memplanes	= 1,
		.colplanes	= 2,
		.flags		= VIN_FMT_YUV | VIN_FMT_RAW,
	}, {
		.name		= "YUV 4:2:0 non-contig. 2p, LBC_2X",
		.fourcc		= V4L2_PIX_FMT_LBC_2_0X,
		.color		= V4L2_COLORSPACE_JPEG,
		.depth		= { 12 },
		.memplanes	= 1,
		.colplanes	= 1,
		.flags		= VIN_FMT_YUV | VIN_FMT_RAW,
	}, {
		.name		= "YUV 4:2:0 non-contig. 2p, LBC_2.5X",
		.fourcc		= V4L2_PIX_FMT_LBC_2_5X,
		.color		= V4L2_COLORSPACE_JPEG,
		.depth		= { 12 },
		.memplanes	= 1,
		.colplanes	= 1,
		.flags		= VIN_FMT_YUV | VIN_FMT_RAW,
	}, {
		.name		= "YUV 4:2:0 non-contig. 2p, LBC_1X",
		.fourcc		= V4L2_PIX_FMT_LBC_1_0X,
		.color		= V4L2_COLORSPACE_JPEG,
		.depth		= { 12 },
		.memplanes	= 1,
		.colplanes	= 1,
		.flags		= VIN_FMT_YUV | VIN_FMT_RAW,
	}, {
		.name		= "RAW Bayer BGGR 8bit",
		.fourcc		= V4L2_PIX_FMT_SBGGR8,
		.depth		= { 8 },
		.memplanes	= 1,
		.colplanes	= 1,
		.mbus_code	= MEDIA_BUS_FMT_SBGGR8_1X8,
		.flags		= VIN_FMT_RAW,
	}, {
		.name		= "RAW Bayer GBRG 8bit",
		.fourcc		= V4L2_PIX_FMT_SGBRG8,
		.depth		= { 8 },
		.memplanes	= 1,
		.colplanes	= 1,
		.mbus_code	= MEDIA_BUS_FMT_SGBRG8_1X8,
		.flags		= VIN_FMT_RAW,
	}, {
		.name		= "RAW Bayer GRBG 8bit",
		.fourcc		= V4L2_PIX_FMT_SGRBG8,
		.depth		= { 8 },
		.memplanes	= 1,
		.colplanes	= 1,
		.mbus_code	= MEDIA_BUS_FMT_SGRBG8_1X8,
		.flags		= VIN_FMT_RAW,
	}, {
		.name		= "RAW Bayer RGGB 8bit",
		.fourcc		= V4L2_PIX_FMT_SRGGB8,
		.depth		= { 8 },
		.memplanes	= 1,
		.colplanes	= 1,
		.mbus_code	= MEDIA_BUS_FMT_SRGGB8_1X8,
		.flags		= VIN_FMT_RAW,
	}, {
		.name		= "RAW Bayer BGGR 10bit",
		.fourcc		= V4L2_PIX_FMT_SBGGR10,
		.depth		= { 16 },
		.memplanes	= 1,
		.colplanes	= 1,
		.mbus_code	= MEDIA_BUS_FMT_SBGGR10_1X10,
		.flags		= VIN_FMT_RAW,
	}, {
		.name		= "RAW Bayer GBRG 10bit",
		.fourcc		= V4L2_PIX_FMT_SGBRG10,
		.depth		= { 16 },
		.memplanes	= 1,
		.colplanes	= 1,
		.mbus_code	= MEDIA_BUS_FMT_SGBRG10_1X10,
		.flags		= VIN_FMT_RAW,
	}, {
		.name		= "RAW Bayer GRBG 10bit",
		.fourcc		= V4L2_PIX_FMT_SGRBG10,
		.depth		= { 16 },
		.memplanes	= 1,
		.colplanes	= 1,
		.mbus_code	= MEDIA_BUS_FMT_SGRBG10_1X10,
		.flags		= VIN_FMT_RAW,
	}, {
		.name		= "RAW Bayer RGGB 10bit",
		.fourcc		= V4L2_PIX_FMT_SRGGB10,
		.depth		= { 16 },
		.memplanes	= 1,
		.colplanes	= 1,
		.mbus_code	= MEDIA_BUS_FMT_SRGGB10_1X10,
		.flags		= VIN_FMT_RAW,
	}, {
		.name		= "RAW Bayer BGGR 12bit",
		.fourcc		= V4L2_PIX_FMT_SBGGR12,
		.depth		= { 16 },
		.memplanes	= 1,
		.colplanes	= 1,
		.mbus_code	= MEDIA_BUS_FMT_SBGGR12_1X12,
		.flags		= VIN_FMT_RAW,
	}, {
		.name		= "RAW Bayer GBRG 12bit",
		.fourcc		= V4L2_PIX_FMT_SGBRG12,
		.depth		= { 16 },
		.memplanes	= 1,
		.colplanes	= 1,
		.mbus_code	= MEDIA_BUS_FMT_SGBRG12_1X12,
		.flags		= VIN_FMT_RAW,
	}, {
		.name		= "RAW Bayer GRBG 12bit",
		.fourcc		= V4L2_PIX_FMT_SGRBG12,
		.depth		= { 16 },
		.memplanes	= 1,
		.colplanes	= 1,
		.mbus_code	= MEDIA_BUS_FMT_SGRBG12_1X12,
		.flags		= VIN_FMT_RAW,
	}, {
		.name		= "RAW Bayer RGGB 12bit",
		.fourcc		= V4L2_PIX_FMT_SRGGB12,
		.depth		= { 16 },
		.memplanes	= 1,
		.colplanes	= 1,
		.mbus_code	= MEDIA_BUS_FMT_SRGGB12_1X12,
		.flags		= VIN_FMT_RAW,
	}, {
		.name		= "YUV 4:2:2 packed, YCbYCr",
		.fourcc		= V4L2_PIX_FMT_YUYV,
		.depth		= { 16 },
		.color		= V4L2_COLORSPACE_JPEG,
		.memplanes	= 1,
		.colplanes	= 1,
		.mbus_code	= MEDIA_BUS_FMT_YUYV8_2X8,
		.flags		= VIN_FMT_YUV,
	}, {
		.name		= "YUV 4:2:2 packed, CbYCrY",
		.fourcc		= V4L2_PIX_FMT_UYVY,
		.depth		= { 16 },
		.color		= V4L2_COLORSPACE_JPEG,
		.memplanes	= 1,
		.colplanes	= 1,
		.mbus_code	= MEDIA_BUS_FMT_UYVY8_2X8,
		.flags		= VIN_FMT_YUV,
	}, {
		.name		= "YUV 4:2:2 packed, CrYCbY",
		.fourcc		= V4L2_PIX_FMT_VYUY,
		.depth		= { 16 },
		.color		= V4L2_COLORSPACE_JPEG,
		.memplanes	= 1,
		.colplanes	= 1,
		.mbus_code	= MEDIA_BUS_FMT_VYUY8_2X8,
		.flags		= VIN_FMT_YUV,
	}, {
		.name		= "YUV 4:2:2 packed, YCrYCb",
		.fourcc		= V4L2_PIX_FMT_YVYU,
		.depth		= { 16 },
		.color		= V4L2_COLORSPACE_JPEG,
		.memplanes	= 1,
		.colplanes	= 1,
		.mbus_code	= MEDIA_BUS_FMT_YVYU8_2X8,
		.flags		= VIN_FMT_YUV,
	}, {
		.name		= "YUV 4:2:2 packed, YCbYCr",
		.fourcc		= V4L2_PIX_FMT_YUYV,
		.depth		= { 16 },
		.color		= V4L2_COLORSPACE_JPEG,
		.memplanes	= 1,
		.colplanes	= 1,
		.mbus_code	= MEDIA_BUS_FMT_YUYV8_1X16,
		.flags		= VIN_FMT_YUV,
	}, {
		.name		= "YUV 4:2:2 packed, CbYCrY",
		.fourcc		= V4L2_PIX_FMT_UYVY,
		.depth		= { 16 },
		.color		= V4L2_COLORSPACE_JPEG,
		.memplanes	= 1,
		.colplanes	= 1,
		.mbus_code	= MEDIA_BUS_FMT_UYVY8_1X16,
		.flags		= VIN_FMT_YUV,
	}, {
		.name		= "YUV 4:2:2 packed, CrYCbY",
		.fourcc		= V4L2_PIX_FMT_VYUY,
		.depth		= { 16 },
		.color		= V4L2_COLORSPACE_JPEG,
		.memplanes	= 1,
		.colplanes	= 1,
		.mbus_code	= MEDIA_BUS_FMT_VYUY8_1X16,
		.flags		= VIN_FMT_YUV,
	}, {
		.name		= "YUV 4:2:2 packed, YCrYCb",
		.fourcc		= V4L2_PIX_FMT_YVYU,
		.depth		= { 16 },
		.color		= V4L2_COLORSPACE_JPEG,
		.memplanes	= 1,
		.colplanes	= 1,
		.mbus_code	= MEDIA_BUS_FMT_YVYU8_1X16,
		.flags		= VIN_FMT_YUV,
	}, {
		.name           = "YUV 4:2:2 packed, CbYCrY",
		.fourcc         = V4L2_PIX_FMT_UYVY,
		.depth          = { 20 },
		.color          = V4L2_COLORSPACE_JPEG,
		.memplanes      = 1,
		.colplanes      = 1,
		.mbus_code      = MEDIA_BUS_FMT_UYVY10_2X10,
		.flags          = VIN_FMT_YUV,
	}, {
		.name           = "YUV 4:2:2 packed, CbYCrY",
		.fourcc         = V4L2_PIX_FMT_VYUY,
		.depth          = { 20 },
		.color          = V4L2_COLORSPACE_JPEG,
		.memplanes      = 1,
		.colplanes      = 1,
		.mbus_code      = MEDIA_BUS_FMT_VYUY10_2X10,
		.flags          = VIN_FMT_YUV,
	}, {
		.name           = "YUV 4:2:2 packed, CbYCrY",
		.fourcc         = V4L2_PIX_FMT_YVYU,
		.depth          = { 20 },
		.color          = V4L2_COLORSPACE_JPEG,
		.memplanes      = 1,
		.colplanes      = 1,
		.mbus_code      = MEDIA_BUS_FMT_YVYU10_2X10,
		.flags          = VIN_FMT_YUV,
	}, {
		.name           = "YUV 4:2:2 packed, CbYCrY",
		.fourcc         = V4L2_PIX_FMT_YUYV,
		.depth          = { 20 },
		.color          = V4L2_COLORSPACE_JPEG,
		.memplanes      = 1,
		.colplanes      = 1,
		.mbus_code      = MEDIA_BUS_FMT_YUYV10_2X10,
		.flags          = VIN_FMT_YUV,
	},
};

struct vin_fmt *vin_find_format(const u32 *pixelformat, const u32 *mbus_code,
				  unsigned int mask, int index, bool have_code)
{
	struct vin_fmt *fmt, *def_fmt = NULL;
	unsigned int i;
	int id = 0;

	if (index >= (int)ARRAY_SIZE(vin_formats))
		return NULL;

	for (i = 0; i < ARRAY_SIZE(vin_formats); ++i) {
		fmt = &vin_formats[i];

		if (!(fmt->flags & mask))
			continue;
		if (have_code && (fmt->mbus_code == 0))
			continue;

		if (pixelformat && fmt->fourcc == *pixelformat)
			return fmt;
		if (mbus_code && fmt->mbus_code == *mbus_code)
			return fmt;
		if (index == id)
			def_fmt = fmt;
		id++;
	}
	return def_fmt;
}

static size_t vin_status_dump(struct vin_core *vinc, char *buf, size_t size)
{
	struct vin_md *vind = dev_get_drvdata(vinc->v4l2_dev->dev);
	size_t count = 0;
	char out_fmt[10] = {'\0'};
	char in_fmt[10] = {'\0'};
	char in_bus[10] = {'\0'};
	char isp_mode[10] = {'\0'};

	switch (vinc->vid_cap.frame.fmt.mbus_code) {
	case MEDIA_BUS_FMT_UYVY8_2X8:
	case MEDIA_BUS_FMT_UYVY8_1X16:
		sprintf(in_fmt, "%s", "UYVY8");
		break;
	case MEDIA_BUS_FMT_VYUY8_2X8:
	case MEDIA_BUS_FMT_VYUY8_1X16:
		sprintf(in_fmt, "%s", "VYUY8");
		break;
	case MEDIA_BUS_FMT_YUYV8_2X8:
	case MEDIA_BUS_FMT_YUYV8_1X16:
		sprintf(in_fmt, "%s", "YUYV8");
		break;
	case MEDIA_BUS_FMT_YVYU8_2X8:
	case MEDIA_BUS_FMT_YVYU8_1X16:
		sprintf(in_fmt, "%s", "YVYU8");
		break;
	case MEDIA_BUS_FMT_SBGGR8_1X8:
		sprintf(in_fmt, "%s", "BGGR8");
		break;
	case MEDIA_BUS_FMT_SGBRG8_1X8:
		sprintf(in_fmt, "%s", "GBRG8");
		break;
	case MEDIA_BUS_FMT_SGRBG8_1X8:
		sprintf(in_fmt, "%s", "GRBG8");
		break;
	case MEDIA_BUS_FMT_SRGGB8_1X8:
		sprintf(in_fmt, "%s", "RGGB8");
		break;
	case MEDIA_BUS_FMT_SBGGR10_1X10:
		sprintf(in_fmt, "%s", "BGGR10");
		break;
	case MEDIA_BUS_FMT_SGBRG10_1X10:
		sprintf(in_fmt, "%s", "GBRG10");
		break;
	case MEDIA_BUS_FMT_SGRBG10_1X10:
		sprintf(in_fmt, "%s", "GRBG10");
		break;
	case MEDIA_BUS_FMT_SRGGB10_1X10:
		sprintf(in_fmt, "%s", "RGGB10");
		break;
	case MEDIA_BUS_FMT_SBGGR12_1X12:
		sprintf(in_fmt, "%s", "BGGR12");
		break;
	case MEDIA_BUS_FMT_SGBRG12_1X12:
		sprintf(in_fmt, "%s", "GBRG12");
		break;
	case MEDIA_BUS_FMT_SGRBG12_1X12:
		sprintf(in_fmt, "%s", "GRBG12");
		break;
	case MEDIA_BUS_FMT_SRGGB12_1X12:
		sprintf(in_fmt, "%s", "RGGB12");
		break;
	default:
		sprintf(in_fmt, "%s", "NULL");
		break;
	}

	switch (vinc->vid_cap.frame.fmt.fourcc) {
	case V4L2_PIX_FMT_YUV420:
		sprintf(out_fmt, "%s", "YUV420");
		break;
	case V4L2_PIX_FMT_YUV420M:
		sprintf(out_fmt, "%s", "YUV420M");
		break;
	case V4L2_PIX_FMT_YVU420:
		sprintf(out_fmt, "%s", "YVU420");
		break;
	case V4L2_PIX_FMT_YVU420M:
		sprintf(out_fmt, "%s", "YVU420M");
		break;
	case V4L2_PIX_FMT_NV21:
		sprintf(out_fmt, "%s", "NV21");
		break;
	case V4L2_PIX_FMT_NV21M:
		sprintf(out_fmt, "%s", "NV21M");
		break;
	case V4L2_PIX_FMT_NV12:
		sprintf(out_fmt, "%s", "NV12");
		break;
	case V4L2_PIX_FMT_NV12M:
		sprintf(out_fmt, "%s", "NV12M");
		break;
	case V4L2_PIX_FMT_NV16:
		sprintf(out_fmt, "%s", "NV16");
		break;
	case V4L2_PIX_FMT_NV61:
		sprintf(out_fmt, "%s", "NV61");
		break;
	case V4L2_PIX_FMT_FBC:
		sprintf(out_fmt, "%s", "FBC");
		break;
	case V4L2_PIX_FMT_LBC_2_0X:
		sprintf(out_fmt, "%s", "LBC_2X");
		break;
	case V4L2_PIX_FMT_LBC_2_5X:
		sprintf(out_fmt, "%s", "LBC_2.5X");
		break;
	case V4L2_PIX_FMT_LBC_1_0X:
		sprintf(out_fmt, "%s", "LBC_1X");
		break;
	default:
		sprintf(out_fmt, "%s", "NULL");
		break;
	}

	switch (vinc->vid_cap.frame.fmt.mbus_type) {
	case V4L2_MBUS_PARALLEL:
		sprintf(in_bus, "%s", "PARALLEL");
		break;
	case V4L2_MBUS_BT656:
		sprintf(in_bus, "%s", "BT656");
		break;
	case V4L2_MBUS_CSI2_DPHY:
		sprintf(in_bus, "%s", "MIPI");
		break;
	case V4L2_MBUS_SUBLVDS:
		sprintf(in_bus, "%s", "SUBLVDS");
		break;
	case V4L2_MBUS_HISPI:
		sprintf(in_bus, "%s", "HISPI");
		break;
	default:
		sprintf(in_bus, "%s", "NULL");
		break;
	}

	switch (vinc->vid_cap.isp_wdr_mode) {
	case ISP_NORMAL_MODE:
		sprintf(isp_mode, "%s", "NORMAL");
		break;
	case ISP_DOL_WDR_MODE:
		sprintf(isp_mode, "%s", "DOL_WDR");
		break;
	case ISP_COMANDING_MODE:
		sprintf(isp_mode, "%s", "CMD_WDR");
		break;
	case ISP_SEHDR_MODE:
		sprintf(isp_mode, "%s", "SEHDR");
		break;
	default:
		sprintf(isp_mode, "%s", "NULL");
		break;
	}

	if (vinc->id == 0) {
		count += scnprintf(buf + count, size - count, "*****************************************************\n");
		count += scnprintf(buf + count, size - count, "VIN hardware feature list:\n"
				"mcsi %d, ncsi %d, parser %d, isp %d, vipp %d, dma %d\n"
				"CSI_VERSION: CSI%x_%x, ISP_VERSION: ISP%x_%x\n"
				"CSI_TOP: %ld, CSI_ISP: %ld\n",
				vind->csic_fl.mcsi_num,	vind->csic_fl.ncsi_num,
				vind->csic_fl.parser_num, vind->csic_fl.isp_num,
				vind->csic_fl.vipp_num,	vind->csic_fl.dma_num,
				vind->csic_ver.ver_big,	vind->csic_ver.ver_small,
				vind->isp_ver_major, vind->isp_ver_minor,
				clk_get_rate(vind->clk[VIN_TOP_CLK].clock),
				clk_get_rate(vind->isp_clk[VIN_ISP_CLK].clock));
		vind->csi_bd_tatol = 0;
		count += scnprintf(buf + count, size - count, "*****************************************************\n");
	}

	if (vinc->mipi_sel == 0xff) {
#ifdef SUPPORT_ISP_TDM
		if (vinc->tdm_rx_sel == 0xff) {
			count += scnprintf(buf + count, size - count, "vi%u:\n%s => csi%d => isp%d => vipp%d\n",
				vinc->id, vinc->vid_cap.pipe.sd[VIN_IND_SENSOR]->name,
				vinc->csi_sel, vinc->isp_sel, vinc->vipp_sel);
		} else {
			count += scnprintf(buf + count, size - count, "vi%u:\n%s => csi%d => tdm_rx%d => isp%d => vipp%d\n",
				vinc->id, vinc->vid_cap.pipe.sd[VIN_IND_SENSOR]->name,
				vinc->csi_sel, vinc->tdm_rx_sel, vinc->isp_sel, vinc->vipp_sel);
		}
#else
	count += scnprintf(buf + count, size - count, "vi%u:\n%s => csi%d => isp%d => vipp%d\n",
			vinc->id, vinc->vid_cap.pipe.sd[VIN_IND_SENSOR]->name,
			vinc->csi_sel, vinc->isp_sel, vinc->vipp_sel);
#endif
	} else {
#ifdef SUPPORT_ISP_TDM
		if (vinc->tdm_rx_sel == 0xff) {
			count += scnprintf(buf + count, size - count, "vi%u:\n%s => mipi%d => csi%d => isp%d => vipp%d\n",
				vinc->id, vinc->vid_cap.pipe.sd[VIN_IND_SENSOR]->name,
				vinc->mipi_sel, vinc->csi_sel, vinc->isp_sel, vinc->vipp_sel);
		} else {
			count += scnprintf(buf + count, size - count, "vi%u:\n%s => mipi%d => csi%d => tdm_rx%d => isp%d => vipp%d\n",
				vinc->id, vinc->vid_cap.pipe.sd[VIN_IND_SENSOR]->name,
				vinc->mipi_sel, vinc->csi_sel, vinc->tdm_rx_sel, vinc->isp_sel, vinc->vipp_sel);
		}
#else
	count += scnprintf(buf + count, size - count, "vi%u:\n%s => mipi%d => csi%d => isp%d => vipp%d\n",
			vinc->id, vinc->vid_cap.pipe.sd[VIN_IND_SENSOR]->name,
			vinc->mipi_sel, vinc->csi_sel, vinc->isp_sel, vinc->vipp_sel);
#endif

	}

	count += scnprintf(buf + count, size - count,
			   "input => hoff: %u, voff: %u, w: %u, h: %u, fmt: %s\n"
			   "output => width: %u, height: %u, fmt: %s\n"
			   "interface: %s, isp_mode: %s, hflip: %u, vflip: %u\n",
			   vinc->vin_status.h_off, vinc->vin_status.v_off,
			   vinc->vin_status.width, vinc->vin_status.height, in_fmt,
			   vinc->vid_cap.frame.o_width, vinc->vid_cap.frame.o_height,
			   out_fmt, in_bus, isp_mode, vinc->hflip, vinc->vflip);

	count += scnprintf(buf + count, size - count,
			   "prs_in => x: %d, y: %d, hb: %d, hs: %d\n",
			   vinc->vin_status.prs_in.input_ht, vinc->vin_status.prs_in.input_vt,
			   vinc->vin_status.prs_in.input_hb, vinc->vin_status.prs_in.input_hs);

	count += scnprintf(buf + count, size - count,
			   "buf => cnt: %u size: %u rest: %u, mode: %s\n",
			   vinc->vin_status.buf_cnt,
			   vinc->vin_status.buf_size,
			   vinc->vin_status.buf_rest,
#ifndef BUF_AUTO_UPDATE
			   "software_update");
#else
			   "hardware_update");
#endif
	count += scnprintf(buf + count, size - count,
			   "frame => cnt: %u, lost_cnt: %u, error_cnt: %u\n"
			   "internal => avg: %u(ms), max: %u(ms), min: %u(ms)\n"
			   "CSI Bandwidth: %d\n",
			   vinc->vin_status.frame_cnt,
			   vinc->vin_status.lost_cnt,
			   vinc->vin_status.err_cnt,
			   vinc->vin_status.frame_internal/1000,
			   vinc->vin_status.max_internal/1000,
			   vinc->vin_status.min_internal/1000,
			   vinc->bandwidth = vinc->vin_status.buf_size * (1000/vinc->vin_status.frame_internal/1000));
	vind->csi_bd_tatol += vinc->bandwidth;
	count += scnprintf(buf + count, size - count, "*****************************************************\n");

	if (vinc->id == VIN_MAX_DEV - 1) {
		count += scnprintf(buf + count, size - count, "CSI Bandwidth total %dM, ISP Bandwidth total %dM\n",
							vind->csi_bd_tatol/(1024*1024), vind->isp_bd_tatol/(1024*1024));
		count += scnprintf(buf + count, size - count, "*****************************************************\n");
	}
	return count;
}

static int vin_debugfs_open(struct inode *inode, struct file *file)
{
	struct vin_debugfs_buffer *buf;
	int i = 0;

	buf = kmalloc(sizeof(*buf), GFP_KERNEL);
	if (buf == NULL)
		return -ENOMEM;

	vi_status_size_sum = 0;
	for (i = 0; i < VIN_MAX_DEV; i++) {
		if (vin_core_gbl[i] != NULL)
			vi_status_size[i] = vin_status_dump(vin_core_gbl[i],
						buf->data + vi_status_size_sum,
						sizeof(buf->data) - vi_status_size_sum);
						vi_status_size_sum += vi_status_size[i];
	}
	buf->count = vi_status_size_sum;
	file->private_data = buf;
	return 0;
}

#if !(IS_ENABLED(CONFIG_DEBUG_FS))
static ssize_t vi_node_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct vin_debugfs_buffer *vd_buf;
	int i = 0;

	vd_buf = kmalloc(sizeof(*vd_buf), GFP_KERNEL);
	if (vd_buf == NULL)
		return -ENOMEM;

	vi_status_size_sum = 0;
	for (i = 0; i < VIN_MAX_DEV; i++) {
		if (vin_core_gbl[i] != NULL)
			vi_status_size[i] = vin_status_dump(vin_core_gbl[i],
						vd_buf->data + vi_status_size_sum,
						sizeof(vd_buf->data) - vi_status_size_sum);
						vi_status_size_sum += vi_status_size[i];
	}
	return strlcpy(buf, vd_buf->data, vi_status_size_sum);
}

static DEVICE_ATTR(vi, S_IRUGO, vi_node_show, NULL);
#endif

static ssize_t vin_debugfs_read(struct file *file, char __user *user_buf,
				      size_t nbytes, loff_t *ppos)
{
	struct vin_debugfs_buffer *buf = file->private_data;

	return simple_read_from_buffer(user_buf, nbytes, ppos, buf->data,
				       buf->count);
}

static int vin_debugfs_release(struct inode *inode, struct file *file)
{
	kfree(file->private_data);
	file->private_data = NULL;

	return 0;
}

static const struct file_operations vin_debugfs_fops = {
	.owner = THIS_MODULE,
	.open = vin_debugfs_open,
	.llseek = no_llseek,
	.read = vin_debugfs_read,
	.release = vin_debugfs_release,
};
void vin_get_timestamp(struct timeval *tv)
{
	struct timespec64 ts64;

	ktime_get_ts64(&ts64);
	tv->tv_sec = ts64.tv_sec;
	tv->tv_usec = ts64.tv_nsec / NSEC_PER_USEC;
}

static void __vin_get_frame_internal(struct vin_core *vinc)
{
	struct timeval ts;

	vin_get_timestamp(&ts);
	if (vinc->vin_status.frame_cnt > 2) {
		vinc->vin_status.frame_internal = (ts.tv_sec * 1000000 + ts.tv_usec - vinc->vid_cap.ts.tv_sec * 1000000 - vinc->vid_cap.ts.tv_usec);
		if (vinc->vin_status.frame_internal > vinc->vin_status.max_internal)
			vinc->vin_status.max_internal = vinc->vin_status.frame_internal;
		if ((vinc->vin_status.frame_internal < vinc->vin_status.min_internal) || (vinc->vin_status.min_internal == 0))
			vinc->vin_status.min_internal = vinc->vin_status.frame_internal;
	}
	vinc->vid_cap.ts = ts;
}

void vin_get_rest_buf_cnt(struct vin_core *vinc)
{
	struct list_head *buf_next = NULL;

	vinc->vin_status.buf_rest = 0;
	buf_next = vinc->vid_cap.vidq_active.next;
	while ((&vinc->vid_cap.vidq_active) != buf_next) {
		vinc->vin_status.buf_rest++;
		buf_next = buf_next->next;
		if (vinc->vin_status.buf_rest >= VIDEO_MAX_FRAME)
			break;
	}
}
#ifndef BUF_AUTO_UPDATE
static void __vin_osd_auto_reverse(struct vin_core *vinc)
{
#if defined CONFIG_ARCH_SUN8IW12P1
	int fps = 30;

	if (vinc->vin_status.frame_internal) {
		fps = USEC_PER_SEC / vinc->vin_status.frame_internal;
		fps = fps < 5 ? 30 : fps;
	}

	if (vinc->vid_cap.osd.overlay_en && !(vinc->vin_status.frame_cnt % (fps / 5))) {
		unsigned int osd_stat[MAX_OVERLAY_NUM];
		unsigned int inverse[MAX_OVERLAY_NUM];
		int i;

		sunxi_vipp_get_osd_stat(vinc->vipp_sel, &osd_stat[0]);
		for (i = 0; i < vinc->vid_cap.osd.overlay_cnt; i++) {
			if (vinc->vid_cap.osd.inverse_close[i]) {
				inverse[i] = 0;
				continue;
			}
			osd_stat[i] /= (vinc->vid_cap.osd.ov_win[i].width * vinc->vid_cap.osd.ov_win[i].height);
			if (abs(osd_stat[i] - vinc->vid_cap.osd.y_bmp_avp[i]) < 80)
				inverse[i] = 1;
			else
				inverse[i] = 0;
		}
		vipp_osd_inverse(vinc->vipp_sel, inverse, vinc->vid_cap.osd.overlay_cnt);
	}
#endif
}

static void __vin_ptn_update(struct vin_core *vinc)
{
#ifdef SUPPORT_PTN
	if (!vinc->ptn_cfg.ptn_en)
		return;
	if (vinc->ptn_cfg.ptn_type > 0) {
		ptn_frame_cnt++;
		if (ptn_frame_cnt%(vinc->ptn_cfg.ptn_type) == 0) {
			if (ptn_frame_cnt/(vinc->ptn_cfg.ptn_type) < 3)
				csic_ptn_addr(0, (unsigned long)(vinc->ptn_cfg.ptn_buf.dma_addr));
			else
				csic_ptn_addr(0, (unsigned long)(vinc->ptn_cfg.ptn_buf.dma_addr + (ptn_frame_cnt/vinc->ptn_cfg.ptn_type-2)%3 * vinc->ptn_cfg.ptn_buf.size / 3));
			csic_ptn_generation_en(0, 1);
		}
	} else {
		csic_ptn_generation_en(0, 1);
	}
#endif
}
#endif

void vin_vsync_isr(struct vin_vid_cap *cap)
{
	struct v4l2_event event;
	struct vin_vsync_event_data *data = (void *)event.u.data;

	memset(&event, 0, sizeof(event));
	event.type = V4L2_EVENT_VSYNC;
	event.id = 0;

	data->frame_number = cap->vinc->vin_status.frame_cnt;
	v4l2_event_queue(&cap->vdev, &event);
}

/*
 *  the interrupt routine
 */
static irqreturn_t vin_isr(int irq, void *priv)
{
	unsigned long flags;
	struct vin_buffer *buf;
	struct vin_vid_cap *cap = (struct vin_vid_cap *)priv;
	struct vin_core *vinc = cap->vinc;
	struct dma_int_status status;
	int need_callback = 0;
	bool display_sync = false;
#ifndef BUF_AUTO_UPDATE
	struct dma_output_size size;
#else
	struct list_head *buf_next = NULL;
	u64 timestamp_ns;
	int i;
#endif

	if (vin_streaming(cap) == 0) {
		csic_dma_int_clear_status(vinc->vipp_sel, DMA_INT_ALL);
		return IRQ_HANDLED;
	}

	if (cap->special_active == 1)
		need_callback = 0;

#ifdef CONFIG_DISPPLAY_SYNC
	if (vinc->id == disp_sync_video)
		display_sync = true;
#endif

	csic_dma_int_get_status(vinc->vipp_sel, &status);

	vin_timer_update(vinc, 2000);

	spin_lock_irqsave(&cap->slock, flags);

	/* exception handle */
	if ((status.buf_0_overflow) || (status.buf_1_overflow) ||
	    (status.buf_2_overflow) || (status.hblank_overflow)) {
		if ((status.buf_0_overflow) || (status.buf_1_overflow) || (status.buf_2_overflow)) {
			csic_dma_int_clear_status(vinc->vipp_sel, DMA_INT_BUF_0_OVERFLOW | DMA_INT_BUF_1_OVERFLOW | DMA_INT_BUF_2_OVERFLOW);
			vinc->vin_status.err_cnt++;
			vin_err("video%d fifo overflow, CSI frame count is %d\n", vinc->id, vinc->vin_status.frame_cnt);
		}
		if (status.hblank_overflow) {
			csic_dma_int_clear_status(vinc->vipp_sel, DMA_INT_HBLANK_OVERFLOW);
			vinc->vin_status.err_cnt++;
			vin_err("video%d hblank overflow, CSI frame count is %d\n", vinc->id, vinc->vin_status.frame_cnt);
		}
		sunxi_isp_reset(v4l2_get_subdevdata(cap->pipe.sd[VIN_IND_ISP]));
	}

	if (status.fbc_ovhd_wrddr_full) {
		csic_dma_int_clear_status(vinc->vipp_sel, DMA_INT_FBC_OVHD_WRDDR_FULL);
		/*when open isp debug please ignore fbc error!*/
		if (!vinc->isp_dbg.debug_en)
			vin_err("video%d fbc overhead write ddr full\n", vinc->id);
	}

	if (status.fbc_data_wrddr_full) {
		csic_dma_int_clear_status(vinc->vipp_sel, DMA_INT_FBC_DATA_WRDDR_FULL);
		vin_err("video%d fbc data write ddr full\n", vinc->id);
	}

	if (status.lbc_hb) {
		csic_dma_int_clear_status(vinc->vipp_sel, DMA_INT_LBC_HB);
		vin_err("video%d lbc hblanking less than 48 bk_clk cycles\n", vinc->id);
	}
#ifndef BUF_AUTO_UPDATE
	if (status.vsync_trig) {
		csic_dma_int_clear_status(vinc->vipp_sel, DMA_INT_VSYNC_TRIG);
		vinc->vin_status.frame_cnt++;
		csic_prs_input_para_get(vinc->csi_sel, 0, &vinc->vin_status.prs_in);
		vin_vsync_isr(cap);
		if (vinc->large_image == 2) {
			size.hor_len = cap->frame.o_width / 2;
			size.ver_len = cap->frame.o_height;
			size.hor_start = cap->frame.offs_h;
			size.ver_start = cap->frame.offs_v;
			if (vinc->vin_status.frame_cnt % 2 == 0)
				size.hor_start = 32;
			csic_dma_output_size_cfg(vinc->vipp_sel, &size);
		}
		if (cap->capture_mode != V4L2_MODE_IMAGE && !display_sync) {
			if (cap->first_flag && vinc->large_image != 2) {
				if ((&cap->vidq_active) == cap->vidq_active.next->next->next) {
					vin_log(VIN_LOG_VIDEO, "Only two buffer left for video%d\n", vinc->id);
					vinc->vin_status.lost_cnt++;
					goto unlock;
				}
				buf = list_entry(cap->vidq_active.next, struct vin_buffer, list);
				buf->vb.sequence = csic_dma_get_frame_cnt(vinc->vipp_sel);
				buf->vb.vb2_buf.timestamp = ktime_get_ns();
				buf->vb.sequence = vinc->vin_status.frame_cnt;
				list_del(&buf->list);

				if (cap->frame_delay_cnt > 0) {
					if (cap->frame_delay_cnt >= 5)
						cap->frame_delay_cnt = 5;
					list_add_tail(&buf->list, &cap->vidq_active);
					cap->frame_delay_cnt--;
					vinc->vin_status.lost_cnt++;
				} else {
					if (cap->special_active == 1) {
						list_add_tail(&buf->list, &cap->vidq_done);
						need_callback = 1;
					} else
						vb2_buffer_done(&buf->vb.vb2_buf, VB2_BUF_STATE_DONE);
				}
			}
			if (list_empty(&cap->vidq_active) || cap->vidq_active.next->next == &cap->vidq_active) {
				vin_log(VIN_LOG_VIDEO, "No active queue to serve\n");
				goto unlock;
			}

#ifdef SUPPORT_PTN
			if (vinc->large_image == 1)
				csic_dma_buffer_address(vinc->vipp_sel, CSI_BUF_0_A, (unsigned long)vinc->ptn_cfg.ptn_buf.dma_addr);
			else if ((vinc->large_image == 2) && (vinc->vin_status.frame_cnt % 2)) {
				buf = list_entry(cap->vidq_active.next, struct vin_buffer, list);
				vin_set_addr(vinc, &buf->vb.vb2_buf, &vinc->vid_cap.frame, &vinc->vid_cap.frame.paddr);
			} else {
				buf = list_entry(cap->vidq_active.next->next, struct vin_buffer, list);
				vin_set_addr(vinc, &buf->vb.vb2_buf, &vinc->vid_cap.frame, &vinc->vid_cap.frame.paddr);
			}
#else
			buf = list_entry(cap->vidq_active.next->next, struct vin_buffer, list);
			vin_set_addr(vinc, &buf->vb.vb2_buf, &vinc->vid_cap.frame, &vinc->vid_cap.frame.paddr);
#endif
		}
#ifdef CONFIG_DISPPLAY_SYNC
		if (cap->first_flag == 1 && display_sync) {
			buf = list_entry(cap->vidq_active.next, struct vin_buffer, list);
			buf->vb.sequence = csic_dma_get_frame_cnt(vinc->vipp_sel);
			buf->vb.vb2_buf.timestamp = ktime_get_ns();
			list_del(&buf->list);
			vb2_buffer_done(&buf->vb.vb2_buf, VB2_BUF_STATE_DONE);
			cap->first_flag++;
		}
#endif
		if (cap->first_flag == 0) {
			cap->first_flag++;
			vin_log(VIN_LOG_VIDEO, "video%d first frame!\n", vinc->id);
		}
	}

	/*when open isp debug line count interrupt would not come!*/
	if (status.line_cnt_flag) {
		csic_dma_int_clear_status(vinc->vipp_sel, DMA_INT_LINE_CNT);
		vin_log(VIN_LOG_VIDEO, "video%d line_cnt interrupt!\n", vinc->id);
	}

	if (status.capture_done) {
		csic_dma_int_clear_status(vinc->vipp_sel, DMA_INT_CAPTURE_DONE);
		if (cap->capture_mode == V4L2_MODE_IMAGE) {
			vin_log(VIN_LOG_VIDEO, "capture image mode!\n");
			buf = list_entry(cap->vidq_active.next, struct vin_buffer, list);
			buf->vb.vb2_buf.timestamp = ktime_get_ns();
			list_del(&buf->list);
			if (cap->special_active == 1) {
				list_add_tail(&buf->list, &cap->vidq_done);
				need_callback = 1;
			} else
				vb2_buffer_done(&buf->vb.vb2_buf, VB2_BUF_STATE_DONE);
		}
	}

	if (status.frame_done) {
		csic_dma_int_clear_status(vinc->vipp_sel, DMA_INT_FRAME_DONE);

		__vin_ptn_update(vinc);

		if (!display_sync) {
			vin_get_rest_buf_cnt(vinc);

			if (list_empty(&cap->vidq_active) || cap->vidq_active.next->next == &cap->vidq_active) {
				vin_log(VIN_LOG_VIDEO, "No active queue to serve\n");
				goto unlock;
			}

			/* video buffer handle */
			if ((&cap->vidq_active) == cap->vidq_active.next->next->next) {
				vin_log(VIN_LOG_VIDEO, "Only two buffer left for video%d\n", vinc->id);
				vin_get_timestamp(&cap->ts);
				goto unlock;
			}
			buf = list_entry(cap->vidq_active.next, struct vin_buffer, list);

			/* Nobody is waiting on this buffer */
			if (!cap->special_active) {
				if (!waitqueue_active(&buf->vb.vb2_buf.vb2_queue->done_wq))
					vin_log(VIN_LOG_VIDEO, "Nobody is waiting on video%d buffer%d\n",
						vinc->id, buf->vb.vb2_buf.index);
			}
		}

		__vin_get_frame_internal(vinc);

		if (vinc->large_image == 2) {
			if (vinc->vin_status.frame_cnt % 2 == 0) {
				list_del(&buf->list);
				vb2_buffer_done(&buf->vb.vb2_buf, VB2_BUF_STATE_DONE);
			}
		}
		__vin_osd_auto_reverse(vinc);
	}
#else
	if (status.vsync_trig) {
		csic_dma_int_clear_status(vinc->vipp_sel, DMA_INT_VSYNC_TRIG);
		vinc->vin_status.frame_cnt++;
		csic_prs_input_para_get(vinc->csi_sel, 0, &vinc->vin_status.prs_in);
		vin_vsync_isr(cap);
		vin_get_rest_buf_cnt(vinc);
		__vin_get_frame_internal(vinc);
	}

	if (status.frm_lost) {
		csic_dma_int_clear_status(vinc->vipp_sel, DMA_INT_FRM_LOST);
		vinc->vin_status.frame_cnt++;
		vinc->vin_status.lost_cnt++;
		vin_log(VIN_LOG_VIDEO, "video%d Frame lost interrupt!\n", vinc->id);
	}

	if (status.buf_addr_fifo) {
		csic_dma_int_clear_status(vinc->vipp_sel, DMA_INT_BUF_ADDR_FIFO);
		vin_log(VIN_LOG_VIDEO, "video%d buf addr set interrupt!\n", vinc->id);
	}

	if (status.capture_done) {
		csic_dma_int_clear_status(vinc->vipp_sel, DMA_INT_CAPTURE_DONE);
		if (cap->capture_mode == V4L2_MODE_IMAGE) {
			vin_log(VIN_LOG_VIDEO, "capture image mode!\n");
			buf = list_entry(cap->vidq_active.next, struct vin_buffer, list);
			buf->vb.vb2_buf.timestamp = ktime_get_ns();
			list_del(&buf->list);
			vb2_buffer_done(&buf->vb.vb2_buf, VB2_BUF_STATE_DONE);
		}
	}

	if (status.stored_frm_cnt) {
		csic_dma_int_clear_status(vinc->vipp_sel, DMA_INT_STORED_FRM_CNT);
		timestamp_ns = ktime_get_ns();
		buf_next = cap->vidq_active.next;
		for (i = 0; i < cap->threshold.stored_frm_threshold; i++) {
			buf = list_entry(buf_next, struct vin_buffer, list);
			if (list_empty(&cap->vidq_active) || (&cap->vidq_active) == cap->vidq_active.next->next) {
				vin_log(VIN_LOG_VIDEO, "video%d rest buf <= 1!\n", vinc->id);
				goto unlock;
			}
			/* Nobody is waiting on this buffer */
			if (!waitqueue_active(&buf->vb.vb2_buf.vb2_queue->done_wq))
				vin_log(VIN_LOG_VIDEO, "Nobody is waiting on video%d buffer%d\n", vinc->id, buf->vb.vb2_buf.index);
			buf->vb.sequence = csic_dma_get_frame_cnt(vinc->vipp_sel) - (cap->threshold.stored_frm_threshold - 1) + i;
			buf->vb.vb2_buf.timestamp = timestamp_ns - ((cap->threshold.stored_frm_threshold - 1 - i) *
							vinc->vin_status.frame_internal * 1000);
			buf->vb.sequence = vinc->vin_status.frame_cnt - (cap->threshold.stored_frm_threshold - 1) + i;

			buf_next = buf_next->next;
			list_del(&buf->list);
			vb2_buffer_done(&buf->vb.vb2_buf, VB2_BUF_STATE_DONE);
		}
	}
#endif

unlock:
	spin_unlock_irqrestore(&cap->slock, flags);

	if (cap->special_active && need_callback && cap->vin_buffer_process)
		cap->vin_buffer_process(vinc->id);

	return IRQ_HANDLED;
}

static int vin_irq_request(struct vin_core *vinc, int i)
{
	int ret;
	struct device_node *np = vinc->pdev->dev.of_node;
	/*get irq resource */
	vinc->irq = irq_of_parse_and_map(np, i);
	if (vinc->irq <= 0) {
		vin_err("failed to get CSI DMA IRQ resource\n");
		return -ENXIO;
	}

	ret = request_irq(vinc->irq, vin_isr, IRQF_SHARED,
			vinc->pdev->name, &vinc->vid_cap);

	if (ret) {
		vin_err("failed to install CSI DMA irq (%d)\n", ret);
		return -ENXIO;
	}
	return 0;
}

static void vin_irq_release(struct vin_core *vinc)
{
	if (vinc->irq > 0)
		free_irq(vinc->irq, &vinc->vid_cap);
}

#ifdef CONFIG_PM_SLEEP
int vin_core_suspend(struct device *d)
{
	struct vin_core *vinc = (struct vin_core *)dev_get_drvdata(d);
	struct vin_vid_cap *cap = &vinc->vid_cap;
	__maybe_unused struct isp_dev *isp;
	__maybe_unused int ret;

	vin_log(VIN_LOG_POWER, "%s\n", __func__);
	if (!vinc->vid_cap.registered)
		return 0;
	if (test_and_set_bit(VIN_LPM, &vinc->vid_cap.state))
		return 0;

	if (vin_busy(cap)) {
#if defined CONFIG_ARCH_SUN8IW16P1 || defined CONFIG_ARCH_SUN8IW19P1
		vin_timer_del(vinc);

		mutex_lock(&cap->vdev.entity.graph_obj.mdev->graph_mutex);
		if (!cap->pipe.sd[VIN_IND_SENSOR]->entity.use_count) {
			vin_err("%s is not used, cannot be suspend!\n", cap->pipe.sd[VIN_IND_SENSOR]->name);
			mutex_unlock(&cap->vdev.entity.graph_obj.mdev->graph_mutex);
			return -1;
		}

		if (cap->pipe.sd[VIN_IND_ISP] != NULL) {
			isp = container_of(cap->pipe.sd[VIN_IND_ISP], struct isp_dev, subdev);
			isp->runtime_flag = 1;
		}

		clear_bit(VIN_STREAM, &cap->state);
		ret = vin_pipeline_call(vinc, set_stream, &cap->pipe, 0);
		if (ret)
			vin_err("vin pipeline streamoff failed!\n");

		ret = vin_pipeline_call(vinc, close, &cap->pipe);
		if (ret)
			vin_err("vin pipeline close failed!\n");

		v4l2_subdev_call(cap->pipe.sd[VIN_IND_ISP], core, init, 0);
		mutex_unlock(&cap->vdev.entity.graph_obj.mdev->graph_mutex);
		vin_log(VIN_LOG_POWER, "vinc%d suspend streamoff and close pipeline at %s!\n", vinc->id, __func__);
#else
		vin_print("before %s please close video%d\n", __func__, vinc->id);
#endif
	}
	return 0;
}
int vin_core_resume(struct device *d)
{
	struct vin_core *vinc = (struct vin_core *)dev_get_drvdata(d);
	struct vin_vid_cap *cap = &vinc->vid_cap;
	__maybe_unused struct isp_dev *isp;
	__maybe_unused struct sensor_info *info = container_of(cap->pipe.sd[VIN_IND_SENSOR], struct sensor_info, sd);
#ifdef CONFIG_ENABLE_SENSOR_FLIP_OPTION
	struct v4l2_control c;
#endif

	__maybe_unused int ret;

	vin_log(VIN_LOG_POWER, "%s\n", __func__);
	if (!vinc->vid_cap.registered)
		return 0;

	if (!test_and_clear_bit(VIN_LPM, &vinc->vid_cap.state)) {
		vin_print("VIN not suspend!\n");
		return 0;
	}

	if (vin_busy(cap)) {
#if defined CONFIG_ARCH_SUN8IW16P1 || defined CONFIG_ARCH_SUN8IW19P1
		mutex_lock(&cap->vdev.entity.graph_obj.mdev->graph_mutex);
		if (cap->pipe.sd[VIN_IND_ISP] != NULL) {
			isp = container_of(cap->pipe.sd[VIN_IND_ISP], struct isp_dev, subdev);
			isp->runtime_flag = 0;
		}

		ret = vin_pipeline_call(vinc, open, &cap->pipe, &cap->vdev.entity, true);
		if (ret < 0)
			vin_err("vin pipeline open failed (%d)!\n", ret);

		v4l2_subdev_call(cap->pipe.sd[VIN_IND_ISP], core, init, 1);
		if (ret < 0)
			vin_err("ISP init error at %s\n", __func__);

		ret = v4l2_subdev_call(cap->pipe.sd[VIN_IND_SCALER], core, init, 1);
		if (ret < 0)
			vin_err("SCALER init error at %s\n", __func__);

		if (info) {
			vinc->exp_gain.exp_val = info->exp;
			vinc->exp_gain.gain_val = info->gain;
			vinc->stream_idx = info->stream_seq + 1;
		}
		vin_timer_init(vinc);
		vin_pipeline_call(cap->vinc, set_stream, &cap->pipe, cap->vinc->stream_idx);
		set_bit(VIN_STREAM, &cap->state);

#ifdef CONFIG_ENABLE_SENSOR_FLIP_OPTION
		if (vinc->sensor_hflip) {
			c.id = V4L2_CID_HFLIP;
			c.value = vinc->sensor_hflip;
			sensor_flip_option(cap, c);
		}
		if (vinc->sensor_vflip) {
			c.id = V4L2_CID_VFLIP;
			c.value = vinc->sensor_vflip;
			sensor_flip_option(cap, c);
		}
#endif

		if (cap->vinc->exp_gain.exp_val && cap->vinc->exp_gain.gain_val) {
			v4l2_subdev_call(cap->pipe.sd[VIN_IND_SENSOR], core, ioctl,
				VIDIOC_VIN_SENSOR_EXP_GAIN, &cap->vinc->exp_gain);
		}
		mutex_unlock(&cap->vdev.entity.graph_obj.mdev->graph_mutex);
		vin_log(VIN_LOG_POWER, "vinc%d resume open pipeline and streamon at %s!\n", vinc->id, __func__);
#else
		vin_print("after %s please open video%d\n", __func__, vinc->id);
#endif
	}
	return 0;
}
#endif

#ifdef CONFIG_PM
int vin_core_runtime_suspend(struct device *d)
{
	vin_log(VIN_LOG_POWER, "%s\n", __func__);
	return 0;
}
int vin_core_runtime_resume(struct device *d)
{
	vin_log(VIN_LOG_POWER, "%s\n", __func__);
	return 0;
}
int vin_core_runtime_idle(struct device *d)
{
	if (d) {
		pm_runtime_mark_last_busy(d);
		pm_request_autosuspend(d);
	} else {
		vin_err("%s, vfe device is null\n", __func__);
	}
	return 0;
}
#endif
static void vin_core_shutdown(struct platform_device *pdev)
{
#if defined(CONFIG_PM) || defined(CONFIG_SUSPEND)
	struct vin_core *vinc = (struct vin_core *)dev_get_drvdata(&pdev->dev);
	struct vin_vid_cap *cap = &vinc->vid_cap;
	struct isp_dev *isp = NULL;
	int ret;

	if (!vin_busy(cap)) {
		vin_warn("video%d device have been closed!\n", vinc->id);
		return;
	}

	if (vin_streaming(cap))
		vin_timer_del(vinc);

	mutex_lock(&cap->vdev.entity.graph_obj.mdev->graph_mutex);
	if (!cap->pipe.sd[VIN_IND_SENSOR]->entity.use_count) {
		vin_err("%s is not used, cannot be shutdown!\n", cap->pipe.sd[VIN_IND_SENSOR]->name);
		mutex_unlock(&cap->vdev.entity.graph_obj.mdev->graph_mutex);
		return;
	}

	if (vin_streaming(cap)) {
		clear_bit(VIN_STREAM, &cap->state);
		if (cap->pipe.sd[VIN_IND_ISP] != NULL) {
			isp = container_of(cap->pipe.sd[VIN_IND_ISP], struct isp_dev, subdev);
			isp->nosend_ispoff = 1;
		}
		vin_pipeline_call(vinc, set_stream, &cap->pipe, 0);
	}

	ret = vin_pipeline_call(vinc, close, &cap->pipe);
	if (ret)
		vin_err("vin pipeline close failed!\n");

	set_bit(VIN_LPM, &cap->state);
	clear_bit(VIN_BUSY, &cap->state);
	mutex_unlock(&cap->vdev.entity.graph_obj.mdev->graph_mutex);
	vin_log(VIN_LOG_VIDEO, "video%d shutdown\n", vinc->id);
#endif
}

static const struct dev_pm_ops vin_core_runtime_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(vin_core_suspend, vin_core_resume)
	SET_RUNTIME_PM_OPS(vin_core_runtime_suspend, vin_core_runtime_resume,
			       vin_core_runtime_idle)
};

static int vin_core_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct vin_core *vinc;
	char property_name[32] = { 0 };
	int ret = 0;

	vin_log(VIN_LOG_VIDEO, "%s\n", __func__);
	/*request mem for vinc */
	vinc = kzalloc(sizeof(struct vin_core), GFP_KERNEL);
	if (!vinc) {
		vin_err("request vinc mem failed!\n");
		ret = -ENOMEM;
		goto ekzalloc;
	}

	of_property_read_u32(np, "device_id", &pdev->id);
	if (pdev->id < 0) {
		vin_err("vin core failed to get device id\n");
		ret = -EINVAL;
		goto freedev;
	}

	sprintf(property_name, "vinc%d_rear_sensor_sel", pdev->id);
	if (of_property_read_u32(np, property_name, &vinc->rear_sensor))
		vinc->rear_sensor = 0;

	sprintf(property_name, "vinc%d_front_sensor_sel", pdev->id);
	if (of_property_read_u32(np, property_name, &vinc->front_sensor))
		vinc->front_sensor = 1;

	sprintf(property_name, "vinc%d_csi_sel", pdev->id);
	if (of_property_read_u32(np, property_name, &vinc->csi_sel))
		vinc->csi_sel = 0;

	sprintf(property_name, "vinc%d_mipi_sel", pdev->id);
	if (of_property_read_u32(np, property_name, &vinc->mipi_sel))
		vinc->mipi_sel = 0xff;

	sprintf(property_name, "vinc%d_isp_sel", pdev->id);
	if (of_property_read_u32(np, property_name, &vinc->isp_sel))
		vinc->isp_sel = 0;

	sprintf(property_name, "vinc%d_isp_tx_ch", pdev->id);
	if (of_property_read_u32(np, property_name, &vinc->isp_tx_ch))
		vinc->isp_tx_ch = 0;

	sprintf(property_name, "vinc%d_tdm_rx_sel", pdev->id);
	if (of_property_read_u32(np, property_name, &vinc->tdm_rx_sel))
		vinc->tdm_rx_sel = 0;

	vinc->vipp_sel = pdev->id;

	vinc->id = pdev->id;
	vinc->pdev = pdev;

	vinc->base = of_iomap(np, 0);
	if (!vinc->base) {
		vinc->is_empty = 1;
		vinc->base = kzalloc(0x100, GFP_KERNEL);
		if (!vinc->base) {
			ret = -EIO;
			goto freedev;
		}
	}

	csic_dma_set_base_addr(vinc->id, (unsigned long)vinc->base);

	dev_set_drvdata(&vinc->pdev->dev, vinc);


	vin_core_gbl[vinc->id] = vinc;

	vin_log(VIN_LOG_VIDEO, "vinc->id = %d\n", vinc->id);
	vin_log(VIN_LOG_VIDEO, "rear_sensor_sel = %d\n", vinc->rear_sensor);
	vin_log(VIN_LOG_VIDEO, "front_sensor_sel = %d\n", vinc->front_sensor);
	vin_log(VIN_LOG_VIDEO, "csi_sel = %d\n", vinc->csi_sel);
	vin_log(VIN_LOG_VIDEO, "mipi_sel = %d\n", vinc->mipi_sel);
	vin_log(VIN_LOG_VIDEO, "isp_sel = %d\n", vinc->isp_sel);
	vin_log(VIN_LOG_VIDEO, "isp_tx_ch = %d\n", vinc->isp_tx_ch);
	vin_log(VIN_LOG_VIDEO, "vipp_sel = %d\n", vinc->vipp_sel);
	vin_log(VIN_LOG_VIDEO, "tdm_rx_sel = %d\n", vinc->tdm_rx_sel);

	vin_irq_request(vinc, 0);

	ret = vin_initialize_capture_subdev(vinc);
	if (ret)
		goto unmap;

	/*initial parameter */
	ret = sysfs_create_group(&vinc->pdev->dev.kobj, &vin_attribute_group);
	if (ret) {
		vin_err("sysfs_create_group failed!\n");
		goto unmap;
	}

#ifdef CONFIG_PM
	pm_runtime_enable(&pdev->dev);
#endif
	return 0;
unmap:
	if (!vinc->is_empty)
		iounmap(vinc->base);
	else
		kfree(vinc->base);
freedev:
	kfree(vinc);
ekzalloc:
	vin_err("%s error!\n", __func__);
	return ret;
}

static int vin_core_remove(struct platform_device *pdev)
{
	struct vin_core *vinc = (struct vin_core *)dev_get_drvdata(&pdev->dev);

	sysfs_remove_group(&vinc->pdev->dev.kobj, &vin_attribute_group);
#ifdef CONFIG_PM
	pm_runtime_disable(&vinc->pdev->dev);
#endif
	vin_irq_release(vinc);
	vin_cleanup_capture_subdev(vinc);
	if (vinc->base) {
		if (!vinc->is_empty)
			iounmap(vinc->base);
		else
			kfree(vinc->base);
	}
	kfree(vinc);
	vin_log(VIN_LOG_VIDEO, "%s end\n", __func__);
	return 0;
}

#if IS_ENABLED(CONFIG_DEBUG_FS)
int sunxi_vin_debug_register_driver(void)
{
#if IS_ENABLED(CONFIG_SUNXI_MPP)
	vi_debugfs_root = debugfs_mpp_root;
#else
	vi_debugfs_root = debugfs_create_dir("mpp", NULL);
#endif
	if (vi_debugfs_root == NULL)
		return -ENOENT;

	vi_node = debugfs_create_file("vi", 0444, vi_debugfs_root,
				   NULL, &vin_debugfs_fops);
	if (IS_ERR_OR_NULL(vi_node)) {
		vin_err("Unable to create debugfs status file.\n");
		vi_debugfs_root = NULL;
		return -ENODEV;
	}

	return 0;
}
#else
int sunxi_vin_debug_register_driver(void)
{
	struct vin_core *vinc;
	int ret, i;

	for (i = 0; i < VIN_MAX_DEV; i++) {
		vinc = sunxi_vin_core_get_dev(i);
		if (vinc)
			break;
	}
	if (!vinc && vinc->v4l2_dev && vinc->v4l2_dev->dev)
		return -1;
	ret = device_create_file(vinc->v4l2_dev->dev, &dev_attr_vi);
	if (ret) {
		vin_err("vin debug node register fail\n");
		return ret;
	}
	return 0;
}
#endif

#if IS_ENABLED(CONFIG_DEBUG_FS)
void sunxi_vin_debug_unregister_driver(void)
{
	if (vi_debugfs_root == NULL)
		return;
#if IS_ENABLED(CONFIG_SUNXI_MPP)
	debugfs_remove_recursive(vi_node);
#else
	debugfs_remove_recursive(vi_debugfs_root);
	vi_debugfs_root = NULL;
#endif
}
#else
void sunxi_vin_debug_unregister_driver(void)
{
	struct vin_core *vinc;
	int i;

	for (i = 0; i < VIN_MAX_DEV; i++) {
		vinc = sunxi_vin_core_get_dev(i);
		if (vinc)
			break;
	}
	if (!vinc && vinc->v4l2_dev && vinc->v4l2_dev->dev)
		return;
	device_remove_file(vinc->v4l2_dev->dev, &dev_attr_vi);
}
#endif

static const struct of_device_id sunxi_vin_core_match[] = {
	{.compatible = "allwinner,sunxi-vin-core",},
	{},
};

static struct platform_driver vin_core_driver = {
	.probe = vin_core_probe,
	.remove = vin_core_remove,
	.shutdown = vin_core_shutdown,
	.driver = {
		   .name = VIN_CORE_NAME,
		   .owner = THIS_MODULE,
		   .of_match_table = sunxi_vin_core_match,
		   .pm = &vin_core_runtime_pm_ops,
		   },
};
int sunxi_vin_core_register_driver(void)
{
	int ret;

	vin_log(VIN_LOG_VIDEO, "vin core register driver\n");
	ret = platform_driver_register(&vin_core_driver);
	return ret;
}

void sunxi_vin_core_unregister_driver(void)
{
	vin_log(VIN_LOG_VIDEO, "vin core unregister driver\n");
	platform_driver_unregister(&vin_core_driver);
}

struct vin_core *sunxi_vin_core_get_dev(int index)
{
	return vin_core_gbl[index];
}

