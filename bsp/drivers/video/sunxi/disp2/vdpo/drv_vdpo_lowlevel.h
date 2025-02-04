/*
 * Allwinner SoCs vdpo lowlevel driver.
 *
 * Copyright (C) 2017 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#ifndef _DRV_VDPO_LOWLEVEL_H
#define _DRV_VDPO_LOWLEVEL_H

#include "drv_vdpo_common.h"

#if defined(CONFIG_ARCH_SUN8IW12)
#define VDPO_NUM 1
#else
#define VDPO_NUM 1
#endif

#define VDPO_PROTOCOL_BT1120 0
#define VDPO_PROTOCOL_BT656 1
#define VDPO_SEPARATE_SYNC 0
#define VDPO_EMBEDDED_SYNC 1
#define VDPO_YC_SEPARATE 1
#define VDPO_YC_INTERLEAVE 0
#define VDPO_8_BIT_DEPTH 0
#define VDPO_10_BIT_DEPTH 1
#define V_INT    1
#define L_INT    2

/* VDPO(Video data parallel output) */
/* offset:0x00 */
union vdpo_ctrl_reg {
	__u32 dwval;
	struct {
		__u32 vdpo_mudule_en:1;
		__u32 separate_sync_en:1;
		__u32 res00:30;
	} bits;

};

/* VDPO(Video data parallel output) */
/* offset:0x04 */
union vdpo_fmt_reg {
	__u32 dwval;
	struct {
		__u32 output_data_width:1;
		__u32 interlace_mode:1;
		__u32 res00:2;
		__u32 embedded_sync_fmt:1;
		__u32 res01:3;
		__u32 data_seq_sel:2;
		__u32 res02:22;
	} bits;

};

/* VDPO(Video data parallel output) */
/* offset:0x08 */
union vdpo_sync_ctrl_reg {
	__u32 dwval;
	struct {
		__u32 h_blank_pol:1;
		__u32 v_blank_pol:1;
		__u32 field_pol:1;
		__u32 dclk_invert:1;
		__u32 dclk_dly_num:6;
		__u32 dclk_dly_en:1;
		__u32 res00:21;
	} bits;

};

/* VDPO(Video data parallel output) */
/* offset:0xc */
union vdpo_int_ctrl_reg {
	__u32 dwval;
	struct {
		__u32 line_match_int_en:1;
		__u32 vb_int_en:1;
		__u32 res00:14;
		__u32 line_match_int_flag:1;
		__u32 vb_int_flag:1;
		__u32 res01:14;
	} bits;
};

/* VDPO(Video data parallel output) */
/* offset:0x10 */
union vdpo_line_int_num_reg {
	__u32 dwval;
	struct {
		__u32 int_line_num:12;
		__u32 res00:20;
	} bits;
};

/* VDPO(Video data parallel output) */
/* offset:0x14 */
union vdpo_status_reg {
	__u32 dwval;
	struct {
		__u32 current_line:13;
		__u32 res00:3;
		__u32 field_pol_sta:1;
		__u32 res010:15;
	} bits;
};

/* VDPO(Video data parallel output) */
/* offset:0x18 */
union vdpo_hor_spl_ctrl_reg {
	__u32 dwval;
	struct {
		__u32 cb_hori_spl_type:3;
		__u32 res00:1;
		__u32 cr_hori_spl_type:3;
		__u32 res01:15;
	} bits;
};

/* VDPO(Video data parallel output) */
/* offset:0x1c */
union vdpo_clamp_reg0 {
	__u32 dwval;
	struct {
		__u32 y_val_range_min:8;
		__u32 res00:8;
		__u32 y_val_range_max:8;
		__u32 res01:8;

	} bits;
};

/* VDPO(Video data parallel output) */
/* offset:0x20 */
union vdpo_clamp_reg1 {
	__u32 dwval;
	struct {
		__u32 cb_val_range_min:8;
		__u32 res00:8;
		__u32 cb_val_range_max:8;
		__u32 res01:8;
	} bits;
};

