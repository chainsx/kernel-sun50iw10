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
#include <drm/drm_plane_helper.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_fourcc.h>

#include "sunxi_drm_drv.h"
#include "sunxi_drm_plane.h"
#include "sunxi_drm_crtc.h"
#include "sunxi_drm_connector.h"
#include "sunxi_drm_fb.h"
#include "sunxi_drm_gem.h"
#include "sunxi_device/sunxi_de.h"

/* void sunxi_plane_dump(struct drm_plane *plane, bool dump_fb)
{
	struct sunxi_drm_plane *sunxi_plane = to_sunxi_plane(plane);

	DRM_INFO("Plane info:\n");
	DRM_INFO("Plane ID:%u  sunxi plane id:%u  drm_plane index:%u\n",
		plane->base.id, sunxi_plane->plane_id, plane->index);
	DRM_INFO("name:%s\n", plane->name);
	DRM_INFO("type:%s\n", (plane->type == DRM_PLANE_TYPE_PRIMARY) ?
		"primary" : ((plane->type == DRM_PLANE_TYPE_OVERLAY) ?
						"overlay" : "cursor"));
	DRM_INFO("possible_crtcs:%u\n", plane->possible_crtcs);

	if (plane->crtc)
		DRM_INFO("Attached crtc index:%d\n", plane->crtc->index);
	else
		DRM_INFO("WARN:No attached crtc\n");

	if (plane->fb && dump_fb) {
		sunxi_fb_dump(plane->fb);
	} else if (plane->fb) {
		DRM_INFO("Has fb, obj ID:%u", plane->fb->base.id);
	} else {
		DRM_INFO("WARN:Has NOT fb\n");
	}

	DRM_INFO("\n");
} */

static int drm_to_disp_format(uint32_t format)
{
	switch (format) {
	case DRM_FORMAT_ARGB8888:
		return DISP_FORMAT_ARGB_8888;

	case DRM_FORMAT_ABGR8888:
		return DISP_FORMAT_ABGR_8888;

	case DRM_FORMAT_RGBA8888:
		return DISP_FORMAT_RGBA_8888;

	case DRM_FORMAT_BGRA8888:
		return DISP_FORMAT_BGRA_8888;

	case DRM_FORMAT_XRGB8888:
		return DISP_FORMAT_XRGB_8888;

	case DRM_FORMAT_XBGR8888:
		return DISP_FORMAT_XBGR_8888;

	case DRM_FORMAT_RGBX8888:
		return DISP_FORMAT_RGBX_8888;

	case DRM_FORMAT_BGRX8888:
		return DISP_FORMAT_BGRX_8888;

	case DRM_FORMAT_RGB888:
		return DISP_FORMAT_RGB_888;

	case DISP_FORMAT_BGR_888:
		return DRM_FORMAT_BGR888;

	case DRM_FORMAT_RGB565:
		return DISP_FORMAT_RGB_565;

	case DRM_FORMAT_BGR565:
		return DISP_FORMAT_BGR_565;

	case DRM_FORMAT_ARGB4444:
		return DISP_FORMAT_ARGB_4444;

	case DRM_FORMAT_ABGR4444:
		return DISP_FORMAT_ABGR_4444;

	case DRM_FORMAT_RGBA4444:
		return DISP_FORMAT_RGBA_4444;

	case DRM_FORMAT_BGRA4444:
		return DISP_FORMAT_BGRA_4444;

	case DRM_FORMAT_ARGB1555:
		return DISP_FORMAT_ARGB_1555;

	case DRM_FORMAT_ABGR1555:
		return DISP_FORMAT_ABGR_1555;

	case DRM_FORMAT_RGBA5551:
		return DISP_FORMAT_RGBA_5551;

	case DRM_FORMAT_BGRA5551:
		return DISP_FORMAT_BGRA_5551;

	/* SP: semi-planar, P:planar, I:interleaved
	* UVUV: U in the LSBs;     VUVU: V in the LSBs */
	case DRM_FORMAT_AYUV:
		return DISP_FORMAT_YUV444_I_AYUV;

	case DRM_FORMAT_YUV444:
		return DISP_FORMAT_YUV444_P;

	case DRM_FORMAT_YUV422:
		return DISP_FORMAT_YUV422_P;

	case DRM_FORMAT_YUV420:
		return DISP_FORMAT_YUV420_P;

	case DRM_FORMAT_YUV411:
		return DISP_FORMAT_YUV411_P;

	case DRM_FORMAT_NV61:
		return DISP_FORMAT_YUV422_SP_UVUV;

	case DRM_FORMAT_NV16:
		return DISP_FORMAT_YUV422_SP_VUVU;

	case DRM_FORMAT_NV21:
		return DISP_FORMAT_YUV420_SP_UVUV;

	case DRM_FORMAT_NV12:
		return DISP_FORMAT_YUV420_SP_VUVU;
	}

	DRM_ERROR("get a err drm format.\n");
	return -1;
}

