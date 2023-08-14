/*
 * drivers/gpu/drm/sunxi/sunxi_device/sunxi_tv/sunxi_tv.c
 *
 * Copyright (c) 2007-2019 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 * Author: zhengwanyu <zhengwanyu@allwinnertech.com>
 *
 * sunxi tvout driver include cvbs out, YPbPr and VGA out
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

#include <linux/dma-mapping.h>
#include <drm/drmP.h>
#include <linux/clk.h>
#include "sunxi_common.h"
#include "sunxi_tcon.h"
#include "sunxi_tv.h"

static struct drv_model_info *p_tv_drv;
static struct sunxi_tv *hwtv[MAX_TV_COUNT];
static unsigned int tv_count;

/* video timing definition start */
static struct disp_video_timings cvbs_timing[] = {
	{
		.vic = 0,
		.tv_mode = DISP_TV_MOD_NTSC,
		.pixel_clk = 216000000,
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
		.hor_sync_polarity = 0,/* 0: negative, 1: positive */
		.ver_sync_polarity = 0,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,

	},
	{
		.vic = 0,
		.tv_mode = DISP_TV_MOD_PAL,
		.pixel_clk = 216000000,
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
		.hor_sync_polarity = 0,/* 0: negative, 1: positive */
		.ver_sync_polarity = 0,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,
	},
};

static struct disp_video_timings vga_timing[] = {
	{
		.vic = 0,
		.tv_mode = DISP_VGA_MOD_1600_900P_60,
		.pixel_clk = 108000000,
		.pixel_repeat = 0,
		.x_res = 1600,
		.y_res = 900,
		.hor_total_time = 1800,
		.hor_back_porch = 96,
		.hor_front_porch = 24,
		.hor_sync_time = 80,
		.ver_total_time = 1000,
		.ver_back_porch = 96,
		.ver_front_porch = 1,
		.ver_sync_time = 3,
		.hor_sync_polarity = 1,/* 0: negative, 1: positive */
		.ver_sync_polarity = 1,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_VGA_MOD_1440_900P_60,
		.pixel_clk = 89000000,
		.pixel_repeat = 0,
		.x_res = 1440,
		.y_res = 900,
		.hor_total_time = 1600,
		.hor_back_porch = 80,
		.hor_front_porch = 48,
		.hor_sync_time = 32,
		.ver_total_time = 926,
		.ver_back_porch = 17,
		.ver_front_porch = 3,
		.ver_sync_time = 6,
		.hor_sync_polarity = 1,/* 0: negative, 1: positive */
		.ver_sync_polarity = 0,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_VGA_MOD_1366_768P_60,
		.pixel_clk = 85800000,
		.pixel_repeat = 0,
		.x_res = 1366,
		.y_res = 768,
		.hor_total_time = 1792,
		.hor_back_porch = 213,
		.hor_front_porch = 70,
		.hor_sync_time = 143,
		.ver_total_time = 798,
		.ver_back_porch = 24,
		.ver_front_porch = 3,
		.ver_sync_time = 3,
		.hor_sync_polarity = 1,/* 0: negative, 1: positive */
		.ver_sync_polarity = 1,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_VGA_MOD_1280_800P_60,
		.pixel_clk = 83500000,
		.pixel_repeat = 0,
		.x_res = 1280,
		.y_res = 800,
		.hor_total_time = 1680,
		.hor_back_porch = 200,
		.hor_front_porch = 72,
		.hor_sync_time = 128,
		.ver_total_time = 831,
		.ver_back_porch = 22,
		.ver_front_porch = 3,
		.ver_sync_time = 6,
		.hor_sync_polarity = 0,/* 0: negative, 1: positive */
		.ver_sync_polarity = 1,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_VGA_MOD_1024_768P_60,
		.pixel_clk = 65000000,
		.pixel_repeat = 0,
		.x_res = 1024,
		.y_res = 768,
		.hor_total_time = 1344,
		.hor_back_porch = 160,
		.hor_front_porch = 24,
		.hor_sync_time = 136,
		.ver_total_time = 806,
		.ver_back_porch = 29,
		.ver_front_porch = 3,
		.ver_sync_time = 6,
		.hor_sync_polarity = 0,/* 0: negative, 1: positive */
		.ver_sync_polarity = 0,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_VGA_MOD_800_600P_60,
		.pixel_clk = 40000000,
		.pixel_repeat = 0,
		.x_res = 800,
		.y_res = 600,
		.hor_total_time = 1056,
		.hor_back_porch = 88,
		.hor_front_porch = 40,
		.hor_sync_time = 128,
		.ver_total_time = 628,
		.ver_back_porch = 23,
		.ver_front_porch = 1,
		.ver_sync_time = 4,
		.hor_sync_polarity = 1,/* 0: negative, 1: positive */
		.ver_sync_polarity = 1,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_VGA_MOD_1280_720P_60,
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
		.hor_sync_polarity = 1,/* 0: negative, 1: positive */
		.ver_sync_polarity = 1,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_VGA_MOD_1920_1080P_60,
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
		.hor_sync_polarity = 1,/* 0: negative, 1: positive */
		.ver_sync_polarity = 1,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,
	},
};

