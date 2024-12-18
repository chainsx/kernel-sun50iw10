/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2019 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 *
 *	File name   :       de_wb_reg.h
 *
 *	Description :       eink engine 2.0 wbc struct declaration
 *
 *	History     :       2019/03/20 liuli   initial version
 *
 */
#ifndef __DE_WB_REG_H__
#define __DE_WB_REG_H__
#include "../include/eink_driver.h"
#include "../include/eink_sys_source.h"

#define ____SEPARATOR_DEFEINE____
#define WB_END_IE	0x1
#define WB_FINISH_IE	(0x1<<4)
#define WB_FIFO_OVERFLOW_ERROR_IE	(0x1<<5)
#define WB_TIMEOUT_ERROR_IE	(0x1<<6)

#define MININWIDTH 8
#define MININHEIGHT 4
/* support 8192,limit by LCD */
#define MAXINWIDTH 4096
/* support 8192,limit by LCD */
#define MAXINHEIGHT 4096
#define LINE_BUF_LEN 2048
#define LOCFRACBIT 18
#define SCALERPHASE 16

#define ____SEPARATOR_REGISTERS____
union __wb_gctrl_reg_t {
	unsigned int dwval;
	struct {
		unsigned int wb_start:1;
		unsigned int r0:3;
		unsigned int soft_reset:1;
		unsigned int self_timing:1; /* bit5 */
		unsigned int r1:10;
		unsigned int in_port_sel:2;
		unsigned int r2:10;
		unsigned int auto_gate_en:1;
		unsigned int clk_gate:1;
		unsigned int r3:1;
		unsigned int bist_en:1;
	} bits;
};

union __wb_size_reg_t {
	unsigned int dwval;
	struct {
		unsigned int width:13;
		unsigned int r0:3;
		unsigned int height:13;
		unsigned int r1:3;
	} bits;
};

union __wb_crop_coord_reg_t {
	unsigned int dwval;
	struct {
		unsigned int crop_left:13;
		unsigned int r0:3;
		unsigned int crop_top:13;
		unsigned int r1:3;
	} bits;
};

union __wb_crop_size_reg_t {
	unsigned int dwval;
	struct {
		unsigned int crop_width:13;
		unsigned int r0:3;
		unsigned int crop_height:13;
		unsigned int r1:3;
	} bits;
};

union __wb_addr_reg_t {
	unsigned int dwval;
	struct {
		unsigned int addr:32;
	} bits;
};

union __wb_high_addr_reg_t {
	unsigned int dwval;
	struct {
		unsigned int ch0_h_addr:8;
		unsigned int ch1_h_addr:8;
		unsigned int ch2_h_addr:8;
		unsigned int r0:8;
	} bits;
};

union __wb_pitch_reg_t {
	unsigned int dwval;
	struct {
		unsigned int pitch:32;
	} bits;
};

union __wb_addr_switch_reg_t {
	unsigned int dwval;
	struct {
		unsigned int cur_group:1;
		unsigned int r0:15;
		unsigned int auto_switch:1;
		unsigned int r1:3;
		unsigned int manual_group:1;
		unsigned int r2:11;
	} bits;
};

union __wb_format_reg_t {
	unsigned int dwval;
	struct {
		unsigned int format:4;
		unsigned int r0:28;
	} bits;
};

union __wb_int_reg_t {
	unsigned int dwval;
	struct {
		unsigned int int_en:1;
		unsigned int r0:31;
	} bits;
};

union __wb_status_reg_t {
	unsigned int dwval;
	struct {
		unsigned int irq:1;
		unsigned int r0:3;
		unsigned int finish:1;
		unsigned int overflow:1;
		unsigned int timeout:1;
		unsigned int r1:1;
		unsigned int busy:1;
		unsigned int r2:23;
	} bits;
};

