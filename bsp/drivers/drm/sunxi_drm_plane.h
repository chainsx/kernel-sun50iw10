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

#ifndef _SUNXI_DRM_PLANE_H_
#define _SUNXI_DRM_PLANE_H_

#include <drm/drm_crtc.h>
#include "de/include.h"

enum sunxi_plane_alpha_mode {
	PIXEL_ALPHA = 0,
	GLOBAL_ALPHA = 1,
	MIXED_ALPHA = 2,
	NONE_ALPHA_MODE,
	ALPHA_MODE_NUM,
};

struct sunxi_drm_plane {
	struct drm_plane plane;
	unsigned int plane_id;

	/* Hardware layer config parameters */
	struct disp_layer_config_data config;

	unsigned long long alpha_mode;
	unsigned long long galpha_value; /* global alpha value */

	struct drm_property *alpha_mode_property;
	struct drm_property *galpha_value_property;
};

#define to_sunxi_plane(x)  container_of(x, struct sunxi_drm_plane, plane)

void sunxi_plane_dump(struct drm_plane *plane, bool dump_fb);
ssize_t sunxi_drm_planes_show(char *buf, struct drm_device *dev,
						struct drm_crtc *crtc);


int sunxi_drm_plane_init(struct drm_device *dev,
			struct sunxi_drm_plane *plane,
			int crtc_id, int plane_id, int type);
#endif