/* void sunxi_plane_state_dump(struct drm_plane_state *state)
{
	DRM_INFO("drm_plane_state info:\n");
	DRM_INFO("CRTC:%d\n", drm_crtc_index(state->crtc));
	DRM_INFO("crtc_x:%d crtc_y:%d crtc_w:%d crtc_h:%d\n",
				state->crtc_x, state->crtc_y,
				state->crtc_w, state->crtc_h);
	DRM_INFO("src_x:%d src_y:%d src_w:%d src_h:%d\n",
				state->src_x, state->src_y,
				state->src_w, state->src_h);
	DRM_INFO("Rotation:%u\n", state->rotation);
	DRM_INFO("Zops:%u normalized_zpos:%u\n", state->zpos,
					state->normalized_zpos);
	DRM_INFO("Rect-src: x1:%d y1:%d x2:%d y2:%d\n", state->src.x1,
				state->src.y1, state->src.x2, state->src.y2);
	DRM_INFO("Rect-dst: x1:%d y1:%d x2:%d y2:%d\n", state->dst.x1,
				state->dst.y1, state->dst.x2, state->dst.y2);
	DRM_INFO("visible:%u\n", state->visible);
	DRM_INFO("\n");

	sunxi_plane_dump(state->plane, false);

	if (state->fb)
		sunxi_fb_dump(state->fb);

} */

ssize_t sunxi_plane_state_show(char *buf, struct drm_plane_state *state)
{
	ssize_t n = 0;

	n += sprintf(buf + n, "attaching CRTC:%d\n", drm_crtc_index(state->crtc));
	n += sprintf(buf + n, "crtc_x:%d crtc_y:%d crtc_w:%d crtc_h:%d\n",
				state->crtc_x, state->crtc_y,
				state->crtc_w, state->crtc_h);
	n += sprintf(buf + n, "src_x:%d src_y:%d src_w:%d src_h:%d\n",
				state->src_x, state->src_y,
				state->src_w, state->src_h);
	n += sprintf(buf + n, "Rotation:%u\n", state->rotation);
	n += sprintf(buf + n, "Zops:%u normalized_zpos:%u\n", state->zpos,
					state->normalized_zpos);
	n += sprintf(buf + n, "Rect-src: x1:%d y1:%d x2:%d y2:%d\n", state->src.x1,
				state->src.y1, state->src.x2, state->src.y2);
	n += sprintf(buf + n, "Rect-dst: x1:%d y1:%d x2:%d y2:%d\n", state->dst.x1,
				state->dst.y1, state->dst.x2, state->dst.y2);
	n += sprintf(buf + n, "visible:%u\n", state->visible);

	if (state->fb)
		n += sprintf(buf + n, "fb drm_mode_object id:%u\n", state->fb->base.id);
	n += sprintf(buf + n, "\n");

	return n;
}

