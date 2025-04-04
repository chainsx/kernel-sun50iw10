/* de_edp_param.h
 *
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
 * some struct definiton
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef _DE_EDP_PARAM_H
#define _DE_EDP_PARAM_H

struct video_para {
	int mvid;
	int nvid;
	int fill;
};

struct tx_current {
	unsigned int mc;
	unsigned int tap1;
	unsigned int tap2;
	unsigned int tap1_op;
	unsigned int tap2_op;
};

struct sink_info {
	unsigned int dp_rev;
	unsigned int eDP_capable;
	unsigned int dp_enhanced_frame_cap;
	unsigned long long dp_max_link_rate;
	unsigned int dp_max_lane_count;
};

struct training_info {
	unsigned int swing_lv;
	unsigned int preemp_lv;
	unsigned int postcur2_lv;
};

#define BIT29 0x20000000
#define BIT30 0x40000000
#define BIT31 0x80000000

enum edp_int {
	LINE0 = BIT31,
	LINE1 = BIT30,
	FIFO_EMPTY = BIT29,
};

enum disp_edp_colordepth {
	EDP_8_BIT = 0,
	EDP_10_BIT = 1,
};

enum edp_video_src_t {
	RGB_INPUT = 0,
	COLOR_BAR_INPUT,
	MOSAIC_INPUT,
};

#define RET_OK (0)
#define RET_FAIL (-1)
/**
 * edp_para
 */
struct edp_para {
	unsigned long long edp_rate;
	unsigned int edp_lane;
	enum disp_edp_colordepth edp_colordepth;
	unsigned int edp_fps;
	unsigned int edp_training_func;
	unsigned int edp_sramble_seed;
};

#endif /* End of file */
