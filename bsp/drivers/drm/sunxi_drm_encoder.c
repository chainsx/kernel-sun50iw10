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
#include <drm/drm_print.h>

#include "sunxi_drm_drv.h"
#include "sunxi_drm_crtc.h"
#include "sunxi_drm_encoder.h"
#include "sunxi_device/sunxi_tcon.h"
#include "sunxi_device/sunxi_de.h"
#ifdef CONFIG_AW_DRM_LCD
#include "sunxi_device/sunxi_lcd.h"
#endif
#ifdef CONFIG_AW_DRM_TV
#include "sunxi_device/sunxi_tv.h"
#endif

static unsigned int sunxi_encoder_cnt;
static struct sunxi_drm_encoder *sunxi_encoder;

unsigned int sunxi_drm_encoder_get_count(void)
{
	return sunxi_encoder_cnt;
}

static bool sunxi_drm_encoder_in_use(struct sunxi_drm_encoder *sencoder)
{
	struct drm_connector *connector;
	struct drm_encoder *encoder = &sencoder->encoder;
	struct drm_device *dev = encoder->dev;

	list_for_each_entry(connector, &dev->mode_config.connector_list, head) {
		if (connector->encoder == encoder)
			return true;
	}

	return false;
}

int sunxi_drm_encoder_get_attached_connector_id(struct drm_encoder *encoder)
{
	struct drm_connector *connector;
	struct drm_device *dev = encoder->dev;

	list_for_each_entry(connector, &dev->mode_config.connector_list, head) {
		if (connector->encoder == encoder)
			return connector->index;
	}

	return -1;
}


ssize_t sunxi_drm_encoder_show(char *buf, struct drm_device *dev)
{
	ssize_t n = 0;
	int conn_id;
	struct drm_encoder *encoder;

	drm_for_each_encoder(encoder, dev) {
		n += sprintf(buf + n, "encoder id:%d  obj_id:%d\n", encoder->index,
							encoder->base.id);
		n += sprintf(buf + n, "name:%s\n", encoder->name);
		n += sprintf(buf + n, "type:%d\n", encoder->encoder_type);
		n += sprintf(buf + n, "possible_crtcs:%u\n", encoder->possible_crtcs);
		n += sprintf(buf + n, "possible_clones:%u\n", encoder->possible_clones);
		if (encoder->crtc)
			n += sprintf(buf + n, "attached crtc id:%d\n", encoder->crtc->index);
		else
			n += sprintf(buf + n, "NO attached crtc\n");

		conn_id = sunxi_drm_encoder_get_attached_connector_id(encoder);
		if (conn_id >= 0)
			n += sprintf(buf + n, "attached connector id:%d\n", conn_id);
		else
			n += sprintf(buf + n, "NO attached connector\n");

		n += sprintf(buf + n, "\n");
	}

	return n;
}

static void sunxi_encoder_reset(struct drm_encoder *encoder)
{
	DRM_INFO("*************%s************\n", __func__);
	return;
}

static void sunxi_encoder_disable(struct drm_encoder *encoder)
{
	int i;
	unsigned int irq_no;

	struct drm_crtc *crtc;
	struct sunxi_drm_crtc *scrtc;

	struct sunxi_drm_connector *sconn;

	struct sunxi_drm_encoder *sunxi_enc =
				to_sunxi_encoder(encoder);

	DRM_INFO("[SUNXI-ENCODER]%s\n", __func__);
	if (!encoder->crtc) {
		DRM_ERROR("This ENCODER has NO attached crtc\n");
		return;
	}
	crtc = encoder->crtc;
	scrtc = to_sunxi_crtc(crtc);

	for (i = 0; i < sunxi_drm_connector_get_count(); i++) {
		sconn = sunxi_drm_connector_get_connector(i);
		if (sconn->connector.encoder != encoder)
			continue;

		if (sconn->use_irq)
			irq_no = sconn->irq_no;
		else if (sunxi_enc->use_irq)
			irq_no = sunxi_enc->irq_no;
		else
			DRM_INFO("WARN: NO irq for tcon%d and lcd%d\n",
				sunxi_enc->encoder_id, sconn->con_id);

		scrtc->irq_unregister(scrtc, irq_no);
		sconn->disable(sconn);
		sunxi_enc->hw_funcs->unset(sunxi_enc->encoder_id);
		sunxi_tcon_unattach_connector_type(sunxi_enc->encoder_id);
	}

	return;
}