/* VDPO(Video data parallel output) */
/* offset:0x24 */
union vdpo_clamp_reg2 {
	__u32 dwval;
	struct {
		__u32 cr_val_range_min:8;
		__u32 res00:8;
		__u32 cr_val_range_max:8;
		__u32 res01:8;
	} bits;
};

/* VDPO(Video data parallel output) */
/* offset:0x28 */
union vdpo_h_timing_reg0 {
	__u32 dwval;
	struct {
		__u32 h_bp:12;
		__u32 res00:4;
		__u32 h_active:12;
		__u32 res01:4;
	} bits;
};

/* VDPO(Video data parallel output) */
/* offset:0x2C */
union vdpo_v_timing_reg0 {
	__u32 dwval;
	struct {
		__u32 v_bp:12;
		__u32 res00:4;
		__u32 v_active:12;
		__u32 res01:4;
	} bits;
};

/* VDPO(Video data parallel output) */
/* offset:0x2C */
union vdpo_v_timing_reg1 {
	__u32 dwval;
	struct {
		__u32 v_total:12;
		__u32 res00:4;
		__u32 itl_mode:1;
		__u32 res01:15;
	} bits;
};

struct __vdpo_dev {
	union vdpo_ctrl_reg module_ctrl;/* 0x00 */
	union vdpo_fmt_reg fmt_ctrl;/* 0x04 */
	union vdpo_sync_ctrl_reg sync_ctrl;/* 0x08 */
	union vdpo_int_ctrl_reg int_ctrl;/* 0x0c */
	union vdpo_line_int_num_reg line_int_num;/* 0x10 */
	union vdpo_status_reg status;/* 0x14 */
	union vdpo_hor_spl_ctrl_reg hor_spl_ctrl;/* 0x18 */
	union vdpo_clamp_reg0 clamp0;/* 0x1c */
	union vdpo_clamp_reg1 clamp1;/* 0x20 */
	union vdpo_clamp_reg2 clamp2;/* 0x24 */
	union vdpo_h_timing_reg0 h_timing;/* 0x28 */
	union vdpo_v_timing_reg0 v_timing;/* 0x2c */
	union vdpo_v_timing_reg1 v_timing1;/* 0x30 */
};

extern volatile struct __vdpo_dev *vdpo_dev[VDPO_NUM];

/* function member */
void __vdpo_module_en(u32 dev_sel, u32 module_en, u32 sepa_sync_en);

void __vdpo_fmt_set(u32 dev_sel, u32 data_seq_sel, u32 sync_fmt, u32 data_width,
		    u32 interlace);

void __vdpo_chroma_spl_set(u32 dev_sel, u8 cr_type, u8 cb_type);

void __vdpo_clamp_set(u32 dev_sel, u16 y_min, u16 y_max, u16 cb_min, u16 cb_max,
		      u16 cr_min, u16 cr_max);

void __vdpo_sync_pol_set(u32 dev_sel, u8 hb_pol, u8 vb_pol, u8 field_pol);

void __vdpo_dclk_adjust(u32 dev_sel, u8 dclk_invt, u8 dly_en, u8 dly_num);

u32 __vdpo_get_curline(u32 sel);

u32 __vdpo_get_field(u32 sel);

s32 __vdpo_irq_en(u32 sel, u32 int_type, u32 line);

s32 __vdpo_irq_disable(u32 sel, u32 int_type);

u32 __vdpo_irq_process(u32 sel);

u32 __vdpo_clr_irq(u32 sel, u32 int_type);

void __vdpo_timing_set(u32 sel, u32 h_active, u32 h_bp, u32 v_active, u32 v_bp,
		       u32 v_total, u32 interlace, u32 itl_mode);

u32 __vdpo_reg_default_test(u32 sel, u32 addr_offset, u32 exp_value);

u32 __vdpo_reg_wr_test(u32 sel, u32 addr_offset, u32 mask, u32 wr_value);

void __vdpo_set_reg_base(u32 sel, void __iomem *base_addr);

#endif /* End of file */
