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
#include <drm/drmP.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_plane_helper.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_sysfs.h>
#include <video/sunxi_display2.h>

#include "../drm_internal.h"
#include "sunxi_drm_drv.h"
#include "sunxi_drm_encoder.h"
#include "sunxi_drm_connector.h"

#include "sunxi_device/sunxi_tv.h"
#include "de/include.h"

static int sunxi_connector_tv_disp_video_timing_to_display_mode(
	struct drm_display_mode *mode, struct disp_video_timings *video_info)
{
	int ret = -1;

	if (!mode || !video_info) {
		DRM_ERROR("Null pointer!\n");
		goto OUT;
	}

	mode->clock = video_info->pixel_clk / 1000;

	mode->hdisplay = video_info->x_res;
	mode->hsync_start = video_info->hor_total_time -
			    video_info->hor_back_porch -
			    video_info->hor_sync_time;
	mode->hsync_end = mode->hsync_start + video_info->hor_sync_time;
	mode->htotal = video_info->hor_total_time;

	mode->vdisplay = video_info->y_res;
	mode->vsync_start = video_info->ver_total_time -
			    video_info->ver_back_porch -
			    video_info->ver_sync_time;
	mode->vsync_end = mode->vsync_start + video_info->ver_sync_time;
	mode->vtotal = video_info->ver_total_time;
	mode->vscan = 0;
	mode->flags = 0;
	mode->width_mm = 0;
	mode->height_mm = 0;
	mode->vrefresh = mode->clock * 1000 / mode->vtotal / mode->htotal;
	DRM_DEBUG_KMS("Modeline %d:%d %d %d %d %d %d %d %d %d %d 0x%x 0x%x\n",
		      mode->base.id, mode->vrefresh, mode->clock,
		      mode->hdisplay, mode->hsync_start, mode->hsync_end,
		      mode->htotal, mode->vdisplay, mode->vsync_start,
		      mode->vsync_end, mode->vtotal, mode->type, mode->flags);

OUT:
	return ret;
}

static int sunxi_connector_tv_type_trans(enum disp_tv_output tv_type)
{
	switch (tv_type) {
	case DISP_TV_CVBS:
		return DRM_MODE_CONNECTOR_Composite;
	case DISP_TV_YPBPR:
		return DRM_MODE_CONNECTOR_Component;
	case DISP_TV_SVIDEO:
		return DRM_MODE_CONNECTOR_SVIDEO;
	case DISP_VGA:
		return DRM_MODE_CONNECTOR_VGA;
	default:
		break;
	}

	DRM_ERROR("wrong sunxi_lcd_type:%d\n", tv_type);
	return -1;
}

/*
 * Chose preferred mode  according to line number of TV format
 */
static void
sunxi_connector_tv_chose_preferred_modes(struct drm_connector *connector,
			       struct drm_display_mode *mode_ptr)
{
	struct sunxi_drm_connector *sunxi_con = to_sunxi_connector(connector);
	enum disp_tv_output tv_type;

	if (sunxi_con) {
		tv_type = sunxi_tv_get_interface_type(sunxi_con->type_id);
		switch (tv_type) {
		case DISP_TV_CVBS:
			if (mode_ptr->vdisplay == 576)
				mode_ptr->type |=
				    DRM_MODE_TYPE_PREFERRED;
			break;
		case DISP_VGA:
		case DISP_TV_YPBPR:
			if (mode_ptr->vdisplay == 1080 &&
			    mode_ptr->hdisplay)
				mode_ptr->type |=
				    DRM_MODE_TYPE_PREFERRED;
			break;
		default:
			break;
		}
	}
}

static enum drm_connector_status sunxi_connector_tv_detect(
				struct drm_connector *connector,
				bool force)
{
	struct drm_modeset_acquire_ctx ctx;
	int tv_connect_status = 0;
	struct sunxi_drm_connector *con = to_sunxi_connector(connector);
	enum drm_connector_status status = connector_status_unknown;

	drm_modeset_acquire_init(&ctx, 0);
	tv_connect_status = sunxi_tv_get_connect_status(con->type_id);
	status = (tv_connect_status) ? connector_status_connected
				     : connector_status_disconnected;
	drm_modeset_drop_locks(&ctx);
	drm_modeset_acquire_fini(&ctx);

	return status;
}

