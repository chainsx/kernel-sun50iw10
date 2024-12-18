/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

/*******************************************************************************
 *  All Winner Tech, All Right Reserved. 2014-2015 Copyright (c)
 *
 *  File name   :       de_fcc_type.h
 *
 *  Description :       display engine 2.0 fcc base struct declaration
 *
 *  History     :       2014/03/28  iptang  v0.1  Initial version
 *
 ******************************************************************************/

#ifndef __DE_FCC_TYPE__
#define __DE_FCC_TYPE__

#include "de_rtmx.h"

union __fcc_ctrl_reg_t {
	unsigned int dwval;
	struct {
		unsigned int en:1;
		unsigned int r0:3;
		unsigned int skin_en:1;
		unsigned int sat_en:1;
		unsigned int light_en:1;
		unsigned int r1:1;
		unsigned int win_en:1;
		unsigned int r2:23;
	} bits;
};

union __fcc_size_reg_t {
	unsigned int dwval;
	struct {
		unsigned int width:13;
		unsigned int r0:3;
		unsigned int height:13;
		unsigned int r1:3;
	} bits;
};

union __fcc_win0_reg_t {
	unsigned int dwval;
	struct {
		unsigned int left:13;
		unsigned int r0:3;
		unsigned int top:13;
		unsigned int r1:3;
	} bits;
};

union __fcc_win1_reg_t {
	unsigned int dwval;
	struct {
		unsigned int right:13;
		unsigned int r0:3;
		unsigned int bot:13;
		unsigned int r1:3;
	} bits;
};

union __fcc_hue_range_reg_t {
	unsigned int dwval;
	struct {
		unsigned int hmin:12;
		unsigned int r0:4;
		unsigned int hmax:12;
		unsigned int r1:4;
	} bits;
};

union __fcc_hs_gain_reg_t {
	unsigned int dwval;
	struct {
		unsigned int sgain:9;
		unsigned int r0:7;
		unsigned int hgain:9;
		unsigned int r1:7;
	} bits;
};

union __fcc_sat_gain_reg_t {
	unsigned int dwval;
	struct {
		unsigned int sgain:9;
		unsigned int r0:23;
	} bits;
};

union __fcc_color_gain_reg_t {
	unsigned int dwval;
	struct {
		unsigned int sb:4;
		unsigned int sg:4;
		unsigned int sr:4;
		unsigned int r0:20;
	} bits;
};

union __fcc_lut_ctrl_reg_t {
	unsigned int dwval;
	struct {
		unsigned int access:1;
		unsigned int r0:31;
	} bits;
};

union __fcc_light_ctrl_reg_t {
	unsigned int dwval;
	struct {
		unsigned int th0:9;
		unsigned int r0:7;
		unsigned int th1:9;
		unsigned int r1:7;
	} bits;
};

struct __fcc_reg_t {
	union __fcc_ctrl_reg_t ctl;	                /* 0x00 */
	union __fcc_size_reg_t size;	                /* 0x04 */
	union __fcc_win0_reg_t win0;	                /* 0x08 */
	union __fcc_win1_reg_t win1;	                /* 0x0c */
	union __fcc_hue_range_reg_t hue_range[6];       /* 0x10-0x24 */
	unsigned int r0[2];	                        /* 0x28-0x2c */
	union __fcc_hs_gain_reg_t hue_gain[6];	        /* 0x30-0x44 */
	union __fcc_sat_gain_reg_t sat_gain;	        /* 0x48 */
	union __fcc_color_gain_reg_t color_gain;	/* 0x4c */
	union __fcc_lut_ctrl_reg_t lut_ctrl;	        /* 0x50 */
	union __fcc_light_ctrl_reg_t light_ctrl;	/* 0x54 */
};

union __fcc_gain_lut_reg_t {
	unsigned int dwval;
	struct {
		unsigned int lut0:10;
		unsigned int r0:6;
		unsigned int lut1:10;
		unsigned int r1:6;
	} bits;
};

struct __fcc_lut_reg_t {
	union __fcc_gain_lut_reg_t lut[256];	        /* 0x100 */
};

struct __fcc_config_data {

	unsigned int level;
};

#endif
