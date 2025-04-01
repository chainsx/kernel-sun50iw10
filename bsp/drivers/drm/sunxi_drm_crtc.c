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
#include <drm/drm_atomic.h>
#include <drm/drm_print.h>
#include <drm/drm_vblank.h>

#include "sunxi_drm_drv.h"
#include "sunxi_drm_crtc.h"
#include "sunxi_drm_plane.h"
#include "sunxi_drm_connector.h"

static unsigned int sunxi_crtc_cnt;
static struct sunxi_drm_crtc *sunxi_crtc;

struct sunxi_drm_crtc *sunxi_drm_crtc_get_crtc(int id)
{
	if (id >= sunxi_crtc_cnt) {
		DRM_ERROR("crtc id:%d is too big\n", id);
		return NULL;
	}

	return &sunxi_crtc[id];
}

unsigned int sunxi_drm_get_crtc_count(void)
{
	return sunxi_crtc_cnt;
}

void sunxi_drm_crtc_destroy(struct drm_crtc *drm_crtc);

static struct drm_connector *
sunxi_crtc_get_attached_connector(struct drm_crtc *crtc)
{
	bool find = false;
	struct drm_encoder *encoder;
	struct drm_connector *connector;
	struct drm_connector_list_iter conn_iter;
	struct drm_device *dev;

	drm_for_each_encoder(encoder, crtc->dev) {
		if (encoder->crtc == crtc) {
			find = true;
			break;
		}
	}

	if (!find) {
		DRM_ERROR("can NOT find the attached encoder for crtc:%d\n",
			crtc->index);
		return NULL;
	}
	dev = encoder->dev;
	find = false;
	drm_connector_list_iter_begin(dev, &conn_iter);
	drm_for_each_connector_iter(connector, &conn_iter) {
		if (connector->encoder == encoder) {
			drm_connector_list_iter_end(&conn_iter);
			find = true;
			break;
		}
	}
	drm_connector_list_iter_end(&conn_iter);

	if (!find) {
		DRM_ERROR("can NOT find the attached connector for encoder:%d\n",
			encoder->index);
		return NULL;
	}

	return connector;
}

static int sunxi_crtc_set_enhance(struct drm_crtc *crtc,
			struct sunxi_drm_crtc_enhance *enhance)
{
	int ret;
	struct drm_connector *connector;
	struct sunxi_drm_connector *sconnector;
	struct sunxi_drm_crtc *scrtc = to_sunxi_crtc(crtc);
	struct sunxi_de_funcs *de_funcs = scrtc->hw_funcs;

	connector = sunxi_crtc_get_attached_connector(crtc);
	if (!connector) {
		DRM_ERROR("Not find attached connector for crtc:%d\n",
			crtc->index);
		return -EINVAL;
	}

	sconnector = to_sunxi_connector(connector);

	mutex_lock(&scrtc->update_reg_lock);
	ret = de_funcs->set_enhance(crtc->index,
			enhance->mode, enhance->enable,
			crtc->mode.hdisplay, crtc->mode.vdisplay,
			sconnector->type);

	atomic_set(&scrtc->update, 1);

	mutex_unlock(&scrtc->update_reg_lock);

	return ret;
}


static void sunxi_crtc_get_enhance(struct drm_crtc *crtc,
			struct sunxi_drm_crtc_enhance *enhance)
{
	struct sunxi_drm_crtc *scrtc = to_sunxi_crtc(crtc);
	struct sunxi_de_funcs *de_funcs = scrtc->hw_funcs;

	mutex_lock(&scrtc->update_reg_lock);
	de_funcs->get_enhance(crtc->index,
	&enhance->mode, &enhance->enable, NULL, NULL);
	mutex_unlock(&scrtc->update_reg_lock);
}

int sunxi_drm_crtc_set_enhance_ioctl(struct drm_device *dev,
		void *data, struct drm_file *file_priv)
{
	struct drm_crtc *crtc;
	struct sunxi_drm_crtc_enhance *enhance =
		(struct sunxi_drm_crtc_enhance *)data;

	crtc = drm_crtc_find(dev, file_priv, enhance->crtc_obj_id);
	if (!crtc) {
		DRM_ERROR("can NOT find crtc for crtc_id:%d\n",
			enhance->crtc_obj_id);
		return -EINVAL;
	}

	return sunxi_crtc_set_enhance(crtc, enhance);
}

