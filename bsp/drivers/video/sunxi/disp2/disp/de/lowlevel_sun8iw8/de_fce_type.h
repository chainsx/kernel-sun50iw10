/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __DE_FCE_TYPE_H__
#define __DE_FCE_TYPE_H__

#include "de_rtmx.h"

#define HIST_FRAME_MASK  0x00000002	/* 0x0: do hist in even frame; 0x1, do hist in odd frame; 0x2, do hist in all frames */
#define CE_FRAME_MASK    0x00000002	/* 0x0: do CE in even frame; 0x1, do CE in odd frame; 0x2, do CE in all frames */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int en:1;	/* Default: 0x0; */
		unsigned int res0:15;	/* Default: ; */
		unsigned int hist_en:1;	/* Default: 0x0; */
		unsigned int ce_en:1;	/* Default: 0x0; */
		unsigned int lce_en:1;	/* Default: 0x0; */
		unsigned int res1:1;	/* Default: 0x0; */
		unsigned int ftc_en:1;	/* Default: 0x0; */
		unsigned int res2:10;	/* Default: 0x0; */
		unsigned int win_en:1;	/* Default: 0x0; */
	} bits;
} FCE_GCTRL_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int width:12;	/* Default: 0x0; */
		unsigned int res0:4;	/* Default: ; */
		unsigned int height:12;	/* Default: 0x0; */
		unsigned int res1:4;	/* Default: ; */
	} bits;
} FCE_SIZE_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int win_left:12;	/* Default: 0x0; */
		unsigned int res0:4;	/* Default: ; */
		unsigned int win_top:12;	/* Default: 0x0; */
		unsigned int res1:4;	/* Default: ; */
	} bits;
} FCE_WIN0_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int win_right:12;	/* Default: 0x0; */
		unsigned int res0:4;	/* Default: ; */
		unsigned int win_bot:12;	/* Default: 0x0; */
		unsigned int res1:4;	/* Default: ; */
	} bits;
} FCE_WIN1_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int lce_gain:6;	/* Default: 0x0; */
		unsigned int res0:2;	/* Default: ; */
		unsigned int lce_blend:8;	/* Default: 0x0; */
		unsigned int res1:16;	/* Default: ; */
	} bits;
} LCE_GAIN_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int sum:32;	/* Default: 0x0; */
	} bits;
} HIST_SUM_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int hist_valid:1;	/* Default: 0x0; */
		unsigned int res0:31;	/* Default: ; */
	} bits;
} HIST_STATUS_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int celut_access_switch:1;	/* Default: 0x0; */
		unsigned int res0:31;	/* Default: ; */
	} bits;
} CE_STATUS_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int ftc_gain1:8;	/* Default: 0x0; */
		unsigned int res0:8;	/* Default: ; */
		unsigned int ftc_gain2:8;	/* Default: 0x0; */
		unsigned int res1:8;	/* Default: ; */
	} bits;
} FTC_GAIN_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int csc_bypass:1;	/* Default: 0x0; */
		unsigned int res0:31;	/* Default: ; */
	} bits;
} FTC_CSCBYPASS_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int lut0:8;	/* Default: 0x0; */
		unsigned int lut1:8;	/* Default: 0x0; */
		unsigned int lut2:8;	/* Default: 0x0; */
		unsigned int lut3:8;	/* Default: 0x0; */
	} bits;
} CE_LUT_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int hist:22;	/* Default: 0x0; */
		unsigned int res0:10;	/* Default: ; */
	} bits;
} HIST_CNT_REG;

typedef struct {
	FCE_GCTRL_REG ctrl;	/* 0x0000 */
	FCE_SIZE_REG size;	/* 0x0004 */
	FCE_WIN0_REG win0;	/* 0x0008 */
	FCE_WIN1_REG win1;	/* 0x000c */
	LCE_GAIN_REG lcegain;	/* 0x0010 */
	unsigned int res0[3];	/* 0x0014-0x001c */
	HIST_SUM_REG histsum;	/* 0x0020 */
	HIST_STATUS_REG histstauts;	/* 0x0024 */
	CE_STATUS_REG cestatus;	/* 0x0028 */
	unsigned int res1;	/* 0x002c */
	FTC_GAIN_REG ftcgain;	/* 0x0030 */
	unsigned int res2[3];	/* 0x0034-0x003c */
	FTC_CSCBYPASS_REG cscbypass;	/* 0x0040 */
	unsigned int res3[47];	/* 0x0044-0x00fc */
	CE_LUT_REG celut[64];	/* 0x0100-0x01fc */
	HIST_CNT_REG hist[256];	/* 0x0200-0x05fc */
} __fce_reg_t;

typedef struct {
	/* global */
	unsigned int fce_en;

	/* ce */
	unsigned int ce_en;
	unsigned int up_precent_thr;
	unsigned int down_precent_thr;

	/* lce */
	unsigned int lce_en;

	/* ftc */
	unsigned int ftc_en;

	/* hist */
	unsigned int hist_en;

	/* window */
	unsigned int win_en;
	de_rect win;
} __fce_config_data;

typedef struct {
	unsigned int Runtime;	/* Frame number of Histogram run */
	unsigned int IsEnable;	/* Histogram enabled */
	unsigned int TwoHistReady;	/* Already get histogram of two frames */
} __hist_status_t;

typedef struct {
	unsigned int IsEnable;	/* CE enabled */
	unsigned int width;
	unsigned int height;
} __ce_status_t;

#endif