static struct disp_video_timings ypbpr_timing[] = {
	{
		.vic = 0,
		.tv_mode = DISP_TV_MOD_NTSC,
		.pixel_clk = 216000000,
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
		.hor_sync_polarity = 0,/* 0: negative, 1: positive */
		.ver_sync_polarity = 0,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,

	},
	{
		.vic = 0,
		.tv_mode = DISP_TV_MOD_PAL,
		.pixel_clk = 216000000,
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
		.hor_sync_polarity = 0,/* 0: negative, 1: positive */
		.ver_sync_polarity = 0,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_TV_MOD_480I,
		.pixel_clk = 216000000,
		.pixel_repeat = 0,
		.x_res = 720,
		.y_res = 480,
		.hor_total_time = 858,
		.hor_back_porch = 57,
		.hor_front_porch = 62,
		.hor_sync_time = 19,
		.ver_total_time = 525,
		.ver_back_porch = 4,
		.ver_front_porch = 1,
		.ver_sync_time = 3,
		.hor_sync_polarity = 0,/* 0: negative, 1: positive */
		.ver_sync_polarity = 0,/* 0: negative, 1: positive */
		.b_interlace = 1,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_TV_MOD_576I,
		.pixel_clk = 216000000,
		.pixel_repeat = 0,
		.x_res = 720,
		.y_res = 576,
		.hor_total_time = 864,
		.hor_back_porch = 69,
		.hor_front_porch = 63,
		.hor_sync_time = 12,
		.ver_total_time = 625,
		.ver_back_porch = 2,
		.ver_front_porch = 44,
		.ver_sync_time = 3,
		.hor_sync_polarity = 0,/* 0: negative, 1: positive */
		.ver_sync_polarity = 0,/* 0: negative, 1: positive */
		.b_interlace = 1,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_TV_MOD_480P,
		.pixel_clk = 54000000,
		.pixel_repeat = 0,
		.x_res = 720,
		.y_res = 480,
		.hor_total_time = 858,
		.hor_back_porch = 60,
		.hor_front_porch = 62,
		.hor_sync_time = 16,
		.ver_total_time = 525,
		.ver_back_porch = 9,
		.ver_front_porch = 30,
		.ver_sync_time = 6,
		.hor_sync_polarity = 1,/* 0: negative, 1: positive */
		.ver_sync_polarity = 1,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_TV_MOD_576P,
		.pixel_clk = 54000000,
		.pixel_repeat = 0,
		.x_res = 720,
		.y_res = 576,
		.hor_total_time = 864,
		.hor_back_porch = 68,
		.hor_front_porch = 64,
		.hor_sync_time = 12,
		.ver_total_time = 625,
		.ver_back_porch = 5,
		.ver_front_porch = 39,
		.ver_sync_time = 5,
		.hor_sync_polarity = 1,/* 0: negative, 1: positive */
		.ver_sync_polarity = 1,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_TV_MOD_720P_60HZ,
		.pixel_clk = 74250000,
		.pixel_repeat = 0,
		.x_res = 1280,
		.y_res = 720,
		.hor_total_time = 1650,
		.hor_back_porch = 220,
		.hor_front_porch = 40,
		.hor_sync_time = 110,
		.ver_total_time = 750,
		.ver_back_porch = 5,
		.ver_front_porch = 20,
		.ver_sync_time = 5,
		.hor_sync_polarity = 1,/* 0: negative, 1: positive */
		.ver_sync_polarity = 1,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_TV_MOD_720P_50HZ,
		.pixel_clk = 74250000,
		.pixel_repeat = 0,
		.x_res = 1280,
		.y_res = 720,
		.hor_total_time = 1980,
		.hor_back_porch = 220,
		.hor_front_porch = 40,
		.hor_sync_time = 440,
		.ver_total_time = 750,
		.ver_back_porch = 5,
		.ver_front_porch = 20,
		.ver_sync_time = 5,
		.hor_sync_polarity = 1,/* 0: negative, 1: positive */
		.ver_sync_polarity = 1,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_TV_MOD_1080I_60HZ,
		.pixel_clk = 74250000,
		.pixel_repeat = 0,
		.x_res = 1920,
		.y_res = 1080,
		.hor_total_time = 2200,
		.hor_back_porch = 148,
		.hor_front_porch = 44,
		.hor_sync_time = 88,
		.ver_total_time = 1125,
		.ver_back_porch = 2,
		.ver_front_porch = 38,
		.ver_sync_time = 5,
		.hor_sync_polarity = 1,/* 0: negative, 1: positive */
		.ver_sync_polarity = 1,/* 0: negative, 1: positive */
		.b_interlace = 1,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_TV_MOD_1080I_50HZ,
		.pixel_clk = 74250000,
		.pixel_repeat = 0,
		.x_res = 1920,
		.y_res = 1080,
		.hor_total_time = 2640,
		.hor_back_porch = 148,
		.hor_front_porch = 44,
		.hor_sync_time = 528,
		.ver_total_time = 1125,
		.ver_back_porch = 2,
		.ver_front_porch = 38,
		.ver_sync_time = 5,
		.hor_sync_polarity = 1,/* 0: negative, 1: positive */
		.ver_sync_polarity = 1,/* 0: negative, 1: positive */
		.b_interlace = 1,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_TV_MOD_1080P_60HZ,
		.pixel_clk = 148500000,
		.pixel_repeat = 0,
		.x_res = 1920,
		.y_res = 1080,
		.hor_total_time = 2200,
		.hor_back_porch = 148,
		.hor_front_porch = 44,
		.hor_sync_time = 88,
		.ver_total_time = 1125,
		.ver_back_porch = 4,
		.ver_front_porch = 36,
		.ver_sync_time = 5,
		.hor_sync_polarity = 1,/* 0: negative, 1: positive */
		.ver_sync_polarity = 1,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_TV_MOD_1080P_50HZ,
		.pixel_clk = 148500000,
		.pixel_repeat = 0,
		.x_res = 1920,
		.y_res = 1080,
		.hor_total_time = 2640,
		.hor_back_porch = 148,
		.hor_front_porch = 44,
		.hor_sync_time = 528,
		.ver_total_time = 1125,
		.ver_back_porch = 4,
		.ver_front_porch = 36,
		.ver_sync_time = 5,
		.hor_sync_polarity = 1,/* 0: negative, 1: positive */
		.ver_sync_polarity = 1,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,
	},
};
/* video timing definition end */