union __wb_bypass_reg_t {
	unsigned int dwval;
	struct {
		unsigned int csc_en:1;
		unsigned int cs_en:1;
		unsigned int fs_en:1;
		unsigned int r0:29;
	} bits;
};

union __wb_cs_reg_t {
	unsigned int dwval;
	struct {
		unsigned int m:13;
		unsigned int r1:3;
		unsigned int n:13;
		unsigned int r0:3;
	} bits;
};

union __wb_fs_size_reg_t {
	unsigned int dwval;
	struct {
		unsigned int width:13;
		unsigned int r1:3;
		unsigned int height:13;
		unsigned int r0:3;
	} bits;
};

union __wb_fs_step_reg_t {
	unsigned int dwval;
	struct {
		unsigned int r1:2;
		unsigned int frac:18;
		unsigned int intg:2;
		unsigned int r0:10;
	} bits;
};

union __wb_coeff_reg_t {
	unsigned int dwval;
	struct {
		unsigned int coef0:8;
		unsigned int coef1:8;
		unsigned int coef2:8;
		unsigned int coef3:8;
	} bits;
};

union __wb_csc_coeff_reg_t {
	unsigned int dwval;
	struct {
		unsigned int coff		:13;/* Default: 0x0; */
		unsigned int res0		:19;/* Default: ; */
	} bits;
};

union __wb_csc_const_reg_t {
	unsigned int dwval;
	struct {
		unsigned int cont			:20;/* Default: 0x0; */
		unsigned int res0			:12;/* Default: ; */
	} bits;
};

union __wb_sftm_reg_t {
	unsigned int dwval;
	struct {
		unsigned int sf_vs		:  10;	   /* Default: 0x0; */
		unsigned int res0		:  22;     /* Default: ; */
	} bits;
};

union __wb_eink_ctl_reg_t {
	unsigned int dwval;
	struct {
		unsigned int wb_eink_en		:1;/* default: 0x0; */
		unsigned int win_en		:1;/* default: 0x0; */
		unsigned int panel_bit_mode	:2;/* default: 0x0; */
		unsigned int dither_mode	:4;/* default: 0x0; */
		unsigned int a2_mode		:1;/* default: 0x0; */
		unsigned int a16_gray		:1;/* default: 0x0; */
		unsigned int res0		:22;/* default: */
	} bits;
};

union __wb_eink_in_pitch_reg_t {
	unsigned int dwval;
	struct {
		unsigned int pitch		:16;/* default: 0x0; */
		unsigned int res0		:16;/* default: */
	} bits;
};

union __wb_eink_in_laddr_reg_t {
	unsigned int dwval;
	struct {
		unsigned int in_laddr		:32;/* default: 0x0; */
	} bits;
};

union __wb_eink_in_haddr_reg_t {
	unsigned int dwval;
	struct {
		unsigned int in_haddr		:8;/* default: 0x0; */
		unsigned int r0			:24;/* default: 0x0; */
	} bits;
};

union __wb_eink_upd_win0_reg_t {
	unsigned int dwval;
	struct {
		unsigned int win_left		:12;/* default: 0x0; */
		unsigned int res0		:4;
		unsigned int win_top		:12;
		unsigned int res1		:4;
	} bits;
};

union __wb_eink_upd_win1_reg_t {
	unsigned int dwval;
	struct {
		unsigned int win_right		:12;/* default: 0x0; */
		unsigned int res0		:4;
		unsigned int win_bottom		:12;
		unsigned int res1		:4;
	} bits;
};

union __wb_eink_hist_reg_t {
	unsigned int dwval;
	struct {
		unsigned int hist		:32;/* default: 0x0; */
	} bits;
};

union __wb_debug_reg_t {
	unsigned int dwval;
	struct {
		unsigned int line_cnt		:  13;	   /* Default: 0x0; */
		unsigned int res0		:  19;     /* Default: ; */
	} bits;
};

