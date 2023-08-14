/* sunxi_drm_fbdev.c
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

#include <drm/drm_crtc_helper.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_fourcc.h>

#include "sunxi_drm_drv.h"
#include "sunxi_drm_connector.h"
#include "sunxi_drm_encoder.h"
#include "sunxi_drm_crtc.h"
#include "sunxi_drm_fbdev.h"
#include "sunxi_drm_gem.h"
#include "sunxi_drm_iommu.h"

struct __fb_addr_para {
	uintptr_t fb_paddr;
	int fb_size;
};

struct __fb_addr_para g_fb_addr;

struct sunxi_drm_fbdev *sunxi_drmfbdev[FBDEV_MAX_NUM];

static void sunxi_drm_fbdev_set_fbdev(struct sunxi_drm_fbdev *sunxi_fbdev, int index)
{
	if (index < FBDEV_MAX_NUM)
		sunxi_drmfbdev[index] = sunxi_fbdev;
}

struct sunxi_drm_fbdev *sunxi_drm_fbdev_get_fbdev(int index)
{
	if (index < FBDEV_MAX_NUM)
		return sunxi_drmfbdev[index];
	return NULL;
}

static int sunxi_drm_fbdev_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
	struct drm_fb_helper *helper = info->par;
	/* @TODO:fix it, now just support 1 fbdev */
	struct sunxi_drm_fbdev *sunxi_fbd = sunxi_drm_fbdev_get_fbdev(0);
	struct sunxi_drm_gem_object *sunxi_gem_obj = sunxi_fbd->sunxi_fb->obj[0];
	unsigned long vm_size;
	int ret;

	vma->vm_flags |= VM_IO | VM_DONTEXPAND | VM_DONTDUMP;

	vm_size = vma->vm_end - vma->vm_start;

	if (vm_size > sunxi_gem_obj->size)
		return -EINVAL;

	ret = dma_mmap_attrs(sunxi_gem_obj->base.dev->dev, vma, sunxi_gem_obj->vaddr,
			     sunxi_gem_obj->dma_addr, sunxi_gem_obj->size,
			     sunxi_gem_obj->dma_attrs);
	if (ret < 0) {
		DRM_ERROR("failed to mmap.\n");
		return ret;
	}

	return 0;
}

/* static int sunxi_drm_fbdev_helper_set_par(struct fb_info *info)
{
	int i, ret;
	struct drm_mode_set *modeset;
	struct drm_framebuffer *fb;
	struct drm_fb_helper *fb_helper = info->par;
	struct fb_var_screeninfo *var = &info->var;

	ret = drm_fb_helper_set_par(info);
	if (ret < 0) {
		DRM_ERROR("drm_fb_helper_set_par failed\n");
		return ret;
	}

	for (i = 0; i < fb_helper->crtc_count; i++) {
		modeset = &fb_helper->crtc_info[i].mode_set;
		if (!modeset->fb || !modeset->crtc)
			continue;
		fb = modeset->fb;
		fb->width = var->xres;
		fb->height = var->yres * 2;

		switch (var->bits_per_pixel) {
		case 16:
			fb->depth = (var->green.length == 6) ? 16 : 15;
			break;
		case 32:
			fb->depth = (var->transp.length > 0) ? 32 : 24;
			break;
		default:
			fb->depth = var->bits_per_pixel;
			break;
		}

		fb->bits_per_pixel = var->bits_per_pixel;
		fb->pitches[0] = var->xres * var->bits_per_pixel / 8;
		fb->offsets[0] = 0;
		fb->pixel_format = drm_mode_legacy_fb_format(
				fb->bits_per_pixel, fb->depth);
	}

	return 0;
} */

/* for other device in kernel like gpu to invoke */
void sunxi_get_fb_addr_para(struct __fb_addr_para *fb_addr_para)
{
	if (fb_addr_para) {
		fb_addr_para->fb_paddr = g_fb_addr.fb_paddr;
		fb_addr_para->fb_size  = g_fb_addr.fb_size;
	}
}
EXPORT_SYMBOL(g_fb_addr);
EXPORT_SYMBOL(sunxi_get_fb_addr_para);

