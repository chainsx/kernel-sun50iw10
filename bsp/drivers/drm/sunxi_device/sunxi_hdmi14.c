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
#include "sunxi_hdmi14.h"
static struct drv_model_info *hdmi_drv;
static struct sunxi_hdmi *hwhdmi;

#define HDMI_PIN_STATE_ACTIVE "active"
#define HDMI_PIN_STATE_SLEEP "sleep"

struct disp_video_timings hdmi_video_timing[] = {
	{
			.vic = HDMI1440_480I,
			.tv_mode = DISP_TV_MOD_480I,
			.pixel_clk = 13500000,
			.pixel_repeat = 1,
			.x_res = 720,
			.y_res = 480,
			.hor_total_time = 858,
			.hor_back_porch = 57,
			.hor_front_porch = 19,
			.hor_sync_time = 62,
			.ver_total_time = 525,
			.ver_back_porch = 15,
			.ver_front_porch = 4,
			.ver_sync_time = 3,
			.hor_sync_polarity = 0,
			.ver_sync_polarity = 0,
			.b_interlace = 1,
			.vactive_space = 0,
			.trd_mode = 0,

		},
		{
			.vic = HDMI1440_576I,
			.tv_mode = DISP_TV_MOD_576I,
			.pixel_clk = 27000000,
			.pixel_repeat = 0,
			.x_res = 720,
			.y_res = 480,
			.hor_total_time = 858,
			.hor_back_porch = 60,
			.hor_front_porch = 16,
			.hor_sync_time = 62,
			.ver_total_time = 525,
			.ver_back_porch = 30,
			.ver_front_porch = 9,
			.ver_sync_time = 6,
			.hor_sync_polarity = 0,
			.ver_sync_polarity = 0,
			.b_interlace = 0,
			.vactive_space = 0,
			.trd_mode = 0,
		},
		{
			.vic = HDMI480P,
			.tv_mode = DISP_TV_MOD_480P,
			.pixel_clk = 27000000,
			.pixel_repeat = 0,
			.x_res = 720,
			.y_res = 480,
			.hor_total_time = 858,
			.hor_back_porch = 60,
			.hor_front_porch = 16,
			.hor_sync_time = 62,
			.ver_total_time = 525,
			.ver_back_porch = 30,
			.ver_front_porch = 9,
			.ver_sync_time = 6,
			.hor_sync_polarity = 0,
			.ver_sync_polarity = 0,
			.b_interlace = 0,
			.vactive_space = 0,
			.trd_mode = 0,
		},
		{
			.vic = HDMI576P,
			.tv_mode = DISP_TV_MOD_576P,
			.pixel_clk = 27000000,
			.pixel_repeat = 0,
			.x_res = 720,
			.y_res = 576,
			.hor_total_time = 864,
			.hor_back_porch = 68,
			.hor_front_porch = 12,
			.hor_sync_time = 64,
			.ver_total_time = 625,
			.ver_back_porch = 39,
			.ver_front_porch = 5,
			.ver_sync_time = 5,
			.hor_sync_polarity = 0,
			.ver_sync_polarity = 0,
			.b_interlace = 0,
			.vactive_space = 0,
			.trd_mode = 0,
		},
		{
			.vic = HDMI720P_50,
			.tv_mode = DISP_TV_MOD_720P_50HZ,
			.pixel_clk = 74250000,
			.pixel_repeat = 0,
			.x_res = 1280,
			.y_res = 720,
			.hor_total_time = 1980,
			.hor_back_porch = 220,
			.hor_front_porch = 440,
			.hor_sync_time = 40,
			.ver_total_time = 750,
			.ver_back_porch = 20,
			.ver_front_porch = 5,
			.ver_sync_time = 5,
			.hor_sync_polarity = 1,
			.ver_sync_polarity = 1,
			.b_interlace = 0,
			.vactive_space = 0,
			.trd_mode = 0,
		},
		{
			.vic = HDMI720P_60,
			.tv_mode = DISP_TV_MOD_720P_60HZ,
			.pixel_clk = 74250000,
			.pixel_repeat = 0,
			.x_res = 1280,
			.y_res = 720,
			.hor_total_time = 1650,
			.hor_back_porch = 220,
			.hor_front_porch = 110,
			.hor_sync_time = 40,
			.ver_total_time = 750,
			.ver_back_porch = 20,
			.ver_front_porch = 5,
			.ver_sync_time = 5,
			.hor_sync_polarity = 1,
			.ver_sync_polarity = 1,
			.b_interlace = 0,
			.vactive_space = 0,
			.trd_mode = 0,
		},
		{
			.vic = HDMI1080I_50,
			.tv_mode = DISP_TV_MOD_1080I_50HZ,
			.pixel_clk = 74250000,
			.pixel_repeat = 0,
			.x_res = 1920,
			.y_res = 1080,
			.hor_total_time = 2640,
			.hor_back_porch = 148,
			.hor_front_porch = 528,
			.hor_sync_time = 44,
			.ver_total_time = 1125,
			.ver_back_porch = 15,
			.ver_front_porch = 2,
			.ver_sync_time = 5,
			.hor_sync_polarity = 1,
			.ver_sync_polarity = 1,
			.b_interlace = 1,
			.vactive_space = 0,
			.trd_mode = 0,
		},
		{
			.vic = HDMI1080I_60,
			.tv_mode = 0,
			.pixel_clk = 74250000,
			.pixel_repeat = DISP_TV_MOD_1080I_60HZ,
			.x_res = 1920,
			.y_res = 1080,
			.hor_total_time = 2200,
			.hor_back_porch = 148,
			.hor_front_porch = 88,
			.hor_sync_time = 44,
			.ver_total_time = 1125,
			.ver_back_porch = 15,
			.ver_front_porch = 2,
			.ver_sync_time = 5,
			.hor_sync_polarity = 1,
			.ver_sync_polarity = 1,
			.b_interlace = 1,
			.vactive_space = 0,
			.trd_mode = 0,
		},
		{
			.vic = HDMI1080P_50,
			.tv_mode = DISP_TV_MOD_1080P_50HZ,
			.pixel_clk = 148500000,
			.pixel_repeat = 0,
			.x_res = 1920,
			.y_res = 1080,
			.hor_total_time = 2640,
			.hor_back_porch = 148,
			.hor_front_porch = 528,
			.hor_sync_time = 44,
			.ver_total_time = 1125,
			.ver_back_porch = 36,
			.ver_front_porch = 4,
			.ver_sync_time = 5,
			.hor_sync_polarity = 1,
			.ver_sync_polarity = 1,
			.b_interlace = 0,
			.vactive_space = 0,
			.trd_mode = 0,
		},
		{
			.vic = HDMI1080P_60,
			.tv_mode = DISP_TV_MOD_1080P_60HZ,
			.pixel_clk = 148500000,
			.pixel_repeat = 0,
			.x_res = 1920,
			.y_res = 1080,
			.hor_total_time = 2200,
			.hor_back_porch = 148,
			.hor_front_porch = 88,
			.hor_sync_time = 44,
			.ver_total_time = 1125,
			.ver_back_porch = 36,
			.ver_front_porch = 4,
			.ver_sync_time = 5,
			.hor_sync_polarity = 1,
			.ver_sync_polarity = 1,
			.b_interlace = 0,
			.vactive_space = 0,
			.trd_mode = 0,
		},
		{
			.vic = HDMI1080P_24,
			.tv_mode = DISP_TV_MOD_1080P_24HZ,
			.pixel_clk = 74250000,
			.pixel_repeat = 0,
			.x_res = 1920,
			.y_res = 1080,
			.hor_total_time = 2750,
			.hor_back_porch = 148,
			.hor_front_porch = 638,
			.hor_sync_time = 44,
			.ver_total_time = 1125,
			.ver_back_porch = 36,
			.ver_front_porch = 4,
			.ver_sync_time = 5,
			.hor_sync_polarity = 1,
			.ver_sync_polarity = 1,
			.b_interlace = 0,
			.vactive_space = 0,
			.trd_mode = 0,
		},
		{
			.vic = HDMI1080P_25,
			.tv_mode = DISP_TV_MOD_1080P_25HZ,
			.pixel_clk = 74250000,
			.pixel_repeat = 0,
			.x_res = 1920,
			.y_res = 1080,
			.hor_total_time = 2640,
			.hor_back_porch = 148,
			.hor_front_porch = 528,
			.hor_sync_time = 44,
			.ver_total_time = 1125,
			.ver_back_porch = 36,
			.ver_front_porch = 4,
			.ver_sync_time = 5,
			.hor_sync_polarity = 0,
			.ver_sync_polarity = 0,
			.b_interlace = 0,
			.vactive_space = 0,
			.trd_mode = 0,
		},
		{
			.vic = HDMI1080P_30,
			.tv_mode = DISP_TV_MOD_1080P_30HZ,
			.pixel_clk = 74250000,
			.pixel_repeat = 0,
			.x_res = 1920,
			.y_res = 1080,
			.hor_total_time = 2200,
			.hor_back_porch = 148,
			.hor_front_porch = 88,
			.hor_sync_time = 44,
			.ver_total_time = 1125,
			.ver_back_porch = 36,
			.ver_front_porch = 4,
			.ver_sync_time = 5,
			.hor_sync_polarity = 0,
			.ver_sync_polarity = 0,
			.b_interlace = 0,
			.vactive_space = 0,
			.trd_mode = 0,
		},
		{
			.vic = HDMI1080P_24_3D_FP,
			.tv_mode = DISP_TV_MOD_1080P_24HZ_3D_FP,
			.pixel_clk = 148500000,
			.pixel_repeat = 0,
			.x_res = 1920,
			.y_res = 2160,
			.hor_total_time = 2750,
			.hor_back_porch = 148,
			.hor_front_porch = 638,
			.hor_sync_time = 44,
			.ver_total_time = 1125,
			.ver_back_porch = 36,
			.ver_front_porch = 4,
			.ver_sync_time = 5,
			.hor_sync_polarity = 1,
			.ver_sync_polarity = 1,
			.b_interlace = 0,
			.vactive_space = 45,
			.trd_mode = 1,
		},
		{
			.vic = HDMI720P_50_3D_FP,
			.tv_mode = DISP_TV_MOD_720P_50HZ_3D_FP,
			.pixel_clk = 148500000,
			.pixel_repeat = 0,
			.x_res = 1280,
			.y_res = 1440,
			.hor_total_time = 1980,
			.hor_back_porch = 220,
			.hor_front_porch = 440,
			.hor_sync_time = 40,
			.ver_total_time = 750,
			.ver_back_porch = 20,
			.ver_front_porch = 5,
			.ver_sync_time = 5,
			.hor_sync_polarity = 1,
			.ver_sync_polarity = 1,
			.b_interlace = 0,
			.vactive_space = 30,
			.trd_mode = 1,
		},
		{
			.vic = HDMI720P_60_3D_FP,
			.tv_mode = DISP_TV_MOD_720P_60HZ_3D_FP,
			.pixel_clk = 148500000,
			.pixel_repeat = 0,
			.x_res = 1280,
			.y_res = 1440,
			.hor_total_time = 1650,
			.hor_back_porch = 220,
			.hor_front_porch = 110,
			.hor_sync_time = 40,
			.ver_total_time = 750,
			.ver_back_porch = 20,
			.ver_front_porch = 5,
			.ver_sync_time = 5,
			.hor_sync_polarity = 1,
			.ver_sync_polarity = 1,
			.b_interlace = 0,
			.vactive_space = 30,
			.trd_mode = 1,
		},
		{
			.vic = HDMI3840_2160P_30,
			.tv_mode = DISP_TV_MOD_3840_2160P_30HZ,
			.pixel_clk = 297000000,
			.pixel_repeat = 0,
			.x_res = 3840,
			.y_res = 2160,
			.hor_total_time = 4400,
			.hor_back_porch = 296,
			.hor_front_porch = 176,
			.hor_sync_time = 88,
			.ver_total_time = 2250,
			.ver_back_porch = 72,
			.ver_front_porch = 8,
			.ver_sync_time = 10,
			.hor_sync_polarity = 1,
			.ver_sync_polarity = 1,
			.b_interlace = 0,
			.vactive_space = 0,
			.trd_mode = 0,
		},
		{
			.vic = HDMI3840_2160P_25,
			.tv_mode = DISP_TV_MOD_3840_2160P_25HZ,
			.pixel_clk = 297000000,
			.pixel_repeat = 0,
			.x_res = 3840,
			.y_res = 2160,
			.hor_total_time = 5280,
			.hor_back_porch = 296,
			.hor_front_porch = 1056,
			.hor_sync_time = 88,
			.ver_total_time = 2250,
			.ver_back_porch = 72,
			.ver_front_porch = 8,
			.ver_sync_time = 10,
			.hor_sync_polarity = 1,
			.ver_sync_polarity = 1,
			.b_interlace = 0,
			.vactive_space = 0,
			.trd_mode = 0,
		},
		{
			.vic = HDMI3840_2160P_24,
			.tv_mode = DISP_TV_MOD_3840_2160P_24HZ,
			.pixel_clk = 297000000,
			.pixel_repeat = 0,
			.x_res = 3840,
			.y_res = 2160,
			.hor_total_time = 5500,
			.hor_back_porch = 296,
			.hor_front_porch = 1276,
			.hor_sync_time = 88,
			.ver_total_time = 2250,
			.ver_back_porch = 72,
			.ver_front_porch = 8,
			.ver_sync_time = 10,
			.hor_sync_polarity = 1,
			.ver_sync_polarity = 1,
			.b_interlace = 0,
			.vactive_space = 0,
			.trd_mode = 0,
		},
		{
			.vic = HDMI4096_2160P_24,
			.tv_mode = DISP_TV_MOD_4096_2160P_24HZ,
			.pixel_clk = 297000000,
			.pixel_repeat = 0,
			.x_res = 4096,
			.y_res = 2160,
			.hor_total_time = 5500,
			.hor_back_porch = 296,
			.hor_front_porch = 1020,
			.hor_sync_time = 88,
			.ver_total_time = 2250,
			.ver_back_porch = 72,
			.ver_front_porch = 8,
			.ver_sync_time = 10,
			.hor_sync_polarity = 1,
			.ver_sync_polarity = 1,
			.b_interlace = 0,
			.vactive_space = 0,
			.trd_mode = 0,
		},
};

