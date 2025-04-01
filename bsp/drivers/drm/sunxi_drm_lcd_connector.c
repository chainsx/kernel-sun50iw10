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
#include <drm/drm_fb_helper.h>
#include <drm/drm_plane_helper.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_sysfs.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_print.h>
#include <video/sunxi_display2.h>

#include "drm_internal.h"
#include "sunxi_drm_drv.h"
#include "sunxi_drm_encoder.h"
#include "sunxi_drm_connector.h"

#include "sunxi_device/sunxi_lcd.h"

#if defined(CONFIG_AW_DRM_BACKLIGHT)
#include "sunxi_device/sunxi_backlight.h"
#endif

#include "de/include.h"

/*
 * transform lcd_type defined in sunxi_lcd to drm connector type
 */
static int sunxi_connector_lcd_type_trans(int sunxi_lcd_type)
{
	switch (sunxi_lcd_type) {
	case LCD_IF_HV:
		return DRM_MODE_CONNECTOR_Unknown;
	case LCD_IF_CPU:
		return DRM_MODE_CONNECTOR_Unknown;
	case LCD_IF_LVDS:
		return DRM_MODE_CONNECTOR_LVDS;
	case LCD_IF_DSI:
		return DRM_MODE_CONNECTOR_DSI;
	case LCD_IF_EDP:
		return DRM_MODE_CONNECTOR_eDP;
	}

	DRM_ERROR("wrong sunxi_lcd_type:%d\n", sunxi_lcd_type);
	return -1;
}


static void sunxi_connector_lcd_convert_panel_to_display_mode(
				struct drm_display_mode *mode,
				struct disp_panel_para  *panel)
{
	mode->clock       = panel->lcd_dclk_freq * 1000;

	mode->hdisplay    = panel->lcd_x;
	mode->hsync_start = panel->lcd_ht - panel->lcd_hbp;
	mode->hsync_end   = panel->lcd_ht - panel->lcd_hbp + panel->lcd_hspw;
	mode->htotal      = panel->lcd_ht;

	mode->vdisplay    = panel->lcd_y;
	mode->vsync_start = panel->lcd_vt - panel->lcd_vbp;
	mode->vsync_end   = panel->lcd_vt - panel->lcd_vbp + panel->lcd_vspw;
	mode->vtotal      = panel->lcd_vt;
	mode->vscan       = 0;
	mode->flags       = 0;
	mode->width_mm    = panel->lcd_width;
	mode->height_mm   = panel->lcd_height;
	/* mode->vrefresh    = mode->clock * 1000 / mode->vtotal / mode->htotal;
	DRM_DEBUG_KMS("Modeline %d:%d %d %d %d %d %d %d %d %d %d 0x%x 0x%x\n",
		mode->base.id, mode->vrefresh, mode->clock,
		mode->hdisplay, mode->hsync_start,
		mode->hsync_end, mode->htotal,
		mode->vdisplay, mode->vsync_start,
		mode->vsync_end, mode->vtotal, mode->type, mode->flags); */
	DRM_DEBUG_KMS("Modeline:%d %d %d %d %d %d %d %d %d 0x%x 0x%x\n",
		mode->clock,
		mode->hdisplay, mode->hsync_start,
		mode->hsync_end, mode->htotal,
		mode->vdisplay, mode->vsync_start,
		mode->vsync_end, mode->vtotal, mode->type, mode->flags);
	DRM_DEBUG_KMS("panel: clk[%d] [x %d, ht %d, hbp %d, hspw %d]\n",
		panel->lcd_dclk_freq * 1000,
		panel->lcd_x, panel->lcd_ht,
		panel->lcd_hbp, panel->lcd_hspw);
	DRM_DEBUG_KMS("[y%d, vt%d, bp %d, pw %d] %dx%d\n",
		panel->lcd_y, panel->lcd_vt,
		panel->lcd_vbp, panel->lcd_vspw, panel->lcd_width,
		panel->lcd_height);
}

static enum drm_connector_status sunxi_connector_lcd_detect(
				struct drm_connector *connector,
				bool force)
{
	return connector_status_connected;
}

static int sunxi_connector_lcd_get_modes(struct drm_connector *connector)
{
	struct drm_display_mode *mode;
	struct sunxi_drm_connector *sunxi_con = to_sunxi_connector(connector);
	struct disp_panel_para *panel_para = NULL;
	struct sunxi_lcd_funcs *lcd_funcs
		= (struct sunxi_lcd_funcs *)sunxi_con->hw_funcs;


	mode = drm_mode_create(connector->dev);
	if (!mode) {
		DRM_ERROR("failed to create a new display mode.\n");
		return 0;
	}

	panel_para = lcd_funcs->get_panel_para(sunxi_con->type_id);
	if (!panel_para) {
		DRM_ERROR("get lcd%d panel para failed\n",
			sunxi_con->type_id);
		return -1;
	}

	sunxi_connector_lcd_convert_panel_to_display_mode(mode, panel_para);
	connector->display_info.width_mm = mode->width_mm;
	connector->display_info.height_mm = mode->height_mm;

	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	drm_mode_set_name(mode);
	drm_mode_probed_add(connector, mode);

	return 1;
}