static struct fb_ops sunxi_drm_fbdev_ops = {
	.owner		= THIS_MODULE,
	DRM_FB_HELPER_DEFAULT_OPS,
	.fb_mmap		= sunxi_drm_fbdev_mmap,
	.fb_fillrect	= drm_fb_helper_cfb_fillrect,
	.fb_copyarea	= drm_fb_helper_cfb_copyarea,
	.fb_imageblit	= drm_fb_helper_cfb_imageblit,
};

/*
 * sunxi_drm_fbdev_create_with_funcs() - create fbdev, with_funcs means that
 *			with funcs for drm_framebuffer instant
 * 1.create a gem with a dma-buf
 * 2.create a fb_info
 * 3.create a drm_framebuffer with one plane
 */
int sunxi_drm_fbdev_create_with_funcs(struct drm_fb_helper *helper,
	struct drm_fb_helper_surface_size *sizes,
	const struct drm_framebuffer_funcs *funcs)
{
	struct sunxi_drm_fbdev *sunxi_fbdev = to_sunxi_fbdev(helper);
	struct drm_mode_fb_cmd2 mode_cmd = { 0 };
	struct drm_device *dev = helper->dev;
	struct sunxi_drm_gem_object *obj;
	struct drm_framebuffer *fb;
	unsigned int bytes_per_pixel;
	unsigned long offset;
	struct fb_info *fbi;
	size_t size;
	unsigned int flags;
	int ret;

#ifndef	CONFIG_DRM_FBDEV_EMULATION_DUAL_DISPLAY_WITH_DIFFERENT_BUFFER
	/* Force to make the fbdev's sizes decided by primary connector,
	 * the bootlogo's sizes are equal to the primary connector's
	 */
	sizes->surface_width = sunxi_drm_get_init_width();
	sizes->surface_height = sunxi_drm_get_init_height();
	sizes->fb_width = sunxi_drm_get_init_width();
	sizes->fb_height = sunxi_drm_get_init_height();
	sizes->surface_bpp = sunxi_drm_get_init_bpp();
#endif

	DRM_INFO("surface width(%d), height(%d) and bpp(%d) depth(%d)\n",
			sizes->surface_width, sizes->surface_height,
			sizes->surface_bpp, sizes->surface_depth);
	DRM_INFO("fb width(%d) height(%d)\n", sizes->fb_width, sizes->fb_height);

	bytes_per_pixel = DIV_ROUND_UP(sizes->surface_bpp, 8);

	mode_cmd.width = sizes->surface_width;
	mode_cmd.height = sizes->surface_height * 2;

	mode_cmd.pitches[0] = sizes->surface_width * bytes_per_pixel;
	mode_cmd.pixel_format = drm_mode_legacy_fb_format(sizes->surface_bpp,
		sizes->surface_depth);

	size = mode_cmd.pitches[0] * mode_cmd.height;
	DRM_INFO("size = %d\n", size);

	if (is_drm_iommu_supported(dev))
		flags = SUNXI_BO_NONCONTIG | SUNXI_BO_WC;
	else
		flags = SUNXI_BO_CONTIG | SUNXI_BO_WC;

	obj = sunxi_drm_gem_create(dev, flags, size);
	if (IS_ERR(obj))
		return PTR_ERR(obj);

	fbi = drm_fb_helper_alloc_fbi(helper);
	if (IS_ERR(fbi)) {
		ret = PTR_ERR(fbi);
		goto err_gem_free_object;
	}

	sunxi_fbdev->sunxi_fb = sunxi_drm_fb_alloc(dev, &mode_cmd, &obj, 1,
						FBDEV_CREATE_FB, funcs);
	if (IS_ERR(sunxi_fbdev->sunxi_fb)) {
		dev_err(dev->dev, "Failed to allocate DRM framebuffer.\n");
		ret = PTR_ERR(sunxi_fbdev->sunxi_fb);
		goto err_gem_free_object;
	}

	fb = &sunxi_fbdev->sunxi_fb->fb;
	helper->fb = fb;

	fbi->par = helper;
	fbi->flags = FBINFO_FLAG_DEFAULT;
	fbi->fbops = &sunxi_drm_fbdev_ops;

	drm_fb_helper_fill_info(fbi, helper, sizes);

	offset = fbi->var.xoffset * bytes_per_pixel;
	offset += fbi->var.yoffset * fb->pitches[0];

	dev->mode_config.fb_base = (resource_size_t)obj->dma_addr;
	fbi->screen_base = obj->vaddr + offset;
	fbi->fix.smem_start = (unsigned long)(obj->dma_addr + offset);
	fbi->screen_size = size;
	fbi->fix.smem_len = size;

/* for other device in kernel like gpu to invoke */
	g_fb_addr.fb_paddr = fbi->fix.smem_start;
	g_fb_addr.fb_size = fbi->fix.smem_len;

	return 0;

err_gem_free_object:
	drm_gem_object_put(&obj->base);
	return ret;
}