static struct sunxi_tv *sunxi_tv_get_tv(int tv_id)
{
	if (tv_id > MAX_TV_COUNT) {
		TV_ERR("tv_id:%d is too big!\n", tv_id);
		return NULL;
	}

	return hwtv[tv_id];
}

/**
 * @name       :sunxi_tv_get_tv_count
 * @brief      :get the number of sunxi
 * @return     :the number of sunxi tv
 */
int sunxi_tv_get_tv_count(void)
{
	return tv_count;
}

static int __pin_config(int sel, char *name)
{
	int ret = 0;
	char type_name[10] = {0};
	struct device_node *node;
	struct platform_device *pdev;
	struct pinctrl *pctl;
	struct pinctrl_state *state;

	snprintf(type_name, sizeof(type_name), "tv%d", sel);

	node = of_find_compatible_node(NULL, type_name, "allwinner,sunxi-tv");
	if (!node) {
		TV_ERR("of_find_tv_node %s fail\n", type_name);
		ret = -EINVAL;
		goto exit;
	}

	pdev = of_find_device_by_node(node);
	if (!node) {
		TV_ERR("of_find_device_by_node for %s fail\n", type_name);
		ret = -EINVAL;
		goto exit;
	}

	pctl = pinctrl_get(&pdev->dev);
	if (IS_ERR(pctl)) {
		TV_ERR("pinctrl_get for %s fail\n", type_name);
		ret = PTR_ERR(pctl);
		goto exit;
	}

	state = pinctrl_lookup_state(pctl, name);
	if (IS_ERR(state)) {
		TV_ERR("pinctrl_lookup_state for %s fail\n", type_name);
		ret = PTR_ERR(state);
		goto exit;
	}

	ret = pinctrl_select_state(pctl, state);
	if (ret < 0) {
		TV_ERR("pinctrl_select_state(%s)fail\n", type_name);
		goto exit;
	}

exit:
	return ret;
}

s32 tv_get_timing_list_num(struct sunxi_tv *p_tv)
{
	if (!p_tv) {
		TV_ERR("Null pointer!\n");
		return 0;
	}
	if (p_tv->tv_type == DISP_TV_CVBS) {
		return sizeof(cvbs_timing) / sizeof(struct disp_video_timings);
	} else if (p_tv->tv_type == DISP_VGA) {
		return sizeof(vga_timing) / sizeof(struct disp_video_timings);
	} else {
		return sizeof(ypbpr_timing) / sizeof(struct disp_video_timings);
	}
}

static int tve_clk_enable(struct sunxi_tv *p_tv)
{
	int ret = -1;

	if (!p_tv) {
		TV_ERR("Null pointer!\n");
		goto OUT;
	}

	ret = clk_prepare_enable(p_tv->mclk);
	if (ret != 0) {
		TV_ERR("fail to enable tve%d's clk!\n", p_tv->id);
		return ret;
	}

OUT:
	return ret;
}

/* static int tve_clk_disable(struct sunxi_tv *p_tv)
{
	if (!p_tv) {
		TV_ERR("Null pointer!\n");
		return -1;
	}
	clk_disable(p_tv->mclk);
	return 0;
} */

static s32 tve_get_pixclk(struct sunxi_tv *p_tv, unsigned long *p_rate)
{
	int i = 0, list_num = 0, ret = -1;
	bool find = false;
	struct disp_video_timings *info = NULL;

	if (!p_rate || !p_tv)
		goto OUT;

	list_num = tv_get_timing_list_num(p_tv);
	info = p_tv->video_info;
	if (!info) {
		TV_ERR("Null pointer!\n");
		goto OUT;
	}

	for (i = 0; i < list_num; i++) {
		if (info->tv_mode == p_tv->tv_mode) {
			find = true;
			break;
		}
		info++;
	}
	if (!find) {
		TV_ERR("tv have no mode(%d)!\n", p_tv->tv_mode);
		goto OUT;
	} else {
		*p_rate = info->pixel_clk;
		ret = 0;
	}
OUT:
	return ret;
}

