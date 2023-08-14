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

#include "hdmi_core/api/core_api.h"

#if defined(CONFIG_AW_DRM_HDMI14)
#include "sunxi_device/sunxi_hdmi14.h"
#elif defined(CONFIG_AW_DRM_HDMI20)
#include "sunxi_device/sunxi_hdmi20.h"
#endif

#include "de/include.h"

extern struct drm_display_mode *
sunxi_edid_get_drm_display_mode(char *mode_name);

int sunxi_drm_hdmi_edid_get_modes(
	struct drm_connector *connector, sink_edid_t *sink);

struct sunxi_drm_hdmi_connector *
to_sunxi_hdmi_connector(struct drm_connector *conn)
{
	struct sunxi_drm_connector *sconn
		= to_sunxi_connector(conn);

	return container_of(sconn,
		struct sunxi_drm_hdmi_connector, sunxi_conn);
}

unsigned int
sunxi_drm_connector_hdmi_get_color_format(struct drm_connector *connector)
{
	struct sunxi_drm_hdmi_connector *hdmi_conn
		= to_sunxi_hdmi_connector(connector);

	return hdmi_conn->work_mode.color_fmt;
}

static void
sunxi_drm_hdmi_set_working_mode(
		struct sunxi_drm_connector *sconn,
		struct sunxi_hdmi_work_mode *work_mode)
{
	struct sunxi_drm_hdmi_connector *hdmi_conn
		= container_of(sconn, struct sunxi_drm_hdmi_connector,
				sunxi_conn);
	struct sunxi_hdmi_funcs *hdmi_funcs = sconn->hw_funcs;

	memcpy(&hdmi_conn->work_mode,
		work_mode, sizeof(*work_mode));
	hdmi_funcs->set_working_mode(work_mode);
}

static enum drm_connector_status sunxi_connector_hdmi_detect(
				struct drm_connector *connector,
				bool force)
{
	struct drm_modeset_acquire_ctx ctx;
	int connect_status = 0;
	struct sunxi_drm_connector *sunxi_con = to_sunxi_connector(connector);
	enum drm_connector_status status = connector_status_disconnected;
	struct sunxi_hdmi_funcs *hdmi_funcs = sunxi_con->hw_funcs;

	drm_modeset_acquire_init(&ctx, 0);
	connect_status = hdmi_funcs->get_connect_status();
	status = (connect_status) ? connector_status_connected
				     : connector_status_disconnected;
	drm_modeset_drop_locks(&ctx);
	drm_modeset_acquire_fini(&ctx);

	return status;
}


static int sunxi_connector_hdmi_atomic_get_property(
			struct drm_connector *connector, const struct drm_connector_state *state,
				   struct drm_property *property, uint64_t *val)
{
	struct sunxi_drm_connector *sconn = to_sunxi_connector(connector);
	struct sunxi_drm_hdmi_connector *hdmi_sconn =
		container_of(sconn, struct sunxi_drm_hdmi_connector,
				sunxi_conn);
	struct sunxi_hdmi_work_mode *mode = &hdmi_sconn->work_mode;

	if (hdmi_sconn->hdmi_mode_property == property)
		*val = mode->hdmi_mode;
	else if (hdmi_sconn->color_fmt_property == property)
		*val = mode->color_fmt;
	else if (hdmi_sconn->color_depth_property == property)
		*val = mode->color_depth;
	else if (hdmi_sconn->color_space_property == property)
		*val = mode->color_space;
	else if (hdmi_sconn->eotf_property == property)
		*val = mode->eotf;
	else if (hdmi_sconn->color_range_property == property)
		*val = mode->color_range;
	else if (hdmi_sconn->aspect_ratio_property == property)
		*val = mode->aspect_ratio;
	else {
		DRM_ERROR("invalid property:%s\n", property->name);
		return -1;
	}

	return 0;
}

static int sunxi_connector_hdmi_atomic_set_property(
				struct drm_connector *connector, struct drm_connector_state *state,
				struct drm_property *property, uint64_t val)
{
	struct sunxi_drm_connector *sconn = to_sunxi_connector(connector);
	struct sunxi_drm_hdmi_connector *hdmi_sconn =
		container_of(sconn, struct sunxi_drm_hdmi_connector,
				sunxi_conn);
	struct sunxi_hdmi_work_mode *mode = &hdmi_sconn->work_mode;