static int sunxi_drm_fbdev_create(struct drm_fb_helper *helper,
	struct drm_fb_helper_surface_size *sizes)
{
	return sunxi_drm_fbdev_create_with_funcs(helper, sizes,
				sunxi_drm_fb_get_funcs());
}

/**
 * For booting:
 * in this func, you can set the default attachments
 * amount crtc/encoder/connector during drm kernel init.
 * it is useful for smoot booting
 *
 * @crtcs: *crtcs[i] means which crtc connector[i] will attached to
 * @modes: *modes[i] means which mode connector[i] will set
 * @offsets: offsets[i] means x/y offset of fb used by connector[i]
 * @enabled: enabled[i] means if connector[i] is enabled
 */
#if DRM_DEBUG
static bool sunxi_drm_fbdev_initial_config(struct drm_fb_helper *fb_helper,
				struct drm_fb_helper_crtc **crtcs,
			       struct drm_display_mode **modes,
			       struct drm_fb_offset *offsets,
			       bool *enabled, int width, int height)
{
	int i, j;

	struct drm_connector *conn;
	struct sunxi_drm_connector *sunxi_conn;
	struct drm_display_mode *mode_tmp;
	const struct drm_connector_helper_funcs *conn_funcs;

	struct drm_encoder *enc;
	struct sunxi_drm_encoder *sunxi_enc;

	struct drm_crtc *crtc = NULL;
	struct sunxi_drm_crtc *sunxi_crtc;

	struct drm_device *dev;
	bool find = false;

	for (i = 0; i < fb_helper->connector_count; i++) {
		find = false;
		if (!enabled[i])
			continue;

		offsets[i].x = 0;
		offsets[i].y = 0;

	/* find the preferred display_mode of the connector */
	/* preferred mode: the mode set by user space */
		conn = fb_helper->connector_info[i]->connector;
		sunxi_conn = to_sunxi_connector(conn);

		list_for_each_entry(mode_tmp, &conn->modes, head) {
			if (mode_tmp->type & DRM_MODE_TYPE_USERDEF) {
				modes[i] = mode_tmp;
				find = true;
				DRM_INFO("[FBDEV]Find the userdef_mode[%s]:"
					"%dx%d%c@%d\n",
					mode_tmp->name,
					mode_tmp->hdisplay, mode_tmp->vdisplay,
					(mode_tmp->flags
					& DRM_MODE_FLAG_INTERLACE) ? 'I' : 'P',
					(mode_tmp->clock * 1000)
					/ (mode_tmp->htotal * mode_tmp->vtotal));
				break;
			}
		}

		/* if there is NO preferred mode, use default mode */
		/* default mode: the mode set in DTS */
		if (!find) {
			list_for_each_entry(mode_tmp, &conn->modes, head) {
				if (mode_tmp->type & DRM_MODE_TYPE_DEFAULT) {
					modes[i] = mode_tmp;
					find =  true;
					DRM_INFO("[FBDEV]Find the default_mode[%s]:"
						"%dx%d%c@%d\n",
						mode_tmp->name,
						mode_tmp->hdisplay,
						mode_tmp->vdisplay,
						(mode_tmp->flags
						& DRM_MODE_FLAG_INTERLACE) ? 'I' : 'P',
						(mode_tmp->clock * 1000)
						/ (mode_tmp->htotal * mode_tmp->vtotal));
					break;
				}
			}
		}

		if (!find) {
			list_for_each_entry(mode_tmp, &conn->modes, head) {
				if (mode_tmp->type & DRM_MODE_TYPE_PREFERRED) {
					modes[i] = mode_tmp;
					find = true;
					DRM_INFO("[FBDEV]Find the preferred_mode[%s]:"
						"%dx%d%c@%d\n",
						mode_tmp->name,
						mode_tmp->hdisplay, mode_tmp->vdisplay,
						(mode_tmp->flags
						& DRM_MODE_FLAG_INTERLACE) ? 'I' : 'P',
						(mode_tmp->clock * 1000)
						/ (mode_tmp->htotal * mode_tmp->vtotal));
					break;
				}
			}
		}

	/* find the best encoder for the connector */
		conn_funcs = conn->helper_private;
		enc = conn_funcs->best_encoder(conn);
		if (!enc)
			continue;
		conn->encoder = enc;
		sunxi_enc = to_sunxi_encoder(enc);
		DRM_INFO("[SUNXI-FBDEV]: connector(%d)--->encoder(%d)\n",
			i, sunxi_enc->encoder_id);

	/* find the best ertc for the connector */
		for (j = 0; j < fb_helper->crtc_count; j++) {
			if (!(enc->possible_crtcs & (1 << i)))
				continue;

			crtc = fb_helper->crtc_info[j].mode_set.crtc;
			if (!crtc)
				continue;
			sunxi_crtc = to_sunxi_crtc(crtc);
			dev = crtc->dev;

		/* this crtc has been attached to this encoder */
			if (enc->crtc == crtc) {
				crtcs[i] = &fb_helper->crtc_info[j];
				enc->crtc = crtc;
				DRM_INFO("[SUNXI-FBDEV]:"
					"encoder(%d)--->crtc(%d)\n",
					sunxi_enc->encoder_id,
					sunxi_crtc->crtc_id);
				break;

		/* this crtc has NOT been attached to this encoder */
			} else {
			/* this crtc has NOT been attached to any encoder */
				if (!sunxi_crtc->is_in_use(sunxi_crtc)) {
					crtcs[i] = &fb_helper->crtc_info[j];
					enc->crtc = crtc;
					DRM_INFO("[SUNXI-FBDEV]:"
					"encoder(%d)--->crtc(%d)\n",
					sunxi_enc->encoder_id,
					sunxi_crtc->crtc_id);
					break;
				}
			}
		}

	}

	for (i = 0; i < fb_helper->connector_count; i++) {
		if (modes[i] && crtcs[i])
			return true;
	}

	DRM_INFO("[WARN]:There is NO suitable crtc/encoder "
			"and display_mode for the connected connectors\n");
	return true;
}
#endif

