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
#ifndef _SUNXI_DRM_BOOTLOGO_H_
#define _SUNXI_DRM_BOOTLOGO_H_

#include <drm/drm_atomic.h>
#include <drm/drm_crtc.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_crtc_helper.h>
#include <linux/dma-buf.h>
#include <linux/dma-mapping.h>
#include <linux/module.h>

enum bootlogo_pictures_format {
	BOOTLOGO_FORMAT_BMP = 0,
	BOOTLOGO_FORMAT_OTHERS = 1,
};

struct bmp_header {
	/* Header */
	char signature[2];
	u32 file_size;
	u32 reserved;
	u32 data_offset;
	/* InfoHeader */
	u32 size;
	u32 width;
	u32 height;
	u16 planes;
	u16 bit_count;
	u32 compression;
	u32 image_size;
	u32 x_pixels_per_m;
	u32 y_pixels_per_m;
	u32 colors_used;
	u32 colors_important;
	/* ColorTable */
} __packed;

struct bootlogo_info {
	unsigned int pics_format;

	unsigned int phy_addr;
	unsigned int width;
	unsigned int height;
	unsigned int bpp;
	unsigned int stride;
	unsigned int left;
	unsigned int top;
	unsigned int right;
	unsigned int bottom;
};

void sunxi_drm_get_bootlogoinfo_from_dts_info(
	struct bootlogo_info *bootlogo, char *dts_attr);
void sunxi_drm_get_bootlogoinfo_from_bmp_header(
	struct bootlogo_info *bootlogo, struct bmp_header *header,
	unsigned int fb_base);
int sunxi_drm_parse_bmp_header(struct bmp_header *header,
				unsigned long phy_addr);
int sunxi_drm_fbdev_copy_bootlogo(
	struct drm_device *dev, struct bootlogo_info *bootlogo);
#endif