int sunxi_drm_crtc_get_enhance_ioctl(struct drm_device *dev,
		void *data, struct drm_file *file_priv)
{
	struct drm_crtc *crtc;
	struct sunxi_drm_crtc_enhance *enhance =
		(struct sunxi_drm_crtc_enhance *)data;

	crtc = drm_crtc_find(dev, file_priv, enhance->crtc_obj_id);
	if (!crtc) {
		DRM_ERROR("can NOT find crtc for crtc_id:%d\n",
			enhance->crtc_obj_id);
		return -EINVAL;
	}

	sunxi_crtc_get_enhance(crtc, enhance);

	return 0;
}

/* smbl */
static int sunxi_crtc_set_smbl(struct drm_crtc *crtc,
			struct sunxi_drm_crtc_smbl *smbl)
{
	int ret;
	struct sunxi_drm_crtc *scrtc = to_sunxi_crtc(crtc);
	struct sunxi_de_funcs *de_funcs = scrtc->hw_funcs;

	mutex_lock(&scrtc->update_reg_lock);
	ret = de_funcs->set_smbl(crtc->index,
					smbl->enable, &smbl->window);

	atomic_set(&scrtc->update, 1);

	mutex_unlock(&scrtc->update_reg_lock);

	return ret;
}

static void sunxi_crtc_get_smbl(struct drm_crtc *crtc,
			struct sunxi_drm_crtc_smbl *smbl)
{
	struct sunxi_drm_crtc *scrtc = to_sunxi_crtc(crtc);
	struct sunxi_de_funcs *de_funcs = scrtc->hw_funcs;

	mutex_lock(&scrtc->update_reg_lock);
	de_funcs->get_smbl(crtc->index,
		&smbl->enable, &smbl->window);
	mutex_unlock(&scrtc->update_reg_lock);
}

int sunxi_drm_crtc_set_smbl_ioctl(struct drm_device *dev,
		void *data, struct drm_file *file_priv)
{
	struct drm_crtc *crtc;
	struct sunxi_drm_crtc_smbl *smbl =
		(struct sunxi_drm_crtc_smbl *)data;

	crtc = drm_crtc_find(dev, file_priv, smbl->crtc_obj_id);
	if (!crtc) {
		DRM_ERROR("can NOT find crtc for crtc_id:%d\n",
			smbl->crtc_obj_id);
		return -EINVAL;
	}

	return sunxi_crtc_set_smbl(crtc, smbl);
}

int sunxi_drm_crtc_get_smbl_ioctl(struct drm_device *dev,
		void *data, struct drm_file *file_priv)
{
	struct drm_crtc *crtc;
	struct sunxi_drm_crtc_smbl *smbl =
		(struct sunxi_drm_crtc_smbl *)data;

	crtc = drm_crtc_find(dev, file_priv, smbl->crtc_obj_id);
	if (!crtc) {
		DRM_ERROR("can NOT find crtc for crtc_id:%d\n",
			smbl->crtc_obj_id);
		return -EINVAL;
	}

	sunxi_crtc_get_smbl(crtc, smbl);

	return 0;
}


void sunxi_crtc_modeinfo_dump(struct drm_mode_modeinfo *mode)
{
	DRM_INFO("MODE INFO:\n");
	DRM_INFO("clock:%u\n flag:%u type:%u name:%s\n",
		mode->clock, mode->flags, mode->type, mode->name);
	DRM_INFO("hdisplay:%u hsync_start:%u hsync_end:%u htotal:%u hskew:%u\n",
		mode->hdisplay, mode->hsync_start, mode->hsync_end,
		mode->htotal, mode->hskew);
		DRM_INFO("vdisplay:%u vsync_start:%u vsync_end:%u "
			"vtotal:%u hscan:%u vrefresh:%u\n",
		mode->vdisplay, mode->vsync_start, mode->vsync_end,
		mode->vtotal, mode->vscan, mode->vrefresh);
}

