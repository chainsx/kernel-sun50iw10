/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#ifndef __DE_WB_TYPE_H__
#define __DE_WB_TYPE_H__

#define ____SEPARATOR_DEFEINE____
#define WB_END_IE				0x1
#define WB_FINISH_IE (0x1<<4)
#define WB_FIFO_OVERFLOW_ERROR_IE (0x1<<5)
#define WB_TIMEOUT_ERROR_IE (0x1<<6)

#define MININWIDTH 8
#define MININHEIGHT 4
#define MAXINWIDTH 4096		/* support 8192,limit by LCD */
#define MAXINHEIGHT 4096	/* support 8192,limit by LCD */
#define LINE_BUF_LEN 2048
#define LOCFRACBIT 18
#define SCALERPHASE 16

#define ____SEPARATOR_REGISTERS____
typedef union {
	unsigned int dwval;
	struct {
		unsigned int wb_start:1;	/* bit0 */
		unsigned int r0:3;	/* bit1~3 */
		unsigned int soft_reset:1;	/* bit4 */
		unsigned int r1:11;	/* bit5~15 */
		unsigned int in_port_sel:2;	/* bit16~17 */
		unsigned int r2:10;	/* bit18~27 */
		unsigned int auto_gate_en:1;	/* bit28 */
		unsigned int clk_gate:1;	/* bit29 */
		unsigned int r3:1;	/* bit30 */
		unsigned int bist_en:1;	/* bit31 */
	} bits;
} __wb_gctrl_reg_t;		/* 0x00 */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int width:13;	/* bit0~12 */
		unsigned int r0:3;	/* bit13~15 */
		unsigned int height:13;	/* bit16~28 */
		unsigned int r1:3;	/* bit29~31 */
	} bits;
} __wb_size_reg_t;		/* 0x04 */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int crop_left:13;	/* bit0~12 */
		unsigned int r0:3;	/* bit13~15 */
		unsigned int crop_top:13;	/* bit16~28 */
		unsigned int r1:3;	/* bit29~31 */
	} bits;
} __wb_crop_coord_reg_t;	/* 0x08 */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int crop_width:13;	/* bit0~12 */
		unsigned int r0:3;	/* bit13~15 */
		unsigned int crop_height:13;	/* bit16~28 */
		unsigned int r1:3;	/* bit29~31 */
	} bits;
} __wb_crop_size_reg_t;		/* 0x0c */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int addr:32;	/* bit0~31 */
	} bits;
} __wb_addr_reg_t;		/* 0x10 0x14 0x18 0x20 0x24 0x28 */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int ch0_h_addr:8;	/* bit0~7 */
		unsigned int ch1_h_addr:8;	/* bit8~15 */
		unsigned int ch2_h_addr:8;	/* bit16~23 */
		unsigned int r0:8;	/* bit24~31 */
	} bits;
} __wb_high_addr_reg_t;		/* 0x1c 0x2c */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int pitch:32;	/* bit0~31 */
	} bits;
} __wb_pitch_reg_t;		/* 0x30 0x34 */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int cur_group:1;	/* bit0 */
		unsigned int r0:15;	/* bit1~15 */
		unsigned int auto_switch:1;	/* bit16 */
		unsigned int r1:3;	/* bit17~19 */
		unsigned int manual_group:1;	/* bit20 */
		unsigned int r2:11;	/* bit21-31 */
	} bits;
} __wb_addr_switch_reg_t;	/* 0x40 */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int format:4;	/* bit0~3 */
		unsigned int r0:28;	/* bit4~31 */
	} bits;
} __wb_format_reg_t;		/* 0x44 */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int int_en:1;	/* bit0 */
		unsigned int r0:31;	/* bit1~31 */
	} bits;
} __wb_int_reg_t;		/* 0x48 */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int irq:1;	/* bit0 */
		unsigned int r0:3;	/* bit1~3 */
		unsigned int finish:1;	/* bit4 */
		unsigned int overflow:1;	/* bit5 */
		unsigned int timeout:1;	/* bit6 */
		unsigned int r1:1;	/* bit7 */
		unsigned int busy:1;	/* bit8 */
		unsigned int r2:23;	/* bit9~31 */
	} bits;
} __wb_status_reg_t;		/* 0x4c */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int csc_en:1;	/* bit0 */
		unsigned int cs_en:1;	/* bit1 */
		unsigned int fs_en:1;	/* bit2 */
		unsigned int r0:29;	/* bit3~31 */
	} bits;
} __wb_bypass_reg_t;		/* 0x54 */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int m:13;	/* bit0~12 */
		unsigned int r1:3;	/* bit13~15 */
		unsigned int n:13;	/* bit16~28 */
		unsigned int r0:3;	/* bit29~31 */
	} bits;
} __wb_cs_reg_t;		/* 0x70 0x74 */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int width:13;	/* bit0~12 */
		unsigned int r1:3;	/* bit13~15 */
		unsigned int height:13;	/* bit16~28 */
		unsigned int r0:3;	/* bit29~31 */
	} bits;
} __wb_fs_size_reg_t;		/* 0x80 0x84 */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int r1:2;	/* bit0~1 */
		unsigned int frac:18;	/* bit2~19 */
		unsigned int intg:2;	/* bit20~21 */
		unsigned int r0:10;	/* bit22~31 */
	} bits;
} __wb_fs_step_reg_t;		/* 0x88 0x8c */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int coef0:8;	/* bit0~7 */
		unsigned int coef1:8;	/* bit8~15 */
		unsigned int coef2:8;	/* bit16~23 */
		unsigned int coef3:8;	/* bit24~31 */
	} bits;
} __wb_coeff_reg_t;		/* 0x200+N*4    0x280+N*4   (N = 0~15) */