static const struct drm_fb_helper_funcs sunxi_drm_fb_helper_funcs = {
	/* .initial_config = sunxi_drm_fbdev_initial_config, */
	.fb_probe = sunxi_drm_fbdev_create,
};

#ifdef CONFIG_DRM_FBDEV_EMULATION_DUAL_DISPLAY_WITH_DIFFERENT_BUFFER
static void sunxi_drm_fb_helper_modeset_release(struct drm_fb_helper *helper,
					  struct drm_mode_set *modeset)
{
	int i;

	for (i = 0; i < modeset->num_connectors; i++) {
		drm_connector_unreference(modeset->connectors[i]);
		modeset->connectors[i] = NULL;
	}
	modeset->num_connectors = 0;

	drm_mode_destroy(helper->dev, modeset->mode);
	modeset->mode = NULL;

	/* FIXME should hold a ref? */
	modeset->fb = NULL;
}

static void sunxi_drm_fb_helper_crtc_free(struct drm_fb_helper *helper)
{
	int i;

	for (i = 0; i < helper->connector_count; i++) {
		drm_connector_unreference(helper->connector_info[i]->connector);
		kfree(helper->connector_info[i]);
	}
	kfree(helper->connector_info);

	for (i = 0; i < helper->crtc_count; i++) {
		struct drm_mode_set *modeset = &helper->crtc_info[i].mode_set;

		sunxi_drm_fb_helper_modeset_release(helper, modeset);
		kfree(modeset->connectors);
	}
	kfree(helper->crtc_info);
}