static void tve_clk_config(struct sunxi_tv *p_tv)
{
	int ret = 0;
	unsigned long rate = 0, prate = 0;
	unsigned long round = 0, parent_round_rate = 0;
	signed long rate_diff = 0, prate_diff = 0, accuracy = 1000000;
	unsigned int div = 1;

	if (!p_tv) {
		TV_ERR("Null pointer!\n");
		return;
	}

	ret = tve_get_pixclk(p_tv, &rate);
	if (ret)
		TV_ERR("%s:tve_get_pixclk fail!\n", __func__);

	round = clk_round_rate(p_tv->mclk, rate);
	rate_diff = (long)(round - rate);
	if ((rate_diff > accuracy) || (rate_diff < -accuracy)) {
		for (accuracy = 1000000; accuracy <= 5000000;
		     accuracy += 1000000) {
			for (div = 1; (rate * div) <= 984000000; div++) {
				prate = rate * div;
				parent_round_rate =
				    clk_round_rate(p_tv->mclk_parent, prate);
				prate_diff = (long)(parent_round_rate - prate);
				if ((prate_diff < accuracy) &&
				    (prate_diff > -accuracy)) {
					ret = clk_set_rate(p_tv->mclk_parent,
							   prate);
					ret += clk_set_rate(
					    p_tv->mclk, rate);
					if (ret)
						TV_ERR("fail to set rate(%ld) "
							"fo tve%d's clock!\n",
							rate, p_tv->id);
					else
						break;
				}
			}
			if (rate * div > 984000000) {
				TV_ERR("fail to set tve clk at %ld accuracy\n",
					accuracy);
				continue;
			}
			break;
		}
	} else {
		prate = clk_get_rate(p_tv->mclk_parent);
		ret = clk_set_rate(p_tv->mclk, rate);
		if (ret)
			TV_ERR("fail to set rate(%ld) fo tve%d's clock!\n",
				rate, p_tv->id);
	}

	TV_DBG("parent prate=%lu(%lu), rate=%lu(%lu), tv_mode=%d\n",
		clk_get_rate(p_tv->mclk_parent), prate,
		clk_get_rate(p_tv->mclk), rate, p_tv->tv_mode);
}

/* static enum disp_tv_mode sunxi_tv_get_mode(int tv_id)
{
	struct sunxi_tv *p_tv = sunxi_tv_get_tv(tv_id);

	if (!p_tv) {
		TV_ERR("Null pointer!\n");
		return DISP_TV_MODE_NUM;
	}

	return p_tv->tv_mode;
} */

struct disp_video_timings *sunxi_tv_get_video_timing(
			int tv_id, unsigned int tv_mode)
{
	int i, num;
	struct disp_video_timings *timing;
	struct sunxi_tv *p_tv = sunxi_tv_get_tv(tv_id);

	if (!p_tv) {
		TV_ERR("Null pointer!\n");
		return NULL;
	}

	num = tv_get_timing_list_num(p_tv);
	timing = p_tv->video_info;

	for (i = 0; i < num; i++)
		if (timing[i].tv_mode == tv_mode)
			break;

	if (i >= num) {
		TV_ERR("cat NOT get timing from tv_mode:%u\n", tv_mode);
		return NULL;
	}

	return &timing[i];

}


int sunxi_tv_set_mode(int tv_id, enum disp_tv_mode tv_mode)
{
	struct sunxi_tv *p_tv = sunxi_tv_get_tv(tv_id);

	if (!p_tv) {
		TV_ERR("Null pointer!\n");
		return -1;
	}

	mutex_lock(&p_tv->mlock);
	p_tv->tv_mode = tv_mode;
	mutex_unlock(&p_tv->mlock);
	return 0;
}

int sunxi_tv_enable(int tv_id)
{
	struct sunxi_tv *p_tv = sunxi_tv_get_tv(tv_id);

	if (!p_tv) {
		TV_ERR("Null pointer!\n");
		return -1;
	}

	if (!p_tv->enable) {
		if (p_tv->tv_type == DISP_VGA)
			__pin_config(p_tv->id, "active");

		tve_clk_config(p_tv);
		tve_low_set_tv_mode(p_tv->id, p_tv->tv_mode, p_tv->cali);
		tve_low_dac_enable(p_tv->id);
		tve_low_open(p_tv->id);
		mutex_lock(&p_tv->mlock);
		p_tv->enable = 1;
		mutex_unlock(&p_tv->mlock);
	}
	return 0;
}