/* void sunxi_crtc_state_dump(struct drm_crtc_state *state)
{
	struct drm_mode_modeinfo modeinfo;

	DRM_INFO("drm_crtc_state info:\n");
	DRM_INFO("CRTC index: %d\n", drm_crtc_index(state->crtc));
	DRM_INFO("enable:%u active:%u\n", state->enable, state->active);
	DRM_INFO("Changed: planes:%u mode:%u active:%u connectors:%u "
		"zops:%u color_mgmt_changed:%u\n", state->planes_changed,
		state->mode_changed, state->active_changed,
		state->connectors_changed,
		state->zpos_changed, state->color_mgmt_changed);
	DRM_INFO("MASK: plane:0x%x connector:0x%x encoder:0x%x\n",
		state->plane_mask, state->connector_mask, state->encoder_mask);
	DRM_INFO("last_vblank_count:%u\n", state->last_vblank_count);

	drm_property_reference_blob(state->mode_blob);
	memcpy(&modeinfo, state->mode_blob->data, state->mode_blob->length);
	sunxi_crtc_modeinfo_dump(&modeinfo);
	drm_property_unreference_blob(state->mode_blob);
} */

static ssize_t sunxi_crtc_modeinfo_show(char *buf, struct drm_mode_modeinfo *mode)
{
	ssize_t n = 0;

	n += sprintf(buf + n, "clock:%u flag:%u type:%u name:%s\n",
		mode->clock, mode->flags, mode->type, mode->name);
	n += sprintf(buf + n, "hdisplay:%u hsync_start:%u hsync_end:%u htotal:%u hskew:%u\n",
		mode->hdisplay, mode->hsync_start, mode->hsync_end,
		mode->htotal, mode->hskew);
	n += sprintf(buf + n, "vdisplay:%u vsync_start:%u vsync_end:%u "
			"vtotal:%u hscan:%u vrefresh:%u\n\n",
		mode->vdisplay, mode->vsync_start, mode->vsync_end,
		mode->vtotal, mode->vscan, mode->vrefresh);

	return n;
}

/* static ssize_t sunxi_crtc_state_show(char *buf, struct drm_crtc_state *state)
{
	ssize_t n = 0;
	struct drm_mode_modeinfo modeinfo;

	n += sprintf(buf + n, "enable:%u active:%u\n", state->enable, state->active);
	n += sprintf(buf + n, "Changed: planes:%u mode:%u active:%u connectors:%u "
		"zops:%u color_mgmt_changed:%u\n", state->planes_changed,
		state->mode_changed, state->active_changed,
		state->connectors_changed,
		state->zpos_changed, state->color_mgmt_changed);
	n += sprintf(buf + n, "MASK: plane:0x%x connector:0x%x encoder:0x%x\n",
		state->plane_mask, state->connector_mask, state->encoder_mask);
	n += sprintf(buf + n, "last_vblank_count:%u\n", state->last_vblank_count);

	if (!state->mode_blob) {
		n += sprintf(buf + n, "No mode blob\n");
		return n;
	}

	n += sprintf(buf + n, "[crtc state MODE INFO]:\n");
	drm_property_reference_blob(state->mode_blob);
	memcpy(&modeinfo, state->mode_blob->data, state->mode_blob->length);
	n += sunxi_crtc_modeinfo_show(buf + n, &modeinfo);
	drm_property_unreference_blob(state->mode_blob);

	return n;
}


ssize_t sunxi_drm_crtc_show(char *buf, struct drm_device *dev)
{
	ssize_t n = 0;
	struct drm_crtc *crtc;

	drm_for_each_crtc(crtc, dev) {
		struct sunxi_drm_crtc *scrtc = to_sunxi_crtc(crtc);

		n += sprintf(buf + n, "crtc id:%d\n\n", scrtc->crtc_id);

		n += sprintf(buf + n, "[crtc id:%d  basic info]:\n", scrtc->crtc_id);
		n += sprintf(buf + n, "enabled(core):%d  enable(hal):%d\n",
				crtc->enabled, scrtc->enabled);
		n += sprintf(buf + n, "overlay_plane_num:%d  plane_num:%d\n",
			scrtc->overlay_plane_num, scrtc->plane_num);
		n += sprintf(buf + n, "crtc name:%s\n", crtc->name);
		n += sprintf(buf + n, "drm_mode_object id:%u\n", crtc->base.id);

		if (!crtc->enabled || !scrtc->enabled) {
			n += sprintf(buf + n, "NOT enabled!!!\n\n");
			return n;
		}

		n += sprintf(buf + n, "x-y:%d-%d\n", crtc->x, crtc->y);
		n += sprintf(buf + n, "fence_seqno:%lu\n", crtc->fence_seqno);
		n += sprintf(buf + n, "timeline_name:%s\n\n", crtc->timeline_name);

		if (crtc->state) {
			n += sprintf(buf + n, "[crtc id:%d state info]:\n", scrtc->crtc_id);
			n += sunxi_crtc_state_show(buf + n, crtc->state);
		}

		n += sprintf(buf + n, "[crtc id:%d enabled planes info]:\n", scrtc->crtc_id);
		n += sunxi_drm_planes_show(buf + n, crtc->dev, crtc);

		n += sprintf(buf + n, "\n");
	}

	return n;
} */