static void sunxi_encoder_enable(struct drm_encoder *encoder)
{
	int i;
	unsigned int irq_no;
	struct disp_video_timings p_info;
	struct sunxi_connector_work_mode work_mode;

	struct drm_crtc *crtc;
	struct sunxi_drm_crtc *scrtc;
	struct drm_crtc_state *crtc_state;

	/* struct drm_connector *conn; */
	struct sunxi_drm_connector *sconn;
	struct drm_connector_state *conn_state;

	struct sunxi_drm_encoder *sunxi_enc =
				to_sunxi_encoder(encoder);

#ifdef CONFIG_VIDEO_SUNXI_CAR_REVERSE
	if (sunxi_drm_get_force_plane_en())
		return;
#endif

	DRM_DEBUG_DRIVER("[SUNXI-ENCODER]%s\n", __func__);
	if (!encoder->crtc) {
		DRM_ERROR("This ENCODER has NO attached crtc\n");
		return;
	}
	crtc = encoder->crtc;
	scrtc = to_sunxi_crtc(crtc);
	crtc_state = crtc->state;

	for (i = 0; i < sunxi_drm_connector_get_count(); i++) {
		if (!((crtc_state->connector_mask >> i) & 0x1))
			continue;

		sconn = sunxi_drm_connector_get_connector(i);
		conn_state = sconn->connector.state;
		if (conn_state->best_encoder != encoder) {
			DRM_ERROR("connector%d state's best_encoder is NOT encoder%d",
				i, sunxi_enc->encoder_id);
			continue;
		}

		/* set an encoder working for a certain kind of connector,
		  * NOTE: if you want to use the callback functions(hw_funcs) of a encoder,
		  * you must call sunxi_tcon_attach_connector_type() to set a tcon to a kind of
		  * connector.
		  */
		sunxi_enc->hw_funcs =
			sunxi_tcon_attach_connector_type(sunxi_enc->encoder_id,
							sconn->type);
		sunxi_enc->use_irq =
			sunxi_enc->hw_funcs->is_use_irq(sunxi_enc->encoder_id);
		sunxi_enc->irq_no =
			sunxi_enc->hw_funcs->get_irq_no(sunxi_enc->encoder_id);

		if (sconn->use_irq)
			irq_no = sconn->irq_no;
		else if (sunxi_enc->use_irq)
			irq_no = sunxi_enc->irq_no;
		else
			DRM_INFO("WARN: NO irq for tcon%d and lcd%d\n",
				sunxi_enc->encoder_id, sconn->con_id);

		if (sconn->get_video_timing)
			sconn->get_video_timing(&p_info, &crtc_state->mode);

		if (scrtc->irq_register(scrtc, irq_no) < 0) {
				DRM_ERROR("sunxi_drm_crtc_irq_register failed\n");
				return;
		}

		if (sconn->get_work_mode)
			sconn->get_work_mode(sconn, &work_mode);

		sunxi_enc->hw_funcs->set(scrtc->crtc_id,
					 sunxi_enc->encoder_id,
					 sconn->type_id,
					 &p_info, &work_mode);
		sconn->enable(sconn, &crtc_state->mode);
	}

	return;
}

static bool sunxi_encoder_connector_is_supported(
			struct sunxi_drm_encoder *senc,
			struct sunxi_drm_connector *sconn)

{
	return senc->hw_funcs->conn_is_support(senc->encoder_id,
						sconn->con_id);
}