int sunxi_tv_disable(int tv_id)
{
	struct sunxi_tv *p_tv = sunxi_tv_get_tv(tv_id);

	if (!p_tv) {
		TV_ERR("Null pointer!\n");
		return -1;
	}

	TV_DBG("tv %d\n", p_tv->id);

	mutex_lock(&p_tv->mlock);
	if (p_tv->enable) {
		tve_low_close(p_tv->id);
		tve_low_dac_autocheck_enable(p_tv->id);
		p_tv->enable = 0;
	}
	mutex_unlock(&p_tv->mlock);
	if (p_tv->tv_type == DISP_VGA)
		__pin_config(p_tv->id, "sleep");

	return 0;
}

int  sunxi_tv_get_working_mode(int tv_id,
		struct sunxi_tv_work_mode *work_mode)
{
	struct sunxi_tv *p_tv = sunxi_tv_get_tv(tv_id);

	if (!p_tv) {
		TV_ERR("Null pointer!\n");
		return 1;
	}

	/* vga interface is rgb mode. */
	if (p_tv->tv_type == DISP_VGA)
		work_mode->color_fmt = COLOR_FMT_RGB444;
	else
		work_mode->color_fmt = COLOR_FMT_YUV444;

	return 0;
}

enum disp_tv_output sunxi_tv_get_interface_type(int tv_id)
{
	struct sunxi_tv *p_tv = sunxi_tv_get_tv(tv_id);

	if (!p_tv) {
		TV_ERR("Null pointer!\n");
		return DISP_TV_NONE;
	}

	return p_tv->tv_type;
}

int sunxi_tv_get_connect_status(int tv_id)
{
	struct sunxi_tv *p_tv = sunxi_tv_get_tv(tv_id);

	if (!p_tv) {
		TV_ERR("Null pointer!\n");
		return DISP_TV_NONE;
	}

	if (p_tv->tv_type == DISP_VGA ||
	    p_tv->tv_type == DISP_TV_YPBPR)
		return 1;

	return tve_low_get_dac_status(p_tv->id);
}

int sunxi_tv_mode_support(int tv_id, enum disp_tv_mode tv_mode)
{
	struct sunxi_tv *p_tv = sunxi_tv_get_tv(tv_id);
	unsigned int i, list_num;
	struct disp_video_timings *info;

	if (!p_tv) {
		TV_ERR("Null pointer!\n");
		return 0;
	}

	list_num = tv_get_timing_list_num(p_tv);
	if (!list_num) {
		TV_ERR("list number is zero!\n");
		return 0;
	}

	if (p_tv->tv_type == DISP_TV_CVBS)
		info = cvbs_timing;
	else if (p_tv->tv_type == DISP_VGA)
		info = vga_timing;
	else
		info = ypbpr_timing;

	for (i = 0; i < list_num; i++) {
		if (info->tv_mode == tv_mode)
			return 1;
		info++;
	}
	return 0;
}

int sunxi_tv_get_video_mode_info(int tv_id,
			       struct disp_video_timings **video_info,
			       enum disp_tv_mode tv_mode)
{
	struct disp_video_timings *info;
	int ret = -1;
	int i, list_num;
	struct sunxi_tv *p_tv = sunxi_tv_get_tv(tv_id);
	if (!p_tv || !video_info) {
		TV_ERR("Null pointer!\n");
		return 0;
	}

	TV_DBG("tv %d\n", p_tv->id);
	info = p_tv->video_info;

	list_num = tv_get_timing_list_num(p_tv);
	for (i = 0; i < list_num; i++) {
		mutex_lock(&p_tv->mlock);
		if (info->tv_mode == tv_mode) {
			*video_info = info;
			ret = 0;
			mutex_unlock(&p_tv->mlock);
			break;
		}
		mutex_unlock(&p_tv->mlock);
		info++;
	}
	return ret;
}

int sunxi_tv_get_video_timing_info(int tv_id, unsigned int *num,
			       struct disp_video_timings **video_info)
{
	struct sunxi_tv *p_tv = sunxi_tv_get_tv(tv_id);

	if (!p_tv || !num || !video_info) {
		TV_ERR("Null pointer!\n");
		return -1;
	}

	*num = tv_get_timing_list_num(p_tv);
	*video_info = p_tv->video_info;
	if (!*num || !*video_info) {
		TV_ERR("Fail to get num or video_info!\n");
		return -1;
	}
	return 0;
}

#ifdef CONFIG_AW_AXP
static int tv_power_enable(const char *name)
{
	struct regulator *regu = NULL;
	int ret = -1;

	if (!name) {
		goto exit;
	}

	regu = regulator_get(NULL, name);
	if (IS_ERR(regu)) {
		TV_ERR("%s: some error happen, fail to get regulator %s\n",
			__func__, name);
		goto exit;
	}

	/* enalbe regulator */
	ret = regulator_enable(regu);
	if (0 != ret) {
		TV_ERR("%s: some error happen, fail to enable regulator %s!\n",
			__func__, name);
		goto exit1;
	}


exit1:
	/* put regulater, when module exit */
	regulator_put(regu);
exit:
	return ret;
}