static struct sunxi_hdmi *sunxi_hdmi_get_hdmi(void)
{
	if (hwhdmi)
		return hwhdmi;
	return NULL;
}

struct sunxi_hdmi_funcs *sunxi_hdmi_get_funcs(void)
{
	struct sunxi_hdmi *hdmi = sunxi_hdmi_get_hdmi();

	return (struct sunxi_hdmi_funcs *)hdmi->funcs;
}

static unsigned int hdmi_get_soc_version(void)
{
	unsigned int version = 0;
#if defined(CONFIG_ARCH_SUN8IW7)
#if defined(SUN8IW7P1_REV_A) || defined(SUN8IW7P2_REV_B)
	unsigned int chip_ver = sunxi_get_soc_ver();

	switch (chip_ver) {
	case SUN8IW7P1_REV_A:
	case SUN8IW7P2_REV_A:
		version = 0;
		break;
	case SUN8IW7P1_REV_B:
	case SUN8IW7P2_REV_B:
		version = 1;
	}
#else
	version = 1;
#endif /* endif  SUN8IW7P1_REV_A */
#endif
	return version;
}


static int sunxi_hdmi_get_timing_list_num(struct sunxi_hdmi *hdmi)
{
	int ret = sizeof(hdmi_video_timing)
		/ sizeof(struct disp_video_timings);

	return ret;
}