	if (hdmi_sconn->hdmi_mode_property == property)
		mode->hdmi_mode = val;
	else if (hdmi_sconn->color_fmt_property == property)
		mode->color_fmt = val;
	else if (hdmi_sconn->color_depth_property == property)
		mode->color_depth = val;
	else if (hdmi_sconn->color_space_property == property)
		mode->color_space = val;
	else if (hdmi_sconn->eotf_property == property)
		mode->eotf = val;
	else if (hdmi_sconn->color_range_property == property)
		mode->color_range = val;
	else if (hdmi_sconn->aspect_ratio_property == property)
		mode->aspect_ratio = val;
	else {
		DRM_ERROR("invalid property:%s\n", property->name);
		return -1;
	}

	sunxi_drm_hdmi_set_working_mode(sconn, mode);

	return 0;

}

static void
sunxi_connector_bad_mode_process(
		struct drm_connector *connector)
{
	struct drm_display_mode *tmp_mode, *display_mode;

	struct sunxi_drm_connector *sunxi_con
		= to_sunxi_connector(connector);

/* set hdmi booting mode as default drm display mode */
	tmp_mode = sunxi_con->get_init_mode(sunxi_con);

	display_mode = drm_mode_create(connector->dev);
	if (!display_mode) {
		DRM_ERROR("failed to creat a new drm_display_mode");
		return;
	}

	memcpy(tmp_mode, display_mode, sizeof(*display_mode));

	display_mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_DEFAULT;

	drm_mode_probed_add(connector, display_mode);
}

static int
sunxi_connector_good_mode_process(
	struct drm_connector *connector)
{
	bool find = false;
	struct drm_display_mode *mode, *default_mode;
	struct sunxi_drm_connector *sconn = to_sunxi_connector(connector);

	/* set the init mode that set in sysconfig.fex(board.dts)
	 * as default mode
	 */
	default_mode = sconn->get_init_mode(sconn);
	if (!default_mode) {
		DRM_ERROR("can NOT get default mode\n");
		return -1;
	}

	list_for_each_entry(mode, &connector->probed_modes, head) {
		if (!strcmp(mode->name, default_mode->name)) {
			mode->type |= DRM_MODE_TYPE_DEFAULT;
			find = true;
			break;
		}
	}

	if (!find) {
		DRM_ERROR("can NOT find the default modes:%s "
			"from probed_modes list\n", default_mode->name);
		return -1;
	}

	DRM_INFO("find the default modes:%s "
			"from probed_modes list\n", default_mode->name);

	return 0;
}

static int
sunxi_connector_hdmi_get_modes(
		struct drm_connector *connector)
{
	int modes_count;
#ifdef CONFIG_AW_DRM_HDMI14
	struct sunxi_hdmi_work_mode work_mode;
#endif
	struct sunxi_drm_connector *sconn = to_sunxi_connector(connector);

	struct sunxi_hdmi_funcs *hdmi_funcs
		= (struct sunxi_hdmi_funcs *)sconn->hw_funcs;

	sink_edid_t *sink = hdmi_funcs->get_sink_caps();

	modes_count = sunxi_drm_hdmi_edid_get_modes(connector, sink);
	if (modes_count) {
		sunxi_connector_good_mode_process(connector);
		return modes_count;
	}

	sunxi_connector_bad_mode_process(connector);

#ifdef CONFIG_AW_DRM_HDMI14
	if (!modes_count) {
/* If edid is bad, work_mode = 0, and hdmi1.4 will work
 * in default mode:hdmi-rgb-8depth
 */
		work_mode.hdmi_mode = MODE_HDMI;
		work_mode.color_fmt = COLOR_FMT_RGB444;

		sunxi_drm_hdmi_set_working_mode(sunxi_con, &work_mode);
	} else {
/* hdmi14's working mode is mainly decided internally by edid,
 * NOT by user-space. But also, you can still set it by user-space
 */
		if (connector->display_info.color_formats
			& DRM_COLOR_FORMAT_YCRCB444)
			work_mode.color_fmt = COLOR_FMT_YUV444;
		else
			work_mode.color_fmt = COLOR_FMT_RGB444;

		sunxi_drm_hdmi_set_working_mode(sunxi_con, &work_mode);
	}
#endif

	return 1;
}

static int
sunxi_connector_hdmi_mode_valid(struct drm_connector *connector,
				struct drm_display_mode *mode)
{
	return MODE_OK;
}