static int tv_power_disable(const char *name)
{
	struct regulator *regu = NULL;
	int ret = 0;

	if (!name) {
		goto exit;
	}

	regu = regulator_get(NULL, name);
	if (IS_ERR(regu)) {
		TV_ERR("%s: some error happen, fail to get regulator %s\n",
			__func__, name);
		goto exit;
	}

	/* disalbe regulator */
	ret = regulator_disable(regu);
	if (0 != ret) {
		TV_ERR("%s: some error happen, fail to disable regulator %s!\n",
			__func__, name);
		goto exit1;
	}
exit1:
	/* put regulater, when module exit */
	regulator_put(regu);
exit:
	return ret;
}
#else
static int tv_power_enable(char *name)
{
	return 0;
}
static int tv_power_disable(char *name)
{
	return 0;
}
#endif

#if defined(TVE_TOP_SUPPORT)
static int tv_top_init(struct sunxi_tv *p_tv)
{
	int ret = -1;
	struct platform_device *pdev = p_tv->pdev;

	pdev->id = of_alias_get_id(pdev->dev.of_node, "tv");
	if (pdev->id < 0) {
		TV_DBG("failed to get alias id\n");
		return -EINVAL;
	}

	p_tv->top_addr = of_iomap(pdev->dev.of_node, 0);
	if (IS_ERR_OR_NULL(p_tv->top_addr)) {
		dev_err(&pdev->dev, "unable to map tve common registers\n");
		ret = -EINVAL;
		goto err_iomap;
	}

	p_tv->top_clk = of_clk_get(pdev->dev.of_node, 0);
	if (IS_ERR(p_tv->top_clk)) {
		dev_err(&pdev->dev, "fail to get clk for tve common module!\n");
		goto err_iomap;
	}

	tve_low_set_top_reg_base(p_tv->top_addr);

	return 0;

err_iomap:
	if (p_tv->top_addr)
		iounmap((char __iomem *)p_tv->top_addr);
	return ret;
}

#endif

static int tve_top_clk_enable(struct sunxi_tv *p_tv)
{
	int ret = -1;

	if (!p_tv || !p_tv->top_clk) {
		TV_ERR("NULL pointer!\n");
		goto OUT;
	}

	ret = clk_prepare_enable(p_tv->top_clk);
	if (ret != 0)
		TV_ERR("fail to enable tve's top clk!\n");

OUT:
	return ret;
}

static int tve_top_clk_disable(struct sunxi_tv *p_tv)
{
	int ret = -1;

	if (!p_tv || !p_tv->top_clk) {
		TV_ERR("NULL pointer!\n");
		goto OUT;
	}

	clk_disable(p_tv->top_clk);
	ret = 0;
OUT:
	return ret;
}

static s32 __get_offset(struct device_node *node, int i)
{
	char sub_key[20] = {0};
	s32 value = 0;
	int ret = 0;

	snprintf(sub_key, sizeof(sub_key), "dac_offset%d", i);
	ret = of_property_read_u32(node, sub_key, (u32 *)&value);
	if (ret < 0) {
		TV_DBG("there is no tve dac(%d) offset value.\n", i);
	} else {
		/* Sysconfig can not use signed params, however,
		 * dac_offset as a signed param which ranges from
		 * -100 to 100, is maping sysconfig params from
		 * 0 to 200.
		 */
		if ((value > 200) || (value < 0))
			TV_ERR("dac offset is out of range.\n");
		else
			return value - 100;
	}

	return 0;
}

static void tve_clk_init(struct sunxi_tv *p_tv)
{
	if (!p_tv) {
		TV_ERR("Null pointer!\n");
		return;
	}

	p_tv->mclk_parent = clk_get_parent(p_tv->mclk);
}