static int
sunxi_hdmi_get_video_mode_info(struct disp_video_timings **video_info,
			       enum disp_tv_mode tv_mode)
{
	int i;
	struct sunxi_hdmi *hdmi = sunxi_hdmi_get_hdmi();
	int num = sunxi_hdmi_get_timing_list_num(hdmi);

	for (i = 0; i < num; i++) {
		if (hdmi_video_timing[i].tv_mode == tv_mode)
			break;
	}

	if (i >= num) {
		HDMI_INFO("can NOT get timing info of tv_mode:%d\n", tv_mode);
		*video_info = NULL;
		return -1;
	}

	*video_info = &hdmi_video_timing[i];

	return 0;
}

unsigned int hdmi_clk_get_div(void)
{
	unsigned long rate = 1, rate_parent = 1;
	unsigned int div = 4;
	struct sunxi_hdmi *hdmi = sunxi_hdmi_get_hdmi();

	if (hdmi->mclk)
		rate = clk_get_rate(hdmi->mclk);
	if (hdmi->mclk_parent)
		rate_parent = clk_get_rate(hdmi->mclk_parent);

	if (rate != 0)
		div = rate_parent / rate;
	else
		HDMI_ERR("hdmi clk rate is ZERO!\n");

	return div;
}

/* get actually hdmi reg address */
static u32 sunxi_hdmi_reg_mapping(u32 reg)
{
	int i = 0;
	u32 reg_map = 0;
	unsigned int offset[16] = {1, 3, 5, 7, 9, 11, 13, 15, 14, 12, 10, 8, 6, 4, 2, 0};

	for (i = 0; i < 16; i++)
		reg_map |= ((((reg >> offset[i]) & 0x1)) << (15 - i));

	return reg_map;
}