/* ssize_t sunxi_drm_planes_show(char *buf, struct drm_device *dev,
						struct drm_crtc *crtc)
{
	ssize_t n = 0;
	struct drm_plane *plane;
	struct sunxi_drm_plane *sunxi_plane;

	drm_for_each_plane(plane, dev) {
		if (crtc && !((plane->possible_crtcs >> drm_crtc_index(crtc)) & 0x1))
			continue;

		if (crtc && (!plane->crtc || !plane->fb))
			continue;

		sunxi_plane = to_sunxi_plane(plane);

		if (!crtc && (!plane->crtc || !plane->fb)) {
			n += sprintf(buf + n, "drm_plane index:%u  sunxi plane id:%u NOT enabled!!!\n",
						plane->index, sunxi_plane->plane_id);
			continue;
		}

		n += sprintf(buf + n, "sunxi plane id:%u  drm_plane index:%u\n",
						sunxi_plane->plane_id, plane->index);
		n += sprintf(buf + n, "drm_mode_object id:%u\n", plane->base.id);
		n += sprintf(buf + n, "name:%s\n", plane->name);
		n += sprintf(buf + n, "type:%s\n", (plane->type == DRM_PLANE_TYPE_PRIMARY) ?
				"primary" : ((plane->type == DRM_PLANE_TYPE_OVERLAY) ?
								"overlay" : "cursor"));
		n += sprintf(buf + n, "possible_crtcs:%u\n", plane->possible_crtcs);
		n += sprintf(buf + n, "Attached crtc index:%d\n", plane->crtc->index);

		n += sprintf(buf + n, "alpha_mode:%lld\n", sunxi_plane->alpha_mode);
		n += sprintf(buf + n, "galpha_value:%lld\n", sunxi_plane->galpha_value);

		if (plane->state) {
			n += sprintf(buf + n, "[sunxi plane id:%u sate]:\n",
						sunxi_plane->plane_id);
			n += sunxi_plane_state_show(buf + n, plane->state);
		}

		if (plane->fb) {
			n += sprintf(buf + n, "[sunxi plane id:%u attached fb(obj_id:%u) info]:\n",
						sunxi_plane->plane_id, plane->fb->base.id);
			n += sunxi_drm_fb_show(buf + n, plane->fb);
		}

		n += sprintf(buf + n, "\n");
	}

	return n;
} */


/*
 *@pixel_format: fourcc format
 *@return: 0:pixel alpha  1:global alpha 2:mixed alpha
 */
static int sunxi_plane_get_alpha_mode(unsigned int pixel_format, unsigned int alpha)
{
	switch (pixel_format) {
	case DRM_FORMAT_ARGB4444:
	case DRM_FORMAT_ABGR4444:
	case DRM_FORMAT_RGBA4444:
	case DRM_FORMAT_BGRA4444:

	case DRM_FORMAT_ARGB1555:
	case DRM_FORMAT_ABGR1555:
	case DRM_FORMAT_RGBA5551:
	case DRM_FORMAT_BGRA5551:

	case DRM_FORMAT_ARGB8888:
	case DRM_FORMAT_ABGR8888:
	case DRM_FORMAT_RGBA8888:
	case DRM_FORMAT_BGRA8888:

	case DRM_FORMAT_ARGB2101010:
	case DRM_FORMAT_ABGR2101010:
	case DRM_FORMAT_RGBA1010102:
	case DRM_FORMAT_BGRA1010102:
		if (alpha)
			return MIXED_ALPHA;
		else
			return PIXEL_ALPHA;

	case DRM_FORMAT_C8:
	case DRM_FORMAT_R8:
	case DRM_FORMAT_RG88:
	case DRM_FORMAT_GR88:
	case DRM_FORMAT_RGB332:
	case DRM_FORMAT_BGR233:
	case DRM_FORMAT_XRGB4444:
	case DRM_FORMAT_XBGR4444:
	case DRM_FORMAT_RGBX4444:
	case DRM_FORMAT_BGRX4444:
	case DRM_FORMAT_XRGB1555:
	case DRM_FORMAT_XBGR1555:
	case DRM_FORMAT_RGBX5551:
	case DRM_FORMAT_BGRX5551:
	case DRM_FORMAT_RGB565:
	case DRM_FORMAT_BGR565:

/* 24 bpp RGB */
	case DRM_FORMAT_RGB888:
	case DRM_FORMAT_BGR888:

/* 32 bpp RGB */
	case DRM_FORMAT_XRGB8888:
	case DRM_FORMAT_XBGR8888:
	case DRM_FORMAT_RGBX8888:
	case DRM_FORMAT_BGRX8888:

	case DRM_FORMAT_XRGB2101010:
	case DRM_FORMAT_XBGR2101010:
	case DRM_FORMAT_RGBX1010102:
	case DRM_FORMAT_BGRX1010102:

/* packed YCbCr */
	case DRM_FORMAT_YUYV:
	case DRM_FORMAT_YVYU:
	case DRM_FORMAT_UYVY:
	case DRM_FORMAT_VYUY:

	case DRM_FORMAT_AYUV:

/* 2 plane YCbCr */
	case DRM_FORMAT_NV12:
	case DRM_FORMAT_NV21:
	case DRM_FORMAT_NV16:
	case DRM_FORMAT_NV61:
	case DRM_FORMAT_NV24:
	case DRM_FORMAT_NV42:

	case DRM_FORMAT_YUV410:
	case DRM_FORMAT_YVU410:
	case DRM_FORMAT_YUV411:
	case DRM_FORMAT_YVU411:
	case DRM_FORMAT_YUV420:
	case DRM_FORMAT_YVU420:
	case DRM_FORMAT_YUV422:
	case DRM_FORMAT_YVU422:
	case DRM_FORMAT_YUV444:
	case DRM_FORMAT_YVU444:

		return GLOBAL_ALPHA;
	}

	DRM_ERROR("wrong pixel_format input\n");
	return GLOBAL_ALPHA;
}

