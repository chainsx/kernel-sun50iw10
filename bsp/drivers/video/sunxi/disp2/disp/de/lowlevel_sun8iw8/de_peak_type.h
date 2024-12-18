/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#ifndef __DE_PEAK_TYPE_H__
#define __DE_PEAK_TYPE_H__

#include "de_rtmx.h"

typedef union {
	unsigned int dwval;
	struct {
		unsigned int en:1;	/* Default: 0x0; */
		unsigned int res0:7;	/* Default: ; */
		unsigned int win_en:1;	/* Default: 0x0; */
		unsigned int res1:23;	/* Default: ; */
	} bits;
} LP_CTRL_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int width:12;	/* Default: 0x0; */
		unsigned int res0:4;	/* Default: ; */
		unsigned int height:12;	/* Default: 0x0; */
		unsigned int res1:4;	/* Default: ; */
	} bits;
} LP_SIZE_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int win_left:12;	/* Default: 0x0; */
		unsigned int res0:4;	/* Default: ; */
		unsigned int win_top:12;	/* Default: 0x0; */
		unsigned int res1:4;	/* Default: ; */
	} bits;
} LP_WIN0_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int win_right:12;	/* Default: 0x0; */
		unsigned int res0:4;	/* Default: ; */
		unsigned int win_bot:12;	/* Default: 0x0; */
		unsigned int res1:4;	/* Default: ; */
	} bits;
} LP_WIN1_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int bp1_ratio:6;	/* Default: 0x0; */
		unsigned int res0:2;	/* Default: ; */
		unsigned int bp0_ratio:6;	/* Default: 0x0; */
		unsigned int res1:2;	/* Default: ; */
		unsigned int hp_ratio:6;	/* Default: 0x0; */
		unsigned int res2:9;	/* Default: ; */
		unsigned int filter_sel:1;	/* Default: 0x0; */
	} bits;
} LP_FILTER_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int c0:9;	/* Default: 0x0; */
		unsigned int res0:7;	/* Default: ; */
		unsigned int c1:9;	/* Default: 0x0; */
		unsigned int res1:7;	/* Default: ; */
	} bits;
} LP_CSTM_FILTER0_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int c2:9;	/* Default: 0x0; */
		unsigned int res0:7;	/* Default: ; */
		unsigned int c3:9;	/* Default: 0x0; */
		unsigned int res1:7;	/* Default: ; */
	} bits;
} LP_CSTM_FILTER1_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int c4:9;	/* Default: 0x0; */
		unsigned int res0:23;	/* Default: ; */
	} bits;
} LP_CSTM_FILTER2_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int gain:8;	/* Default: 0x0; */
		unsigned int res0:24;	/* Default: ; */
	} bits;
} LP_GAIN_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int beta:5;	/* Default: 0x0; */
		unsigned int res0:11;	/* Default: ; */
		unsigned int dif_up:8;	/* Default: 0x0; */
		unsigned int res1:8;	/* Default: ; */
	} bits;
} LP_GAINCTRL_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int neg_gain:6;	/* Default: 0x0; */
		unsigned int res0:26;	/* Default: ; */
	} bits;
} LP_SHOOTCTRL_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int corthr:8;	/* Default: 0x0; */
		unsigned int res0:24;	/* Default: ; */
	} bits;
} LP_CORING_REG;

typedef struct {
	LP_CTRL_REG ctrl;	/* 0x0000 */
	LP_SIZE_REG size;	/* 0x0004 */
	LP_WIN0_REG win0;	/* 0x0008 */
	LP_WIN1_REG win1;	/* 0x000c */
	LP_FILTER_REG filter;	/* 0x0010 */
	LP_CSTM_FILTER0_REG cfilter0;	/* 0x0014 */
	LP_CSTM_FILTER1_REG cfilter1;	/* 0x0018 */
	LP_CSTM_FILTER2_REG cfilter2;	/* 0x001c */
	LP_GAIN_REG gain;	/* 0x0020 */
	LP_GAINCTRL_REG gainctrl;	/* 0x0024 */
	LP_SHOOTCTRL_REG shootctrl;	/* 0x0028 */
	LP_CORING_REG coring;	/* 0x002c */
} __peak_reg_t;

typedef struct {
	/* peak */
	unsigned int peak_en;
	unsigned int gain;

	/* window */
	unsigned int win_en;
	de_rect win;

} __peak_config_data;

#endif
