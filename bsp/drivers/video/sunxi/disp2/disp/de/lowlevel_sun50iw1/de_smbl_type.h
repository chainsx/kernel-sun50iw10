/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

/* ********************************************************************************************************************* */
/* All Winner Tech, All Right Reserved. 2014-2015 Copyright (c) */
/*  */
/* File name   :   de_smbl_type.h */
/*  */
/* Description :   display engine 2.0 smbl struct declaration */
/*  */
/* History     :   2014/05/13  vito cheng  v0.1  Initial version */
/*  */
/* ********************************************************************************************************************* */

#ifndef __DE_SMBL_TYPE_H__
#define __DE_SMBL_TYPE_H__

#include "de_rtmx.h"

#define SMBL_FRAME_MASK    0x00000002    /* 0x0: do SMBL in even frame; 0x1, do SMBL in odd frame; 0x2, do SMBL in all frames */

#define IEP_LH_INTERVAL_NUM 8
#define IEP_LH_PWRSV_NUM 24


typedef union {
	u32 dwval;
	struct {
		u32	en				:1;	/* bit0 */
		u32   incsc_en		:1;	/* bit1 */
		u32	r0				:2;	/* bit2~3 */
		u32   coef_switch_en  :1; /* bit4 */
		u32   r1				:3; /* bit5~7 */
		u32	mod				:2;	/* bit8~9 */
		u32	r2				:21;/* bit10~30 */
		u32	bist_en			:1;	/* bit31 */
	} bits;
} __imgehc_gnectl_reg_t;	/* 0x0 */

typedef union {
	u32 dwval;
	struct {
		u32 disp_w			:13; /* bit0~12	 12-03-29 */
		u32 r0				:3;	 /* bit13~15	12-03-29 */
		u32 disp_h			:13; /* bit16~28	12-03-29 */
		u32 r1				:3;  /* bit29~31	12-03-29 */
	} bits;
} __imgehc_drcsize_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32	db_en			:1;	/* bit0 */
		u32	r0				:7;	/* bit1~7 */
		u32	win_en			:1; /* bit8 */
		u32   hsv_en			:1;	/* bit9 */
		u32	r1				:22;/* bit10~31 */
	} bits;
} __imgehc_drcctl_reg_t;	/* 0x10 */

typedef union {
	u32 dwval;
	struct {
		u32	lgc_abslumshf	:1;	/* bit0 */
		u32	adjust_en		:1;	/* bit1 */
		u32	r0				:6;	/* bit2~7 */
		u32	lgc_abslumperval:8;	/* bit8~15 */
		u32	r1				:16;/* bit16~31 */
	} bits;
} __imgehc_drc_set_reg_t;		/* 0x18 */

typedef union {
	u32 dwval;
	struct {
		u32	win_left        :12;/* bit0~11 */
		u32	r0				:4;	/* bit12~15 */
		u32	win_top			:12;/* bit16~27 */
		u32	r1				:4;	/* bit28~31 */
	} bits;
} __imgehc_drc_wp_reg0_t;		/* 0x1c */

typedef union {
	u32 dwval;
	struct {
		u32	win_right		:12;/* bit0~11 */
		u32	r0				:4;	/* bit12~15 */
		u32	win_bottom		:12;/* bit16~27 */
		u32	r1				:4;	/* bit28~31 */
	} bits;
} __imgehc_drc_wp_reg1_t;		/* 0x20 */

typedef union {
	u32 dwval;
	struct {
		u32	lh_rec_clr		:1;	/* bit0 */
		u32	lh_mod			:1;	/* bit1 */
		u32	r0				:30;/* bit2~31 */
	} bits;
} __imgehc_lhctl_reg_t;		/* 0x30 */

typedef union {
	u32 dwval;
	struct {
		u32	lh_thres_val1	:8;	/* bit0~7 */
		u32	lh_thres_val2	:8;	/* bit8~15 */
		u32	lh_thres_val3	:8;	/* bit16~23 */
		u32	lh_thres_val4	:8;	/* bit24~31 */
	} bits;
} __imgehc_lhthr_reg0_t;		/* 0x34 */

typedef union {
	u32 dwval;
	struct {
		u32	lh_thres_val5	:8;	/* bit0~7 */
		u32	lh_thres_val6	:8;	/* bit8~15 */
		u32	lh_thres_val7	:8;	/* bit16~23 */
		u32	r0				:8;	/* bit24~31 */
	} bits;
} __imgehc_lhthr_reg1_t;		/* 0x38 */

typedef union {
	u32 dwval;
	struct {
		u32	lh_lum_data		:32;/* bit0~31 */
	} bits;
} __imgehc_lhslum_reg_t;		/* 0x0040 ~ 0x005c */

typedef union {
	u32 dwval;
	struct {
		u32	lh_cnt_data		:32;/* bit0~31 */
	} bits;
} __imgehc_lhscnt_reg_t;		/* 0x0060 ~ 0x007c */

typedef union {
	u32 dwval;
	struct {
		u32	csc_yg_coff		:13;/* bit0~12 */
		u32	r0				:19;/* bit13~31 */
	} bits;
} __imgehc_cscygcoff_reg_t;	/* 0xc0~0xc8 */

typedef union {
	u32 dwval;
	struct {
		u32	csc_yg_con		:14;/* bit0~13 */
		u32	r0				:18;/* bit14~31 */
	} bits;
} __imgehc_cscygcon_reg_t;	/* 0xcc */

