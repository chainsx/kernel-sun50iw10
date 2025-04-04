/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#ifndef __DE_RTMX_TYPE_H__
#define __DE_RTMX_TYPE_H__

/* /////////////////////for global control/////////////////////// */
typedef union {
	unsigned int dwval;
	struct {
		unsigned int rt_en:1;	/* bit0 */
		unsigned int r0:3;	/* bit1~3 */
		unsigned int finish_irq_en:1;	/* bit4 */
		unsigned int error_irq_en:1;	/* bit5 */
		unsigned int r1:2;	/* bit6~7 */
		unsigned int sync_rev:1;	/* bit8 */
		unsigned int flied_rev:1;	/* bit9 */
		unsigned int r2:2;	/* bit10~11 */
		unsigned int rtwb_port:2;	/* bit12~13 */
		unsigned int r3:18;	/* bit14~31 */
	} bits;
} __glb_ctl_reg_t;		/* 0x0 */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int finish_irq:1;	/* bit0 */
		unsigned int error_irq:1;	/* bit1 */
		unsigned int r0:2;	/* bit2~3 */
		unsigned int busy_status:1;	/* bit4 */
		unsigned int error_status:1;	/* bit5 */
		unsigned int r1:2;	/* bit6~7 */
		unsigned int even_odd_flag:1;	/* bit8 */
		unsigned int r2:23;	/* bit9~31 */
	} bits;
} __glb_status_reg_t;		/* 0x4 */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int dbuff_rdy:1;	/* bit0 */
		unsigned int r0:31;	/* bit1~31 */
	} bits;
} __glb_dbuff_reg_t;		/* 0x8 */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int width:13;	/* bit0~12 */
		unsigned int r0:3;	/* bit13~15 */
		unsigned int height:13;	/* bit16~28 */
		unsigned int r1:3;	/* bit29~31 */
	} bits;
} __glb_size_reg_t;		/* 0xc */

typedef struct {
	__glb_ctl_reg_t glb_ctl;	/* 0x00 */
	__glb_status_reg_t glb_status;	/* 0x04 */
	__glb_dbuff_reg_t glb_dbuff;	/* 0x08 */
	__glb_size_reg_t glb_size;	/* 0x0c */
} __glb_reg_t;

/* /////////////////////for video overlay/////////////////////// */
typedef union {
	unsigned int dwval;
	struct {
		unsigned int lay_en:1;	/* bit0 */
		unsigned int r0:3;	/* bit1~3 */
		unsigned int lay_fcolor_en:1;	/* bit4 */
		unsigned int r1:3;	/* bit5~7 */
		unsigned int lay_fmt:5;	/* bit8~12 */
		unsigned int r2:2;	/* bit13~14 */
		unsigned int ui_sel:1;	/* bit15 */
		unsigned int r3:7;	/* bit16~22 */
		unsigned int lay_top_down:1;	/* bit23 */
		unsigned int r4:8;	/* bit24~31 */
	} bits;
} __vi_lay_attr_reg_t;		/* 0x0+N*0x30(N=0,1,2,3) */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int lay_width:13;	/* bit0~12 */
		unsigned int r0:3;	/* bit13~15 */
		unsigned int lay_height:13;	/* bit16~28 */
		unsigned int r1:3;	/* bit29~31 */
	} bits;
} __vi_lay_size_reg_t;		/* 0x4+N*0x30(N=0,1,2,3) */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int lay_coorx:16;	/* bit0~15 */
		unsigned int lay_coory:16;	/* bit16~31 */
	} bits;
} __vi_lay_coor_reg_t;		/* 0x8+N*0x30(N=0,1,2,3) */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int lay_pitch:32;	/* bit0~31 */
	} bits;
} __vi_lay_pitch_reg_t;		/* 0xc/0x010/0x014+N*0x30(N=0,1,2,3) */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int lay_top_laddr:32;	/* bit0~31 */
	} bits;
} __vi_lay_top_laddr_reg_t;	/* 0x018/0x01c/0x020+N*0x30(N=0,1,2,3) */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int lay_bot_laddr:32;	/* bit0~31 */
	} bits;
} __vi_lay_bot_laddr_reg_t;	/* 0x024/0x028/0x02c+N*0x30(N=0,1,2,3) */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int lay_vb:8;	/* bit0~7 */
		unsigned int lay_ug:8;	/* bit8~15 */
		unsigned int lay_yr:8;	/* bit16~23 */
		unsigned int r0:8;	/* bit24~31 */
	} bits;
} __vi_lay_fcolor_reg_t;	/* 0x0c0+N*0x4(N=0,1,2,3) */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int lay0_top_haddr:8;	/* bit0~7 */
		unsigned int lay1_top_haddr:8;	/* bit8~15 */
		unsigned int lay2_top_haddr:8;	/* bit16~23 */
		unsigned int lay3_top_haddr:8;	/* bit24~31 */
	} bits;
} __vi_lay_top_haddr_reg_t;	/* 0xd0~0x0xd8 */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int lay0_bot_haddr:8;	/* bit0~7 */
		unsigned int lay1_bot_haddr:8;	/* bit8~15 */
		unsigned int lay2_bot_haddr:8;	/* bit16~23 */
		unsigned int lay3_bot_haddr:8;	/* bit24~31 */
	} bits;
} __vi_lay_bot_haddr_reg_t;	/* 0xdc~0xe4 */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int ovl_width:13;	/* bit0~12 */
		unsigned int r0:3;	/* bit13~15 */
		unsigned int ovl_height:13;	/* bit16~28 */
		unsigned int r1:3;	/* bit29~31 */
	} bits;
} __vi_ovl_size_reg_t;		/* 0xe8~0xec */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int m:14;	/* bit0~13 */
		unsigned int r0:2;	/* bit14~15 */
		unsigned int n:14;	/* bit16~29 */
		unsigned int r1:2;	/* bit30~31 */
	} bits;
} __vi_hori_ds_reg_t;		/* 0xf0~0xf4 */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int m:14;	/* bit0~13 */
		unsigned int r0:2;	/* bit14~15 */
		unsigned int n:14;	/* bit16~29 */
		unsigned int r1:2;	/* bit30~31 */
	} bits;
} __vi_vert_ds_reg_t;		/* 0xf8~0xfc */