static void sunxi_hdmi_write(u32 reg, u8 value)
{
	struct sunxi_hdmi *hdmi = sunxi_hdmi_get_hdmi();
	unsigned long addr = (unsigned long)hdmi->reg_base;
	u32 actual_reg;

	actual_reg = sunxi_hdmi_reg_mapping(reg);
	addr += actual_reg;

	*((volatile unsigned char *)(hdmi->reg_base + 0x10010)) = 0x45;
	*((volatile unsigned char *)(hdmi->reg_base + 0x10011)) = 0x45;
	*((volatile unsigned char *)(hdmi->reg_base + 0x10012)) = 0x52;
	*((volatile unsigned char *)(hdmi->reg_base + 0x10013)) = 0x54;

	*((volatile unsigned char *)(addr)) = value;

	*((volatile unsigned char *)(hdmi->reg_base + 0x10010)) = 0x52;
	*((volatile unsigned char *)(hdmi->reg_base + 0x10011)) = 0x54;
	*((volatile unsigned char *)(hdmi->reg_base + 0x10012)) = 0x41;
	*((volatile unsigned char *)(hdmi->reg_base + 0x10013)) = 0x57;
}

static u8 sunxi_hdmi_read(u32 reg)
{
	struct sunxi_hdmi *hdmi = sunxi_hdmi_get_hdmi();
	unsigned long addr = (unsigned long)hdmi->reg_base;
	u32 actual_reg;
	unsigned char val;

	actual_reg = sunxi_hdmi_reg_mapping(reg);

	addr += actual_reg;

	*((volatile unsigned char *)(hdmi->reg_base + 0x10010)) = 0x45;
	*((volatile unsigned char *)(hdmi->reg_base + 0x10011)) = 0x45;
	*((volatile unsigned char *)(hdmi->reg_base + 0x10012)) = 0x52;
	*((volatile unsigned char *)(hdmi->reg_base + 0x10013)) = 0x54;

	val = *((volatile unsigned char *)(addr));

	*((volatile unsigned char *)(hdmi->reg_base + 0x10010)) = 0x52;
	*((volatile unsigned char *)(hdmi->reg_base + 0x10011)) = 0x54;
	*((volatile unsigned char *)(hdmi->reg_base + 0x10012)) = 0x41;
	*((volatile unsigned char *)(hdmi->reg_base + 0x10013)) = 0x57;

	return val;
}


static void
sunxi_hdmi_timing_convert_to_video_para(struct disp_video_timings *timing,
					struct video_para *para)
{
	para->vic = timing->vic;
	para->pixel_clk = timing->pixel_clk;
	para->clk_div = hdmi_clk_get_div();
	para->pixel_repeat = timing->pixel_repeat;
	para->x_res = timing->x_res;
	para->y_res = timing->y_res;
	para->hor_total_time = timing->hor_total_time;
	para->hor_back_porch = timing->hor_back_porch;
	para->hor_front_porch = timing->hor_front_porch;
	para->hor_sync_time = timing->hor_sync_time;
	para->ver_total_time = timing->ver_total_time;
	para->ver_back_porch = timing->ver_back_porch;
	para->ver_front_porch = timing->ver_front_porch;
	para->ver_sync_time = timing->ver_sync_time;
	para->hor_sync_polarity = timing->hor_sync_polarity;
	para->ver_sync_polarity = timing->ver_sync_polarity;
	para->b_interlace = timing->b_interlace;
}

void sunxi_dump_video_para(struct video_para *para)
{
	HDMI_INFO("HDMI VIDEO PARA:\n");
	HDMI_INFO("vic:%u  csc:%u is_hdmi:%u is_yuv:%u is_hcts:%d\n",
		para->vic, para->csc, para->is_hdmi, para->is_yuv, para->is_hcts);
	HDMI_INFO("pixel_clk:%u clk_div:%u pixel_repeat:%u\n",
		para->pixel_clk, para->clk_div, para->pixel_repeat);
	HDMI_INFO("x-y:%ux%u "
		"htotal:%u hor_back_porch:%u hor_front_porch:%u hsync:%u "
		"vtotal:%u ver_back_porch:%u ver_front_porch:%u vsync:%u "
		"hpol:%u vpol:%u  interlace:%u\n",
		para->x_res, para->y_res,
		para->hor_total_time, para->hor_back_porch, para->hor_front_porch, para->hor_sync_time,
		para->ver_total_time, para->ver_back_porch, para->ver_front_porch, para->ver_sync_time,
		para->hor_sync_polarity, para->ver_sync_polarity, para->b_interlace);
}

static bool hdmi_clk_enable;
int sunxi_hdmi_clk_enable(struct sunxi_hdmi *hdmi)
{
	int ret;

	if (hdmi_clk_enable)
		return 0;
	hdmi_clk_enable = true;

	ret = clk_prepare_enable(hdmi->mclk);
	if (ret < 0) {
		HDMI_ERR("fail to enable hdmi mclk\n");
		return -1;
	}

	ret = clk_prepare_enable(hdmi->ddc_clk);
	if (ret < 0) {
		HDMI_ERR("fail to enable hdmi ddc clk\n");
		return -1;
	}

#if defined(CONFIG_ARCH_SUN8IW12)
	ret = clk_prepare_enable(hdmi->cec_clk);
	if (ret < 0) {
		HDMI_ERR("fail to enable hdmi cec clk\n");
		return -1;
	}
#endif

	return 0;
}

static int sunxi_hdmi_clk_disable(struct sunxi_hdmi *hdmi)
{
	if (!hdmi_clk_enable)
		return 0;
	hdmi_clk_enable = false;

	if (__clk_get_enable_count(hdmi->mclk))
		clk_disable_unprepare(hdmi->mclk);

	if (__clk_get_enable_count(hdmi->ddc_clk))
		clk_disable_unprepare(hdmi->ddc_clk);

#if defined(CONFIG_ARCH_SUN8IW12)
	if (__clk_get_enable_count(hdmi->cec_clk))
		clk_disable_unprepare(hdmi->cec_clk);
#endif

	return 0;
}

static int sunxi_hdmi_get_connect_status(void)
{
	int status, time_out = 5;

	/* read hdmi connected status with anti-shake check */
	/* the connected status should remain unchanged for 20*5 ms */
	status = bsp_hdmi_get_hpd();
	while (time_out) {
		msleep(20);
		if (status == bsp_hdmi_get_hpd())
			--time_out;
		else {
			time_out = 5;
			status = bsp_hdmi_get_hpd();
		}
	}

	return status;
}

