/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __DE_CSC_TYPE_H__
#define __DE_CSC_TYPE_H__

typedef union {
	unsigned int dwval;
	struct {
		unsigned int enable:1;	/* Default : 0x0; */
		unsigned int res0:31;	/* Default : ; */
	} bits;
} CSC_BYPASS_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int res0:1;	/* Default : ; */
		unsigned int enable:1;	/* Default : 0x0; */
		unsigned int res1:30;	/* Default : ; */
	} bits;
} CSC_BYPASS_REG2;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int coeff:13;	/* Default: 0x0; */
		unsigned int res0:19;	/* Default: ; */
	} bits;
} CSC_COEFF_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int cnst:20;	/* Default: 0x0; */
		unsigned int res0:12;	/* Default: ; */
	} bits;
} CSC_CONST_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int cnst:14;	/* Default: 0x0; */
		unsigned int res0:18;	/* Default: ; */
	} bits;
} CSC_CONST_REG2;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int cnst:24;	/* Default:; */
		unsigned int alpha:8;	/* Default: 0xff; */
	} bits;
} GLB_ALPHA_REG;

/* Channel CSC and Device CSC */
typedef struct {
	CSC_BYPASS_REG bypass;
	unsigned int res[3];
	CSC_COEFF_REG c00;
	CSC_COEFF_REG c01;
	CSC_COEFF_REG c02;
	CSC_CONST_REG c03;
	CSC_COEFF_REG c10;
	CSC_COEFF_REG c11;
	CSC_COEFF_REG c12;
	CSC_CONST_REG c13;
	CSC_COEFF_REG c20;
	CSC_COEFF_REG c21;
	CSC_COEFF_REG c22;
	CSC_CONST_REG c23;
	GLB_ALPHA_REG alpha;
} __csc_reg_t;

/* CSC IN SMBL */
typedef struct {
	CSC_BYPASS_REG2 bypass;
	unsigned int res[31];
	CSC_COEFF_REG c00;
	CSC_COEFF_REG c01;
	CSC_COEFF_REG c02;
	CSC_CONST_REG2 c03;
	CSC_COEFF_REG c10;
	CSC_COEFF_REG c11;
	CSC_COEFF_REG c12;
	CSC_CONST_REG2 c13;
	CSC_COEFF_REG c20;
	CSC_COEFF_REG c21;
	CSC_COEFF_REG c22;
	CSC_CONST_REG2 c23;
} __csc2_reg_t;

/* Input CSC in FCE */
typedef struct {
	CSC_BYPASS_REG bypass;
} __icsc_reg_t;
#endif
