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
#include <drm/drm_print.h>
#include <video/sunxi_display2.h>

#include "drm_internal.h"
#include "sunxi_drm_drv.h"
#include "sunxi_drm_encoder.h"
#include "sunxi_drm_connector.h"

#include "de/include.h"

#ifdef CONFIG_AW_DRM_TV
#include "sunxi_device/sunxi_tv.h"
#endif

static unsigned int sunxi_conector_cnt;
static struct sunxi_drm_connector *sunxi_con[MAX_CONNECTOR_COUNT];

unsigned int sunxi_drm_connector_get_count(void)
{
	return sunxi_conector_cnt;
}

struct sunxi_drm_connector *sunxi_drm_connector_get_connector(int id)
{
	if (id >= sunxi_conector_cnt) {
		DRM_ERROR("wrong connector id, out of range\n");
		return NULL;
	}

	return sunxi_con[id];
}

void
sunxi_drm_connector_set_connector(int id, struct sunxi_drm_connector *conn)
{
	if (id >= sunxi_conector_cnt) {
		DRM_ERROR("wrong connector id, out of range\n");
		return;
	}

	sunxi_con[id] = conn;
}

static ssize_t sunxi_drm_connector_state(char *buf, struct drm_connector_state *state)
{
	ssize_t n = 0;

	n += sprintf(n + buf, "[connector_state]:\n");

	if (state->crtc)
		n += sprintf(n + buf, "attaching crtc index:%d\n", state->crtc->index);

	if (state->best_encoder)
		n += sprintf(n + buf, "attaching best encoder index:%d\n", state->best_encoder->index);

	return n;
}

/* ssize_t sunxi_drm_connector_show(char *buf, struct drm_device *dev)
{
	int ret;
	ssize_t n = 0;
	struct drm_connector *connector;
	struct sunxi_drm_connector *sconn;

	mutex_lock(&dev->mode_config.mutex);
	ret = drm_modeset_lock(&dev->mode_config.connection_mutex, NULL);
	if (ret)
		return n;

	drm_for_each_connector(connector, dev) {
		sconn = to_sunxi_connector(connector);
		n += sprintf(n + buf, "connector id:%d\n", connector->index);
		if (sconn->type == DISP_OUTPUT_TYPE_LCD)
			n += sprintf(n + buf, "LCD%d\n", sconn->type_id);
		else if (sconn->type == DISP_OUTPUT_TYPE_HDMI)
			n += sprintf(n + buf, "HDMI%d\n", sconn->type_id);
		else if (sconn->type == DISP_OUTPUT_TYPE_TV)
			n += sprintf(n + buf, "TV%d\n", sconn->type_id);
		else
			n += sprintf(n + buf, "Unknow connector type!\n");

		n += sprintf(n + buf, "name:%s\n", connector->name);
		n += sprintf(n + buf, "allow interlace:%d double_scan:%d stereo:%d\n",
				connector->interlace_allowed, connector->doublescan_allowed,
				connector->stereo_allowed);
		n += sprintf(n + buf, "register:%d\n", connector->registered);
		n += sprintf(n + buf, "status:%d  polled:%d\n",
				connector->status, connector->polled);
		n += sprintf(n + buf, "dpms:%s\n", connector->dpms ? "off" : "on");
		n += sprintf(n + buf, "possible encoder id:%d %d %d\n",
			connector->encoder_ids[0], connector->encoder_ids[1],
			connector->encoder_ids[2]);
		if (connector->encoder)
			n += sprintf(n + buf, "attached encoder:%d\n", connector->encoder->index);
		else
			n += sprintf(n + buf, "No attached encoder\n");

		n += sunxi_drm_connector_state(buf + n, connector->state);
		n += sprintf(n + buf, "\n");
	}

	drm_modeset_unlock(&dev->mode_config.connection_mutex);
	mutex_unlock(&dev->mode_config.mutex);

	return n;
} */