typedef struct {
	__vi_lay_attr_reg_t lay_attr;	/* 0x00 */
	__vi_lay_size_reg_t lay_size;	/* 0x04 */
	__vi_lay_coor_reg_t lay_coor;	/* 0x08 */
	__vi_lay_pitch_reg_t lay_pitch[3];	/* 0x0c~0x14 */
	__vi_lay_top_laddr_reg_t lay_top_laddr[3];	/* 0x18~0x20 */
	__vi_lay_bot_laddr_reg_t lay_bot_laddr[3];	/* 0x24~0x2c */
} __vi_lay_reg_t;

typedef struct {
	__vi_lay_reg_t vi_lay_cfg[4];	/* 0x00~0x2c,0x30~0x5c,0x60~0x8c,0x90~0xbc, */
	__vi_lay_fcolor_reg_t vi_lay_fcolor[4];	/* 0xc0~0xcc */
	__vi_lay_top_haddr_reg_t vi_lay_top_haddr[3];	/* 0xd0~0xd8 */
	__vi_lay_bot_haddr_reg_t vi_lay_bot_haddr[3];	/* 0xdc~0xe4 */
	__vi_ovl_size_reg_t vi_ovl_size[2];	/* 0xe8~0xec */
	__vi_hori_ds_reg_t vi_hori_ds[2];	/* 0xf0~0xf4 */
	__vi_vert_ds_reg_t vi_vert_ds[2];	/* 0xf8~0xfc */
} __vi_ovl_reg_t;