/* This function is created from drm_fb_help_init() */
static int sunxi_drm_fb_helper_init(struct drm_device *dev,
		       struct drm_fb_helper *fb_helper, int index)
{
	struct drm_crtc *crtc;

	fb_helper->crtc_info = kcalloc(1, sizeof(struct drm_fb_helper_crtc), GFP_KERNEL);
	if (!fb_helper->crtc_info)
		return -ENOMEM;

	fb_helper->crtc_count = 1;
	fb_helper->connector_info = kcalloc(1, sizeof(struct drm_fb_helper_connector *), GFP_KERNEL);
	if (!fb_helper->connector_info) {
		kfree(fb_helper->crtc_info);
		return -ENOMEM;
	}
	fb_helper->connector_info_alloc_count = 1;
	fb_helper->connector_count = 0;

	fb_helper->crtc_info[0].mode_set.connectors =
		kcalloc(1, sizeof(struct drm_connector *), GFP_KERNEL);

	if (!fb_helper->crtc_info[0].mode_set.connectors)
		goto out_free;
	fb_helper->crtc_info[0].mode_set.num_connectors = 0;

	drm_for_each_crtc(crtc, dev) {
		if (crtc->index == index)
			fb_helper->crtc_info[0].mode_set.crtc = crtc;
	}

	return 0;
out_free:
	sunxi_drm_fb_helper_crtc_free(fb_helper);
	return -ENOMEM;
}
#endif

struct sunxi_drm_fbdev *sunxi_drm_fbdev_init_with_funcs(struct drm_device *dev,

	unsigned int preferred_bpp, unsigned int num_crtc,
	const struct drm_fb_helper_funcs *funcs, int index)
{
	struct sunxi_drm_fbdev *sunxi_fbdev;
	struct drm_fb_helper *helper;
	int ret;
#ifdef CONFIG_DRM_FBDEV_EMULATION_DUAL_DISPLAY_WITH_DIFFERENT_BUFFER
		struct sunxi_drm_connector *sconn;
#endif

	sunxi_fbdev = kzalloc(sizeof(*sunxi_fbdev), GFP_KERNEL);
	if (!sunxi_fbdev) {
		dev_err(dev->dev, "Failed to allocate drm fbdev.\n");
		return ERR_PTR(-ENOMEM);
	}

	helper = &sunxi_fbdev->fb_helper;

	drm_fb_helper_prepare(dev, helper, funcs);

#ifdef CONFIG_DRM_FBDEV_EMULATION_DUAL_DISPLAY_WITH_DIFFERENT_BUFFER
	/* init fb_helper and set crtc for this fb_helper */
	ret = sunxi_drm_fb_helper_init(dev, helper, index);
	if (ret < 0) {
		dev_err(dev->dev, "Failed to initialize drm fb helper.\n");
		goto err_free;
	}

	/* set connector for this fb_helper */
	sconn = sunxi_drm_connector_get_connector(index);
	if (!sconn) {
		dev_err(dev->dev, "NO sunxi connector for create fbdev\n");
		goto err_free;
	}
#else
	ret = drm_fb_helper_init(dev, helper);
	if (ret < 0) {
		dev_err(dev->dev, "Failed to initialize drm fb helper.\n");
		goto err_free;
	}
#endif

	ret = drm_fb_helper_initial_config(helper, preferred_bpp);
	if (ret < 0) {
		dev_err(dev->dev, "Failed to set initial hw configuration.\n");
		goto err_drm_fb_helper_fini;
	}

	return sunxi_fbdev;

err_drm_fb_helper_fini:
	drm_fb_helper_fini(helper);
err_free:
	kfree(sunxi_fbdev);

	return ERR_PTR(ret);
}

