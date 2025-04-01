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

#ifndef _SUNXI_DRM_CRTC_H_
#define _SUNXI_DRM_CRTC_H_

#include <drm/drm_crtc.h>
#include "drm/sunxi_drm.h"

#include "sunxi_drm_encoder.h"
#include "de/include.h"
#include "sunxi_device/sunxi_de.h"

enum enhance_mode {
	ENHANCE_STANDARD = 0,
	ENHANCE_VIVID = 1,
	ENHANCE_SOFT = 2,
	ENHANCE_DEMO_VIVID = 3,
	ENHANCE_MAX,
};

struct sunxi_drm_crtc {
	struct drm_crtc crtc;

	unsigned int crtc_id;

	bool enabled;

	/* NOTE: this is a ARRAY(!!!) of plane */
	struct sunxi_drm_plane *plane;

	/* overlay plane: in sunxi,
	it means all of the planes except primary plane */
	unsigned int overlay_plane_num;

	/* primary plane + overlayer planes */
	unsigned int plane_num;

	/* the vblank event that is going to be sent */
	struct drm_pending_vblank_event *event;
	/* if there is plane that needs to be update in next vsync */
	atomic_t update;
	struct mutex update_reg_lock;

	struct drm_property *support_smbl;

	struct sunxi_de_funcs *hw_funcs;

	/* There are common funcs that all kinds of encoders must implement.
	  *
	  * These funcs can be called by sunxi_drm_plane/crtc/encoder/connector.
	  *
	  * But these funcs can NOT be called by DRM CORE
	  * and sunxi hw layer, like sunxi_tv/lcd/hdmi.
	  */
	bool (*is_in_use)(struct sunxi_drm_crtc *scrtc);
	void (*sw_enable)(struct sunxi_drm_crtc *scrtc);
	int (*irq_register)(struct sunxi_drm_crtc *scrtc,
					unsigned int irq_no);
	void (*irq_unregister)(struct sunxi_drm_crtc *scrtc,
					unsigned int irq_no);
	bool (*encoder_is_supported)(struct sunxi_drm_crtc *scrtc,
		struct sunxi_drm_encoder *senc);
};

#define to_sunxi_crtc(x)  container_of(x, struct sunxi_drm_crtc, crtc)

ssize_t sunxi_drm_crtc_show(char *buf, struct drm_device *dev);
struct sunxi_drm_crtc *sunxi_drm_crtc_get_crtc(int id);
unsigned int sunxi_drm_get_crtc_count(void);

int sunxi_drm_crtc_set_enhance_ioctl(struct drm_device *dev,
		void *data, struct drm_file *file_priv);
int sunxi_drm_crtc_get_enhance_ioctl(struct drm_device *dev,
		void *data, struct drm_file *file_priv);
int sunxi_drm_crtc_set_smbl_ioctl(struct drm_device *dev,
		void *data, struct drm_file *file_priv);
int sunxi_drm_crtc_get_smbl_ioctl(struct drm_device *dev,
		void *data, struct drm_file *file_priv);

int sunxi_drm_crtc_init(struct drm_device *dev);
void sunxi_drm_crtc_exit(struct drm_device *dev);

#endif
