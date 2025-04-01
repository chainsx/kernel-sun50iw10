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
#ifndef _SUNXI_DRM_FB_H_
#define _SUNXI_DRM_FB_H_

#include <drm/drm_atomic.h>
#include <drm/drm_crtc.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_crtc_helper.h>
#include <linux/dma-buf.h>
#include <linux/dma-mapping.h>
#include <linux/module.h>

enum sunxi_fb_type {
	DRM_CREATE_FB = 0,
	FBDEV_CREATE_FB = 1,
};

struct sunxi_drm_fb {
	/* sunxi_fb_type */
	int				flag;
	struct drm_framebuffer		fb;
	struct sunxi_drm_gem_object	*obj[4];
};

static inline struct sunxi_drm_fb *to_sunxi_fb(struct drm_framebuffer *fb)
{
	return container_of(fb, struct sunxi_drm_fb, fb);
}

void sunxi_fb_dump(struct drm_framebuffer *fb);
ssize_t sunxi_drm_fb_show(char *buf, struct drm_framebuffer *fb);

void sunxi_drm_fb_destroy(struct drm_framebuffer *fb);
struct drm_framebuffer_funcs *sunxi_drm_fb_get_funcs(void);
int sunxi_drm_fb_prepare_fb(struct drm_plane *plane,
			  struct drm_plane_state *state);

struct sunxi_drm_fb *sunxi_drm_fb_alloc(struct drm_device *dev,
			const struct drm_mode_fb_cmd2 *mode_cmd,
				struct sunxi_drm_gem_object **obj,
				unsigned int num_planes, int flag,
			const struct drm_framebuffer_funcs *funcs);

struct drm_framebuffer *sunxi_drm_fb_create(struct drm_device *dev,
	struct drm_file *file_priv, const struct drm_mode_fb_cmd2 *mode_cmd);
#endif
