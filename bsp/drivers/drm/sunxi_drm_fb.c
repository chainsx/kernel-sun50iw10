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
#include <drm/drm_crtc_helper.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_fourcc.h>
#include <drm/drm_gem_framebuffer_helper.h>

#include "sunxi_drm_fb.h"
#include "sunxi_drm_gem.h"

/* void sunxi_fb_dump(struct drm_framebuffer *fb)
{
	int i;
	struct sunxi_drm_fb *sunxi_fb = to_sunxi_fb(fb);

	DRM_INFO("Framebuffer info:\n");
	DRM_INFO("FB ID:%u\n", fb->base.id);

	if (sunxi_fb->flag == DRM_CREATE_FB)
		DRM_INFO("DRM_CREATE_FB:\n");
	else if (sunxi_fb->flag == FBDEV_CREATE_FB)
		DRM_INFO("FBDEV_CREATE_FB:\n");
	else
		DRM_INFO("ERROR Flag!!!\n");

	DRM_INFO("pitches(3~0): %u %u %u %u\n", fb->pitches[3], fb->pitches[2],
						fb->pitches[1], fb->pitches[0]);
	DRM_INFO("offsets(3~0): %u %u %u %u\n", fb->offsets[3], fb->offsets[2],
						fb->offsets[1], fb->offsets[0]);
	DRM_INFO("modifier(3~0): %llu %llu %llu %llu\n",
					fb->modifier[3], fb->modifier[2],
					fb->modifier[1], fb->modifier[0]);
	DRM_INFO("width:%u height:%u\n", fb->width, fb->height);
	DRM_INFO("depth:%u bpp:%d\n", fb->depth, fb->bits_per_pixel);
	DRM_INFO("flags:%d\n", fb->flags);
	DRM_INFO("Pixel_format:%c%c%c%c", (char)(fb->pixel_format & 0xff),
				   (char)((fb->pixel_format >> 8) & 0xff),
				  (char)((fb->pixel_format >> 16) & 0xff),
				 (char)((fb->pixel_format >> 24) & 0xff));
	DRM_INFO("\n");

	for (i = 0; i < 4; i++) {
		if (!sunxi_fb->obj[i])
			continue;
		DRM_INFO("Pixel-Plane%d info:\n", i);
		DRM_INFO("pitches:%u offsets:%u modifier:%llu\n",
			fb->pitches[i], fb->offsets[i], fb->modifier[i]);
		sunxi_gem_dump(&sunxi_fb->obj[i]->base);
		DRM_INFO("\n");
	}
} */

/* ssize_t sunxi_drm_fb_show(char *buf, struct drm_framebuffer *fb)
{
	ssize_t n = 0;
	int i;
	struct sunxi_drm_fb *sunxi_fb = to_sunxi_fb(fb);

	if (sunxi_fb->flag == DRM_CREATE_FB)
		n += sprintf(buf + n, "DRM_CREATE_FB:\n");
	else if (sunxi_fb->flag == FBDEV_CREATE_FB)
		n += sprintf(buf + n, "FBDEV_CREATE_FB:\n");
	else
		n += sprintf(buf + n, "ERROR Flag!!!\n");

	n += sprintf(buf + n, "drm_mode_obj id:%d type:0x%x\n",
		fb->base.id, fb->base.type);
	n += sprintf(buf + n, "fb width:%u height:%u depth:%u bpp:%d "
		"flags:%d pixel_format:%c%c%c%c\n",
		fb->width, fb->height, fb->depth, fb->bits_per_pixel, fb->flags,
		(char)(fb->format->format & 0xff),
		(char)((fb->format->format >> 8) & 0xff),
		(char)((fb->format->format >> 16) & 0xff),
		(char)((fb->format->format >> 24) & 0xff));

	for (i = 0; i < 4; i++) {
		if (!sunxi_fb->obj[i])
			continue;
		n += sprintf(buf + n, "[fb(obj_id:%u) pixel component %d info]:\n",
									fb->base.id, i);
		n += sprintf(buf + n, "pitches:%u offsets:%u modifier:%llu\n",
			fb->pitches[i], fb->offsets[i], fb->modifier[i]);
		n += sunxi_drm_gem_show(buf + n, sunxi_fb->obj[i],
					fb->offsets[i], fb->pitches[i]);
	}

	return n;
} */