/* /////////////////////for ui overlay/////////////////////// */
typedef union {
	unsigned int dwval;
	struct {
		unsigned int lay_en:1;	/* bit0 */
		unsigned int lay_alpmod:2;	/* bit1~2 */
		unsigned int r0:1;	/* bit3 */
		unsigned int lay_fcolor_en:1;	/* bit4 */
		unsigned int r1:3;	/* bit5~7 */
		unsigned int lay_fmt:5;	/* bit8~12 */
		unsigned int r2:3;	/* bit13~15 */
		unsigned int lay_alpctl:2;	/* bit16~17 */
		unsigned int r3:5;	/* bit18~22 */
		unsigned int lay_top_down:1;	/* bit23 */
		unsigned int lay_alpha:8;	/* bit24~31 */
	} bits;
} __ui_lay_attr_reg_t;		/* 0x0+N*0x20(N=0,1,2,3) */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int lay_width:13;	/* bit0~12 */
		unsigned int r0:3;	/* bit13~15 */
		unsigned int lay_height:13;	/* bit16~28 */
		unsigned int r1:3;	/* bit29~31 */
	} bits;
} __ui_lay_size_reg_t;		/* 0x4+N*0x20(N=0,1,2,3) */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int lay_coorx:16;	/* bit0~15 */
		unsigned int lay_coory:16;	/* bit16~31 */
	} bits;
} __ui_lay_coor_reg_t;		/* 0x8+N*0x20(N=0,1,2,3) */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int lay_pitch:32;	/* bit0~31 */
	} bits;
} __ui_lay_pitch_reg_t;		/* 0xc+N*0x20(N=0,1,2,3) */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int lay_top_laddr:32;	/* bit0~31 */
	} bits;
} __ui_lay_top_laddr_reg_t;	/* 0x10+N*0x20(N=0,1,2,3) */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int lay_bot_laddr:32;	/* bit0~31 */
	} bits;
} __ui_lay_bot_laddr_reg_t;	/* 0x14+N*0x20(N=0,1,2,3) */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int lay_blue:8;	/* bit0~7 */
		unsigned int lay_green:8;	/* bit8~15 */
		unsigned int lay_red:8;	/* bit16~23 */
		unsigned int lay_alpha:8;	/* bit24~31 */
	} bits;
} __ui_lay_fcolor_reg_t;	/* 0x18+N*0x20(N=0,1,2,3) */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int lay0_top_haddr:8;	/* bit0~7 */
		unsigned int lay1_top_haddr:8;	/* bit8~15 */
		unsigned int lay2_top_haddr:8;	/* bit16~23 */
		unsigned int lay3_top_haddr:8;	/* bit24~31 */
	} bits;
} __ui_lay_top_haddr_reg_t;	/* 0x80 */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int lay0_bot_haddr:8;	/* bit0~7 */
		unsigned int lay1_bot_haddr:8;	/* bit8~15 */
		unsigned int lay2_bot_haddr:8;	/* bit16~23 */
		unsigned int lay3_bot_haddr:8;	/* bit24~31 */
	} bits;
} __ui_lay_bot_haddr_reg_t;	/* 0x84 */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int ovl_width:13;	/* bit0~12 */
		unsigned int r0:3;	/* bit13~15 */
		unsigned int ovl_height:13;	/* bit16~28 */
		unsigned int r1:3;	/* bit29~31 */
	} bits;
} __ui_ovl_size_reg_t;		/* 0x88 */

typedef struct {
	__ui_lay_attr_reg_t lay_attr;	/* 0x00 */
	__ui_lay_size_reg_t lay_size;	/* 0x04 */
	__ui_lay_coor_reg_t lay_coor;	/* 0x08 */
	__ui_lay_pitch_reg_t lay_pitch;	/* 0x0c */
	__ui_lay_top_laddr_reg_t lay_top_laddr;	/* 0x10 */
	__ui_lay_bot_laddr_reg_t lay_bot_laddr;	/* 0x14 */
	__ui_lay_fcolor_reg_t lay_fcolor;	/* 0x18 */
	unsigned int r0;	/* 0x1c */
} __ui_lay_reg_t;

typedef struct {
	__ui_lay_reg_t ui_lay_cfg[4];	/* 0x00~0x1c,0x20~0x3c,0x40~0x5c,0x60~0x7c, */
	__ui_lay_top_haddr_reg_t ui_lay_top_haddr;	/* 0x80 */
	__ui_lay_bot_haddr_reg_t ui_lay_bot_haddr;	/* 0x84 */
	__ui_ovl_size_reg_t ui_ovl_size;	/* 0x88 */
} __ui_ovl_reg_t;