static struct drm_connector_funcs sunxi_connector_hdmi_funcs = {
	/* .dpms			= drm_atomic_helper_connector_dpms, */
	.detect			= sunxi_connector_hdmi_detect,
	.fill_modes		= drm_helper_probe_single_connector_modes,
	.destroy		= drm_connector_cleanup,
	.reset			= drm_atomic_helper_connector_reset,
	.atomic_get_property = sunxi_connector_hdmi_atomic_get_property,
	.atomic_set_property = sunxi_connector_hdmi_atomic_set_property,
	.atomic_duplicate_state	= drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state	= drm_atomic_helper_connector_destroy_state,
};

static struct drm_connector_helper_funcs sunxi_connector_helper_hdmi_funcs = {
	.get_modes	= sunxi_connector_hdmi_get_modes,
	.mode_valid	= sunxi_connector_hdmi_mode_valid,
	/* .best_encoder	= sunxi_connector_best_encoder, */
};

static void sunxi_connector_hdmi_get_init_resolution(
		struct sunxi_drm_connector *sunxi_conn,
		unsigned int *w, unsigned int *h, unsigned int *vfresh)
{
	struct drm_display_mode *mode;

	mode = sunxi_edid_get_drm_display_mode(
			sunxi_drm_get_connector_mode(sunxi_conn->con_id));
	if (w)
		*w = mode->hdisplay;
	if (h)
		*h = mode->vdisplay;
	if (vfresh)
		*vfresh = mode->hdisplay * mode->vdisplay * 1000
		/ mode->htotal / mode->vtotal;
}

static struct drm_display_mode *
sunxi_connector_hdmi_get_init_mode(
		struct sunxi_drm_connector *sunxi_conn)
{
	return sunxi_edid_get_drm_display_mode(
			sunxi_drm_get_connector_mode(sunxi_conn->con_id));
}

int
sunxi_connector_hdmi_get_video_timing(
		struct disp_video_timings *timing,
		struct drm_display_mode *mode)
{
	memset(timing, 0, sizeof(*timing));
	memcpy(timing->name, mode->name,
		DRM_DISPLAY_MODE_LEN);

	if (!strncmp(mode->name, "CEA", 3)) {
		timing->vic = drm_match_cea_mode(mode);
	} else if (!strncmp(mode->name, "HDMI14-4k", 9)) {
		if (!strncmp(mode->name, "HDMI14-4k-3840x2160@30", 22))
			timing->vic = 1;
		else if (!strncmp(mode->name, "HDMI14-4k-3840x2160@25", 22))
			timing->vic = 2;
		else if (!strncmp(mode->name, "HDMI14-4k-3840x2160@24", 22))
			timing->vic = 3;
		else if (!strncmp(mode->name, "HDMI14-4k-4096x2160@24", 22))
			timing->vic = 4;
		else
			DRM_ERROR("Invalid hdmi 4k mode:%s\n", mode->name);
	}

	timing->pixel_clk = mode->clock * 1000;
	if (mode->clock < 27000)
		timing->pixel_repeat = 1;

	timing->b_interlace = mode->flags & DRM_MODE_FLAG_INTERLACE;
	timing->x_res = mode->hdisplay;
	timing->y_res = mode->vdisplay;
	timing->hor_total_time = mode->htotal;
	timing->hor_back_porch = mode->htotal - mode->hsync_end;
	timing->hor_front_porch = mode->hsync_start - mode->hdisplay;
	timing->hor_sync_time = mode->hsync_end - mode->hsync_start;
	timing->hor_sync_polarity = (mode->flags & DRM_MODE_FLAG_PHSYNC) ? 1 : 0;

	timing->ver_total_time = mode->vtotal;
	timing->ver_back_porch = (mode->vtotal - mode->vsync_end)
		/ (timing->b_interlace + 1);
	timing->ver_front_porch = (mode->vsync_start - mode->vdisplay)
		/ (timing->b_interlace + 1);
	timing->ver_sync_time = (mode->vsync_end - mode->vsync_start)
		/ (timing->b_interlace + 1);
	timing->ver_sync_polarity = (mode->flags & DRM_MODE_FLAG_PVSYNC) ? 1 : 0;

	return 0;
}

void
sunxi_connector_hdmi_get_working_mode(
		struct sunxi_drm_connector *sconn,
		struct sunxi_connector_work_mode *mode)
{
	struct sunxi_drm_hdmi_connector *hdmi_sconn
		= to_sunxi_hdmi_connector(&sconn->connector);

	mode->color_fmt = hdmi_sconn->work_mode.color_fmt;
	mode->color_depth = hdmi_sconn->work_mode.color_depth;
}