/* TODO:handle smooth display */
static int tv_init(struct sunxi_tv *p_tv)
{
	s32 i = 0, ret = 0;
	u32 cali_value = 0;
	char sub_key[20] = {0};
	unsigned int value;
	unsigned int interface = 0;
	unsigned long rate = 0;
	struct platform_device *pdev = NULL;
#if defined(CONFIG_ARCH_SUN8IW7)
	unsigned int cali_default[4] = {512, 512, 512, 512};
#else
	unsigned int cali_default[4] = {625, 625, 625, 625};
#endif

#if defined(CONFIG_ARCH_SUN8IW7)
	s32 sid_turn = 0;
#endif
	if (!p_tv) {
		TV_ERR("Null pointer!\n");
		goto OUT;
	}

	pdev = p_tv->pdev;
	ret = of_property_read_u32(pdev->dev.of_node, "interface", &interface);
	if (ret < 0) {
		TV_ERR("get tv interface failed!\n");
		goto OUT;
	}
	p_tv->tv_type = interface;

	if (p_tv->tv_type == DISP_TV_CVBS)
		p_tv->video_info = cvbs_timing;
	else if (p_tv->tv_type == DISP_VGA)
		p_tv->video_info = vga_timing;
	else
		p_tv->video_info = ypbpr_timing;

	memcpy(p_tv->cali, cali_default, TVE_DAC_NUM * sizeof(unsigned int));
	memset(p_tv->cali_offset, 0, TVE_DAC_NUM * sizeof(int));

	tve_top_clk_enable(p_tv);
	/* get mapping dac */
	for (i = 0; i < TVE_DAC_NUM; i++) {
		u32 dac_no;

		snprintf(sub_key, sizeof(sub_key), "dac_src%d", i);
		ret = of_property_read_u32(pdev->dev.of_node, sub_key, &value);
		if (ret < 0) {
			TV_DBG("tve%d have no dac %d\n", p_tv->id, i);
		} else {
			dac_no = value;
			p_tv->dac_no[i] = value;
			++p_tv->dac_num;
			cali_value = tve_low_get_sid(dac_no);

			TV_DBG("cali_temp = %u\n", cali_value);
			/* VGA mode: 16~31 bits
			 * CVBS & YPBPR mode: 0~15 bits
			 * zero is not allow
			 */
			if (cali_value) {
				if (interface == DISP_VGA)
					p_tv->cali[dac_no] =
					    (cali_value >> 16) & 0xffff;
				else {
#if defined(CONFIG_ARCH_SUN8IW7)
					if (cali_value & (1 << 9))
						sid_turn =
						    0 + (cali_value & 0x1ff);
					else
						sid_turn =
						    0 - (cali_value & 0x1ff);

					sid_turn += 91;

					if (sid_turn >= 0)
						sid_turn = (1 << 9) | sid_turn;
					else
						sid_turn = 0 - sid_turn;
					cali_value = (u32)sid_turn;
#endif
					p_tv->cali[dac_no] = cali_value & 0xffff;
				}
			}
			p_tv->cali_offset[dac_no] = __get_offset(pdev->dev.of_node, i);
			TV_DBG("cali[%u] = %u, offset[%u] = %u\n", dac_no,
				 p_tv->cali[dac_no], dac_no, p_tv->cali_offset[dac_no]);
		}

		snprintf(sub_key, sizeof(sub_key), "dac_type%d", i);
		ret = of_property_read_u32(pdev->dev.of_node, sub_key, &value);
		if (ret < 0) {
			TV_DBG("tve%d have no type%d\n", p_tv->id, i);
			/* if do'not config type, set disabled status */
			p_tv->dac_type[i] = DISP_TV_DAC_SRC_NONE;
		} else {
			p_tv->dac_type[i] = value;
		}
	}

	/* parse boot params */

	mutex_init(&p_tv->mlock);
	p_tv->tv_mode = DISP_TV_MOD_PAL;
	ret = tve_get_pixclk(p_tv, &rate);
	if (ret)
		TV_ERR("%s:tve_get_pixclk fail!\n", __func__);
	ret = clk_set_rate(p_tv->mclk, rate);
	if (ret)
		TV_ERR("fail to set rate(%ld) fo tve%d's clock!\n",
		       rate, p_tv->id);

	tve_low_set_reg_base(p_tv->id, p_tv->base_addr);
	tve_clk_init(p_tv);
#if !defined(CONFIG_COMMON_CLK_ENABLE_SYNCBOOT)
	tve_clk_enable(p_tv);
#endif

	tve_low_init(p_tv->id, &p_tv->dac_no[0], p_tv->cali,
		     p_tv->cali_offset, p_tv->dac_type,
		     p_tv->dac_num);

	tve_low_dac_autocheck_enable(p_tv->id);

OUT:
	return ret;
}

static int sunxi_tv_probe(struct platform_device *pdev)
{
	int ret = -1;
	int index = 0;
	struct sunxi_tv *p_tv = NULL;

	TV_INFO(" start\n");
	pdev->id = of_alias_get_id(pdev->dev.of_node, "tv");
	if (pdev->id < 0) {
		TV_ERR("failed to get alias id\n");
		goto OUT;
	}

	if (pdev->id > MAX_TV_COUNT) {
		TV_ERR("alias id:%d is too big!\n", pdev->id);
		goto OUT;
	}

	p_tv = kmalloc(sizeof(*p_tv), GFP_KERNEL | __GFP_ZERO);
	if (!p_tv) {
		TV_ERR("Malloc sunxi_tv fail!\n");
		goto OUT;
	}

	hwtv[pdev->id] = p_tv;

	p_tv->id = pdev->id;
	p_tv->pdev = pdev;

	if (of_property_read_string(pdev->dev.of_node, "tv_power",
				    &p_tv->p_tv_power))
		TV_DBG("of_property_read_string tv_power failed!\n");
	else
		tv_power_enable(p_tv->p_tv_power);

#if defined(TVE_TOP_SUPPORT)
	tv_top_init(p_tv);
	index = 1;
#endif

	p_tv->base_addr =
	    of_iomap(pdev->dev.of_node, index);
	if (IS_ERR_OR_NULL(p_tv->base_addr)) {
		dev_err(&pdev->dev, "fail to get addr for tve%d!\n", pdev->id);
		goto FREE_TV;
	}

	p_tv->mclk = of_clk_get(pdev->dev.of_node, index);
	if (IS_ERR_OR_NULL(p_tv->mclk)) {
		dev_err(&pdev->dev, "fail to get clk for tve%d's!\n", pdev->id);
		goto err_iomap;
	}

	ret = tv_init(p_tv);
	if (ret)
		goto err_iomap;

	tv_count++;

	return 0;

err_iomap:
	if (p_tv->base_addr)
		iounmap((char __iomem *)p_tv->base_addr);

	if (p_tv->top_addr)
		iounmap((char __iomem *)p_tv->top_addr);

FREE_TV:
	kfree(p_tv);
OUT:
	return ret;
}

