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

#ifndef _SUNXI_DRM_CONNECTOR_H_
#define _SUNXI_DRM_CONNECTOR_H_

#include <drm/drm_modes.h>
#include <drm/drm_connector.h>
#include <video/sunxi_display2.h>
#include "sunxi_device/sunxi_common.h"

#define MAX_CONNECTOR_COUNT 10
#define DRM_DEBUG 0
struct sunxi_drm_connector {
	struct drm_connector connector;

	/* index of connectors */
	unsigned int con_id;

	unsigned int type;

	/* index of a certain type of connector
	 * for example, if type = lcd, type_id = 2,
	 * it means that this connector is lcd2
	 */
	unsigned int type_id;

	bool use_irq;
	unsigned int irq_no;

	/* mainly used by tv/hdmi/edp */
	struct sunxi_connector_work_mode work_mode;

	void *hw_funcs;

	/* There are common funcs that all kinds of connectors must implement.
	  *
	  * These funcs can be called by sunxi_drm_plane/crtc/encoder/connector.
	  *
	  * But these funcs can NOT be called by DRM CORE
	  * and sunxi hw layer, like sunxi_tv/lcd/hdmi.
	  */
	void (*get_init_resolution)(
		struct sunxi_drm_connector *sconn,
		unsigned int *w, unsigned int *h, unsigned int *vfresh);

	struct drm_display_mode *(*get_init_mode)(
		struct sunxi_drm_connector *sconn);

	void (*get_work_mode)(struct sunxi_drm_connector *sconn,
				struct sunxi_connector_work_mode *mode);

	int (*get_video_timing)(struct disp_video_timings *timing,
					struct drm_display_mode *mode);

	int (*enable)(struct sunxi_drm_connector *sconn,
					struct drm_display_mode *mode);
	int (*sw_enable)(struct sunxi_drm_connector *sconn,
					struct drm_display_mode *mode);
	void (*disable)(struct sunxi_drm_connector *sconn);
};

#if defined(CONFIG_AW_DRM_HDMI14) || defined(CONFIG_AW_DRM_HDMI20)
#if defined(CONFIG_AW_DRM_HDMI14)
#include "sunxi_device/sunxi_hdmi14.h"
#elif defined(CONFIG_AW_DRM_HDMI20)
#include "sunxi_device/sunxi_hdmi20.h"
#endif

struct sunxi_drm_hdmi_connector {
	struct sunxi_drm_connector sunxi_conn;

	/* indicate hdmi current working mode */
	struct sunxi_hdmi_work_mode work_mode;

	struct drm_property *hdmi_mode_property;
	struct drm_property *color_fmt_property;
	struct drm_property *color_depth_property;
	struct drm_property *color_space_property;
	struct drm_property *eotf_property;
	struct drm_property *color_range_property;
	struct drm_property *aspect_ratio_property;
};
#endif

#define to_sunxi_connector(x) \
	(container_of(x, struct sunxi_drm_connector, connector))

extern void drm_get_displayid(struct drm_connector *connector,
			      struct edid *edid);


/* These funcs bellows will be called by sunxi_drm_plane/crtc/encoder */
unsigned int sunxi_drm_connector_get_count(void);
ssize_t sunxi_drm_connector_show(char *buf, struct drm_device *dev);
struct sunxi_drm_connector *
sunxi_drm_connector_get_connector(int id);
void
sunxi_drm_connector_set_connector(int id, struct sunxi_drm_connector *conn);

int sunxi_drm_connector_init(struct drm_device *dev);
void sunxi_drm_connector_exit(struct drm_device *dev);


/* These funcs bellows will be called by sunxi_drm_lcd/tv/hdmi_connector */
struct drm_encoder *
sunxi_connector_best_encoder(struct drm_connector *connector);


/* These funcs bellows will be called by sunxi_drm_connector */
struct sunxi_drm_connector *
sunxi_drm_connector_lcd_create(struct drm_device *dev, int conn_id, int lcd_id);
struct sunxi_drm_connector *
sunxi_drm_connector_tv_create(struct drm_device *dev, int conn_id, int tv_id);
struct sunxi_drm_connector *
sunxi_drm_connector_hdmi_create(struct drm_device *dev, int conn_id);

unsigned int
sunxi_drm_connector_hdmi_get_color_format(struct drm_connector *connector);

extern unsigned int drm_debug;
extern bool drm_edid_block_valid(u8 *raw_edid, int block,
		bool print_bad_edid, bool *edid_corrupt);
#endif
