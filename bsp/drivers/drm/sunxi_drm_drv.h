/* sunxi_drm_drv.h
 *
 * Copyright (C) 2022 Allwinnertech Co., Ltd.
 * Authors: zhengwanyu <zhengwanyu@allwinnertech.com>
 * Authors: hongyaobin <hongyaobin@allwinnertech.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef _SUNXI_DRM_DRV_H_
#define _SUNXI_DRM_DRV_H_

#include <linux/of.h>
#include <drm/drm_crtc.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_atomic_helper.h>
#include "drm/sunxi_drm.h"

#include "sunxi_drm_bootlogo.h"

#include "sunxi_drm_connector.h"

#ifdef CONFIG_DRM_KMS_CMA_HELPER
#include <drm/drm_fb_cma_helper.h>
#endif

#include "sunxi_device/sunxi_common.h"

#include "sunxi_device/sunxi_de.h"
#include "sunxi_device/sunxi_tcon.h"
#include "sunxi_device/sunxi_lcd.h"

#ifdef CONFIG_AW_DRM_TV
#include "sunxi_device/sunxi_tv.h"
#endif

#if defined(CONFIG_AW_DRM_HDMI14)
#include "sunxi_device/sunxi_hdmi14.h"
#elif defined(CONFIG_AW_DRM_HDMI20)
#include "sunxi_device/sunxi_hdmi20.h"
#endif

struct sunxi_drm_private {
	struct platform_device *pdev;
	struct drm_device *drm_dev;
	unsigned int fb_base;
	char boot_fb[50];/* used by bootGUI for bootlogo */

	unsigned char connector_num;
	unsigned int connector_type[MAX_CONNECTOR_COUNT];
	char connector_mode[MAX_CONNECTOR_COUNT][20];

	/* NOTE: Only surpport single display and dual display */
	unsigned int connector0_type;
	char *connector0_mode;

	unsigned int connector1_type;
	char *connector1_mode;

#ifdef CONFIG_DRM_FBDEV_EMULATION
	bool boot_disp;
	struct bmp_header bootlogo_bmp_header;
	struct bootlogo_info bootlogo;
#endif
	void *mapping;
};

#ifdef CONFIG_VIDEO_SUNXI_CAR_REVERSE
int sunxi_drm_get_force_plane_en(void);
int sunxi_drm_get_num_crtc(void);
int sunxi_drm_force_set_plane(int crtc_id,
		struct disp_layer_config *config,
		unsigned int plane_num);
void sunxi_drm_force_set_plane_exit(int crtc_id);
#endif

#ifdef CONFIG_DRM_FBDEV_EMULATION
bool sunxi_drm_is_need_smooth_boot(void);
int sunxi_drm_copy_bootlogo(unsigned char *d_addr, unsigned int w,
			unsigned int h, struct bootlogo_info *bootlogo);

#endif

unsigned int sunxi_drm_get_init_bpp(void);
unsigned int sunxi_drm_get_init_width(void);
unsigned int sunxi_drm_get_init_height(void);

unsigned int sunxi_drm_get_connector_type(int i);
char *sunxi_drm_get_connector_mode(int i);
unsigned int sunxi_drm_get_connector_count(void);

struct drm_device *sunxi_drm_get_drm_device(void);

#endif