/*
 * group index is actually layer chanel index of SUNXI DE
 */
static int sunxi_plane_create_group_index_property(
		struct sunxi_drm_plane *sunxi_plane, unsigned int crtc_id)
{
	struct drm_property *prop;
	unsigned int group_index;
	struct sunxi_drm_crtc *scrtc = sunxi_drm_crtc_get_crtc(crtc_id);

	/* Here we make group_index's range from 0 to 6,
	 * but actually, in general, SUNXI DE has 4 groups(channels) at most
	 * for layers
	 */
	prop = drm_property_create_range(sunxi_plane->plane.dev,
		DRM_MODE_PROP_IMMUTABLE, "group_index", 0, 6);
	if (!prop) {
		DRM_ERROR("Create group_index property for plane:%d failed!",
			sunxi_plane->plane_id);
		return -ENOMEM;
	}

	group_index = scrtc->hw_funcs->get_layer_channel_id(crtc_id,
					sunxi_plane->plane_id);

	drm_object_attach_property(&sunxi_plane->plane.base,
					prop, group_index);

	return 0;
}

/*
 * the sub_index is a plane sub index within a plane group(chanel)
 */
static int sunxi_plane_create_sub_index_property(
		struct sunxi_drm_plane *sunxi_plane, unsigned int crtc_id)
{
	struct drm_property *prop;
	unsigned int sub_index;
	struct sunxi_drm_crtc *scrtc = sunxi_drm_crtc_get_crtc(crtc_id);

	/* Here we make sub_index's range from 0 to 6,
	 * but actually, in general, SUNXI DE has 4 sub layers at most
	 * within a layer channel
	 */
	prop = drm_property_create_range(sunxi_plane->plane.dev,
		DRM_MODE_PROP_IMMUTABLE, "sub_index", 0, 6);
	if (!prop) {
		DRM_ERROR("Create sub_index property for plane:%d failed!",
			sunxi_plane->plane_id);
		return -ENOMEM;
	}

	sub_index = scrtc->hw_funcs->get_layer_id_within_chanel(crtc_id,
					sunxi_plane->plane_id);

	drm_object_attach_property(&sunxi_plane->plane.base,
					prop, sub_index);

	return 0;
}