static int
sunxi_connector_hdmi_enable(struct sunxi_drm_connector *sconn,
				struct drm_display_mode *mode_set)
{
	struct sunxi_drm_hdmi_connector *hdmi_sconn
		= to_sunxi_hdmi_connector(&sconn->connector);
	struct sunxi_hdmi_funcs *hdmi_funcs
		= (struct sunxi_hdmi_funcs *)sconn->hw_funcs;
	struct drm_display_mode *mode;
	struct disp_video_timings timing;
	bool find = false;

	memset(&timing, 0, sizeof(timing));
	/* set the mode that user-space config as preferred mode */
	list_for_each_entry(mode, &sconn->connector.modes, head) {
		if (!strcmp(mode->name, mode_set->name)) {
			mode->type |= DRM_MODE_TYPE_USERDEF;
			find = true;
			break;
		}
	}

	if (find) {
		DRM_INFO("Enable HDMI Mode:%dx%d%c@%d(%s)\n",
			mode->hdisplay, mode->vdisplay,
			(mode->flags & DRM_MODE_FLAG_INTERLACE) ?
			'I' : 'P',
			(mode->clock * 1000) / (mode->htotal * mode->vtotal),
			mode->name);

		hdmi_funcs->set_working_mode(&hdmi_sconn->work_mode);

		sunxi_connector_hdmi_get_video_timing(&timing, mode_set);

		return hdmi_funcs->enable(&timing);
	}

	DRM_ERROR("Can NOT enable HDMI Mode:%dx%d%c@%d(%s)\n",
			mode->hdisplay, mode->vdisplay,
			(mode->flags & DRM_MODE_FLAG_INTERLACE) ?
			'I' : 'P',
			(mode->clock * 1000) / (mode->htotal * mode->vtotal),
			mode->name);
	return 0;
}

static int
sunxi_connector_hdmi_sw_enable(struct sunxi_drm_connector *sconn,
					struct drm_display_mode *mode_set)
{
	struct disp_video_timings timing;
	struct sunxi_hdmi_funcs *hdmi_funcs
		= (struct sunxi_hdmi_funcs *)sconn->hw_funcs;

	memset(&timing, 0, sizeof(timing));
	sunxi_connector_hdmi_get_video_timing(&timing, mode_set);
	hdmi_funcs->sw_enable(&timing);
	return 0;
}

static void sunxi_connector_hdmi_disable(struct sunxi_drm_connector *sconn)
{
	struct sunxi_hdmi_funcs *hdmi_funcs
		= (struct sunxi_hdmi_funcs *)sconn->hw_funcs;
	hdmi_funcs->disable();
}

struct drm_property *sunxi_drm_connector_property_create(
		struct drm_connector *connector, const char *name,
		unsigned int init_value, unsigned int min, unsigned int max)
{
	struct drm_property *prop;

	prop = drm_property_create_range(connector->dev, 0, name, min, max);
	if (!prop) {
		DRM_ERROR("Creat property:%s failed\n", name);
		return NULL;
	}

	drm_object_attach_property(&connector->base, prop, init_value);

	return prop;
}

static  void sunxi_drm_connector_hdmi_property_init(
		struct sunxi_drm_hdmi_connector *sunxi_hdmi_conn)
{
	struct drm_connector *conn = &sunxi_hdmi_conn->sunxi_conn.connector;
	struct sunxi_hdmi_funcs *hdmi_funcs
		= (struct sunxi_hdmi_funcs *)sunxi_hdmi_conn->sunxi_conn.hw_funcs;

	struct sunxi_hdmi_work_mode *params = hdmi_funcs->get_init_params();

	sunxi_hdmi_conn->hdmi_mode_property
		= sunxi_drm_connector_property_create(conn, "hdmi_mode",
			params->hdmi_mode, DISP_DVI_HDMI_UNDEFINED, DISP_HDMI);

	sunxi_hdmi_conn->color_fmt_property
		= sunxi_drm_connector_property_create(conn, "color_format",
			params->color_fmt, DISP_CSC_TYPE_RGB, DISP_CSC_TYPE_YUV420);

	sunxi_hdmi_conn->color_depth_property
		= sunxi_drm_connector_property_create(conn, "color_depth",
			params->color_depth,
			DISP_DATA_8BITS, DISP_DATA_16BITS);

	sunxi_hdmi_conn->color_space_property
		= sunxi_drm_connector_property_create(conn, "color_space",
			params->color_space, 0x000, 0xfff);

	sunxi_hdmi_conn->color_range_property
		= sunxi_drm_connector_property_create(conn, "color_range",
			params->color_range,
			DISP_COLOR_RANGE_DEFAULT, DISP_COLOR_RANGE_16_235);