bool sunxi_drm_crtc_in_use(struct sunxi_drm_crtc *scrtc)
{
	struct drm_encoder *encoder;
	struct sunxi_drm_encoder *sencoder;
	struct drm_crtc *crtc = &scrtc->crtc;
	struct drm_device *dev = crtc->dev;

	drm_for_each_encoder(encoder, dev) {
		sencoder = to_sunxi_encoder(encoder);
		if (encoder->crtc == crtc && sencoder->is_in_use(sencoder))
			return true;
	}

	return false;
}

static void sunxi_crtc_finish_page_flip(struct drm_device *dev,
					struct sunxi_drm_crtc *scrtc)
{
	unsigned long flags;

	/* send the vblank of drm_crtc_state->event */
	spin_lock_irqsave(&dev->event_lock, flags);
	if (scrtc->event) {
		drm_crtc_send_vblank_event(&scrtc->crtc, scrtc->event);
		drm_crtc_vblank_put(&scrtc->crtc);
		scrtc->event = NULL;
	}
	spin_unlock_irqrestore(&dev->event_lock, flags);
}

static irqreturn_t sunxi_crtc_event_proc(int irq, void *parg)
{
	int ret = 0;
	struct drm_crtc *crtc = (struct drm_crtc *)parg;
	struct sunxi_drm_crtc *scrtc = to_sunxi_crtc(crtc);
	struct sunxi_de_funcs *hw_funcs = scrtc->hw_funcs;

	ret = hw_funcs->query_irq(scrtc->crtc_id);
	if (ret < 0) {
		DRM_ERROR("sunxi_de_query_irq FAILED!\n");
		goto out;
	}

	ret = hw_funcs->event_proc(scrtc->crtc_id,
					atomic_read(&scrtc->update));
	if (ret < 0) {
		DRM_ERROR("sunxi_de_event_proc FAILED!\n");
		goto out;
	}
	atomic_set(&scrtc->update, 0);

out:
	drm_crtc_handle_vblank(&scrtc->crtc);
	sunxi_crtc_finish_page_flip(crtc->dev, scrtc);

	return IRQ_HANDLED;
}

static bool
sunxi_crtc_encoder_is_supported(struct sunxi_drm_crtc *scrtc,
						struct sunxi_drm_encoder *senc)
{
	struct sunxi_de_funcs *hw_funcs = scrtc->hw_funcs;

	return hw_funcs->is_support_tcon(scrtc->crtc_id, senc->encoder_id);
}

static char crtc_irq_name[30][4];
static int sunxi_drm_crtc_irq_register(struct sunxi_drm_crtc *scrtc,
				unsigned int irq_no)
{
	int ret;
	struct drm_crtc *crtc = &scrtc->crtc;

	sprintf(crtc_irq_name[scrtc->crtc_id], "sunxi-crtc%d", scrtc->crtc_id);
	DRM_INFO("irq name:%s, irq num:%d\n", crtc_irq_name[scrtc->crtc_id], irq_no);

	ret = devm_request_irq(crtc->dev->dev, irq_no,
		sunxi_crtc_event_proc, IRQF_TRIGGER_NONE,
		crtc_irq_name[scrtc->crtc_id], crtc);
	if (ret < 0) {
		DRM_ERROR("sunxi crtc request irq failed\n");
		return -1;
	}

	drm_crtc_vblank_on(crtc);

	return 0;
}

static void sunxi_drm_crtc_irq_unregister(struct sunxi_drm_crtc *scrtc,
				unsigned int irq_no)
{
	struct drm_crtc *crtc = &scrtc->crtc;
	devm_free_irq(crtc->dev->dev, irq_no, crtc);

	if (crtc->state->event && !crtc->state->active) {
		spin_lock_irq(&crtc->dev->event_lock);
		drm_crtc_send_vblank_event(crtc, crtc->state->event);
		spin_unlock_irq(&crtc->dev->event_lock);
		crtc->state->event = NULL;
	}
}

static void sunxi_crtc_destroy(struct drm_crtc *crtc)
{
	sunxi_drm_crtc_destroy(crtc);
}