static int sunxi_hdmi_get_support_tv_mode(unsigned int pixel_clk,
				       unsigned int hdisplay,
				       unsigned int vdisplay,
				       unsigned int htotal,
				       unsigned int vtotal,
				bool is_interlace, bool is_3d)
{
	int i;
	struct disp_video_timings *timing = hdmi_video_timing;
	struct sunxi_hdmi *hdmi = sunxi_hdmi_get_hdmi();
	int num = sunxi_hdmi_get_timing_list_num(hdmi);

	DRM_DEBUG_DRIVER("pixel_clk:%u  %ux%u  total:%ux%u interlace:%d 3d:%d\n",
			pixel_clk, hdisplay, vdisplay, htotal, vtotal, is_interlace, is_3d);

	for (i = 0; i < num; i++) {
		if (timing[i].pixel_clk * (1 + timing[i].pixel_repeat)
			/(1 + timing[i].b_interlace) == pixel_clk * (1 + is_3d)
			&& timing[i].x_res == hdisplay
			&& timing[i].y_res == (vdisplay * (1 + is_3d))
			&& timing[i].hor_total_time == htotal
			&& timing[i].ver_total_time == vtotal
			&& timing[i].b_interlace == is_interlace
			&& timing[i].trd_mode == is_3d)
			break;
	}

	if (i >= num) {
		DRM_DEBUG_DRIVER("WARN:hdmi_source NOT support:%ux%u%c@%u %s\n",
			hdisplay, vdisplay, is_interlace ? 'I' : 'P',
			pixel_clk / (vtotal * htotal),
			is_3d ? "(3D)" : "(NOT 3D)");
		return -1;
	}

	return timing[i].tv_mode;
}

struct disp_video_timings *
sunxi_hdmi_get_timing_from_tv_mode(unsigned int tv_mode)
{
	struct disp_video_timings *timing;

	sunxi_hdmi_get_video_mode_info(&timing, tv_mode);

	return timing;
}

static int sunxi_hdmi_get_resolution_from_tv_mode(
					   unsigned int tv_mode,
					   unsigned int *hdisplay,
					   unsigned int *vdisplay,
					   unsigned int *vrefresh,
					   bool *is_interlace, bool *is_3d)
{
	int i;
	struct disp_video_timings *timing = hdmi_video_timing;
	struct sunxi_hdmi *hdmi = sunxi_hdmi_get_hdmi();
	int num = sunxi_hdmi_get_timing_list_num(hdmi);

	for (i = 0; i < num; i++)
		if (timing[i].tv_mode == tv_mode)
			break;

	if (i >= num) {
		HDMI_INFO("WARN:NOT support tv_mode:%u\n", tv_mode);
		return -1;
	}

	if (hdisplay)
		*hdisplay = timing[i].x_res;
	if (vdisplay)
		*vdisplay = timing[i].y_res;
	if (vrefresh)
		*vrefresh = (timing[i].pixel_clk * (timing[i].pixel_repeat + 1))
					/ (timing[i].b_interlace + 1)
					/ (timing[i].hor_total_time * timing[i].ver_total_time);
	if (is_interlace)
		*is_interlace = timing[i].b_interlace;
	if (is_3d)
		*is_3d = timing[i].trd_mode;

	return 0;
}

void
sunxi_hdmi_get_working_mode(struct sunxi_hdmi_work_mode *work_mode)
{
	struct sunxi_hdmi *hdmi = sunxi_hdmi_get_hdmi();

	work_mode->hdmi_mode = hdmi->video.is_hdmi ?
		 MODE_HDMI : MODE_DVI;

	work_mode->color_fmt = hdmi->video.is_yuv ?
		 COLOR_FMT_YUV444 : COLOR_FMT_RGB444;

	work_mode->color_depth = 8;
}

static int
sunxi_hdmi_set_working_mode(struct sunxi_hdmi_work_mode *work_mode)
{
	struct sunxi_hdmi *hdmi = sunxi_hdmi_get_hdmi();

	if (work_mode->color_fmt == COLOR_FMT_RGB444)
		hdmi->video.is_yuv = 0;
	else if (work_mode->color_fmt == COLOR_FMT_YUV444)
		hdmi->video.is_yuv = 1;
	else {
		HDMI_ERR("invalid color format:%d\n", work_mode->color_fmt);
		return -1;
	}

	if (work_mode->hdmi_mode == DISP_DVI)
		hdmi->video.is_hdmi = 0;
	else if (work_mode->hdmi_mode == DISP_HDMI)
		hdmi->video.is_hdmi = 1;
	else {
		HDMI_ERR("invalid hdmi mode:%d\n", work_mode->hdmi_mode);
		return -1;
	}

	return 0;
}

static int sunxi_hdmi_get_edid_block(void *data, unsigned char *buf,
		unsigned int block, size_t len)
{
	int ret = 0;
	unsigned char offset = (block & 0x01) ? 128:0;

	ret = bsp_hdmi_ddc_read(Explicit_Offset_Address_E_DDC_Read,
		((unsigned char)block) >> 1, offset, len, buf);

	/* dump hdmi raw data */
	/* for (i = 0; i < len; i++) {
		if (((i % 16) == 0) && i)
			printk("\n");
		printk("0x%02x ", buf[i]);
	} */

	return ret;
}

/* static void sunxi_hdmi_mode_related_params_correct(
		struct sunxi_mode_related_info *info)
{
	int i;
	struct color_format_depth *fmt_depth = info->fmt_depth;

	for (i = 0; i < 16; i++) {
		if (((fmt_depth[i].format != COLOR_FMT_RGB444)
			&& (fmt_depth[i].format != COLOR_FMT_YUV444))
			|| (fmt_depth[i].depth != SUNXI_COLOR_DEPTH_8)) {
			fmt_depth[i].format = 0;
			fmt_depth[i].depth = 0;
		}
	}
} */

static void sunxi_hdmi_mode_independent_params_correct(
		struct sunxi_drm_display_info *info)
{
	info->bpc = 8;
	info->support_yuv422 = false;
	info->support_yuv420 = false;

	/* 10/12/16bits */
	info->support_deep_color30 = false;
	info->support_deep_color36 = false;
	info->support_deep_color48 = false;