struct __wb_reg_t {
	union __wb_gctrl_reg_t gctrl;			/* 0x0000 */
	union __wb_size_reg_t size;			/* 0x0004 */
	union __wb_crop_coord_reg_t crop_coord;		/* 0x0008 */
	union __wb_crop_size_reg_t crop_size;		/* 0x000c */
	union __wb_addr_reg_t wb_addr_a0;		/* 0x0010 */
	union __wb_addr_reg_t wb_addr_a1;		/* 0x0014 */
	union __wb_addr_reg_t wb_addr_a2;		/* 0x0018 */
	union __wb_high_addr_reg_t wb_addr_ah;		/* 0x001c */
	union __wb_addr_reg_t wb_addr_b0;		/* 0x0020 */
	union __wb_addr_reg_t wb_addr_b1;		/* 0x0024 */
	union __wb_addr_reg_t wb_addr_b2;		/* 0x0028 */
	union __wb_high_addr_reg_t wb_addr_bh;		/* 0x002c */
	union __wb_pitch_reg_t wb_pitch0;		/* 0x0030 */
	union __wb_pitch_reg_t wb_pitch1;		/* 0x0034 */
	unsigned int res0[2];				/* 0x0038-0x003c */
	union __wb_addr_switch_reg_t addr_switch;	/* 0x0040 */
	union __wb_format_reg_t fmt;			/* 0x0044 */
	union __wb_int_reg_t intr;			/* 0x0048 */
	union __wb_status_reg_t status;			/* 0x004c */
	union __wb_sftm_reg_t sftm;			/* 0x0050 */
	union __wb_bypass_reg_t bypass;			/* 0x0054 */
	unsigned int res1[6];				/* 0x0058-0x006c */
	union __wb_cs_reg_t cs_horz;			/* 0x0070 */
	union __wb_cs_reg_t cs_vert;			/* 0x0074 */
	unsigned int res2[2];				/* 0x0078-0x007c */
	union __wb_fs_size_reg_t fs_insize;		/* 0x0080 */
	union __wb_fs_size_reg_t fs_outsize;		/* 0x0084 */
	union __wb_fs_step_reg_t fs_hstep;		/* 0x0088 */
	union __wb_fs_step_reg_t fs_vstep;		/* 0x008c */
	unsigned int res3[27];				/* 0x0090-0x00f8 */
	union __wb_debug_reg_t debug;			/* 0x00fc */
	union __wb_csc_coeff_reg_t	c00;/* 0x0100 */
	union __wb_csc_coeff_reg_t	c01;/* 0x0104 */
	union __wb_csc_coeff_reg_t	c02;/* 0x0108 */
	union __wb_csc_const_reg_t	c03;/* 0x010C */
	union __wb_csc_coeff_reg_t	c10;/* 0x0110 */
	union __wb_csc_coeff_reg_t	c11;/* 0x0114 */
	union __wb_csc_coeff_reg_t	c12;/* 0x0118 */
	union __wb_csc_const_reg_t	c13;/* 0x011C */
	union __wb_csc_coeff_reg_t	c20;/* 0x0120 */
	union __wb_csc_coeff_reg_t	c21;/* 0x0124 */
	union __wb_csc_coeff_reg_t	c22;/* 0x0128 */
	union __wb_csc_const_reg_t	c23;/* 0x012C */
	union __wb_eink_ctl_reg_t	eink_ctl;	/* 0x0130 */
	unsigned int res4;				/* 0x0134 */
	union __wb_eink_in_pitch_reg_t   eink_in_pitch; /* 0x0138 */
	union __wb_eink_in_laddr_reg_t   eink_in_laddr;  /* 0x013c */
	union __wb_eink_in_haddr_reg_t   eink_in_haddr; /* 0x0140 */
	union __wb_eink_upd_win0_reg_t   eink_upd_win0; /* 0x0144 */
	union __wb_eink_upd_win1_reg_t   eink_upd_win1; /* 0x0148 */
	unsigned int res5;				/* 0x014c */
	union __wb_eink_hist_reg_t       eink_hist[32];	/* 0x0150-0x01cc */
	unsigned int res6[12];			/* 0x01d0-0x01fc */
	union __wb_coeff_reg_t yhcoeff[16];	/* 0X0200-0x23c */
	unsigned int res7[16];			/* 0X0240-0x27c */
	union __wb_coeff_reg_t chcoeff[16];	/* 0X0280-0x2bc */
	unsigned int res8[16];			/* 0x02c0-0x02fc */
};