static int sunxi_tv_remove(struct platform_device *pdev)
{
	struct sunxi_tv *p_tv = hwtv[pdev->id];

	if (!p_tv) {
		TV_ERR("Null pointer!\n");
		return -1;
	}

	sunxi_tv_disable(pdev->id);
	tve_top_clk_disable(p_tv);
	tv_power_disable(p_tv->p_tv_power);
	kfree(p_tv);
	return 0;
}

static const struct of_device_id sunxi_tv_match[] = {
	{ .compatible = "allwinner,sunxi-tv", },

	{},
};

struct platform_driver sunxi_tv_platform_driver = {
	.probe = sunxi_tv_probe,
	.remove = sunxi_tv_remove,
	.driver = {
		   .name = "tv",
		   .owner = THIS_MODULE,
		   .of_match_table = sunxi_tv_match,
	},
};

static ssize_t tv_state_show(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	ssize_t n = 0;
	struct sunxi_tv *p_tv = NULL;
	int num_of_tv = 0, i = 0;
	char name[40];

	num_of_tv = sunxi_tv_get_tv_count();

	for (i = 0; i < num_of_tv; ++i) {
		p_tv = sunxi_tv_get_tv(i);

		switch (p_tv->tv_type) {
		case DISP_TV_CVBS:
			strncpy(name, "cvbs", 40);
			break;
		case DISP_VGA:
			strncpy(name, "vga", 40);
			break;
		case DISP_TV_YPBPR:
			strncpy(name, "ypbpr", 40);
			break;
		default:
			strncpy(name, "unknown", 40);
			break;
		}
		n += sprintf(buf + n, "%s%d=%d\n", name, p_tv->id,
			     sunxi_tv_get_connect_status(p_tv->id));
	}

	return n;
}

static DEVICE_ATTR(tv_state, 0660, tv_state_show, NULL);

static struct attribute *tv_attributes[] = {
	&dev_attr_tv_state.attr,
	NULL
};

static struct attribute_group tv_attribute_group = {
	.name = "attr",
	.attrs = tv_attributes,
};

int sunxi_tv_module_init(void)
{
	int ret = -1;

	TV_INFO(" start\n");
	p_tv_drv = kmalloc(sizeof(*p_tv_drv), GFP_KERNEL | __GFP_ZERO);
	if (!p_tv_drv) {
		TV_ERR("Null drv_model_info pointer\n");
		goto OUT;
	}
	ret = alloc_chrdev_region(&p_tv_drv->devid, 0, 1, "tv");
	if (ret < 0) {
		TV_ERR("alloc_chrdev_region failed\n");
		goto FREE_DRV;
	}

	p_tv_drv->cdev = cdev_alloc();
	if (!p_tv_drv->cdev) {
		TV_ERR("cdev_alloc failed\n");
		goto FREE_DRV;
	}

	cdev_init(p_tv_drv->cdev, NULL);
	p_tv_drv->cdev->owner = THIS_MODULE;
	ret = cdev_add(p_tv_drv->cdev, p_tv_drv->devid, 1);
	if (ret) {
		TV_ERR("cdev_add major number:%d failed\n",
		       MAJOR(p_tv_drv->devid));
		goto FREE_DRV;
	}

	p_tv_drv->sysclass = class_create(THIS_MODULE, "tv");
	if (IS_ERR(p_tv_drv->sysclass)) {
		TV_ERR("create class error\n");
		goto FREE_DRV;
	}

	p_tv_drv->dev = device_create(p_tv_drv->sysclass, NULL, p_tv_drv->devid,
				      NULL, "tv");
	if (!p_tv_drv->dev) {
		TV_ERR("device_create failed\n");
		goto FREE_DRV;
	}

	tv_count = 0;

	ret = platform_driver_register(&sunxi_tv_platform_driver);
	if (ret) {
		TV_ERR("platform_driver_register failed\n");
		goto FREE_DEVICE;
	}

	ret = sysfs_create_group(&p_tv_drv->dev->kobj, &tv_attribute_group);
	if (ret < 0) {
		TV_ERR("sysfs_create_file fail!\n");
		goto UNREGISTER;
	}


	TV_INFO(" end\n");
	return ret;

UNREGISTER:
	platform_driver_unregister(&sunxi_tv_platform_driver);
FREE_DEVICE:
	device_destroy(p_tv_drv->sysclass, p_tv_drv->devid);
FREE_DRV:
	kfree(p_tv_drv);
OUT:
	TV_ERR(" failed\n");
	return -EINVAL;
}

void sunxi_tv_module_exit(void)
{
	TV_INFO("\n");
	if (p_tv_drv) {
		platform_driver_unregister(&sunxi_tv_platform_driver);

		device_destroy(p_tv_drv->sysclass, p_tv_drv->devid);
		class_destroy(p_tv_drv->sysclass);

		cdev_del(p_tv_drv->cdev);
		kfree(p_tv_drv);
	}
}