static int sunxi_connector_lcd_mode_valid(struct drm_connector *connector,
					struct drm_display_mode *mode)
{
	return MODE_OK;
}


/**
 * @name       :sunxi_connector_register
 * @brief      :register sysfs backlight
 * @param[IN]  :connector:pointer of drm_connector
 * @return     :0 if success, -1 else
 */
int sunxi_connector_lcd_register(struct drm_connector *connector)
{
	int ret = -1;
#if defined(CONFIG_AW_DRM_BACKLIGHT)
	struct sunxi_drm_connector *sconn = to_sunxi_connector(connector);

	if (!sconn) {
		DRM_ERROR("Null sunxi connector pointer!\n");
		goto OUT;
	}

	if (sconn->type != DISP_OUTPUT_TYPE_LCD) {
		DRM_INFO("[WARN]connector:%d is NOT lcd!\n", connector->index);
		ret = 0;
		goto OUT;
	}

	ret = sunxi_backlight_device_register(connector->kdev, sconn->type_id);
	if (ret < 0)
		DRM_ERROR("sunxi_backlight_device_register for lcd:%d failed\n",
			sconn->type_id);
#else
	DRM_INFO("[WARN]: NOT support backlight for connector:%d\n", connector->index);
	ret = 0;
#endif

OUT:
	return ret;
}


void sunxi_connector_lcd_unregister(struct drm_connector *connector)
{
#if defined(CONFIG_AW_DRM_BACKLIGHT)
	struct sunxi_drm_connector *sconn = to_sunxi_connector(connector);

	if (!sconn) {
		DRM_ERROR("Null sunxi_drm_connector pointer!\n");
		return;
	}
	if (sconn->type != DISP_OUTPUT_TYPE_LCD)
		return;

	sunxi_backlight_device_unregister(sconn->type_id);
#endif

}

static const struct drm_connector_funcs sunxi_connector_lcd_funcs = {
	.detect			= sunxi_connector_lcd_detect,
	.fill_modes		= drm_helper_probe_single_connector_modes,
	.destroy		= drm_connector_cleanup,
	.late_register          = sunxi_connector_lcd_register,
	.early_unregister	= sunxi_connector_lcd_unregister,
	.reset			= drm_atomic_helper_connector_reset,
	.atomic_duplicate_state	= drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state	= drm_atomic_helper_connector_destroy_state,
};

static const struct drm_connector_helper_funcs sunxi_connector_helper_lcd_funcs = {
	.get_modes	= sunxi_connector_lcd_get_modes,
	.mode_valid	= sunxi_connector_lcd_mode_valid,
	.best_encoder	= sunxi_connector_best_encoder,
};

static void sunxi_connector_lcd_get_prefered_resolution(
		struct sunxi_drm_connector *sunxi_conn, /* unsigned int *tvmode, */
		unsigned int *w, unsigned int *h, unsigned int *vfresh)
{
	struct disp_panel_para *panel_para = NULL;
	struct sunxi_lcd_funcs *lcd_funcs
			= (struct sunxi_lcd_funcs *)sunxi_conn->hw_funcs;

	panel_para = lcd_funcs->get_panel_para(sunxi_conn->type_id);
	if (!panel_para) {
		DRM_ERROR("get lcd%d panel para failed\n",
			sunxi_conn->type_id);
		return;
	}

	*w = panel_para->lcd_x;
	*h = panel_para->lcd_y;
	*vfresh = panel_para->lcd_dclk_freq * 1000 * 1000
		/ (panel_para->lcd_vt * panel_para->lcd_ht);
	DRM_INFO("w:%u h:%u vfresh:%u\n", *w, *h, *vfresh);
}

bool sunxi_connector_lcd_is_use_irq(struct sunxi_drm_connector *sconn)
{
	struct sunxi_lcd_funcs *lcd_funcs
		= (struct sunxi_lcd_funcs *)sconn->hw_funcs;

	return lcd_funcs->is_use_irq(sconn->type_id);
}

static int sunxi_connector_lcd_enable(struct sunxi_drm_connector *sconn,
					struct drm_display_mode *mode)
{
	struct sunxi_lcd_funcs *lcd_funcs
		= (struct sunxi_lcd_funcs *)sconn->hw_funcs;