static void sunxi_crtc_reset(struct drm_crtc *crtc)
{
	drm_atomic_helper_crtc_reset(crtc);
}

int sunxi_crtc_atomic_get_property(struct drm_crtc *crtc,
				   const struct drm_crtc_state *state,
				   struct drm_property *property,
				   uint64_t *val)
{
	struct sunxi_drm_crtc *scrtc = to_sunxi_crtc(crtc);

	if (scrtc->support_smbl == property) {
		*val = scrtc->hw_funcs->is_support_smbl(crtc->index);
		return 0;
	}

	return -1;
}

static void sunxi_crtc_atomic_enable(struct drm_crtc *crtc,
					struct drm_crtc_state *old_state)
{
	struct sunxi_drm_crtc *scrtc = to_sunxi_crtc(crtc);
	struct drm_crtc_state *new_state = crtc->state;
	struct sunxi_drm_connector *sconn;
	struct drm_mode_modeinfo modeinfo;
	int crtc_id, enc_id = 0, i;

	struct disp_manager_data config;
	struct disp_manager_info *info = &config.config;
	struct sunxi_de_funcs *hw_funcs = scrtc->hw_funcs;

#ifdef CONFIG_VIDEO_SUNXI_CAR_REVERSE
	if (sunxi_drm_get_force_plane_en())
		return;
#endif

	DRM_DEBUG_DRIVER("[SUNXI-CRTC]%s\n", __func__);
	if (scrtc->enabled) {
		DRM_INFO("[SUNXI-CRTC]Warn: crtc has been enable,"
			"do NOT enable again\n");
		return;
	}

	if ((!new_state->enable) || (!new_state->active)) {
		DRM_INFO("Warn: DRM do NOT want to enable or active crtc%d,"
			" so can NOT be enabled\n", scrtc->crtc_id);
		return;
	}

	memset(&config, 0, sizeof(config));
	crtc_id = drm_crtc_index(new_state->crtc);

	/* sunxi_crtc_state_dump(new_state); */

	drm_property_blob_get(new_state->mode_blob);
	memcpy(&modeinfo, new_state->mode_blob->data,
					new_state->mode_blob->length);
	drm_property_blob_put(new_state->mode_blob);

	config.flag = MANAGER_ALL_DIRTY;
	info->size.x = crtc->x;
	info->size.y = crtc->y;
	info->size.width = modeinfo.hdisplay;
	info->size.height = modeinfo.vdisplay;

	/* check which connector will be attached to this crtc */
	for (i = 0; i < sunxi_drm_connector_get_count(); i++) {
		if ((new_state->connector_mask >> i) & 0x1)
			break;
	}

	sconn = sunxi_drm_connector_get_connector(i);
	if (!sconn) {
		DRM_ERROR("Get sunxi connector:%d failed!\n", i);
		return;
	}

	for (i = 0; i < sunxi_drm_encoder_get_count(); i++) {
		if ((new_state->encoder_mask >> i) & 0x1) {
			enc_id = i;
			break;
		}
	}

	info->color_space = DISP_BT601_F;

	if (sconn->get_work_mode) {
		struct sunxi_connector_work_mode conn_work_mode;

		sconn->get_work_mode(sconn, &conn_work_mode);
		info->cs = conn_work_mode.color_fmt;
	} else {
		info->cs = DISP_CSC_TYPE_RGB;
	}

	if (info->cs == DISP_CSC_TYPE_RGB)
		info->color_range = DISP_COLOR_RANGE_0_255;
	else
		info->color_range = DISP_COLOR_RANGE_16_235;

	/* info->conn_type = sconn->type; */

	info->enable = true;
	info->disp_device = crtc_id;

	info->hwdev_index = enc_id;

	info->blank = false;
	info->de_freq = hw_funcs->get_freq(crtc_id);
	info->device_fps = modeinfo.vrefresh;
	info->eotf = DISP_EOTF_GAMMA22;
	info->data_bits = DISP_DATA_8BITS;

	/* DRM_INFO("%s de_freq:%u  fps:%u\n", __func__,
		info->de_freq, info->fps);
	 */

	if (hw_funcs->enable(crtc_id, &config) < 0)
		DRM_ERROR("de_al_mgr_apply FAILED\n");
	else
		DRM_INFO("%s success\n", __func__);

