/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __DE_BWS_TYPE_H__
#define __DE_BWS_TYPE_H__

#include "de_rtmx.h"

#define BWS_FRAME_MASK	0x00000002	/* 0x0: do bws in odd frame; 0x1, do bws in even frame; 0x2, do bws in all frames */
#define BWS_DEFAULT_SLOPE 0x100

typedef union {
	unsigned int dwval;
	struct {
		unsigned int en:1;	/* Default: 0x0; */
		unsigned int res:30;	/* Default: ; */
		unsigned int win_en:1;	/* Default: 0x0; */
	} bits;
} BWS_CTRL_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int width:12;	/* Default: 0x0; */
		unsigned int res0:4;	/* Default: ; */
		unsigned int height:12;	/* Default: 0x0; */
		unsigned int res1:4;	/* Default: ; */
	} bits;
} BWS_SIZE_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int win_left:12;	/* Default: 0x0; */
		unsigned int res0:4;	/* Default: ; */
		unsigned int win_top:12;	/* Default: 0x0; */
		unsigned int res1:4;	/* Default: ; */
	} bits;
} BWS_WIN0_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int win_right:12;	/* Default: 0x0; */
		unsigned int res0:4;	/* Default: ; */
		unsigned int win_bot:12;	/* Default: 0x0; */
		unsigned int res1:4;	/* Default: ; */
	} bits;
} BWS_WIN1_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int min:8;	/* Default: 0x0; */
		unsigned int res0:8;	/* Default: ; */
		unsigned int black:8;	/* Default: 0x0; */
		unsigned int res1:8;	/* Default: ; */
	} bits;
} BWS_LS_THR0_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int white:8;	/* Default: 0x0; */
		unsigned int res0:8;	/* Default: ; */
		unsigned int max:8;	/* Default: 0x0; */
		unsigned int res1:8;	/* Default: ; */
	} bits;
} BWS_LS_THR1_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int slope0:10;	/* Default: 0x0; */
		unsigned int res0:6;	/* Default: ; */
		unsigned int slope1:10;	/* Default: 0x0; */
		unsigned int res1:6;	/* Default: ; */
	} bits;
} BWS_LS_SLP0_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int slope2:10;	/* Default: 0x0; */
		unsigned int res0:6;	/* Default: ; */
		unsigned int slope3:10;	/* Default: 0x0; */
		unsigned int res1:6;	/* Default: ; */
	} bits;
} BWS_LS_SLP1_REG;

typedef struct {
	BWS_CTRL_REG ctrl;	/* 0x0000 */
	BWS_SIZE_REG size;	/* 0x0004 */
	BWS_WIN0_REG win0;	/* 0x0008 */
	BWS_WIN1_REG win1;	/* 0x000c */
	unsigned int res0[4];	/* 0x0010-0x001c */
	BWS_LS_THR0_REG blkthr;	/* 0x0020 */
	BWS_LS_THR1_REG whtthr;	/* 0x0024 */
	BWS_LS_SLP0_REG blkslp;	/* 0x0028 */
	BWS_LS_SLP1_REG whtslp;	/* 0x002c */
} __bws_reg_t;

typedef struct {
	/* bws */
	unsigned int bws_en;
	unsigned int bld_high_thr;
	unsigned int bld_low_thr;
	unsigned int bld_weight_lmt;
	unsigned int present_black;
	unsigned int present_white;
	unsigned int slope_black_lmt;
	unsigned int slope_white_lmt;
	unsigned int black_prec;
	unsigned int white_prec;
	unsigned int lowest_black;
	unsigned int highest_white;

	/* window */
	unsigned int win_en;
	de_rect win;

} __bws_config_data;

typedef struct {
	unsigned int IsEnable;	/* BWS enabled */
	unsigned int Runtime;	/* Frame number of BWS run */
	unsigned int PreSlopeReady;	/* Get two slope */
	unsigned int width;
	unsigned int height;
	unsigned int slope_black;
	unsigned int slope_white;
} __bws_status_t;
#endif