	return lcd_funcs->enable(sconn->type_id);
}

static int
sunxi_connector_lcd_sw_enable(struct sunxi_drm_connector *sconn,
						struct drm_display_mode *mode)
{
	struct sunxi_lcd_funcs *lcd_funcs
		= (struct sunxi_lcd_funcs *)sconn->hw_funcs;

	lcd_funcs->sw_enable(sconn->type_id);

	return 0;
}

static void sunxi_connector_lcd_disable(struct sunxi_drm_connector *sconn)
{
	struct sunxi_lcd_funcs *lcd_funcs
		= (struct sunxi_lcd_funcs *)sconn->hw_funcs;

	lcd_funcs->disable(sconn->type_id);
}

struct sunxi_drm_connector *
sunxi_drm_connector_lcd_create(struct drm_device *dev, int conn_id, int lcd_id)
{
	struct drm_encoder *enc;
	struct sunxi_drm_encoder *sunxi_enc;
	struct sunxi_drm_connector *sunxi_conn;
	int lcd_type = 0;
	int drm_con_type;
	int ret;
	struct sunxi_lcd_funcs *lcd_funcs;

	sunxi_conn = kzalloc(sizeof(*sunxi_conn), GFP_KERNEL);
	if (!sunxi_conn) {
		DRM_ERROR("can NOT allocate memory for sunxi_connector\n");
		goto lcd_conn_err;
	}

	sunxi_conn->con_id = conn_id;
	sunxi_conn->type_id = lcd_id;
	sunxi_conn->type = DISP_OUTPUT_TYPE_LCD;

	lcd_funcs = sunxi_lcd_get_hw_funcs(lcd_id);
	if (!lcd_funcs) {
		DRM_ERROR("lcd:%d has NO funcs\n", lcd_id);
		goto lcd_conn_err;
	}
	sunxi_conn->hw_funcs = lcd_funcs;
	lcd_type = lcd_funcs->get_type(lcd_id);
	drm_con_type
		= sunxi_connector_lcd_type_trans(lcd_type);
	if (drm_con_type < 0) {
		DRM_ERROR("get drm connector type failed\n");
		goto lcd_conn_err;
	}

	sunxi_conn->connector.connector_type = drm_con_type;
	sunxi_conn->connector.interlace_allowed = false;
	sunxi_conn->connector.polled = 0;

	/* search for all of the encoders that can be attached to this connector */
	list_for_each_entry(enc,
		&dev->mode_config.encoder_list, head) {
		sunxi_enc = to_sunxi_encoder(enc);

		/* set an encoder working for a certain kind of connector,
		  * NOTE: if you want to use the callback functions(hw_funcs) of a encoder,
		  * you must call sunxi_tcon_attach_connector_type() to set a tcon to a kind of
		  * connector.
		  */
		sunxi_enc->hw_funcs =
			sunxi_tcon_attach_connector_type(sunxi_enc->encoder_id,
			DISP_OUTPUT_TYPE_LCD);

		if (sunxi_enc->conn_is_supported(
			sunxi_enc, sunxi_conn)) {
			ret = drm_connector_attach_encoder(
					&sunxi_conn->connector, enc);
			if (ret) {
				DRM_ERROR("failed to attach a"
				"connector to a encoder\n");
				goto lcd_conn_err;
			}
		}

		sunxi_tcon_unattach_connector_type(sunxi_enc->encoder_id);
		sunxi_enc->hw_funcs = NULL;
	}

	sunxi_conn->connector.dpms = DRM_MODE_DPMS_OFF;

	ret = drm_connector_init(dev, &sunxi_conn->connector,
				&sunxi_connector_lcd_funcs,
					drm_con_type);
	if (ret < 0) {
		DRM_ERROR("drm_connector_init failed\n");
		goto lcd_conn_err;
	}

	drm_connector_helper_add(&sunxi_conn->connector,
			&sunxi_connector_helper_lcd_funcs);

	drm_atomic_helper_connector_reset(&sunxi_conn->connector);

	sunxi_conn->use_irq = lcd_funcs->is_use_irq(lcd_id);
	sunxi_conn->irq_no = lcd_funcs->get_irq_no(lcd_id);
	sunxi_conn->get_init_resolution
		= sunxi_connector_lcd_get_prefered_resolution;
	sunxi_conn->enable = sunxi_connector_lcd_enable;
	sunxi_conn->sw_enable = sunxi_connector_lcd_sw_enable;
	sunxi_conn->disable = sunxi_connector_lcd_disable;
	return sunxi_conn;

lcd_conn_err:
	drm_connector_cleanup(&sunxi_conn->connector);
	kfree(sunxi_conn);
	return NULL;
}