	if (hw_funcs->is_use_irq(crtc_id)) {
		if (scrtc->irq_register(scrtc,
				hw_funcs->get_irq_no(crtc_id)) < 0) {
			DRM_ERROR("sunxi_drm_crtc_irq_register failed\n");
			return;
		}
	}
	scrtc->enabled = true;
	drm_crtc_vblank_on(crtc);
}

static void sunxi_crtc_sw_enable(struct sunxi_drm_crtc *scrtc)
{
	int crtc_id;
	struct disp_manager_data config;
	struct drm_display_mode *mode;
	struct disp_manager_info *info = &config.config;
	struct drm_crtc *crtc = &scrtc->crtc;
	struct sunxi_drm_connector *sconn;
	struct drm_encoder *encoder;
	bool find = false;
	struct sunxi_de_funcs *hw_funcs = scrtc->hw_funcs;

	memset(&config, 0, sizeof(config));
	crtc_id = drm_crtc_index(crtc);

	/* In general, bootlogo is output through connector 0 */
	sconn = sunxi_drm_connector_get_connector(0);
	if (!sconn) {
		DRM_ERROR("Get sunxi connector 0 failed!\n");
		return;
	}

	if (!crtc->state) {
		DRM_ERROR("crtc:%d has no state\n", crtc_id);
		return;
	}
	mode = &crtc->state->mode;

	config.flag = MANAGER_ALL_DIRTY;
	info->size.x = crtc->x;
	info->size.y = crtc->y;
	info->size.width = mode->hdisplay;
	info->size.height = mode->vdisplay;

	info->color_space = DISP_BT601_F;

	if (sconn->get_work_mode) {
		struct sunxi_connector_work_mode conn_work_mode;

		sconn->get_work_mode(sconn, &conn_work_mode);
		info->cs = conn_work_mode.color_fmt;
	} else {
		info->cs = DISP_CSC_TYPE_RGB;
	}

	if (info->cs == DISP_CSC_TYPE_RGB)
		info->color_range = DISP_COLOR_RANGE_0_255;
	else
		info->color_range = DISP_COLOR_RANGE_16_235;

	/* info->conn_type = sconn->type; */

	info->enable = true;
	info->disp_device = crtc_id;

	drm_for_each_encoder(encoder, crtc->dev) {
		if (encoder->crtc == crtc) {
			find = true;
			break;
		}
	}

	if (!find) {
		DRM_ERROR("crtc:%d has NO attaching encoder!\n", crtc->index);
		return;
	}

	info->hwdev_index = encoder->index;

	info->blank = false;
	info->de_freq = hw_funcs->get_freq(crtc_id);
	/* info->device_fps = mode->vrefresh; */
	info->eotf = DISP_EOTF_GAMMA22;
	info->data_bits = DISP_DATA_8BITS;

	if (hw_funcs->enable(crtc_id, &config) < 0)
		DRM_ERROR("de_al_mgr_apply FAILED\n");
	else
		DRM_INFO("%s success\n", __func__);

	if (hw_funcs->is_use_irq(crtc_id)) {
		if (scrtc->irq_register(scrtc,
				hw_funcs->get_irq_no(crtc_id)) < 0) {
			DRM_ERROR("sunxi_drm_crtc_irq_register failed\n");
			return;
		}
	}
	scrtc->enabled = true;
}

static void sunxi_crtc_atomic_disable(struct drm_crtc *crtc,
					struct drm_crtc_state *old_state)

{
	struct sunxi_drm_crtc *scrtc = to_sunxi_crtc(crtc);
	struct drm_crtc_state *new_state = crtc->state;
	struct disp_manager_data config;
	struct disp_manager_info *info = &config.config;
	int crtc_id;
	struct sunxi_de_funcs *hw_funcs = scrtc->hw_funcs;

	DRM_INFO("[SUNXI-CRTC]%s\n", __func__);
	drm_crtc_vblank_off(crtc);
	if (!scrtc->enabled) {
		DRM_ERROR("%s: crtc has been disable\n", __func__);
		return;
	}

	memset(&config, 0, sizeof(config));
	crtc_id = drm_crtc_index(new_state->crtc);
	scrtc->enabled = false;

	if (hw_funcs->is_use_irq(crtc_id))
		scrtc->irq_unregister(scrtc, hw_funcs->get_irq_no(crtc_id));

	config.flag = MANAGER_ALL_DIRTY;
	info->enable = false;
	info->blank = true;

