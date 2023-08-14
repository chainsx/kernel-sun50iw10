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
#ifndef _SUNXI_DRM_SYSFS_H_
#define _SUNXI_DRM_SYSFS_H_
#define DRM_DEBUG 0
extern unsigned int drm_debug;

int sunxi_drm_sysfs_init(struct drm_device *dev);
void sunxi_drm_sysfs_exit(struct drm_device *dev);
#endif