	/* HDMI2.0 HF_VSDB */
	info->scramble_340mcs = false;

	/* hdr */
	memset(&info->hdr_static_metadata, 0,
		sizeof(struct hdr_static_metadata_data_block));
}

static int sunxi_hdmi_enable(int tv_mode)
{
	int ret;
	struct disp_video_timings *timing;
	struct sunxi_hdmi *hdmi = sunxi_hdmi_get_hdmi();
	struct audio_para *audio_para = &hdmi->audio;
	struct video_para *video_para = &hdmi->video;

	ret = sunxi_hdmi_get_video_mode_info(&timing, tv_mode);
	if (ret < 0) {
		HDMI_ERR("tv mode:%d has NO coresponsible timing\n", tv_mode);
		return ret;
	}
	hdmi->video_info = timing;

	clk_set_rate(hdmi->mclk, timing->pixel_clk);

	bsp_hdmi_hrst();
	bsp_hdmi_standby();

	if (video_para->is_hcts)
		bsp_hdmi_hrst();

	sunxi_hdmi_timing_convert_to_video_para(timing, video_para);

	/* sunxi_dump_video_para(video_para); */
	ret = bsp_hdmi_video(video_para);
	if (ret) {
		HDMI_ERR("set hdmi video error!");
		return -1;
	}

	bsp_hdmi_set_video_en(1);

	if (video_para->is_hdmi) {
		ret = bsp_hdmi_audio(audio_para);
		if (ret) {
			HDMI_ERR("set hdmi audio error!\n");
			return -1;
		}
	}

	return 0;
}

static int sunxi_hdmi_sw_enable(int tv_mode)
{
	int ret;
	struct disp_video_timings *timing;
	struct sunxi_hdmi *hdmi = sunxi_hdmi_get_hdmi();
	/* struct audio_para *audio_para = &hdmi->audio; */
	struct video_para *video_para = &hdmi->video;

	ret = sunxi_hdmi_get_video_mode_info(&timing, tv_mode);
	if (ret < 0) {
		HDMI_ERR("tv mode:%d has NO coresponsible timing\n", tv_mode);
		return ret;
	}
	hdmi->video_info = timing;
	video_para->vic = timing->vic;

	clk_set_rate(hdmi->mclk, timing->pixel_clk);

	return 0;
}

static void sunxi_hdmi_disable(void)
{
	/* struct sunxi_hdmi *hdmi = sunxi_hdmi_get_hdmi(); */

	bsp_hdmi_set_video_en(0);
	/* sunxi_hdmi_clk_disable(); */
}

static unsigned int reg_read_start, reg_read_end, reg_read_cmd;
static ssize_t sunxi_hdmi_read_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;
	unsigned int reg;

	if (!reg_read_cmd) {
		n += sprintf(buf + n, "read hdmi register, Usage:\n");
		n += sprintf(buf + n, "echo [reg_offset_start] [reg_offset_end] > read\n");
		n += sprintf(buf + n, "OR:echo [reg_offset_start],[reg_offset_end] > read\n");
		return n;
	}

	for (reg = reg_read_start; reg <= reg_read_end; reg++) {
		if (reg % 16 == 0) {
			n += sprintf(buf + n, "\n");
			n += sprintf(buf + n, "0x%x: ", reg);
		}

		n += sprintf(buf + n, "0x%02x ", sunxi_hdmi_read(reg));
	}

	n += sprintf(buf + n, "\n");

	reg_read_cmd = 0;

	return n;
}

static ssize_t sunxi_hdmi_read_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	char *end;

	reg_read_start = (unsigned int)simple_strtoull(buf, &end, 0);

	if ((*end != ' ') && (*end != ',')) {
		HDMI_ERR("error separator:%c\n", *end);
		return count;
	}

	reg_read_end = (unsigned int)simple_strtoull(end + 1, &end, 0);

	reg_read_cmd = 1;

	return count;
}

static DEVICE_ATTR(read, 0660,
		sunxi_hdmi_read_show, sunxi_hdmi_read_store);

static ssize_t sunxi_hdmi_write_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	char *end;
	unsigned int reg_addr;
	unsigned char value;

	reg_addr = (unsigned int)simple_strtoull(buf, &end, 0);

	if ((*end != ' ') && (*end != ',')) {
		HDMI_ERR("error separator:%c\n", *end);
		return count;
	}

	value = (unsigned char)simple_strtoull(end + 1, &end, 0);

	sunxi_hdmi_write(reg_addr, value);

	return count;
}

static DEVICE_ATTR(write, 0660, NULL, sunxi_hdmi_write_store);



static ssize_t sunxi_hdmi_source_para_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;
	struct sunxi_hdmi *hdmi = sunxi_hdmi_get_hdmi();
	struct video_para *video = &hdmi->video;
	struct audio_para *audio = &hdmi->audio;

	n += sprintf(buf + n, "HDMI VIDEO PARA:\n");
	n += sprintf(buf + n, "vic:%u  csc:%u is_hdmi:%u is_yuv:%u is_hcts:%d\n",
		video->vic, video->csc, video->is_hdmi, video->is_yuv, video->is_hcts);
	n += sprintf(buf + n, "pixel_clk:%u clk_div:%u pixel_repeat:%u\n",
		video->pixel_clk, video->clk_div, video->pixel_repeat);
	n += sprintf(buf + n, "x-y:%ux%u "
		"htotal:%u hor_back_porch:%u hor_front_porch:%u hsync:%u "
		"vtotal:%u ver_back_porch:%u ver_front_porch:%u vsync:%u "
		"hpol:%u vpol:%u  interlace:%u\n",
		video->x_res, video->y_res,
		video->hor_total_time, video->hor_back_porch, video->hor_front_porch, video->hor_sync_time,
		video->ver_total_time, video->ver_back_porch, video->ver_front_porch, video->ver_sync_time,
		video->hor_sync_polarity, video->ver_sync_polarity, video->b_interlace);

	n += sprintf(buf + n, "HDMI AUDIO PARA:\n");
	n += sprintf(buf + n, "type:%u  chanel allocation:%u sample_rate:%u "
		"sample_bit:%u  chanel_num:%u vic:%u\n",
		audio->type, audio->ca, audio->sample_rate,
		audio->sample_bit, audio->ch_num,
		audio->vic);

	return n;
}