	hw_funcs->disable(crtc_id, &config);

	if (crtc->state->event && !crtc->state->active) {
		spin_lock_irq(&crtc->dev->event_lock);
		drm_crtc_send_vblank_event(crtc, crtc->state->event);
		spin_unlock_irq(&crtc->dev->event_lock);

		crtc->state->event = NULL;
	}

	return;
}

static void sunxi_crtc_atomic_begin(struct drm_crtc *crtc,
			     struct drm_crtc_state *old_crtc_state)
{
	struct sunxi_drm_crtc *scrtc = to_sunxi_crtc(crtc);
	struct drm_device *dev = crtc->dev;
	unsigned long flags;

	DRM_DEBUG_DRIVER("[SUNXI-CRTC]%s\n", __func__);

	atomic_set(&scrtc->update, 0);

	if (crtc->state->event) {
		WARN_ON(drm_crtc_vblank_get(crtc) != 0);

		spin_lock_irqsave(&dev->event_lock, flags);
		scrtc->event = crtc->state->event;
		spin_unlock_irqrestore(&dev->event_lock, flags);
		crtc->state->event = NULL;
	}

	return;
}

static void sunxi_crtc_atomic_flush(struct drm_crtc *crtc,
			     struct drm_crtc_state *old_crtc_state)
{
	struct sunxi_drm_crtc *scrtc = to_sunxi_crtc(crtc);
	struct drm_pending_vblank_event *event = crtc->state->event;

	DRM_DEBUG_DRIVER("[SUNXI-CRTC]%s\n", __func__);
	if (event) {
		crtc->state->event = NULL;

		spin_lock_irq(&crtc->dev->event_lock);
		if (drm_crtc_vblank_get(crtc) == 0)
			drm_crtc_arm_vblank_event(crtc, event);
		else
			drm_crtc_send_vblank_event(crtc, event);
		spin_unlock_irq(&crtc->dev->event_lock);
	}

	atomic_set(&scrtc->update, 1);

	return;
}

int sunxi_drm_crtc_enable_vblank(struct drm_crtc *crtc)
{
	return 0;
}

static const struct drm_crtc_funcs sunxi_crtc_funcs = {
	.set_config		= drm_atomic_helper_set_config,
	.page_flip		= drm_atomic_helper_page_flip,
	.destroy		= sunxi_crtc_destroy,
	.reset			= drm_atomic_helper_crtc_reset,
	.atomic_get_property	= sunxi_crtc_atomic_get_property,
	.atomic_duplicate_state	= drm_atomic_helper_crtc_duplicate_state,
	.atomic_destroy_state	= drm_atomic_helper_crtc_destroy_state,
	.enable_vblank	= sunxi_drm_crtc_enable_vblank,
};

static const struct drm_crtc_helper_funcs sunxi_crtc_helper_funcs = {
	.atomic_enable	= sunxi_crtc_atomic_enable,
	.atomic_disable	= sunxi_crtc_atomic_disable,
	/* .atomic_begin	= sunxi_crtc_atomic_begin, */
	.atomic_flush	= sunxi_crtc_atomic_flush,
};


