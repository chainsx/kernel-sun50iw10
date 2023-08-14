/*
 * Copyright (c) 2007-2019 Allwinnertech Co., Ltd.
 * Author: zhengwanyu <zhengwanyu@allwinnertech.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef _SUNXI_HDMI_H_
#define _SUNXI_HDMI_H_
#include<video/sunxi_display2.h>

#include "hdmi_core/api/core_api.h"

struct sunxi_hdmi_work_mode {
	unsigned int hdmi_mode; /* 0:DVI 1:HDMI */
	unsigned int color_fmt;
	unsigned int color_depth;
	unsigned int color_space;
	unsigned int eotf;
	unsigned int color_range;
	unsigned int aspect_ratio;
};

struct sunxi_mode_related_info {
	/* name of struct drm_display_mode */
	char mode_type[20];
	unsigned char mode_index;
	bool native;

	unsigned int rgb[4];
	unsigned int rgb_count;

	unsigned int yuv444[4];
	unsigned int yuv444_count;

	unsigned int yuv422[4];
	unsigned int yuv422_count;

	unsigned int yuv420[4];
	unsigned int yuv420_count;

	unsigned int reserved0;
	unsigned int reserved1;
	unsigned int reserved2;
	unsigned int reserved3;
	unsigned int reserved4;
};

struct hdr_static_metadata_db {
	unsigned char et_n;
	unsigned char sm_n;

	/* Desired Content Max Luminance data */
	unsigned char dc_max_lum_data;

	/* Desired Content Max Frame-average Luminance data */
	unsigned char dc_max_fa_lum_data;

	/* Desired Content Min Luminance data */
	unsigned char dc_min_lum_data;
};

struct sunxi_mode_independent_info {
	bool bad_edid;

	unsigned int hdmi_mode;

/* VSDB */
	/* for cec */
	unsigned short phyaddr;

/* HDMI2.0 HF_VSDB */
	/* bool scramble_340mcs; */

/* hdr */
	struct hdr_static_metadata_db hdr_static_metadata;

};

struct sunxi_hdmi_funcs {
	/* get gotplug status */
	int (*get_connect_status)(void);
	struct sunxi_hdmi_work_mode *(*get_init_params)(void);

	sink_edid_t *(*get_sink_caps)(void);

	/* set hdmi working mode */
	int (*set_working_mode)(struct sunxi_hdmi_work_mode *work_mode);
	/* get hdmi working mode */
	void (*get_working_mode)(struct sunxi_hdmi_work_mode *work_mode);

	/* read edid */
	int (*get_edid_block)(void *data, unsigned char *buf,
			unsigned int block, size_t len);

	/* correct the params parsed from edid */
	/* void (*mode_related_params_correct)(
		struct sunxi_mode_related_info *info); */
	void (*mode_independent_params_correct)(
		struct sunxi_mode_independent_info *info);

	int (*enable)(struct disp_video_timings *timing);
	int (*sw_enable)(struct disp_video_timings *timing);
	void (*disable)(void);
};

struct sunxi_hdmi_funcs *sunxi_hdmi_get_funcs(void);

#endif