static int sunxi_connector_tv_get_modes(struct drm_connector *connector)
{
	int num_of_mode = 0, i = 0;
	struct disp_video_timings *p_info;
	struct drm_display_mode *mode;
	struct sunxi_drm_connector *sunxi_con = to_sunxi_connector(connector);

	sunxi_tv_get_video_timing_info(sunxi_con->type_id,
					&num_of_mode, &p_info);

	for (i = 0; i < num_of_mode; ++i, ++p_info) {
		mode = drm_mode_create(connector->dev);
		if (!mode) {
			DRM_ERROR("failed to create a new display mode.\n");
			return 0;
		}
		sunxi_connector_tv_disp_video_timing_to_display_mode(mode, p_info);

		mode->type = DRM_MODE_TYPE_DRIVER;
		/* Important: to distinguish with different resolution */
		snprintf(mode->name, DRM_DISPLAY_MODE_LEN, "%d",
			 p_info->tv_mode);
		sunxi_connector_tv_chose_preferred_modes(connector, mode);
		drm_mode_probed_add(connector, mode);
	}

	return num_of_mode;
}

static int sunxi_connector_tv_mode_valid(struct drm_connector *connector,
					struct drm_display_mode *mode)
{
	return MODE_OK;
}

static struct drm_connector_funcs sunxi_connector_tv_funcs = {
	.dpms			= drm_atomic_helper_connector_dpms,
	.detect			= sunxi_connector_tv_detect,
	.fill_modes		= drm_helper_probe_single_connector_modes,
	.destroy		= drm_connector_cleanup,
	.reset			= drm_atomic_helper_connector_reset,
	.atomic_duplicate_state	= drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state	= drm_atomic_helper_connector_destroy_state,
};

static struct drm_connector_helper_funcs sunxi_connector_helper_tv_funcs = {
	.get_modes	= sunxi_connector_tv_get_modes,
	.mode_valid	= sunxi_connector_tv_mode_valid,
	/* .best_encoder	= sunxi_connector_best_encoder, */
};

static void sunxi_connector_tv_get_prefered_resolution(
		struct sunxi_drm_connector *sunxi_conn, unsigned int *tvmode,
		unsigned int *w, unsigned int *h, unsigned int *vfresh)
{
	int num_of_mode = 0;
	struct disp_video_timings *p_info;

	sunxi_tv_get_video_timing_info(sunxi_conn->type_id,
					&num_of_mode, &p_info);
	*w = p_info->x_res;
	*h = p_info->y_res;
	*vfresh = p_info->pixel_clk /
		(p_info->ver_total_time * p_info->hor_total_time);
}

static struct disp_video_timings *
	sunxi_connector_tv_get_video_timing(
	struct sunxi_drm_connector *sunxi_conn, unsigned int tv_mode)
{
	return sunxi_tv_get_video_timing(sunxi_conn->type_id, tv_mode);
}


static void
sunxi_connector_tv_get_work_mode(
			struct sunxi_drm_connector *sunxi_conn,
			struct sunxi_connector_work_mode *work_mode)
{
		struct sunxi_tv_work_mode tv_work_mode;

		sunxi_tv_get_working_mode(sunxi_conn->type_id, &tv_work_mode);
		work_mode->color_fmt = tv_work_mode.color_fmt;
		work_mode->color_depth = SUNXI_COLOR_DEPTH_8;
}

static int sunxi_connector_tv_enable(struct sunxi_drm_connector *sconn,
					unsigned int tv_mode)
{
	struct drm_display_mode *mode;
	unsigned int tv_mode_tmp;
	bool find = false;

	/* set the mode that user-space config as preferred mode */
	list_for_each_entry(mode, &sconn->connector.modes, head) {
		if (kstrtou32(mode->name, 10,
			&tv_mode_tmp)) {
			DRM_ERROR("%dx%d%c@%d NOT use tv_mode\n",
				mode->hdisplay, mode->vdisplay,
				(mode->flags & DRM_MODE_FLAG_INTERLACE) ?
				'I' : 'P',
				(mode->clock * 1000)
				/ (mode->htotal * mode->vtotal));
			return -1;
		}

		if (tv_mode_tmp == tv_mode) {
			mode->type = DRM_MODE_TYPE_PREFERRED;
			find = true;
			break;
		}
	}

	if (find) {
		sunxi_tv_set_mode(sconn->type_id, tv_mode);
		sunxi_tv_enable(sconn->type_id);

		return 0;
	}

	DRM_ERROR("tv_mode:%d can NOT be set\n", tv_mode);
	return 0;
}