static int sunxi_plane_create_alpha_mode_property(
			struct sunxi_drm_plane *sunxi_plane,
				   unsigned int alpha_mode,
				   unsigned int min, unsigned int max)
{
	struct drm_property *prop;

	prop = drm_property_create_range(sunxi_plane->plane.dev, 0,
			"alpha_mode", min, max);
	if (!prop)
		return -ENOMEM;

	drm_object_attach_property(&sunxi_plane->plane.base,
					prop, alpha_mode);

	sunxi_plane->alpha_mode_property = prop;

	sunxi_plane->alpha_mode = alpha_mode;

	return 0;
}

static int sunxi_plane_create_galpha_value_property(
			struct sunxi_drm_plane *sunxi_plane,
				   unsigned int galpha_value,
				   unsigned int min, unsigned int max)
{
	struct drm_property *prop;

	prop = drm_property_create_range(sunxi_plane->plane.dev, 0,
			"alpha", min, max);
	if (!prop)
		return -ENOMEM;

	drm_object_attach_property(&sunxi_plane->plane.base,
					prop, galpha_value);

	sunxi_plane->galpha_value_property = prop;

	sunxi_plane->galpha_value = galpha_value;

	return 0;
}


static void sunxi_plane_reset(struct drm_plane *plane)
{
	drm_atomic_helper_plane_reset(plane);
}

static int sunxi_plane_atomic_get_property(struct drm_plane *plane,
			   const struct drm_plane_state *state,
			   struct drm_property *property,
			   uint64_t *val)
{
	struct sunxi_drm_plane *sunxi_plane = to_sunxi_plane(plane);

	if (property == sunxi_plane->alpha_mode_property) {
		*val = sunxi_plane->alpha_mode;
		return 0;
	} else if (property == sunxi_plane->galpha_value_property) {
		*val = sunxi_plane->galpha_value;
		return 0;
	}

	return -1;
}

static int sunxi_plane_atomic_set_property(struct drm_plane *plane,
			   struct drm_plane_state *state,
			   struct drm_property *property,
			   uint64_t val)
{
	struct sunxi_drm_plane *sunxi_plane = to_sunxi_plane(plane);

	if (property == sunxi_plane->alpha_mode_property) {
		sunxi_plane->alpha_mode = val;
		return 0;
	} else if (property == sunxi_plane->galpha_value_property) {
		sunxi_plane->galpha_value = val;
		return 0;
	}

	return -1;
}

/**
 * update a certain plane except disable it
 * all that want to set/change plane configs will call function finally.
 *
 * NOTE: drm disable or suspend will also call this function, instead of calling
 * sunxi_plane_atomic_disable
 */
static void sunxi_plane_atomic_update(struct drm_plane *plane,
			    struct drm_plane_state *old_state)
{
	int i;

	struct drm_plane_state *new_state = NULL;
	struct drm_crtc_state *crtc_state = NULL;
	struct sunxi_drm_plane *sunxi_plane = to_sunxi_plane(plane);

	struct drm_framebuffer *fb;
	struct sunxi_drm_fb *sunxi_fb;
	struct sunxi_drm_gem_object	*sunxi_gem_obj;
	struct sunxi_drm_crtc *scrtc;
	struct drm_crtc_helper_funcs *crtc_helper_funcs;
	struct sunxi_drm_connector *sconn;

	struct disp_layer_config_data *layer_config = &sunxi_plane->config;
	struct disp_layer_config_inner *layer_config_inner = &layer_config->config;
	struct disp_layer_info_inner *layer_inner = &layer_config_inner->info;
	struct disp_fb_info_inner *fb_inner = &layer_inner->fb;

#ifdef CONFIG_VIDEO_SUNXI_CAR_REVERSE
	if (sunxi_drm_get_force_plane_en())
		return;
#endif

	DRM_DEBUG_DRIVER("[SUNXI-PLANE]%s\n", __func__);
	new_state = plane->state;
	if (!new_state) {
		DRM_ERROR("drm_plane state is NULL\n");
		return;
	}

	if (!new_state->crtc) {
		DRM_ERROR("plane state has NO crtc\n");
		return;
	}

