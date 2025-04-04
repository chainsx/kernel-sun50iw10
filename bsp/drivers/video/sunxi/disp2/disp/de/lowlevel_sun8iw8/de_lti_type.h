/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __DE_LTI_TYPE_H__
#define __DE_LTI_TYPE_H__

#include "de_rtmx.h"

typedef union {
	unsigned int dwval;
	struct {
		unsigned int en:1;	/* Default: 0x0; */
		unsigned int res0:7;	/* Default: ; */
		unsigned int sel:1;	/* Default: 0x0; */
		unsigned int res1:7;	/* Default: ; */
		unsigned int nonl_en:1;	/* Default: 0x0; */
		unsigned int res2:7;	/* Default: ; */
		unsigned int win_en:1;	/* Default: 0x0; */
		unsigned int res3:7;	/* Default: ; */

	} bits;
} LTI_EN;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int width:12;	/* Default: 0x0; */
		unsigned int res0:4;	/* Default: ; */
		unsigned int height:12;	/* Default: 0x0; */
		unsigned int res1:4;	/* Default: ; */
	} bits;
} LTI_SIZE;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int c0:8;	/* Default: 0x0; */
		unsigned int res0:8;	/* Default: ; */
		unsigned int c1:8;	/* Default: 0x0; */
		unsigned int res1:8;	/* Default: ; */
	} bits;
} LTI_FIR_COFF0;
typedef union {
	unsigned int dwval;
	struct {
		unsigned int c2:8;	/* Default: 0x0; */
		unsigned int res0:8;	/* Default: ; */
		unsigned int c3:8;	/* Default: 0x0; */
		unsigned int res1:8;	/* Default: ; */
	} bits;
} LTI_FIR_COFF1;
typedef union {
	unsigned int dwval;
	struct {
		unsigned int c4:8;	/* Default: 0x0; */
		unsigned int res0:24;	/* Default: ; */
	} bits;
} LTI_FIR_COFF2;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int lti_fil_gain:4;	/* Default: 0x0; */
		unsigned int res0:28;	/* Default: ; */

	} bits;
} LTI_FIR_GAIN;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int lti_cor_th:10;	/* Default: 0x0; */
		unsigned int res0:22;	/* Default: ; */
	} bits;
} LTI_COR_TH;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int offset:8;	/* Default: 0x0; */
		unsigned int res0:8;	/* Default: ; */
		unsigned int slope:5;	/* Default: 0x0; */
		unsigned int res1:11;	/* Default: ; */
	} bits;
} LTI_DIFF_CTL;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int edge_gain:5;	/* Default: 0x0; */
		unsigned int res0:27;	/* Default: ; */
	} bits;
} LTI_EDGE_GAIN;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int core_x:8;	/* Default: 0x0; */
		unsigned int res0:8;	/* Default: ; */
		unsigned int clip:8;	/* Default: 0x0; */
		unsigned int res1:4;	/* Default: ; */
		unsigned int peak_limit:3;	/* Default: 0x0; */
		unsigned int res2:1;	/* Default: ; */
	} bits;
} LTI_OS_CON;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int win_range:8;	/* Default: 0x0; */
		unsigned int res0:24;	/* Default: ; */
	} bits;
} LTI_WIN_EXPANSION;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int elvel_th:8;	/* Default: 0x0; */
		unsigned int res0:24;	/* Default: ; */
	} bits;
} LTI_EDGE_ELVEL_TH;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int win_left:12;	/* Default: 0x0; */
		unsigned int res0:4;	/* Default: ; */
		unsigned int win_top:12;	/* Default: 0x0; */
		unsigned int res1:4;	/* Default: ; */
	} bits;
} LTI_WIN0_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int win_right:12;	/* Default: 0x0; */
		unsigned int res0:4;	/* Default: ; */
		unsigned int win_bot:12;	/* Default: 0x0; */
		unsigned int res1:4;	/* Default: ; */
	} bits;
} LTI_WIN1_REG;

typedef struct {
	LTI_EN ctrl;		/* 0x0000 */
	unsigned int res0[2];	/* 0x0004-0x0008 */
	LTI_SIZE size;		/* 0x000c */
	LTI_FIR_COFF0 coef0;	/* 0x0010 */
	LTI_FIR_COFF1 coef1;	/* 0x0014 */
	LTI_FIR_COFF2 coef2;	/* 0x0018 */
	LTI_FIR_GAIN gain;	/* 0x001c */
	LTI_COR_TH corth;	/* 0x0020 */
	LTI_DIFF_CTL diff;	/* 0x0024 */
	LTI_EDGE_GAIN edge_gain;	/* 0x0028 */
	LTI_OS_CON os_con;	/* 0x002c */
	LTI_WIN_EXPANSION win_range;	/* 0x0030 */
	LTI_EDGE_ELVEL_TH elvel_th;	/* 0x0034 */
	LTI_WIN0_REG win0;	/* 0x0038 */
	LTI_WIN1_REG win1;	/* 0x003c */
} __lti_reg_t;

typedef struct {
	/* lti */
	unsigned int lti_en;
	unsigned int gain;

	/* window */
	unsigned int win_en;
	de_rect win;

} __lti_config_data;

#endif