struct drm_encoder *
sunxi_connector_best_encoder(struct drm_connector *connector)
{
#if DRM_DEBUG
	struct drm_device *dev = connector->dev;
	struct drm_mode_object *obj;
	struct drm_encoder *encoder = NULL, *best_encoder = NULL;
	struct sunxi_drm_encoder *sencoder;
	int i;

	for (i = 0; i < DRM_CONNECTOR_MAX_ENCODER; i++) {
		if (connector->encoder_ids[i] != 0) {
			/* check if it has drm_mode_object */
			obj = drm_mode_object_find(dev,
				NULL,
				connector->encoder_ids[i],
				DRM_MODE_OBJECT_ENCODER);
			if (!obj) {
				DRM_ERROR("Unknown ENCODER ID %d\n",
				connector->encoder_ids[i]);
				continue;
			}

			encoder = obj_to_encoder(obj);
			sencoder = to_sunxi_encoder(encoder);

			if (best_encoder == NULL)
				best_encoder = encoder;

			if (connector->encoder == encoder) {
				best_encoder = encoder;
				break;
			} else {
				if (!sencoder->is_in_use(sencoder)) {
					best_encoder = encoder;
					break;
				}
			}
		}
	}

	DRM_DEBUG_KMS("GET best encoder id:%d for connector:%d\n",
		best_encoder->base.id, connector->base.id);
	return best_encoder;
#endif
	struct drm_encoder *encoder;
	drm_connector_for_each_possible_encoder(connector, encoder)
		return encoder;

	return NULL;
}

int sunxi_drm_connector_init(struct drm_device *dev)
{
	int i, max_con = 0;
	struct sunxi_drm_connector *scon = NULL;
	int sunxi_con_type;
#ifdef CONFIG_AW_DRM_LCD
	int lcd_type_id = 0;
#endif

#ifdef CONFIG_AW_DRM_TV
	int tv_type_id = 0;
#endif

/* create drm_connectors according boot info,
 * NOTE: Only surpport single connector display and dual connectors display!!!
 */
	max_con = sunxi_drm_get_connector_count();
	sunxi_conector_cnt = max_con;

	for (i = 0; i < max_con; i++) {
		sunxi_con_type = sunxi_drm_get_connector_type(i);
		switch (sunxi_con_type) {
		/* LCD */
		case 1:
#ifdef CONFIG_AW_DRM_LCD
			scon = sunxi_drm_connector_lcd_create(dev,
					i, lcd_type_id);
			if (!scon) {
				DRM_ERROR("create lcd:%d connector failed!\n",
					lcd_type_id);
				goto con_err;
			}

			++lcd_type_id;
#endif
			break;

		/* TV */
		case 2:
#ifdef CONFIG_AW_DRM_TV
			scon = sunxi_drm_connector_tv_create(dev,
					i, tv_type_id);
			if (!scon) {
				DRM_ERROR("create tv:%d connector failed!\n",
					tv_type_id);
				goto con_err;
			}

			++tv_type_id;
#endif
			break;

		/* HDMI */
		case 3:
#if defined(CONFIG_AW_DRM_HDMI14) || defined(CONFIG_AW_DRM_HDMI20)
			scon = sunxi_drm_connector_hdmi_create(dev,
					i);
			if (!scon) {
				DRM_ERROR("create hdmi connector failed!\n");
				goto con_err;
			}
#endif
			break;

		default:
			DRM_ERROR("Unknown connector output type:%d\n", sunxi_con_type);
			goto con_err;
		}

		sunxi_con[i] = scon;
	}

	return 0;

con_err:
	DRM_ERROR("sunxi connector init failed\n");
	return -1;
}

void sunxi_drm_connector_destroy(int conn_id)
{
	struct sunxi_drm_connector *sunxi_conn;

	sunxi_conn = sunxi_drm_connector_get_connector(conn_id);
	if (!sunxi_conn)
		return;

	if (sunxi_conn) {
		drm_connector_cleanup(&sunxi_conn->connector);
		kfree(sunxi_conn);
		sunxi_drm_connector_set_connector(conn_id, NULL);
	}
}

void sunxi_drm_connector_exit(struct drm_device *dev)
{
	int i, max_con;

	max_con = sunxi_drm_connector_get_count();

	for (i = 0; i < max_con; i++)
		sunxi_drm_connector_destroy(i);
}

s32 bsp_disp_hdmi_get_color_format(void)
{
	struct sunxi_drm_connector *sconn;
	unsigned int i, cnt;

	cnt = sunxi_drm_connector_get_count();

	for (i = 0; i < cnt; i++) {
		sconn = sunxi_drm_connector_get_connector(i);
#if defined(CONFIG_AW_DRM_HDMI14) || defined(CONFIG_AW_DRM_HDMI20)
		if (sconn->type == DISP_OUTPUT_TYPE_HDMI)
			return sunxi_drm_connector_hdmi_get_color_format(
				&sconn->connector);
#endif
	}

	return 0;
}