typedef union {
	u32 dwval;
	struct {
		u32	csc_ur_coff		:13;/* bit0~12 */
		u32	r0				:19;/* bit13~31 */
	} bits;
} __imgehc_cscurcoff_reg_t;	/* 0xd0~0xd8 */

typedef union {
	u32 dwval;
	struct {
		u32	csc_ur_con		:14;/* bit0~13 */
		u32	r0				:18;/* bit14~31 */
	} bits;
} __imgehc_cscurcon_reg_t;	/* 0xdc */

typedef union {
	u32 dwval;
	struct {
		u32	csc_vb_coff		:13;/* bit0~12 */
		u32	r0				:19;/* bit13~31 */
	} bits;
} __imgehc_cscvbcoff_reg_t;	/* 0xe0~0xe8 */

typedef union {
	u32 dwval;
	struct {
		u32	csc_vb_con		:14;/* bit0~13 */
		u32	r0				:18;/* bit14~31 */
	} bits;
} __imgehc_cscvbcon_reg_t;	/* 0xec */

typedef union {
	u32 dwval;
	struct {
		u32	spa_coff0		:8;	/* bit0~7 */
		u32	spa_coff1		:8;	/* bit8~15 */
		u32	spa_coff2		:8;	/* bit16~23 */
		u32	r0				:8;	/* bit24~31 */
	} bits;
} __imgehc_drcspacoff_reg_t;		/* 0xf0~0xf8 */

typedef union {
	u32 dwval;
	struct {
		u32	inten_coff0		:8;	/* bit0~7 */
		u32	inten_coff1		:8;	/* bit8~15 */
		u32	inten_coff2		:8;	/* bit16~23 */
		u32	inten_coff3		:8;	/* bit24~31 */
	} bits;
} __imgehc_drcintcoff_reg_t;		/* 0x0100 ~ 0x01fc */

typedef union {
	u32 dwval;
	struct {
		u32	lumagain_coff0	:16;/* bit0~15 */
		u32	lumagain_coff1	:16;/* bit16~31 */
	} bits;
} __imgehc_drclgcoff_reg_t;		/* 0x0200 ~ 0x03fc */


typedef struct {
	__imgehc_gnectl_reg_t			gnectl;	/* 0x00 */
	__imgehc_drcsize_reg_t          drcsize;/* 0x04 */
	u32							r0[2];	/* 0x08~0x0c */
	__imgehc_drcctl_reg_t			drcctl;	/* 0x10 */
	u32							r1;		/* 0x14 */
	__imgehc_drc_set_reg_t			drc_set;/* 0x18 */
	__imgehc_drc_wp_reg0_t			drc_wp0;/* 0x1c */
	__imgehc_drc_wp_reg1_t			drc_wp1;/* 0x20 */
	u32                           r5[3];  /* 0x24~0x2c */
	__imgehc_lhctl_reg_t			lhctl;	/* 0x30 */
	__imgehc_lhthr_reg0_t			lhthr0;	/* 0x34 */
	__imgehc_lhthr_reg1_t			lhthr1;	/* 0x38 */
	u32							r2;		/* 0x3c */
	__imgehc_lhslum_reg_t			lhslum[8];	/* 0x40~0x5c */
	__imgehc_lhscnt_reg_t			lhscnt[8];	/* 0x0060 ~ 0x007c */
	__imgehc_cscygcoff_reg_t		incscycoff[3];	/* 0x80~0x88 */
	__imgehc_cscygcon_reg_t			incscycon;	/* 0x8c */
	__imgehc_cscurcoff_reg_t		incscucoff[3];	/* 0x90~0x98 */
	__imgehc_cscurcon_reg_t			incscucon;	/* 0x9c */
	__imgehc_cscvbcoff_reg_t		incscvcoff[3];	/* 0xa0~0xa8 */
	__imgehc_cscvbcon_reg_t			incscvcon;	/* 0xac */
	u32							r6[4];		/* 0xb0~0xbc */
	__imgehc_cscygcoff_reg_t		cscrcoff[3];	/* 0xc0~0xc8 */
	__imgehc_cscygcon_reg_t			cscrcon;	/* 0xcc */
	__imgehc_cscurcoff_reg_t		cscgcoff[3];	/* 0xd0~0xd8 */
	__imgehc_cscurcon_reg_t			cscgcon;	/* 0xdc */
	__imgehc_cscvbcoff_reg_t		cscbcoff[3];	/* 0xe0~0xe8 */
	__imgehc_cscvbcon_reg_t			cscbcon;	/* 0xec */
	__imgehc_drcspacoff_reg_t		drcspacoff[3];	/* 0xf0~0xf8 */
	u32							r4;			/* 0xfc */
	__imgehc_drcintcoff_reg_t		drcintcoff[64];/* 0x0100 ~ 0x01fc */
	__imgehc_drclgcoff_reg_t		drclgcoff[128];/* 0x0200 ~ 0x03fc */
} __smbl_reg_t;

typedef struct {
	unsigned int IsEnable;
	unsigned int Runtime;
	unsigned int backlight;
	unsigned int dimming;
	unsigned char min_adj_index_hist[IEP_LH_PWRSV_NUM];
	unsigned int size; /* size = width*height/100; */
} __smbl_status_t;

#endif