static DEVICE_ATTR(source_para, 0660,
		sunxi_hdmi_source_para_show, NULL);


static ssize_t sunxi_hdmi_hdcp_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;
	struct sunxi_hdmi *hdmi = sunxi_hdmi_get_hdmi();

	n += sprintf(buf + n, "%d", hdmi->video.is_hcts);

	return n;
}

static ssize_t sunxi_hdmi_hdcp_enable_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct sunxi_hdmi *hdmi = sunxi_hdmi_get_hdmi();

	if (count < 1)
		return -EINVAL;

	if (strncasecmp(buf, "on", 2) == 0 || strncasecmp(buf, "1", 1) == 0)
		hdmi->video.is_hcts = 1;
	else if (strncasecmp(buf, "off", 3) == 0 ||
			strncasecmp(buf, "0", 1) == 0)
		hdmi->video.is_hcts = 0;
	else
		return -EINVAL;

	return count;
}

static DEVICE_ATTR(hdcp_enable, 0660,
		sunxi_hdmi_hdcp_enable_show, sunxi_hdmi_hdcp_enable_store);

static struct attribute *hdmi_attributes[] = {
	&dev_attr_hdcp_enable.attr,
	&dev_attr_source_para.attr,
	&dev_attr_read.attr,
	&dev_attr_write.attr,
	NULL
};

static const struct sunxi_hdmi_funcs hdmi_funcs = {
	.get_connect_status = sunxi_hdmi_get_connect_status,
	.get_support_tv_mode = sunxi_hdmi_get_support_tv_mode,
	.get_timing_from_tv_mode = sunxi_hdmi_get_timing_from_tv_mode,
	.get_resolution_from_tv_mode = sunxi_hdmi_get_resolution_from_tv_mode,
	.set_working_mode = sunxi_hdmi_set_working_mode,
	.get_edid_block = sunxi_hdmi_get_edid_block,
	/* .mode_related_params_correct = sunxi_hdmi_mode_related_params_correct, */
	.mode_independent_params_correct = sunxi_hdmi_mode_independent_params_correct,
	.enable = sunxi_hdmi_enable,
	.sw_enable = sunxi_hdmi_sw_enable,
	.disable = sunxi_hdmi_disable,
};

static void sunxi_hdmi_para_init(struct sunxi_hdmi *hdmi)
{
	struct audio_para *audio = &hdmi->audio;
	struct video_para *video = &hdmi->video;

	video->vic = HDMI720P_50;
	video->csc = BT601;
	video->is_hdmi = 1;
	video->is_yuv = 0;
	video->is_hcts = 0;
	audio->type = 1; /* default pcm */
	audio->sample_rate = 44100;
	audio->sample_bit = 16;
	audio->ch_num = 2;
	audio->ca = 0;
}

static int sunxi_hdmi_clk_init(struct platform_device *pdev,
				struct sunxi_hdmi *hdmi)
{
	int ret;

	/* get hdmi main clk */
	hdmi->mclk = of_clk_get(pdev->dev.of_node, 0);
	if (IS_ERR(hdmi->mclk)) {
		HDMI_ERR("fail to get clk for hdmi\n");
		return -1;
	}

	hdmi->mclk_parent = clk_get_parent(hdmi->mclk);
	if (IS_ERR(hdmi->mclk_parent)) {
		HDMI_ERR("fail to get clk parent for hdmi\n");
		return -1;
	}

	if (__clk_get_enable_count(hdmi->mclk) == 0) {
		ret = clk_prepare_enable(hdmi->mclk);
		if (ret < 0) {
			HDMI_ERR("fail to enable hdmi mclk\n");
			return -1;
		}
	}

	/* get ddc clk for hdmi ddc function like edid reading
	 * and hdcp authentication
	 */
	hdmi->ddc_clk = of_clk_get(pdev->dev.of_node, 1);
	if (IS_ERR(hdmi->ddc_clk)) {
		HDMI_ERR("fail to get clk for hdmi ddc\n");
		return -1;
	}

	if (__clk_get_enable_count(hdmi->ddc_clk) == 0) {
		ret = clk_prepare_enable(hdmi->ddc_clk);
		if (ret < 0) {
			HDMI_ERR("fail to enable hdmi ddc clk\n");
			return -1;
		}
	}

#if defined(CONFIG_ARCH_SUN8IW12)
	/* get cec clk for hdmi cec function */
	hdmi->cec_clk = of_clk_get(pdev->dev.of_node, 2);
	if (IS_ERR_OR_NULL(hdmi->cec_clk)) {
		HDMI_ERR("fail to get hdmi_cec_clk\n");
		goto HDMI_FREE;
	}

	if (__clk_get_enable_count(hdmi->cec_clk) == 0) {
		ret = clk_prepare_enable(hdmi->cec_clk);
		if (ret < 0) {
			HDMI_ERR("fail to enable hdmi cec clk\n");
			return -1;
		}
	}
#endif
	hdmi_clk_enable = true;

	return 0;
}

static int sunxi_hdmi_clk_exit(struct sunxi_hdmi *hdmi)
{
	return sunxi_hdmi_clk_disable(hdmi);
}

static void hdmi_delay_us(unsigned long us)
{
	udelay(us);
}

static void hdmi_delay_ms(unsigned long ms)
{
	mdelay(ms);
}

static int sunxi_hdmi_core_init(struct sunxi_hdmi *hdmi)
{
	hdmi_bsp_func func;

	sunxi_hdmi_para_init(hdmi);

	bsp_hdmi_set_version(hdmi_get_soc_version());

	func.delay_us = hdmi_delay_us;
	func.delay_ms = hdmi_delay_ms;
	bsp_hdmi_set_func(&func);

	sunxi_hdmi_get_video_mode_info(&hdmi->video_info,
				DISP_TV_MOD_720P_50HZ);
#if defined(HDMI_USING_INNER_BIAS)
	bsp_hdmi_set_bias_source(HDMI_USING_INNER_BIAS);
#endif
	bsp_hdmi_init();

	hdmi->funcs = &hdmi_funcs;
	return 0;
}

