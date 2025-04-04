/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __DE_VSU_TYPE_H__
#define __DE_VSU_TYPE_H__

/* //////////////////////////////////////////////////////////////////////// */
/* //////////////////////////////////////////////////////////////////////// */
/* ////                      __vsu_reg_t                          ///////// */
/* //////////////////////////////////////////////////////////////////////// */
/* //////////////////////////////////////////////////////////////////////// */
typedef union {
	unsigned int dwval;
	struct {
		unsigned int en:1;	/* Default: 0x0; */
		unsigned int res0:3;	/* Default: ; */
		unsigned int coef_switch_rdy:1;	/* Default: 0x0; */
		unsigned int res1:25;	/* Default: ; */
		unsigned int reset:1;	/* Default: 0x0; */
		unsigned int bist:1;	/* Default: 0x0; */
	} bits;
} VSU_CTRL_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int res0:4;	/* Default: ; */
		unsigned int busy:1;	/* Default: 0x0; */
		unsigned int res1:11;	/* Default: ; */
		unsigned int line_cnt:12;	/* Default: 0x0; */
		unsigned int res2:4;	/* Default: ; */
	} bits;
} VSU_STATUS_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int vphase_sel_en:1;	/* Default: 0x0; */
		unsigned int res0:31;	/* Default: ; */
	} bits;
} VSU_FIELD_CTRL_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int width:13;	/* Default: 0x0; */
		unsigned int res0:3;	/* Default: ; */
		unsigned int height:13;	/* Default: 0x0; */
		unsigned int res1:3;	/* Default: ; */
	} bits;
} VSU_OUTSIZE_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int width:13;	/* Default: 0x0; */
		unsigned int res0:3;	/* Default: ; */
		unsigned int height:13;	/* Default: 0x0; */
		unsigned int res1:3;	/* Default: ; */
	} bits;
} VSU_INSIZE_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int res0:1;	/* Default: ; */
		unsigned int frac:19;	/* Default: 0x0; */
		unsigned int integer:4;	/* Default: 0x0; */
		unsigned int res1:8;	/* Default: ; */
	} bits;
} VSU_HSTEP_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int res0:1;	/* Default: ; */
		unsigned int frac:19;	/* Default: 0x0; */
		unsigned int integer:4;	/* Default: 0x0; */
		unsigned int res1:8;	/* Default: ; */
	} bits;
} VSU_VSTEP_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int res0:1;	/* Default: ; */
		unsigned int frac:19;	/* Default: 0x0; */
		unsigned int integer:4;	/* Default: 0x0; */
		unsigned int res1:8;	/* Default: ; */
	} bits;
} VSU_HPHASE_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int res0:1;	/* Default: ; */
		unsigned int frac:19;	/* Default: 0x0; */
		unsigned int integer:4;	/* Default: 0x0; */
		unsigned int res1:8;	/* Default: ; */
	} bits;
} VSU_VPHASE0_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int res0:1;	/* Default: ; */
		unsigned int frac:19;	/* Default: 0x0; */
		unsigned int integer:4;	/* Default: 0x0; */
		unsigned int res1:8;	/* Default: ; */
	} bits;
} VSU_VPHASE1_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int coef0:8;	/* Default: 0x0; */
		unsigned int coef1:8;	/* Default: 0x0; */
		unsigned int coef2:8;	/* Default: 0x0; */
		unsigned int coef3:8;	/* Default: 0x0; */
	} bits;
} VSU_COEFF_REG;

typedef struct {
	VSU_CTRL_REG ctrl;	/* 0x0000 */
	unsigned int res0;	/* 0x0004 */
	VSU_STATUS_REG status;	/* 0x0008 */
	VSU_FIELD_CTRL_REG field;	/* 0x000c */
	unsigned int res1[12];	/* 0x0010-3c */
	VSU_OUTSIZE_REG outsize;	/* 0x0040 */
	unsigned int res13[15];	/* 0x0044-0x07c */
	VSU_INSIZE_REG ysize;	/* 0x0080 */
	unsigned int res2;	/* 0x0084 */
	VSU_HSTEP_REG yhstep;	/* 0x0088 */
	VSU_VSTEP_REG yvstep;	/* 0x008C */
	VSU_HPHASE_REG yhphase;	/* 0x0090 */
	unsigned int res3;	/* 0x0094 */
	VSU_VPHASE0_REG yvphase0;	/* 0x0098 */
	VSU_VPHASE1_REG yvphase1;	/* 0x009c */
	unsigned int res4[8];	/* 0x00a0-bc */
	VSU_INSIZE_REG csize;	/* 0x00c0 */
	unsigned int res5;	/* 0x00c4 */
	VSU_HSTEP_REG chstep;	/* 0x00c8 */
	VSU_VSTEP_REG cvstep;	/* 0x00cC */
	VSU_HPHASE_REG chphase;	/* 0x00d0 */
	unsigned int res6;	/* 0x00d4 */
	VSU_VPHASE0_REG cvphase0;	/* 0x00d8 */
	VSU_VPHASE1_REG cvphase1;	/* 0x00dc */
	unsigned int res7[72];	/* 0x00e0-0x1fc */
	VSU_COEFF_REG yhcoeff0[32];	/* 0X0200-0x27c */
	unsigned int res8[32];	/* 0X0280-0x2fc */
	VSU_COEFF_REG yhcoeff1[32];	/* 0X0300-0x37c */
	unsigned int res9[32];	/* 0X0380-0x3fc */
	VSU_COEFF_REG yvcoeff[32];	/* 0X0400-0x47c */
	unsigned int res10[96];	/* 0x0480-0x5fc */
	VSU_COEFF_REG chcoeff0[32];	/* 0X0600-0x67c */
	unsigned int res11[32];	/* 0X0680-0x6fc */
	VSU_COEFF_REG chcoeff1[32];	/* 0X0700-0x77c */
	unsigned int res12[32];	/* 0X0780-0x7fc */
	VSU_COEFF_REG cvcoeff[32];	/* 0X0800-0x87c */

} __vsu_reg_t;

#endif