void sunxi_drm_fb_destroy(struct drm_framebuffer *fb)
{
	struct sunxi_drm_fb *sunxi_fb = to_sunxi_fb(fb);
	int i;

	for (i = 0; i < 4; i++) {
		if (sunxi_fb->obj[i])
			drm_gem_object_put(&sunxi_fb->obj[i]->base);
	}

	drm_framebuffer_cleanup(fb);
	kfree(sunxi_fb);
}

int sunxi_drm_fb_create_handle(struct drm_framebuffer *fb,
	struct drm_file *file_priv, unsigned int *handle)
{
	struct sunxi_drm_fb *sunxi_fb = to_sunxi_fb(fb);

	return drm_gem_handle_create(file_priv,
			&sunxi_fb->obj[0]->base, handle);
}

static struct drm_framebuffer_funcs sunxi_drm_fb_funcs = {
	.destroy	= sunxi_drm_fb_destroy,
	.create_handle	= sunxi_drm_fb_create_handle,
};

struct drm_framebuffer_funcs *sunxi_drm_fb_get_funcs(void)
{
	return &sunxi_drm_fb_funcs;
}

struct sunxi_drm_fb *sunxi_drm_fb_alloc(struct drm_device *dev,
			const struct drm_mode_fb_cmd2 *mode_cmd,
			struct sunxi_drm_gem_object **obj,
			unsigned int num_planes, int flag,
			const struct drm_framebuffer_funcs *funcs)
{
	struct sunxi_drm_fb *sunxi_fb;
	int ret;
	int i;

	sunxi_fb = kzalloc(sizeof(*sunxi_fb), GFP_KERNEL);
	if (!sunxi_fb)
		return ERR_PTR(-ENOMEM);

	drm_helper_mode_fill_fb_struct(dev, &sunxi_fb->fb, mode_cmd);

	for (i = 0; i < num_planes; i++)
		sunxi_fb->obj[i] = obj[i];

	ret = drm_framebuffer_init(dev, &sunxi_fb->fb, funcs);
	if (ret) {
		DRM_ERROR("Failed to initialize framebuffer: %d\n", ret);
		kfree(sunxi_fb);
		return ERR_PTR(ret);
	}

	sunxi_fb->flag = flag;

	return sunxi_fb;
}

/**
 * sunxi_drm_fb_create_with_funcs() - helper function for the
 *                                  &drm_mode_config_funcs ->fb_create
 *                                  callback function
 * @dev: DRM device
 * @file_priv: drm file for the ioctl call
 * @mode_cmd: metadata from the userspace fb creation request
 * @funcs: vtable to be used for the new framebuffer object
 *
 * This can be used to set &drm_framebuffer_funcs for drivers that need the
 * dirty() callback. Use sunxi_drm_fb_create() if you don't need to change
 * &drm_framebuffer_funcs.
 */