static int sunxi_hdmi_probe(struct platform_device *pdev)
{
	int ret = -1;
	struct sunxi_hdmi *hdmi = NULL;

	HDMI_INFO(" start\n");
	hdmi = kmalloc(sizeof(*hdmi), GFP_KERNEL | __GFP_ZERO);
	if (!hdmi) {
		HDMI_ERR("Malloc sunxi_hdmi fail!\n");
		goto OUT;
	}

	hwhdmi = hdmi;

	hdmi->pdev = pdev;

	/* iomap */
	hdmi->reg_base = of_iomap(pdev->dev.of_node, 0);
	if (hdmi->reg_base == 0) {
		HDMI_ERR("unable to map hdmi registers\n");
		ret = -EINVAL;
		goto FREE_HDMI;
	}
	bsp_hdmi_set_addr((unsigned long)hdmi->reg_base);

	ret = sunxi_hdmi_clk_init(pdev, hdmi);
	if (ret < 0) {
		HDMI_ERR("sunxi_hdmi_clk_init failed\n");
		goto err_iomap;
	}

	/* set hdmi pin like ddc pin to active state */
	sunxi_drm_sys_pin_set_state("hdmi", HDMI_PIN_STATE_ACTIVE);

	/* get hdmi power and enable it */
	if (of_property_read_string(pdev->dev.of_node, "hdmi_power",
				    &hdmi->power))
		HDMI_ERR("of_property_read_string hdmi_power failed!\n");
	else
		sunxi_drm_sys_power_enable(NULL, hdmi->power); /* fix me */

	sunxi_hdmi_core_init(hdmi);

	return 0;

err_iomap:
	if (hdmi->reg_base)
		iounmap((char __iomem *)hdmi->reg_base);
FREE_HDMI:
	kfree(hdmi);
OUT:
	return ret;
}

static int sunxi_hdmi_remove(struct platform_device *pdev)
{
	struct sunxi_hdmi *hdmi = hwhdmi;

	if (!hdmi) {
		HDMI_ERR("Null pointer!\n");
		return -1;
	}

	sunxi_hdmi_clk_exit(hdmi);
	sunxi_drm_sys_power_disable(NULL, hdmi->power); /* fix me */
	iounmap((char __iomem *)hdmi->reg_base);
	kfree(hdmi);
	return 0;
}

static const struct of_device_id sunxi_hdmi_match[] = {
	{ .compatible = "allwinner,sunxi-hdmi", },

	{},
};

struct platform_driver sunxi_hdmi_platform_driver = {
	.probe = sunxi_hdmi_probe,
	.remove = sunxi_hdmi_remove,
	.driver = {
		   .name = "hdmi",
		   .owner = THIS_MODULE,
		   .of_match_table = sunxi_hdmi_match,
	},
};

static struct attribute_group hdmi_attribute_group = {
	.name = "attr",
	.attrs = hdmi_attributes,
};

int __init sunxi_hdmi_module_init(void)
{
	int ret = -1;

	HDMI_INFO(" start\n");
	hdmi_drv = kmalloc(sizeof(*hdmi_drv),
				GFP_KERNEL | __GFP_ZERO);
	if (!hdmi_drv) {
		HDMI_ERR("Null drv_model_info pointer\n");
		goto OUT;
	}
	ret = alloc_chrdev_region(&hdmi_drv->devid, 0, 1, "hdmi");
	if (ret < 0) {
		HDMI_ERR("alloc_chrdev_region failed\n");
		goto FREE_DRV;
	}

	hdmi_drv->cdev = cdev_alloc();
	if (!hdmi_drv->cdev) {
		HDMI_ERR("cdev_alloc failed\n");
		goto FREE_DRV;
	}

	cdev_init(hdmi_drv->cdev, NULL);
	hdmi_drv->cdev->owner = THIS_MODULE;
	ret = cdev_add(hdmi_drv->cdev, hdmi_drv->devid, 1);
	if (ret) {
		HDMI_ERR("cdev_add major number:%d failed\n",
		       MAJOR(hdmi_drv->devid));
		goto FREE_DRV;
	}

	hdmi_drv->sysclass = class_create(THIS_MODULE, "hdmi");
	if (IS_ERR(hdmi_drv->sysclass)) {
		HDMI_ERR("create class error\n");
		goto FREE_DRV;
	}

	hdmi_drv->dev = device_create(hdmi_drv->sysclass, NULL,
			hdmi_drv->devid, NULL, "hdmi");
	if (!hdmi_drv->dev) {
		HDMI_ERR("device_create failed\n");
		goto FREE_DRV;
	}

	ret = platform_driver_register(&sunxi_hdmi_platform_driver);
	if (ret) {
		HDMI_ERR("platform_driver_register failed\n");
		goto FREE_DEVICE;
	}

	ret = sysfs_create_group(&hdmi_drv->dev->kobj, &hdmi_attribute_group);
	if (ret < 0) {
		HDMI_ERR("sysfs_create_file fail!\n");
		goto UNREGISTER;
	}


	HDMI_INFO(" end\n");
	return ret;

UNREGISTER:
	platform_driver_unregister(&sunxi_hdmi_platform_driver);
FREE_DEVICE:
	device_destroy(hdmi_drv->sysclass, hdmi_drv->devid);
FREE_DRV:
	kfree(hdmi_drv);
OUT:
	HDMI_ERR(" failed\n");
	return -EINVAL;
}

void __exit sunxi_hdmi_module_exit(void)
{
	HDMI_INFO("\n");
	if (hdmi_drv) {
		platform_driver_unregister(&sunxi_hdmi_platform_driver);

		device_destroy(hdmi_drv->sysclass, hdmi_drv->devid);
		class_destroy(hdmi_drv->sysclass);

		cdev_del(hdmi_drv->cdev);
		kfree(hdmi_drv);
	}
}