static void sunxi_encoder_sw_enable(struct sunxi_drm_encoder *sunxi_enc)
{
	int irq_no;
	struct drm_crtc *crtc;
	struct sunxi_drm_crtc *scrtc;
	struct drm_crtc_state *crtc_state;

	/* struct drm_connector *conn; */
	struct sunxi_drm_connector *sconn;
	struct drm_encoder *encoder = &sunxi_enc->encoder;

	DRM_INFO("[SUNXI-ENCODER]%s\n", __func__);
	if (!encoder->crtc) {
		DRM_ERROR("This ENCODER has NO attached crtc\n");
		return;
	}
	crtc = encoder->crtc;
	crtc_state = crtc->state;
	scrtc = to_sunxi_crtc(crtc);

	sconn = sunxi_drm_connector_get_connector(0);

	/* set an encoder working for a certain kind of connector,
	  * NOTE: if you want to use the callback functions(hw_funcs) of a encoder,
	  * you must call sunxi_tcon_attach_connector_type() to set a tcon to a kind of
	  * connector.
	  */
	sunxi_enc->hw_funcs =
		sunxi_tcon_attach_connector_type(sunxi_enc->encoder_id, sconn->type);
	sunxi_enc->use_irq =
		sunxi_enc->hw_funcs->is_use_irq(sunxi_enc->encoder_id);
	sunxi_enc->irq_no =
		sunxi_enc->hw_funcs->get_irq_no(sunxi_enc->encoder_id);

	if (sconn->use_irq)
		irq_no = sconn->irq_no;
	else if (sunxi_enc->use_irq)
		irq_no = sunxi_enc->irq_no;
	else
		DRM_INFO("WARN: NO irq for tcon%d and lcd%d\n",
				sunxi_enc->encoder_id, sconn->con_id);

	if (scrtc->irq_register(scrtc, irq_no) < 0) {
		DRM_ERROR("sunxi_drm_crtc_irq_register failed\n");
		return;
	}

	sunxi_enc->hw_funcs->sw_set(sunxi_enc->encoder_id, sconn->type_id);
	sconn->sw_enable(sconn, 0);
	return;
}


static const struct drm_encoder_funcs sunxi_encoder_funcs = {
	.reset = sunxi_encoder_reset,
	.destroy = drm_encoder_cleanup,
};

static const struct drm_encoder_helper_funcs sunxi_encoder_helper_funcs = {
	/* .atomic_check	= sunxi_encoder_atomic_check, */
	.disable	= sunxi_encoder_disable,
	.enable		= sunxi_encoder_enable,
	/* .mode_set	= sunxi_encoder_mode_set, */
};

/* init all of the encoders */
int sunxi_drm_encoder_init(struct drm_device *dev)
{
	int i, ret, max_en;
	struct sunxi_drm_encoder *enc;
	struct drm_crtc *crtc;
	struct sunxi_drm_crtc *scrtc;

	max_en = sunxi_tcon_get_count();
	sunxi_encoder_cnt = max_en;
	sunxi_encoder = kzalloc(
			max_en * sizeof(*sunxi_encoder), GFP_KERNEL);
	if (!sunxi_encoder) {
		DRM_ERROR("can NOT allocate memory for sunxi_encoder\n");
		goto en_err;
	}
	sunxi_encoder_cnt = max_en;

	for (i = 0; i < max_en; i++) {
		enc = &sunxi_encoder[i];
		enc->encoder_id = i;

		list_for_each_entry(crtc,
			&dev->mode_config.crtc_list, head) {
			scrtc = to_sunxi_crtc(crtc);
			if (scrtc->encoder_is_supported(scrtc, enc))
				enc->encoder.possible_crtcs |= 1 << scrtc->crtc_id;
		}

		ret = drm_encoder_init(dev, &enc->encoder,
					&sunxi_encoder_funcs,
					DRM_MODE_ENCODER_NONE,
					"sunxi-encoder%d", i);
		if (ret < 0) {
			DRM_ERROR("drm_encoder_init failed\n");
			return -1;
		}

		drm_encoder_helper_add(&enc->encoder,
				&sunxi_encoder_helper_funcs);

		/* NOTE: since tcon has NOT been attached a sunxi_tcon_funcs, now we can NOT
		  * use enc->hw_funcs
		  */
		/* enc->use_irq = enc->hw_funcs->is_use_irq(encoder->index); */
		/* enc->irq_no = enc->hw_funcs->get_irq_no(encoder->index); */

		enc->is_in_use = sunxi_drm_encoder_in_use;
		enc->conn_is_supported = sunxi_encoder_connector_is_supported;
		enc->sw_enable = sunxi_encoder_sw_enable;
	}

	return 0;
en_err:
	for (i = 0; i < max_en; i++) {
		enc = &sunxi_encoder[i];
		drm_encoder_cleanup(&enc->encoder);
	}
	kfree(sunxi_encoder);
	DRM_ERROR("sunxi encoder init failed\n");
	return -1;
}

void sunxi_drm_encoder_exit(struct drm_device *dev)
{
	int i;
	struct sunxi_drm_encoder *enc;

	for (i = 0; i < sunxi_encoder_cnt; i++) {
		if (!sunxi_encoder)
			break;
		enc = &sunxi_encoder[i];
		if (!enc)
			return;
		drm_encoder_cleanup(&enc->encoder);
	}

	kfree(sunxi_encoder);
	sunxi_encoder = NULL;
}