/* /////////////////////for alpha blending/////////////////////// */
typedef union {
	unsigned int dwval;
	struct {
		unsigned int p0_fcolor_en:1;	/* bit0 */
		unsigned int p1_fcolor_en:1;	/* bit1 */
		unsigned int p2_fcolor_en:1;	/* bit2 */
		unsigned int p3_fcolor_en:1;	/* bit3 */
		unsigned int p4_fcolor_en:1;	/* bit4 */
		unsigned int r0:3;	/* bit5~7 */
		unsigned int p0_en:1;	/* bit8 */
		unsigned int p1_en:1;	/* bit9 */
		unsigned int p2_en:1;	/* bit10 */
		unsigned int p3_en:1;	/* bit11 */
		unsigned int p4_en:1;	/* bit12 */
		unsigned int r1:19;	/* bit13~31 */
	} bits;
} __bld_fcolor_ctl_reg_t;	/* 0x0 */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int blue:8;	/* bit0~7 */
		unsigned int green:8;	/* bit8~15 */
		unsigned int red:8;	/* bit16~23 */
		unsigned int alpha:8;	/* bit24~31 */
	} bits;
} __bld_fcolor_reg_t;		/* 0x4+N*0x10(N=0,1,2,3,4) */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int width:13;	/* bit0~12 */
		unsigned int r0:3;	/* bit13~15 */
		unsigned int height:13;	/* bit16~28 */
		unsigned int r1:3;	/* bit29~31 */
	} bits;
} __bld_isize_reg_t;		/* 0x8+N*0x10(N=0,1,2,3,4) */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int coorx:16;	/* bit0~15 */
		unsigned int coory:13;	/* bit16~31 */
	} bits;
} __bld_offset_reg_t;		/* 0xc+N*0x10(N=0,1,2,3,4) */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int ch0_routr_ctl:4;	/* bit0~3 */
		unsigned int ch1_routr_ctl:4;	/* bit4~7 */
		unsigned int ch2_routr_ctl:4;	/* bit8~11 */
		unsigned int ch3_routr_ctl:4;	/* bit12~15 */
		unsigned int ch4_routr_ctl:4;	/* bit16~19 */
		unsigned int r0:12;	/* bit20~31 */
	} bits;
} __bld_route_ctl_reg_t;	/* 0x80 */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int p0_alpha_mode:1;	/* bit0 */
		unsigned int p1_alpha_mode:1;	/* bit1 */
		unsigned int p2_alpha_mode:1;	/* bit2 */
		unsigned int p3_alpha_mode:1;	/* bit3 */
		unsigned int p4_alpha_mode:1;	/* bit4 */
		unsigned int r0:27;	/* bit5~31 */
	} bits;
} __bld_premultiply_ctl_reg_t;	/* 0x84 */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int blue:8;	/* bit0~7 */
		unsigned int green:8;	/* bit8~15 */
		unsigned int red:8;	/* bit16~23 */
		unsigned int alpha:8;	/* bit24~31 */
	} bits;
} __bld_bkcolor_reg_t;		/* //0x88 */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int width:13;	/* bit0~12 */
		unsigned int r0:3;	/* bit13~15 */
		unsigned int height:13;	/* bit16~28 */
		unsigned int r1:3;	/* bit29~31 */
	} bits;
} __bld_output_size_reg_t;	/* 0x8c */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int pixel_fs:4;	/* bit0~3 */
		unsigned int r0:4;	/* bit4~7 */
		unsigned int pixel_fd:4;	/* bit8~11 */
		unsigned int r1:4;	/* bit12~15 */
		unsigned int alpha_fs:4;	/* bit16~19 */
		unsigned int r2:4;	/* bit20~23 */
		unsigned int alpha_fd:4;	/* bit24~27 */
		unsigned int r3:4;	/* bit28~31 */
	} bits;
} __bld_ctl_reg_t;		/* 0x90+N*0x4(N=0,1,2,3) */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int key0_en:1;	/* bit0 */
		unsigned int key0_dir:2;	/* bit1~2 */
		unsigned int r0:1;	/* bit3 */
		unsigned int key1_en:1;	/* bit4 */
		unsigned int key1_dir:2;	/* bit5~6 */
		unsigned int r1:1;	/* bit7 */
		unsigned int key2_en:1;	/* bit8 */
		unsigned int key2_dir:2;	/* bit9~10 */
		unsigned int r2:1;	/* bit11 */
		unsigned int key3_en:1;	/* bit12 */
		unsigned int key3_dir:2;	/* bit13~14 */
		unsigned int r3:17;	/* bit15~31 */
	} bits;
} __bld_colorkey_ctl_reg_t;	/* 0xb0 */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int key0_blue:1;	/* bit0 */
		unsigned int key0_green:1;	/* bit1 */
		unsigned int key0_red:1;	/* bit2 */
		unsigned int r0:5;	/* bit3~7 */
		unsigned int key1_blue:1;	/* bit8 */
		unsigned int key1_green:1;	/* bit9 */
		unsigned int key1_red:1;	/* bit10 */
		unsigned int r1:5;	/* bit11~15 */
		unsigned int key2_blue:1;	/* bit16 */
		unsigned int key2_green:1;	/* bit17 */
		unsigned int key2_red:1;	/* bit18 */
		unsigned int r2:5;	/* bit19~23 */
		unsigned int key3_blue:1;	/* bit24 */
		unsigned int key3_green:1;	/* bit25 */
		unsigned int key3_red:1;	/* bit26 */
		unsigned int r3:5;	/* bit27~31 */
	} bits;
} __bld_colorkey_cfg_reg_t;	/* 0xb4 */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int max_blue:8;	/* bit0~7 */
		unsigned int max_green:8;	/* bit8~15 */
		unsigned int max_red:8;	/* bit16~23 */
		unsigned int r0:8;	/* bit24~31 */
	} bits;
} __bld_colorkey_max_reg_t;	/* 0xc0+N*0x4(N=0,1,2,3) */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int min_blue:8;	/* bit0~7 */
		unsigned int min_green:8;	/* bit8~15 */
		unsigned int min_red:8;	/* bit16~23 */
		unsigned int r0:8;	/* bit24~31 */
	} bits;
} __bld_colorkey_min_reg_t;	/* 0xe0+N*0x4(N=0,1,2,3) */

