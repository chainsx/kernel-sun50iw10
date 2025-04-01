/* sunxi_drm_fbdev.h
 *
 * Copyright (C) 2022 Allwinnertech Co., Ltd.
 * Authors: zhengwanyu <zhengwanyu@allwinnertech.com>
 * Authors: hongyaobin <hongyaobin@allwinnertech.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef _SUNXI_DRM_FBDEV_H_
#define _SUNXI_DRM_FBDEV_H_

#include <drm/drm_atomic.h>
#include <drm/drm_crtc.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_crtc_helper.h>
#include <linux/dma-buf.h>
#include <linux/dma-mapping.h>
#include <linux/module.h>

#include "sunxi_drm_fb.h"
#define DRM_DEBUG 0
#ifdef CONFIG_DRM_FBDEV_EMULATION_DUAL_DISPLAY_WITH_DIFFERENT_BUFFER
#define FBDEV_MAX_NUM 2
#else
#define FBDEV_MAX_NUM 1
#endif

struct sunxi_drm_fbdev {
	struct sunxi_drm_fb *sunxi_fb;
	struct drm_fb_helper fb_helper;
};

static inline struct sunxi_drm_fbdev *to_sunxi_fbdev(struct drm_fb_helper *helper)
{
	return container_of(helper, struct sunxi_drm_fbdev, fb_helper);
}

struct sunxi_drm_fbdev *sunxi_drm_fbdev_get_fbdev(int index);

void sunxi_drm_fbdev_output_poll_changed(struct drm_device *dev);
int sunxi_drm_fbdev_init(struct drm_device *dev);
void sunxi_drm_fbdev_exit(struct drm_device *dev);
#endif