static int
sunxi_connector_tv_sw_enable(struct sunxi_drm_connector *sconn,
							unsigned int tv_mode)
{
	return 0;
}

static void sunxi_connector_tv_disable(struct sunxi_drm_connector *sconn)
{
	sunxi_tv_disable(sconn->type_id);
}


struct sunxi_drm_connector *
sunxi_drm_connector_tv_create(struct drm_device *dev, int conn_id, int tv_id)
{
	struct drm_encoder *enc;
	struct sunxi_drm_encoder *sunxi_enc;
	struct sunxi_drm_connector *sunxi_conn;
	int drm_con_type;
	int ret;

	sunxi_conn = kzalloc(sizeof(*sunxi_conn), GFP_KERNEL);
	if (!sunxi_conn) {
		DRM_ERROR("can NOT allocate memory for sunxi_connector\n");
		goto tv_conn_err;
	}

	sunxi_conn->con_id = conn_id;
	sunxi_conn->type_id = tv_id;
	sunxi_conn->type = DISP_OUTPUT_TYPE_TV;

	drm_con_type = sunxi_connector_tv_type_trans(
	    sunxi_tv_get_interface_type(sunxi_conn->type_id));

	sunxi_conn->connector.connector_type = drm_con_type;
	sunxi_conn->connector.interlace_allowed = true;
	sunxi_conn->connector.polled
		= DRM_CONNECTOR_POLL_CONNECT | DRM_CONNECTOR_POLL_DISCONNECT;

	/* search for the encoders that can be attached to this connector */
	list_for_each_entry(enc,
		&dev->mode_config.encoder_list, head) {
		sunxi_enc = to_sunxi_encoder(enc);

		/* In order to use tcon callback functions */
		sunxi_enc->hw_funcs =
			sunxi_tcon_attach_connector_type(sunxi_enc->encoder_id,
			DISP_OUTPUT_TYPE_TV);
		if (sunxi_enc->conn_is_supported(sunxi_enc, sunxi_conn)) {
			ret = drm_mode_connector_attach_encoder(
					&sunxi_conn->connector, enc);
			if (ret) {
				DRM_ERROR("failed to attach a"
				"connector to a encoder\n");
				goto tv_conn_err;
			}
		}

		sunxi_tcon_unattach_connector_type(sunxi_enc->encoder_id);
		sunxi_enc->hw_funcs = NULL;
	}

	sunxi_conn->connector.dpms = DRM_MODE_DPMS_OFF;

	ret = drm_connector_init(dev, &sunxi_conn->connector,
				&sunxi_connector_tv_funcs,
					drm_con_type);
	if (ret < 0) {
		DRM_ERROR("drm_connector_init failed\n");
		goto tv_conn_err;
	}

	drm_connector_helper_add(&sunxi_conn->connector,
			&sunxi_connector_helper_tv_funcs);

	drm_atomic_helper_connector_reset(&sunxi_conn->connector);

	/* sunxi_conn->use_irq = tv_funcs->is_use_irq(tv_id); */
	/* sunxi_conn->irq_no = tv_funcs->get_irq_no(tv_id); */
	sunxi_conn->get_init_resolution
				= sunxi_connector_tv_get_prefered_resolution;
	sunxi_conn->get_video_timing = sunxi_connector_tv_get_video_timing;
	sunxi_conn->get_work_mode = sunxi_connector_tv_get_work_mode;
	sunxi_conn->enable = sunxi_connector_tv_enable;
	sunxi_conn->sw_enable = sunxi_connector_tv_sw_enable;
	sunxi_conn->disable = sunxi_connector_tv_disable;

	return sunxi_conn;

tv_conn_err:
	drm_connector_cleanup(&sunxi_conn->connector);
	kfree(sunxi_conn);
	return NULL;
}
