/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __DE_FCC_TYPE__
#define __DE_FCC_TYPE__

#include "de_rtmx.h"

typedef union {
	unsigned int dwval;
	struct {
		unsigned int en:1;	/* Default: 0x0; */
		unsigned int res0:7;	/* Default: ; */
		unsigned int win_en:1;	/* Default: 0x0; */
		unsigned int res1:23;	/* Default: ; */
	} bits;
} FCC_CTRL_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int width:13;	/* Default: 0x0; */
		unsigned int res0:3;	/* Default: ; */
		unsigned int height:13;	/* Default: 0x0; */
		unsigned int res1:3;	/* Default: ; */
	} bits;
} FCC_SIZE_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int left:13;	/* Default: 0x0; */
		unsigned int res0:3;	/* Default: ; */
		unsigned int top:13;	/* Default: 0x0; */
		unsigned int res1:3;	/* Default: ; */
	} bits;
} FCC_WIN0_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int right:13;	/* Default: 0x0; */
		unsigned int res0:3;	/* Default: ; */
		unsigned int bot:13;	/* Default: 0x0; */
		unsigned int res1:3;	/* Default: ; */
	} bits;
} FCC_WIN1_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int hmin:12;	/* Default: 0x0; */
		unsigned int res0:4;	/* Default: ; */
		unsigned int hmax:12;	/* Default: 0x0; */
		unsigned int res1:4;	/* Default: ; */
	} bits;
} FCC_HUE_RANGE_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int sgain:9;	/* Default: 0x0; */
		unsigned int res0:7;	/* Default: ; */
		unsigned int hgain:9;	/* Default: 0x0; */
		unsigned int res1:7;	/* Default: ; */
	} bits;
} FCC_HS_GAIN_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int bypass:1;	/* Default: 0x0; */
		unsigned int res0:31;	/* Default: ; */
	} bits;
} FCC_CSC_CTL_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int coff:13;	/* Default: 0x0; */
		unsigned int res0:19;	/* Default: ; */
	} bits;
} FCC_CSC_COEFF_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int cont:20;	/* Default: 0x0; */
		unsigned int res0:12;	/* Default: ; */
	} bits;
} FCC_CSC_CONST_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int res0:24;	/* Default: 0x0; */
		unsigned int alpha:8;	/* Default: 0xff; */
	} bits;
} FCC_GLB_APH_REG;

typedef struct {
	FCC_CTRL_REG fcc_ctl;	/* 0x00 */
	FCC_SIZE_REG fcc_size;	/* 0x04 */
	FCC_WIN0_REG fcc_win0;	/* 0x08 */
	FCC_WIN1_REG fcc_win1;	/* 0x0c */
	FCC_HUE_RANGE_REG fcc_range[6];	/* 0x10-0x24 */
	unsigned int res0[2];	/* 0x28-0x2c */
	FCC_HS_GAIN_REG fcc_gain[6];	/* 0x30-0x44 */
	unsigned int res1[2];	/* 0x48-0x4c */
	FCC_CSC_CTL_REG fcc_csc_ctl;	/* 0x50 */
	unsigned int res2[3];	/* 0x54-0x5c */
	FCC_CSC_COEFF_REG fcc_csc_coff0[3];	/* 0x60-0x68 */
	FCC_CSC_CONST_REG fcc_csc_const0;	/* 0x6c */
	FCC_CSC_COEFF_REG fcc_csc_coff1[3];	/* 0x70-0x78 */
	FCC_CSC_CONST_REG fcc_csc_const1;	/* 0x7c */
	FCC_CSC_COEFF_REG fcc_csc_coff2[3];	/* 0x80-0x88 */
	FCC_CSC_CONST_REG fcc_csc_const2;	/* 0x8c */
	FCC_GLB_APH_REG fcc_glb_alpha;	/* 0x90 */

} __fcc_reg_t;

typedef struct {
	/* ase */
	unsigned int fcc_en;
	unsigned int sgain[6];

	/* window */
	unsigned int win_en;
	de_rect win;
} __fcc_config_data;

#endif