	if (!new_state->fb) {
		DRM_ERROR("plane state has NO fb\n");
		return;
	}

/* get fb that is going to attach with */
	sunxi_fb = to_sunxi_fb(new_state->fb);
	fb = &sunxi_fb->fb;

/* get crtc that is going to attach with */
	scrtc = to_sunxi_crtc(new_state->crtc);
	crtc_state = new_state->crtc->state;

/* get connector that is going to set */
	for (i = 0; i < sunxi_drm_connector_get_count(); i++) {
		if ((crtc_state->connector_mask >> i) & 0x1)
			break;
	}

	sconn = sunxi_drm_connector_get_connector(i);
	if (!sconn) {
		DRM_ERROR("sunxi_drm_connector_get_connector:%d failed\n", i);
		return;
	}

/* ensure that manager_enable who's belongs to lowlevel hw layer is set.
 * because we reference layer/de setting from sunxi_display, so we need to
 * keep the setting sequence same with sunxi_display
 */
	if (!scrtc->enabled) {
		crtc_helper_funcs = (struct drm_crtc_helper_funcs *)
					new_state->crtc->helper_private;
		crtc_helper_funcs->atomic_enable(new_state->crtc, NULL);
	}


	/* sunxi_plane_state_dump(new_state); */

	mutex_lock(&scrtc->update_reg_lock);
	layer_config->flag |= LAYER_ALL_DIRTY;

	/* do NOT make layer_config_inner->enable = new_state->visible,
	 * layer_config_inner->enable always be true in this function,
	 * when layer disable, DRM_CORE will call ->atomic_disable() instead
	 */
	/* layers rely upon de to work, so de must be active, or layers can NOT be work */
	layer_config_inner->enable = crtc_state->active;

	layer_config_inner->channel =
		scrtc->hw_funcs->get_layer_channel_id(
		scrtc->crtc_id, sunxi_plane->plane_id);
	layer_config_inner->layer_id =
		scrtc->hw_funcs->get_layer_id_within_chanel(
		scrtc->crtc_id, sunxi_plane->plane_id);

	layer_inner->mode = LAYER_MODE_BUFFER;

	/* blending setting */
	layer_inner->zorder = new_state->zpos;
	if (sunxi_plane->alpha_mode == NONE_ALPHA_MODE) {
		layer_inner->alpha_mode = sunxi_plane_get_alpha_mode(
						new_state->fb->format->format,
						sunxi_plane->galpha_value);
			layer_inner->alpha_value = sunxi_plane->galpha_value;
	} else {
		layer_inner->alpha_mode = sunxi_plane->alpha_mode;
		layer_inner->alpha_value = sunxi_plane->galpha_value;
	}

	if (!layer_inner->alpha_value)
		layer_inner->alpha_value = 0xff;

	/* screen display setting(scale) */
	layer_inner->screen_win.x = new_state->crtc_x;
	layer_inner->screen_win.y = new_state->crtc_y;
	layer_inner->screen_win.width = new_state->crtc_w;
	layer_inner->screen_win.height = new_state->crtc_h;

	/* framebuffer information setting
	 * NOTE:
	 * (1)fb_inner->size[i].width means how many pixels a line has
	 */
	for (i = 0; i < 3; i++) {
		if (!sunxi_fb->obj[i]) {
			fb_inner->size[i].width = 0; /* have NOT been test */
			fb_inner->size[i].height = 0; /* have NOT been test */
			fb_inner->align[i] = 0;
			continue;
		}

		sunxi_gem_obj = sunxi_fb->obj[i];
		fb_inner->addr[i] = (unsigned long long)sunxi_gem_obj->dma_addr
							+ fb->offsets[i];

		switch (fb->format->num_planes) {
		case 1:
			fb_inner->size[i].width =  new_state->fb->width;
			break;

		case 2:
			if (i == 0)
				fb_inner->size[i].width =  new_state->fb->pitches[i];
			else if (i == 1)
				fb_inner->size[i].width =  new_state->fb->pitches[i] / 2;
			break;

		case 3:
			fb_inner->size[i].width =  new_state->fb->pitches[i];
			break;
		}

		fb_inner->size[i].height = new_state->fb->height;
		fb_inner->align[i] = 0;
	}
	fb_inner->format = drm_to_disp_format(new_state->fb->format->format);