typedef struct {
	__wb_gctrl_reg_t gctrl;	/* 0x0000 */
	__wb_size_reg_t size;	/* 0x0004 */
	__wb_crop_coord_reg_t crop_coord;	/* 0x0008 */
	__wb_crop_size_reg_t crop_size;	/* 0x000c */
	__wb_addr_reg_t wb_addr_A0;	/* 0x0010 */
	__wb_addr_reg_t wb_addr_A1;	/* 0x0014 */
	__wb_addr_reg_t wb_addr_A2;	/* 0x0018 */
	__wb_high_addr_reg_t wb_addr_AH;	/* 0x001c */
	__wb_addr_reg_t wb_addr_B0;	/* 0x0020 */
	__wb_addr_reg_t wb_addr_B1;	/* 0x0024 */
	__wb_addr_reg_t wb_addr_B2;	/* 0x0028 */
	__wb_high_addr_reg_t wb_addr_BH;	/* 0x002c */
	__wb_pitch_reg_t wb_pitch0;	/* 0x0030 */
	__wb_pitch_reg_t wb_pitch1;	/* 0x0034 */
	unsigned int res0[2];	/* 0x0038-0x003c */
	__wb_addr_switch_reg_t addr_switch;	/* 0x0040 */
	__wb_format_reg_t fmt;	/* 0x0044 */
	__wb_int_reg_t intr;	/* 0x0048 */
	__wb_status_reg_t status;	/* 0x004c */
	unsigned int res1;	/* 0x0050 */
	__wb_bypass_reg_t bypass;	/* 0x0054 */
	unsigned int res2[6];	/* 0x0058-0x006c */
	__wb_cs_reg_t cs_horz;	/* 0x0070 */
	__wb_cs_reg_t cs_vert;	/* 0x0074 */
	unsigned int res3[2];	/* 0x0078-0x007c */
	__wb_fs_size_reg_t fs_insize;	/* 0x0080 */
	__wb_fs_size_reg_t fs_outsize;	/* 0x0084 */
	__wb_fs_step_reg_t fs_hstep;	/* 0x0088 */
	__wb_fs_step_reg_t fs_vstep;	/* 0x008c */
	unsigned int res4[92];	/* 0x0090-0x01fc */
	__wb_coeff_reg_t yhcoeff[16];	/* 0X0200-0x23c */
	unsigned int res5[16];	/* 0X0240-0x27c */
	__wb_coeff_reg_t chcoeff[16];	/* 0X0280-0x2bc */
	unsigned int res6[16];	/* 0x02c0-0x02fc */
} __wb_reg_t;

typedef enum {
	WB_FORMAT_RGB_888 = 0x0,
	WB_FORMAT_BGR_888 = 0x1,
	WB_FORMAT_ARGB_8888 = 0x4,
	WB_FORMAT_ABGR_8888 = 0x5,
	WB_FORMAT_BGRA_8888 = 0x6,
	WB_FORMAT_RGBA_8888 = 0x7,
	WB_FORMAT_YUV420_P = 0x8,
	WB_FORMAT_YUV420_SP_VUVU = 0xc,
	WB_FORMAT_YUV420_SP_UVUV = 0xd,
} wb_output_fmt;

extern unsigned int wb_lan2coefftab16[16];
extern unsigned int wb_lan2coefftab16_down[16];

#endif