/* init all of the crtcs */
int sunxi_drm_crtc_init(struct drm_device *dev)
{
	int i, j, ret, max_crtc, max_plane;

	int max_vi_layers;
	struct sunxi_de_funcs *hw_funcs;

	/* The max counts of crtc is decided by connector counts */
	max_crtc = sunxi_drm_get_connector_count();
	if (!max_crtc)
		max_crtc = 1; /* There must be at least one crtc */

	sunxi_crtc = kzalloc(
			max_crtc * sizeof(struct sunxi_drm_crtc), GFP_KERNEL);
	if (!sunxi_crtc) {
		DRM_ERROR("can NOT allocate memory for sunxi_crtc\n");
		goto crtc_err;
	}
	sunxi_crtc_cnt = max_crtc;

	for (i = 0; i < max_crtc; i++) {
		sunxi_crtc[i].crtc_id = i;
		mutex_init(&sunxi_crtc[i].update_reg_lock);

		hw_funcs = sunxi_de_get_funcs(i);
		sunxi_crtc[i].hw_funcs = hw_funcs;

/* init planes of crtc */
		max_plane = hw_funcs->get_layer_count(i);
		sunxi_crtc[i].plane_num = max_plane;
		sunxi_crtc[i].plane = kzalloc(max_plane
				* sizeof(struct sunxi_drm_plane),
							GFP_KERNEL);
		if (!sunxi_crtc[i].plane) {
			DRM_ERROR("can NOT allocate mem for planes of "
						"crtc:%d\n", i);
			goto crtc_err;
		}

		max_vi_layers = hw_funcs->get_vi_layer_count(i);
		sunxi_crtc[i].overlay_plane_num = max_vi_layers;

		for (j = 0; j < max_plane; j++) {
			/* check if it is the first ui layer */
			if ((j - max_vi_layers) == 0) {
				ret = sunxi_drm_plane_init(dev,
					&sunxi_crtc[i].plane[j], i, j,
					DRM_PLANE_TYPE_PRIMARY);
			} else {
				ret = sunxi_drm_plane_init(dev,
					&sunxi_crtc[i].plane[j],
					i, j, DRM_PLANE_TYPE_OVERLAY);
			}

			if (ret < 0) {
				DRM_ERROR("sunxi crtc:%d plane:%d init failed",
					i, j);
				goto crtc_err;
			}
			DRM_DEBUG_DRIVER("INIT crtc:%d plane:%d success\n", i, j);
		}

/* init curtain crtc with a default primary plane */
		ret = drm_crtc_init_with_planes(dev, &sunxi_crtc[i].crtc,
				&sunxi_crtc[i].plane[max_vi_layers].plane,
				NULL, &sunxi_crtc_funcs, "sunxi-crtc%d", i);
		if (ret < 0) {
			DRM_ERROR("drm_crtc_init_with_planes failed\n");
			goto crtc_err;
		}

		drm_crtc_helper_add(&sunxi_crtc[i].crtc,
				&sunxi_crtc_helper_funcs);

		sunxi_crtc_reset(&sunxi_crtc[i].crtc);

		sunxi_crtc[i].encoder_is_supported = sunxi_crtc_encoder_is_supported;
		sunxi_crtc[i].is_in_use = sunxi_drm_crtc_in_use;
		sunxi_crtc[i].sw_enable = sunxi_crtc_sw_enable;
		sunxi_crtc[i].irq_register = sunxi_drm_crtc_irq_register;
		sunxi_crtc[i].irq_unregister = sunxi_drm_crtc_irq_unregister;

		sunxi_crtc[i].support_smbl =
			drm_property_create_bool(dev, DRM_MODE_PROP_IMMUTABLE,
					"support_smbl");
		if (!sunxi_crtc[i].support_smbl)
			return -ENOMEM;

		DRM_DEBUG_DRIVER("INIT crtc:%d success\n", i);
	}

	return 0;

crtc_err:

	if (sunxi_crtc) {
		for (i = 0; i < sunxi_crtc_cnt; i++) {
			drm_crtc_cleanup(&sunxi_crtc[i].crtc);

			max_plane = sunxi_crtc[i].plane_num;
			for (j = 0; j < max_plane; j++)
				drm_plane_cleanup(&sunxi_crtc[i].plane[j].plane);
			kfree(sunxi_crtc[i].plane);
		}
		kfree(sunxi_crtc);
	}
	DRM_ERROR("crtc init failed\n");
	return -1;
}

/* destroy all of the crtcs */
void sunxi_drm_crtc_exit(struct drm_device *dev)
{
	int i, j, max_plane;

	if (sunxi_crtc) {
		for (i = 0; i < sunxi_crtc_cnt; i++) {
			if (!sunxi_crtc)
				break;
			drm_crtc_cleanup(&sunxi_crtc[i].crtc);

			max_plane = sunxi_crtc[i].plane_num;
			for (j = 0; j < max_plane; j++)
				drm_plane_cleanup(&sunxi_crtc[i].plane[j].plane);

			kfree(sunxi_crtc[i].plane);
			sunxi_crtc[i].plane = NULL;
		}

		kfree(sunxi_crtc);
		sunxi_crtc = NULL;
	}
}

/* destroy curtain crtc */
void sunxi_drm_crtc_destroy(struct drm_crtc *drm_crtc)
{
	int j, max_plane;
	struct sunxi_drm_crtc *crtc = to_sunxi_crtc(drm_crtc);

	drm_crtc_cleanup(drm_crtc);

	max_plane = crtc->plane_num;
	for (j = 0; j < max_plane; j++)
		drm_plane_cleanup(&crtc->plane[j].plane);

	kfree(crtc->plane);
	crtc->plane = NULL;
}