int sunxi_drm_fbdev_init(struct drm_device *dev)
{
	struct sunxi_drm_fbdev *sunxi_fbdev;
	int num_crtc, num_connector, bpp;
#ifdef CONFIG_DRM_FBDEV_EMULATION_DUAL_DISPLAY_WITH_DIFFERENT_BUFFER
		int i;
#endif

	num_crtc = dev->mode_config.num_crtc;
	num_connector = dev->mode_config.num_connector;

#if defined(CONFIG_DRM_FBDEV_EMULATION_DUAL_DISPLAY_WITH_SAME_BUFFER) \
		|| defined(CONFIG_DRM_FBDEV_EMULATION_DUAL_DISPLAY_WITH_DIFFERENT_BUFFER)
	if (num_connector != 2) {
		DRM_ERROR("use dual-display, "
			"please check your sys_config.fex to set dual-display\n");
		return -1;
	}

#else
	if (num_connector != 1) {
		DRM_ERROR("NOT use dual-display, num_connector:%d\n",
							num_connector);
		DRM_ERROR("please check your sys_config.fex to set one-display\n");
		return -1;
	}

#endif

	bpp = 32;

	/* decide bpp by bootlogo, or bootlogo can NOT be copy to fbdev correctly */
	if (sunxi_drm_is_need_smooth_boot())
		bpp = sunxi_drm_get_init_bpp();

/* create /dev/fb0 and /dev/fb1
 * /dev/fb0: crtc0--->connector0
 * /dev/fb1: crtc1--->connector1
 */
#ifdef CONFIG_DRM_FBDEV_EMULATION_DUAL_DISPLAY_WITH_DIFFERENT_BUFFER
	for (i = 0; i < FBDEV_MAX_NUM; i++) {
		sunxi_fbdev = sunxi_drm_fbdev_init_with_funcs(dev, bpp,
					1, &sunxi_drm_fb_helper_funcs, i);
		if (!sunxi_fbdev) {
			DRM_ERROR("sunxi_drm_fbdev_init failed\n");
			return -1;
		}

		sunxi_drm_fbdev_set_fbdev(sunxi_fbdev, i);
	}
#else
	sunxi_fbdev = sunxi_drm_fbdev_init_with_funcs(dev, bpp,
				num_crtc, &sunxi_drm_fb_helper_funcs, 0);
	if (!sunxi_fbdev) {
		DRM_ERROR("sunxi_drm_fbdev_init failed\n");
		return -1;
	}

	sunxi_drm_fbdev_set_fbdev(sunxi_fbdev, 0);
#endif

	return 0;
}

void sunxi_drm_fbdev_exit(struct drm_device *dev)
{
	int i = 0;
	struct sunxi_drm_fbdev *sunxi_fbdev;

	for (i = 0; i < FBDEV_MAX_NUM; i++) {
		sunxi_fbdev = sunxi_drm_fbdev_get_fbdev(i);
		if (!sunxi_fbdev)
			return;
		drm_fb_helper_unregister_fbi(&sunxi_fbdev->fb_helper);

		if (sunxi_fbdev->sunxi_fb) {
			drm_framebuffer_unregister_private(&sunxi_fbdev->sunxi_fb->fb);
			sunxi_drm_fb_destroy(&sunxi_fbdev->sunxi_fb->fb);
		}

		drm_fb_helper_fini(&sunxi_fbdev->fb_helper);
		kfree(sunxi_fbdev);
		sunxi_drm_fbdev_set_fbdev(NULL, i);
	}
}

void sunxi_drm_fbdev_output_poll_changed(struct drm_device *dev)
{
	struct sunxi_drm_fbdev *sunxi_fbdev;

#ifdef CONFIG_DRM_FBDEV_EMULATION_DUAL_DISPLAY_WITH_DIFFERENT_BUFFER
	int i;

	for (i = 0; i < FBDEV_MAX_NUM; i++) {
		sunxi_fbdev = sunxi_drm_fbdev_get_fbdev(i);

		drm_fb_helper_hotplug_event(&sunxi_fbdev->fb_helper);
	}
#else
	sunxi_fbdev = sunxi_drm_fbdev_get_fbdev(0);

	drm_fb_helper_hotplug_event(&sunxi_fbdev->fb_helper);
#endif
}