typedef union {
	unsigned int dwval;
	struct {
		unsigned int premultiply_en:1;	/* bit0 */
		unsigned int interlace_en:1;	/* bit1 */
		unsigned int r0:30;	/* bit2~31 */
	} bits;
} __bld_out_color_ctl_reg_t;	/* 0xfc */

typedef struct {
	__bld_fcolor_reg_t fcolor;	/* 0x04 */
	__bld_isize_reg_t insize;	/* 0x08 */
	__bld_offset_reg_t offset;	/* 0x0c */
	unsigned int r0;	/* 0x10 */
} __bld_pipe_reg_t;

typedef struct {
	__bld_fcolor_ctl_reg_t bld_fcolor_ctl;	/* 0x00 */
	__bld_pipe_reg_t bld_pipe_attr[5];	/* 0x04~0x10,0x14~0x20,0x24~0x30,0x34~0x40,0x44~0x50 */
	unsigned int r0[11];	/* 0x54~0x7c */
	__bld_route_ctl_reg_t bld_route_ctl;	/* 0x80 */
	__bld_premultiply_ctl_reg_t bld_premultiply_ctl;	/* 0x84 */
	__bld_bkcolor_reg_t bld_bkcolor;	/* 0x88 */
	__bld_output_size_reg_t bld_output_size;	/* 0x8c */
	__bld_ctl_reg_t bld_mode[4];	/* 0x90~0x9c */
	unsigned int r1[4];	/* 0xa0~0xac */
	__bld_colorkey_ctl_reg_t bld_ck_ctl;	/* 0xb0 */
	__bld_colorkey_cfg_reg_t bld_ck_cfg;	/* 0xb4 */
	unsigned int r2[2];	/* 0xb8~0xbc */
	__bld_colorkey_max_reg_t bld_ck_max[4];	/* 0xc0~0xcc */
	unsigned int r3[4];	/* 0xd0~0xdc */
	__bld_colorkey_min_reg_t bld_ck_min[4];	/* 0xe0~0xec */
	unsigned int r4[3];	/* 0xf0~0xf8 */
	__bld_out_color_ctl_reg_t bld_out_ctl;	/* 0xfc */
} __bld_reg_t;

typedef struct {
	__glb_reg_t *glb_ctl;	/* 0x00000 */
	__bld_reg_t *bld_ctl;	/* 0x01000 */
	__vi_ovl_reg_t *vi_ovl[VI_CHN_NUM];	/* 0x02000 */
	__ui_ovl_reg_t *ui_ovl[UI_CHN_NUM];	/* 0x03000~0x05000 */
} __rtmx_reg_t;

#endif