	if ((new_state->fb->height >= 720) && (new_state->fb->width >= 720))
		fb_inner->color_space = DISP_BT709;
	else
		fb_inner->color_space = DISP_BT601;

	if (layer_inner->alpha_mode == 2)
		fb_inner->pre_multiply = true;

	/* framebuffer crop setting */
	fb_inner->crop.x = (((unsigned long long)new_state->src_x) >> 16) << 32;
	fb_inner->crop.y = (((unsigned long long)new_state->src_y) >> 16) << 32;
	fb_inner->crop.width = (((unsigned long long)new_state->src_w) >> 16) << 32;
	fb_inner->crop.height = (((unsigned long long)new_state->src_h) >> 16) << 32;

	fb_inner->eotf = DISP_EOTF_UNDEF;

	DRM_DEBUG_DRIVER("[SUNXI-PLANE] CH:%d  layer_id:%d plane_id:%d\n",
				layer_config_inner->channel, layer_config_inner->layer_id,
				sunxi_plane->plane_id);
	DRM_DEBUG_DRIVER("[SUNXI-PLANE]zorder:%d alpha_mode:%d alpha_value%d\n",
		layer_inner->zorder, layer_inner->alpha_mode,
		layer_inner->alpha_value);
	DRM_DEBUG_DRIVER("[SUNXI-PLANE]x-y:%d-%d width:%d height:%d\n",
		layer_inner->screen_win.x, layer_inner->screen_win.y,
		layer_inner->screen_win.width, layer_inner->screen_win.height);
	DRM_DEBUG_DRIVER("[SUNXI-PLANE]addr:0x%llx 0x%llx 0x%llx\n", fb_inner->addr[0],
		fb_inner->addr[1], fb_inner->addr[2]);
	DRM_DEBUG_DRIVER("[SUNXI-PLANE]width:%u %u %u\n", fb_inner->size[0].width,
		fb_inner->size[1].width, fb_inner->size[2].width);
	DRM_DEBUG_DRIVER("[SUNXI-PLANE]height:%u %u %u\n", fb_inner->size[0].height,
		fb_inner->size[1].height, fb_inner->size[2].height);
	DRM_DEBUG_DRIVER("[SUNXI-PLANE]format:%d\n", fb_inner->format);

	DRM_DEBUG_DRIVER("[SUNXI-PLANE]crop x:%lld y:%lld w:%lld h:%lld\n",
			fb_inner->crop.x, fb_inner->crop.y,
			fb_inner->crop.width, fb_inner->crop.height);
	DRM_DEBUG_DRIVER("[SUNXI-PLANE]depth:%d\n", fb_inner->depth);

	if (scrtc->hw_funcs->single_layer_apply(drm_crtc_index(new_state->crtc),
					layer_config) < 0) {
		DRM_ERROR("sunxi_de single layer_apply failed\n");
	} else {
		DRM_DEBUG_DRIVER("[SUNXI-PLANE]%s success\n", __func__);
	}

	mutex_unlock(&scrtc->update_reg_lock);
	return;
}

/*
 * disable a certain plane
 * when libdrm API drmModeSetPlane has NO param of fb_id, it means that user want to disable this plane.
 * in this case, sunxi_plane_atomic_disable will be call.
 */
static void sunxi_plane_atomic_disable(struct drm_plane *plane,
			   struct drm_plane_state *old_state)
{
	struct sunxi_drm_crtc *scrtc;
	struct sunxi_drm_plane *sunxi_plane = to_sunxi_plane(plane);
	struct disp_layer_config_data *layer_config = &sunxi_plane->config;
	struct disp_layer_config_inner *layer_config_inner = &layer_config->config;
	struct disp_layer_info_inner *layer_inner = &layer_config_inner->info;

	DRM_DEBUG_DRIVER("[SUNXI-PLANE]%s plane:%d\n", __func__, plane->index);
	scrtc = to_sunxi_crtc(old_state->crtc);