	sunxi_hdmi_conn->eotf_property
		= sunxi_drm_connector_property_create(conn, "eotf",
			params->eotf, 0, 0xff);

	sunxi_hdmi_conn->aspect_ratio_property
		= sunxi_drm_connector_property_create(conn, "aspect_ratio",
			params->aspect_ratio, 0, 0xff);
}

struct sunxi_drm_connector *
sunxi_drm_connector_hdmi_create(struct drm_device *dev, int conn_id)
{
	struct drm_encoder *enc;
	struct sunxi_drm_encoder *sunxi_enc;
	struct sunxi_drm_connector *sunxi_conn;
	struct sunxi_drm_hdmi_connector *sunxi_hdmi_conn;
	int ret;
	struct sunxi_hdmi_funcs *hdmi_funcs;

	sunxi_hdmi_conn = kzalloc(sizeof(*sunxi_hdmi_conn),
				GFP_KERNEL);
	if (!sunxi_hdmi_conn) {
		DRM_ERROR("can NOT allocate memory for sunxi_hdmi_connector\n");
		goto hdmi_conn_err;
	}

	sunxi_conn = &sunxi_hdmi_conn->sunxi_conn;

	sunxi_conn->con_id = conn_id;
	sunxi_conn->type_id = 0;
	sunxi_conn->type = DISP_OUTPUT_TYPE_HDMI;

	hdmi_funcs = sunxi_hdmi_get_funcs();
	if (!hdmi_funcs) {
		DRM_ERROR("hdmi has NO funcs\n");
		goto hdmi_conn_err;
	}
	sunxi_conn->hw_funcs = hdmi_funcs;

	sunxi_conn->connector.connector_type = DRM_MODE_CONNECTOR_HDMIA;
	sunxi_conn->connector.interlace_allowed = true;
	sunxi_conn->connector.polled
		= DRM_CONNECTOR_POLL_CONNECT | DRM_CONNECTOR_POLL_DISCONNECT;

	/* search for the encoders that can be attached to this connector */
	list_for_each_entry(enc,
		&dev->mode_config.encoder_list, head) {
		sunxi_enc = to_sunxi_encoder(enc);

		/* In order to use tcon callback functions */
		sunxi_enc->hw_funcs = sunxi_tcon_attach_connector_type(
			sunxi_enc->encoder_id, DISP_OUTPUT_TYPE_HDMI);
		if (sunxi_enc->conn_is_supported(sunxi_enc, sunxi_conn)) {
			ret = drm_connector_attach_encoder(
					&sunxi_conn->connector, enc);
			if (ret) {
				DRM_ERROR("failed to attach a"
				"connector to a encoder\n");
				goto hdmi_conn_err;
			}
		}

		sunxi_tcon_unattach_connector_type(sunxi_enc->encoder_id);
		sunxi_enc->hw_funcs = NULL;
	}

	sunxi_conn->connector.dpms = DRM_MODE_DPMS_OFF;

	memcpy(&sunxi_hdmi_conn->work_mode,
		hdmi_funcs->get_init_params(),
		sizeof(sunxi_hdmi_conn->work_mode));

	ret = drm_connector_init(dev, &sunxi_conn->connector,
				&sunxi_connector_hdmi_funcs,
					DRM_MODE_CONNECTOR_HDMIA);
	if (ret < 0) {
		DRM_ERROR("drm_connector_init failed\n");
		goto hdmi_conn_err;
	}

	drm_connector_helper_add(&sunxi_conn->connector,
			&sunxi_connector_helper_hdmi_funcs);

	drm_atomic_helper_connector_reset(&sunxi_conn->connector);
	sunxi_conn->connector.status = connector_status_disconnected;

	sunxi_conn->get_init_resolution
			= sunxi_connector_hdmi_get_init_resolution;
	sunxi_conn->get_init_mode
			= sunxi_connector_hdmi_get_init_mode;
	sunxi_conn->get_work_mode = sunxi_connector_hdmi_get_working_mode,

	sunxi_conn->get_video_timing
			= sunxi_connector_hdmi_get_video_timing;

	sunxi_conn->enable
			= sunxi_connector_hdmi_enable;
	sunxi_conn->sw_enable
			= sunxi_connector_hdmi_sw_enable;
	sunxi_conn->disable
			= sunxi_connector_hdmi_disable;

	sunxi_drm_connector_hdmi_property_init(sunxi_hdmi_conn);

	return sunxi_conn;

hdmi_conn_err:
	drm_connector_cleanup(&sunxi_conn->connector);
	kfree(sunxi_conn);
	return NULL;
}
