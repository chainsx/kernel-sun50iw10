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

#ifndef _SUNXI_DRM_ENCODER_H_
#define _SUNXI_DRM_ENCODER_H_

#include <drm/drm_crtc.h>
#include <drm/drm_encoder.h>
#include "sunxi_drm_connector.h"
#include "sunxi_device/sunxi_tcon.h"

struct sunxi_drm_encoder {
	struct drm_encoder encoder;
	unsigned int encoder_id;

	bool use_irq;
	unsigned int irq_no;

	struct sunxi_tcon_funcs *hw_funcs;

	/* There are common funcs that all kinds of encoders must implement.
	  *
	  * These funcs can be called by sunxi_drm_plane/crtc/encoder/connector.
	  *
	  * But these funcs can NOT be called by DRM CORE
	  * and sunxi hw layer, like sunxi_tv/lcd/hdmi.
	  */
	bool (*is_in_use)(struct sunxi_drm_encoder *senc);
	void (*sw_enable)(struct sunxi_drm_encoder *senc);
	bool (*conn_is_supported)(struct sunxi_drm_encoder *senc,
		struct sunxi_drm_connector *sconn);
};

#define to_sunxi_encoder(x) (container_of(x, struct sunxi_drm_encoder, encoder))

unsigned int sunxi_drm_encoder_get_count(void);
ssize_t sunxi_drm_encoder_show(char *buf, struct drm_device *dev);

int sunxi_drm_encoder_init(struct drm_device *dev);
void sunxi_drm_encoder_exit(struct drm_device *dev);
#endif