enum wb_output_fmt {
	WB_FORMAT_RGB_888	 = 0x0,
	WB_FORMAT_BGR_888	 = 0x1,
	WB_FORMAT_ARGB_8888	 = 0x4,
	WB_FORMAT_ABGR_8888	 = 0x5,
	WB_FORMAT_BGRA_8888	 = 0x6,
	WB_FORMAT_RGBA_8888	 = 0x7,
	WB_FORMAT_YUV420_P	 = 0x8,
	WB_FORMAT_Y8		 = 0x9,
	WB_FORMAT_Y5		 = 0xa,
	WB_FORMAT_Y4		 = 0xb,
	WB_FORMAT_YUV420_SP_VUVU = 0xc,
	WB_FORMAT_YUV420_SP_UVUV = 0xd,
	WB_FORMAT_Y3		 = 0xe,
};

struct eink_rect {
	int x;
	int y;
	unsigned int width;
	unsigned int height;
};

struct eink_size {
	unsigned int width;
	unsigned int height;
};

struct eink_win {
	int left;
	int right;
	int top;
	int bottom;
};

struct eink_frame {
	struct eink_rect crop;
	struct eink_size size;
	unsigned int addr;
};

typedef struct {
	enum wb_output_fmt out_fmt;
	unsigned int csc_std;
	enum dither_mode dither_mode;
	bool win_en;
	struct eink_frame frame;
} __eink_wb_config_t;

#define EWB_OK 0
#define EWB_OVFL 1
#define EWB_TIMEOUT 2
#define EWB_BUSY 3
#define EWB_ERR 4
extern unsigned int wb_lan2coefftab16[16];
extern unsigned int wb_lan2coefftab16_down[16];

extern int wb_eink_set_reg_base(unsigned int sel, void __iomem *reg_base);
extern unsigned long wb_eink_get_reg_base(unsigned int sel);
extern int wb_eink_reset(unsigned int sel);
extern int wb_eink_dereset(unsigned int sel);
extern int wb_eink_set_para(unsigned int sel, __eink_wb_config_t *cfg);
extern int wb_eink_writeback_enable(unsigned int sel);
extern int wb_eink_get_status(unsigned int sel);
extern int wb_eink_enableint(unsigned int sel);
extern int wb_eink_disableint(unsigned int sel);
extern int wb_eink_set_panel_bit(unsigned int sel, u32 bit_num);
extern int wb_eink_set_gray_level(unsigned int sel, unsigned int val);

extern u8 __bits_num_config_to_reg(u32 bit_num);
extern int wb_eink_get_hist_val(unsigned int sel, unsigned int gray_level_cnt, unsigned int *eink_hist);
extern int wb_eink_get_upd_win(unsigned int sel, struct upd_win *upd_win);
extern int wb_eink_enable(unsigned int sel);
extern int wb_eink_disable(unsigned int sel);
extern int wb_eink_interrupt_enable(unsigned int sel);
extern int wb_eink_interrupt_disable(unsigned int sel);
extern int wb_eink_set_last_img(unsigned int sel, unsigned int last_img_addr);
extern int wb_eink_set_a2_mode(unsigned int sel);
extern enum upd_mode wb_eink_auto_mode_select(unsigned int sel, unsigned int gray_cnt,
		struct eink_img *last_img, struct eink_img *cur_img);
extern int wb_eink_interrupt_clear(unsigned int sel);
#endif