struct drm_framebuffer *sunxi_drm_fb_create_with_funcs(struct drm_device *dev,
	struct drm_file *file_priv, const struct drm_mode_fb_cmd2 *mode_cmd,
	const struct drm_framebuffer_funcs *funcs)
{
	struct sunxi_drm_fb *sunxi_fb;
	struct sunxi_drm_gem_object *objs[4];
	struct drm_gem_object *obj;
	int ret;
	unsigned int i;

	const struct drm_format_info *info = drm_get_format_info(dev, mode_cmd);

	for (i = 0; i < info->num_planes; i++) {
		unsigned int width = mode_cmd->width / (i ? info->hsub : 1);
		unsigned int height = mode_cmd->height / (i ? info->vsub : 1);
		unsigned int min_size;

		obj = drm_gem_object_lookup(file_priv, mode_cmd->handles[i]);
		if (!obj) {
			DRM_ERROR("Failed to lookup GEM object, handle:0x%x\n",
				 mode_cmd->handles[i]);
			ret = -ENXIO;
			goto err_gem_object_unreference;
		}

		min_size = (height - 1) * mode_cmd->pitches[i]
			 + width * info->cpp[i];
			 + mode_cmd->offsets[i];

		if (obj->size < min_size) {
			DRM_ERROR("obj[%d] size so small, "
				"obj->size:%d min_size:%d\n",
				i, (int)obj->size, min_size);
			drm_gem_object_put(obj);
			ret = -EINVAL;
			goto err_gem_object_unreference;
		}
		objs[i] = to_sunxi_drm_gem_obj(obj);
	}

	sunxi_fb = sunxi_drm_fb_alloc(dev, mode_cmd, objs, i,
					DRM_CREATE_FB, funcs);
	if (IS_ERR(sunxi_fb)) {
		DRM_ERROR("Failed to alloc fb\n");
		ret = PTR_ERR(sunxi_fb);
		goto err_gem_object_unreference;
	}

	return &sunxi_fb->fb;

err_gem_object_unreference:
	for (i--; i >= 0; i--)
		drm_gem_object_put(&objs[i]->base);
	return ERR_PTR(ret);
}

/**
 * sunxi_drm_fb_create() - &drm_mode_config_funcs ->fb_create callback function
 * @dev: DRM device
 * @file_priv: drm file for the ioctl call
 * @mode_cmd: metadata from the userspace fb creation request
 *
 * If your hardware has special alignment or pitch requirements these should be
 * checked before calling this function. Use sunxi_drm_fb_create_with_funcs() if
 * you need to set &drm_framebuffer_funcs ->dirty.
 */
struct drm_framebuffer *sunxi_drm_fb_create(struct drm_device *dev,
	struct drm_file *file_priv, const struct drm_mode_fb_cmd2 *mode_cmd)
{
	return sunxi_drm_fb_create_with_funcs(dev, file_priv, mode_cmd,
					    &sunxi_drm_fb_funcs);
}

/**
 * sunxi_drm_fb_get_gem_obj() - Get SUNXI GEM object for framebuffer
 * @fb: The framebuffer
 * @plane: Which plane
 *
 * Return the SUNXI GEM object for given framebuffer.
 *
 * This function will usually be called from the CRTC callback functions.
 */
struct sunxi_drm_gem_object *sunxi_drm_fb_get_gem_obj(struct drm_framebuffer *fb,
						  unsigned int plane)
{
	struct sunxi_drm_fb *sunxi_fb = to_sunxi_fb(fb);

	if (plane >= 4)
		return NULL;

	return sunxi_fb->obj[plane];
}

/**
 * sunxi_drm_fb_prepare_fb() - Prepare SUNXI framebuffer
 * @plane: Which plane
 * @state: Plane state attach fence to
 *
 * This should be put into prepare_fb hook of struct &drm_plane_helper_funcs .
 *
 * This function checks if the plane FB has an dma-buf attached, extracts
 * the exclusive fence and attaches it to plane state for the atomic helper
 * to wait on.
 *
 * There is no need for cleanup_fb for SUNXI based framebuffer drivers.
 */
int sunxi_drm_fb_prepare_fb(struct drm_plane *plane,
			  struct drm_plane_state *state)
{
	struct dma_buf *dma_buf;
	struct dma_fence *fence;

	if ((plane->state->fb == state->fb) || !state->fb)
		return 0;

	dma_buf = sunxi_drm_fb_get_gem_obj(state->fb, 0)->base.dma_buf;
	if (dma_buf)
		drm_gem_fb_prepare_fb(plane, state);

	return 0;
}