	mutex_lock(&scrtc->update_reg_lock);
	layer_config->flag = LAYER_ALL_DIRTY;
	memset(layer_inner, 0, sizeof(*layer_inner));

	layer_config_inner->enable = false;

	layer_config->config.channel =
		scrtc->hw_funcs->get_layer_channel_id(
				scrtc->crtc_id, plane->index);
	layer_config->config.layer_id =
		scrtc->hw_funcs->get_layer_id_within_chanel(
				scrtc->crtc_id, plane->index);

	if (scrtc->hw_funcs->single_layer_apply(drm_crtc_index(old_state->crtc),
				layer_config) < 0) {
		DRM_ERROR("sunxi_de single layer_apply failed\n");
	} else {
		DRM_DEBUG_DRIVER("%s success\n", __func__);
	}

	mutex_unlock(&scrtc->update_reg_lock);
	return;
}

static const struct drm_plane_funcs sunxi_plane_funcs = {
	.update_plane = drm_atomic_helper_update_plane,
	.disable_plane = drm_atomic_helper_disable_plane,
	.destroy = drm_plane_cleanup,
	.reset = drm_atomic_helper_plane_reset,
	.atomic_duplicate_state = drm_atomic_helper_plane_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_plane_destroy_state,
	.atomic_get_property = sunxi_plane_atomic_get_property,
	.atomic_set_property = sunxi_plane_atomic_set_property,
};

static const struct drm_plane_helper_funcs sunxi_plane_helper_funcs = {
	.prepare_fb = sunxi_drm_fb_prepare_fb,
//	.atomic_check = sunxi_plane_atomic_check,
	.atomic_update = sunxi_plane_atomic_update,
	.atomic_disable = sunxi_plane_atomic_disable,
};

int sunxi_drm_plane_init(struct drm_device *dev,
			struct sunxi_drm_plane *plane,
			int crtc_id, int plane_id, int type)
{
	const uint32_t *formats;
	unsigned int format_count;
	struct sunxi_drm_crtc *scrtc = sunxi_drm_crtc_get_crtc(crtc_id);

	if (type == DRM_PLANE_TYPE_CURSOR) {
		DRM_ERROR("sunxi hw do NOT support cursor plane\n");
		return -1;
	}

	plane->plane_id = plane_id;

	if (scrtc->hw_funcs->get_layer_formats(crtc_id, plane_id,
			&formats, &format_count) < 0) {
		DRM_ERROR("sunxi_de_get_layer_format failed\n");
		return -1;
	}

	if (drm_universal_plane_init(dev, &plane->plane, 1 << crtc_id,
			     &sunxi_plane_funcs,
			     formats, format_count,
			     NULL,
			     type,
			     "crtc%d-plane%d", crtc_id, plane_id)) {
		DRM_ERROR("drm_universal_plane_init failed\n");
		return -1;
	}

	if (drm_plane_create_zpos_property(&plane->plane, 0, 0,
			scrtc->hw_funcs->get_layer_count(crtc_id)) < 0) {
		DRM_ERROR("drm_plane_create_zpos_property Failed\n");
		return -1;
	}

	if (sunxi_plane_create_alpha_mode_property(plane, NONE_ALPHA_MODE,
				0, ALPHA_MODE_NUM) < 0) {
		DRM_ERROR("sunxi_plane_create_alpha_mode_property Failed\n");
		return -1;
	}

	if (sunxi_plane_create_galpha_value_property(plane, 0xff, 0, 0xff) < 0) {
		DRM_ERROR("sunxi_plane_create_galpha_value_property Failed\n");
		return -1;
	}

	if (sunxi_plane_create_group_index_property(plane, crtc_id) < 0) {
		DRM_ERROR("sunxi_plane_create_group_index_property Failed\n");
		return -1;
	}


	if (sunxi_plane_create_sub_index_property(plane, crtc_id) < 0) {
		DRM_ERROR("sunxi_plane_create_sub_index_property Failed\n");
		return -1;
	}

	drm_plane_helper_add(&plane->plane, &sunxi_plane_helper_funcs);

	sunxi_plane_reset(&plane->plane);
	return 0;
}
