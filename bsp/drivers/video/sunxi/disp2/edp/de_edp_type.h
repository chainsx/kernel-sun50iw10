/* de_edp_type.h
 *
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
 * edp driver reg type file
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef _DE_EDP_TYPE_H
#define _DE_EDP_TYPE_H

#include "drv_edp_common.h"

union dp_version_id_reg {
	u32 dwval;
	struct {
		u32 ver_l:16; /* default: 0x0; */
		u32 ver_h:16; /* default: 0x1; */
	} bits;
};

union dp_int_mask_reg {
	u32 dwval;
	struct {
		u32 hpd_flag_mask:1; /* default: 0x1; */
		u32 hpd_change_mask:1; /* default: 0x1; */
		u32 aux_finish_mask:1; /* default: 0x1; */
		u32 aux_timeout_flag_mask:1; /* default: 0x1; */
		u32 timer_over_flow_mask:1; /* default: 0x1; */
		u32 res0:25; /* default: ; */
		u32 line_triger1_int_mask:1; /* default: 0x1; */
		u32 line_triger0_int_mask:1; /* default: 0x1; */
	} bits;
};

union dp_int_status_reg {
	u32 dwval;
	struct {
		u32 hpd_flag:1; /* default: ; */
		u32 hpd_change:1; /* default: ; */
		u32 hpd_irq:1; /* default: ; */
		u32 aux_finish:1; /* default: ; */
		u32 aux_timeout_flag:1; /* default: ; */
		u32 timer_over_flow:1; /* default: ; */
		u32 res0:23; /* default: ; */
		u32 rd_empty:1; /* default: ; */
		u32 line_triger1_int:1; /* default: ; */
		u32 line_triger0_int:1; /* default: ; */
	} bits;
};

union dp_line_triger_ctrl_reg {
	u32 dwval;
	struct {
		u32 line_triger1_num:16; /* default: 0x0; */
		u32 line_triger0_num:16; /* default: 0x0; */
	} bits;
};

union dp_ctrl_reg {
	u32 dwval;
	struct {
		u32 vid_en:1; /* default: 0x0; */
		u32 no_video_stream_flag:1; /* default: 0x0; */
		u32 res0:19; /* default: ; */
		u32 ANSI8B10B:1; /* default: 0x0; */
		u32 scramble_dis:1; /* default: 0x0; */
		u32 framing_mode_en:1; /* default: 0x0; */
		u32 dp_speed_cfg:2; /* default: 0x0; */
		u32 dp_lane_cfg:2; /* default: 0x0; */
		u32 res1:3; /* default: ; */
		u32 module_en:1; /* default: 0x0; */
	} bits;
};

union dp_start_dly_reg {
	u32 dwval;
	struct {
		u32 start_dly:10; /* default: 0x8; */
		u32 res0:22; /* default: ; */
	} bits;
};

union dp_general_timer_reg {
	u32 dwval;
	struct {
		u32 preload_val:16; /* default: 0x0; */
		u32 N:5; /* default: 0x0; */
		u32 res0:8; /* default: ; */
		u32 start:1; /* default: 0x0; */
		u32 timeout_flag:1; /* default: 0x0; */
		u32 mode:1; /* default: 0x0; */
	} bits;
};

union dp_scramble_seed_reg {
	u32 dwval;
	struct {
		u32 seed:16; /* default: 0x0; */
		u32 res0:16; /* default: ; */
	} bits;
};

union dp_vid_ctrl_reg {
	u32 dwval;
	struct {
		u32 vid_src_sel:2; /* default: 0x0; */
		u32 vid_color_mode:2; /* default: 0x0; */
		u32 res0:28; /* default: ; */
	} bits;
};

union dp_vid_idle_ctrl_reg {
	u32 dwval;
	struct {
		u32 idle_pattern_count:16; /* default: 0x5; */
		u32 res0:15; /* default: ; */
		u32 idle_pattern_loop:1; /* default: 0x0; */
	} bits;
};

union dp_vid_tu_ctrl_reg {
	u32 dwval;
	struct {
		u32 tu_size:7; /* default: 0x0; */
		u32 res0:9; /* default: ; */
		u32 tu_fill_len:7; /* default: 0x0; */
		u32 res1:9; /* default: ; */
	} bits;
};

union dp_vid_frame_ctrl_reg {
	u32 dwval;
	struct {
		u32 htotal_in_symbol_per_lane:16; /* default: 0x0; */
		u32 vtotal_line_per_frame:16; /* default: 0x0; */
	} bits;
};

union dp_hwidth_reg {
	u32 dwval;
	struct {
		u32 hwidth:16; /* default: 0x0; */
		u32 res0:16; /* default: ; */
	} bits;
};

union dp_vheight_reg {
	u32 dwval;
	struct {
		u32 vheight:16; /* default: 0x0; */
		u32 res0:16; /* default: ; */
	} bits;
};

union dp_htotal_reg {
	u32 dwval;
	struct {
		u32 htotal:16; /* default: 0x0; */
		u32 res0:16; /* default: ; */
	} bits;
};

union dp_vtotal_reg {
	u32 dwval;
	struct {
		u32 vtotal:16; /* default: 0x0; */
		u32 res0:16; /* default: ; */
	} bits;
};

union dp_hstart_reg {
	u32 dwval;
	struct {
		u32 hstart:16; /* default: 0x0; */
		u32 res0:16; /* default: ; */
	} bits;
};

union dp_vstart_reg {
	u32 dwval;
	struct {
		u32 vstart:16; /* default: 0x0; */
		u32 res0:16; /* default: ; */
	} bits;
};

union dp_hsp_hsw_reg {
	u32 dwval;
	struct {
		u32 hsync_width:15; /* default: 0x0; */
		u32 hsync_polarity:1; /* default: 0x0; */
		u32 res0:16; /* default: ; */
	} bits;
};

union dp_vsp_vsw_reg {
	u32 dwval;
	struct {
		u32 vsync_width:15; /* default: 0x0; */
		u32 vsync_polarity:1; /* default: 0x0; */
		u32 res0:16; /* default: ; */
	} bits;
};

union dp_misc_reg {
	u32 dwval;
	struct {
		u32 misc0:8; /* default: 0x0; */
		u32 misc1:8; /* default: 0x0; */
		u32 res0:16; /* default: ; */
	} bits;
};

union dp_mvid_reg {
	u32 dwval;
	struct {
		u32 mvid:24; /* default: 0x0; */
		u32 res0:8; /* default: ; */
	} bits;
};

union dp_nvid_reg {
	u32 dwval;
	struct {
		u32 nvid:24; /* default: 0x0; */
		u32 res0:8; /* default: ; */
	} bits;
};

union dp_training_ctrl_reg {
	u32 dwval;
	struct {
		u32 link_training_pattern:2; /* default: 0x0; */
		u32 res0:2; /* default: ; */
		u32 link_quality_pattern:3; /* default: 0x0; */
		u32 res1:24; /* default: ; */
		u32 training_en:1; /* default: 0x0; */
	} bits;
};

union dp_80b_ctrl0_reg {
	u32 dwval;
	struct {
		u32 custom_80b_31_0:32; /* default: 0x0; */
	} bits;
};

union dp_80b_ctrl1_reg {
	u32 dwval;
	struct {
		u32 custom_80b_63_32:32; /* default: 0x0; */
	} bits;
};

union dp_80b_ctrl2_reg {
	u32 dwval;
	struct {
		u32 custom_80b_79_64:16; /* default: 0x0; */
		u32 res0:16; /* default: ; */
	} bits;
};

union dp_aux_ctrl_reg {
	u32 dwval;
	struct {
		u32 res0:25; /* default: ; */
		u32 native_aux_mode:1; /* default: 0x0; */
		u32 res1:2; /* default: ; */
		u32 hpd_ignore:1; /* default: 0x0; */
		u32 res2:1; /* default: ; */
		u32 aux_reset:1; /* default: 0x0; */
		u32 aux_start:1; /* default: 0x0; */
	} bits;
};

union dp_aux_addr_reg {
	u32 dwval;
	struct {
		u32 addr:20; /* default: 0x0; */
		u32 res0:12; /* default: ; */
	} bits;
};

union dp_aux_rw_len_reg {
	u32 dwval;
	struct {
		u32 rlen:8; /* default: 0x0; */
		u32 wlen:8; /* default: 0x0; */
		u32 res0:16; /* default: ; */
	} bits;
};

union dp_aux_fifo_status_reg {
	u32 dwval;
	struct {
		u32 write_fifo_valid_cnt:5; /* default: 0x0; */
		u32 res0:10; /* default: ; */
		u32 write_fifo_rst:1; /* default: 0x0; */
		u32 read_fifo_valid_cnt:5; /* default: 0x0; */
		u32 res1:10; /* default: ; */
		u32 read_fifo_rst:1; /* default: 0x0; */
	} bits;
};

union dp_aux_rfifo_reg {
	u32 dwval;
	struct {
		u32 aux_read_fifo:8; /* default: 0x0; */
		u32 res0:24; /* default: ; */
	} bits;
};

union dp_aux_wfifo_reg {
	u32 dwval;
	struct {
		u32 aux_write_fifo:8; /* default: 0x0; */
		u32 res0:24; /* default: ; */
	} bits;
};

union dp_aux_status_reg {
	u32 dwval;
	struct {
		u32 status:4; /* default: 0x0; */
		u32 res0:28; /* default: ; */
	} bits;
};

union dp_debug3_reg {
	u32 dwval;
	struct {
		u32 vsc_packet_size:7; /* default: 0x0; */
		u32 res0:9; /* default: ; */
		u32 bist_mode_clk_src:1; /* default: 0x0; */
		u32 res1:13; /* default: ; */
		u32 mchstr_rx_mode:1; /* default: 0x0; */
		u32 hpd_det_en:1; /* default: 0x0; */
	} bits;
};

union dp_debug4_reg {
	u32 dwval;
	struct {
		u32 thred_p1_set:6; /* default: 0x7; */
		u32 thred_n1_set:6; /* default: 0x7; */
		u32 thred_p2_set:6; /* default: 0x13; */
		u32 thred_n2_set:6; /* default: 0x13; */
		u32 thred_f1_set:6; /* default: 0x19; */
		u32 res0:2; /* default: ; */
	} bits;
};

union dp_debug5_reg {
	u32 dwval;
	struct {
		u32 start_edgenum:8; /* default: 0x16; */
		u32 entx_force:1; /* default: 0x0; */
		u32 entx_set:1; /* default: 0x0; */
		u32 enrx_force:1; /* default: 0x0; */
		u32 enrx_set:1; /* default: 0x0; */
		u32 sr_mode:1; /* default: 0x0; */
		u32 res0:18; /* default: ; */
		u32 bist_en:1; /* default: 0x0; */
	} bits;
};

union dp_pll_ctrl0_reg {
	u32 dwval;
	struct {
		u32 N:8; /* default: 0x2; */
		u32 ldo_vset:3; /* default: 0x4; */
		u32 cp:5; /* default: 0x2; */
		u32 cp1:5; /* default: 0x2; */
		u32 res0:3; /* default: ; */
		u32 sdiv3_5:1; /* default: 0x1; */
		u32 output_div2:1; /* default: 0x1; */
		u32 N_sel:1; /* default: 0x0; */
		u32 res1:1; /* default: ; */
		u32 sdm_en:1; /* default: 0x0; */
		u32 ldo_en:1; /* default: 0x0; */
		u32 cp_en:1; /* default: 0x0; */
		u32 dp_pll_en:1; /* default: 0x0; */
	} bits;
};

union dp_pll_ctrl1_reg {
	u32 dwval;
	struct {
		u32 res0:32; /* default: ; */
	} bits;
};

union dp_pll_ctrl2_reg {
	u32 dwval;
	struct {
		u32 sdm_bot:17; /* default: 0x0; */
		u32 sdm_frq:2; /* default: 0x0; */
		u32 sdm_m12:1; /* default: 0x0; */
		u32 sdm_stp:9; /* default: 0x0; */
		u32 sdm_mode:2; /* default: 0x0; */
		u32 sdm_en:1; /* default: 0x0; */
	} bits;
};

union dp_pll_ctrl3_reg {
	u32 dwval;
	struct {
		u32 res0:32; /* default: ; */
	} bits;
};

union dp_drv_ctrl0_reg {
	u32 dwval;
	struct {
		u32 en_bi:5; /* default: 0x0; */
		u32 res0:3; /* default: ; */
		u32 en_res:1; /* default: 0x0; */
		u32 res1:3; /* default: ; */
		u32 en_cald:1; /* default: 0x0; */
		u32 en_cala:1; /* default: 0x0; */
		u32 res2:2; /* default: ; */
		u32 en_p2s:4; /* default: 0x0; */
		u32 en_tx:4; /* default: 0x0; */
		u32 en_bv:4; /* default: 0x0; */
		u32 en_ck:1; /* default: 0x0; */
		u32 en_cp:1; /* default: 0x0; */
		u32 en_ib:1; /* default: 0x0; */
		u32 en_ldo:1; /* default: 0x0; */
	} bits;
};

union dp_drv_ctrl1_reg {
	u32 dwval;
	struct {
		u32 reg_saux:5; /* default: 0x4; */
		u32 reg_slv:3; /* default: 0x3; */
		u32 reg_sres2a:5; /* default: 0x0; */
		u32 res0:3; /* default: ; */
		u32 reg_svr:2; /* default: 0x2; */
		u32 reg_csmps:2; /* default: 0x0; */
		u32 reg_calopt:2; /* default: 0x1; */
		u32 reg_ckss:2; /* default: 0x0; */
		u32 res1:4; /* default: ; */
		u32 reg_ckpdlyopt:1; /* default: 0x1; */
		u32 reg_swhv:1; /* default: 0x0; */
		u32 reg_swbv:1; /* default: 0x0; */
		u32 reg_bswopt:1; /* default: 0x0; */
	} bits;
};

union dp_drv_ctrl2_reg {
	u32 dwval;
	struct {
		u32 reg_p2_0:4; /* default: 0x0; */
		u32 reg_p2_1:4; /* default: 0x0; */
		u32 reg_p2_2:4; /* default: 0x0; */
		u32 reg_p2_3:4; /* default: 0x0; */
		u32 reg_sp2_0:4; /* default: 0x0; */
		u32 reg_sp2_1:4; /* default: 0x0; */
		u32 reg_sp2_2:4; /* default: 0x0; */
		u32 reg_sp2_3:4; /* default: 0x0; */
	} bits;
};

union dp_drv_ctrl3_reg {
	u32 dwval;
	struct {
		u32 reg_p1opt:4; /* default: 0x0; */
		u32 reg_p2opt:4; /* default: 0x0; */
		u32 reg_plr:4; /* default: 0x0; */
		u32 reg_den:4; /* default: 0xf; */
		u32 reg_mc0:4; /* default: 0xf; */
		u32 reg_mc1:4; /* default: 0xf; */
		u32 reg_mc2:4; /* default: 0xf; */
		u32 reg_mc3:4; /* default: 0xf; */
	} bits;
};

union dp_drv_ctrl4_reg {
	u32 dwval;
	struct {
		u32 reg_sp1_0:5; /* default: 0x0; */
		u32 res0:3; /* default: ; */
		u32 reg_sp1_1:5; /* default: 0x0; */
		u32 res1:3; /* default: ; */
		u32 reg_sp1_2:5; /* default: 0x0; */
		u32 res2:3; /* default: ; */
		u32 reg_sp1_3:5; /* default: 0x0; */
		u32 reg_sron:2; /* default: 0x0; */
		u32 ret_ck_test:1; /* default: 0x0; */
	} bits;
};

union dp_drv_ctrl5_reg {
	u32 dwval;
	struct {
		u32 reg_p1_0:5; /* default: 0x1; */
		u32 res0:3; /* default: ; */
		u32 reg_p1_1:5; /* default: 0x1; */
		u32 res1:3; /* default: ; */
		u32 reg_p1_2:5; /* default: 0x1; */
		u32 res2:3; /* default: ; */
		u32 reg_p1_3:5; /* default: 0x1; */
		u32 res3:3; /* default: ; */
	} bits;
};

union dp_dbg_status0_reg {
	u32 dwval;
	struct {
		u32 cur_line:16; /* default: 0x0; */
		u32 res0:16; /* default: ; */
	} bits;
};

union dp_mem_test_reg {
	u32 dwval;
	struct {
		u32 sram_pat:32; /* default: ; */
	} bits;
};

union dp_reservd_reg {
	u32 dwval;
	struct {
		u32 res0; /* default: ; */
	} bits;
};

/* device define */
struct dp_dev_t {
	union dp_version_id_reg dp_version_id; /* 0xffffffffffffffff */
	union dp_int_mask_reg dp_int_mask; /* 0x004 */
	union dp_int_status_reg dp_int_status; /* 0x008 */
	union dp_line_triger_ctrl_reg dp_line_triger_ctrl; /* 0x00c */
	union dp_ctrl_reg dp_ctrl; /* 0x010 */
	union dp_start_dly_reg dp_start_dly; /* 0x014 */
	union dp_general_timer_reg dp_general_timer; /* 0x018 */
	union dp_scramble_seed_reg dp_scramble_seed; /* 0x01c */
	union dp_vid_ctrl_reg dp_vid_ctrl; /* 0x020 */
	union dp_vid_idle_ctrl_reg dp_vid_idle_ctrl; /* 0x024 */
	union dp_vid_tu_ctrl_reg dp_vid_tu_ctrl; /* 0x028 */
	union dp_reservd_reg dp_top_reg02c; /* 0x02c */
	union dp_vid_frame_ctrl_reg dp_vid_frame_ctrl; /* 0x030 */
	union dp_reservd_reg dp_top_reg034[3]; /* 0x034 */
	union dp_hwidth_reg dp_hwidth; /* 0x040 */
	union dp_vheight_reg dp_vheight; /* 0x044 */
	union dp_htotal_reg dp_htotal; /* 0x048 */
	union dp_vtotal_reg dp_vtotal; /* 0x04c */
	union dp_hstart_reg dp_hstart; /* 0x050 */
	union dp_vstart_reg dp_vstart; /* 0x054 */
	union dp_hsp_hsw_reg dp_hsp_hsw; /* 0x058 */
	union dp_vsp_vsw_reg dp_vsp_vsw; /* 0x05c */
	union dp_misc_reg dp_misc; /* 0x060 */
	union dp_mvid_reg dp_mvid; /* 0x064 */
	union dp_nvid_reg dp_nvid; /* 0x068 */
	union dp_reservd_reg dp_top_reg06c[101];/* 0x06c~0x1fc */
	union dp_training_ctrl_reg dp_training_ctrl; /* 0x200 */
	union dp_reservd_reg dp_top_reg204[11];/* 0x204~0x22c */
	union dp_80b_ctrl0_reg dp_80b_test_ctrl0; /* 0x230 */
	union dp_80b_ctrl1_reg dp_80b_test_ctrl1; /* 0x234 */
	union dp_80b_ctrl2_reg dp_80b_test_ctrl2; /* 0x238 */
	union dp_reservd_reg dp_top_reg23c[49];/* 0x23c~0x2fc */
	union dp_aux_ctrl_reg dp_aux_ctrl;/* 0x300 */
	union dp_aux_addr_reg dp_aux_addr;/* 0x304 */
	union dp_aux_rw_len_reg dp_aux_rw_len;/* 0x308 */
	union dp_aux_fifo_status_reg dp_aux_fifo_status;/* 0x30c */
	union dp_aux_rfifo_reg dp_aux_rfifo; /* 0x310 */
	union dp_aux_wfifo_reg dp_aux_wfifo; /* 0x314 */
	union dp_aux_status_reg dp_aux_status; /* 0x318 */
	union dp_reservd_reg dp_top_reg31c[60];/* 0x31c~0x408 */
	union dp_debug3_reg dp_debug3; /* 0x40c */
	union dp_debug4_reg dp_debug4; /* 0x410 */
	union dp_debug5_reg dp_debug5; /* 0x414 */
	union dp_reservd_reg dp_top_reg418[58];/* 0x418~0x4fc */
	union dp_pll_ctrl0_reg dp_pll_ctrl0; /* 0x500 */
	union dp_pll_ctrl1_reg dp_pll_ctrl1; /* 0x504 */
	union dp_pll_ctrl2_reg dp_pll_ctrl2; /* 0x508 */
	union dp_pll_ctrl3_reg dp_pll_ctrl3; /* 0x50c */
	union dp_reservd_reg dp_top_reg510[4];/* 0x510~0x51f */
	union dp_drv_ctrl0_reg dp_drv_ctrl0; /* 0x520 */
	union dp_drv_ctrl1_reg dp_drv_ctrl1; /* 0x524 */
	union dp_drv_ctrl2_reg dp_drv_ctrl2; /* 0x528 */
	union dp_drv_ctrl3_reg dp_drv_ctrl3; /* 0x52c */
	union dp_drv_ctrl4_reg dp_drv_ctrl4; /* 0x530 */
	union dp_drv_ctrl5_reg dp_drv_ctrl5; /* 0x534 */
	union dp_reservd_reg dp_top_reg538[46];/* 0x538~0x5ec */
	union dp_dbg_status0_reg dp_dbg_status0; /* 0x5f0 */
	union dp_reservd_reg dp_top_reg5f4[642];/* 0x5f4~0xff8 */
	union dp_mem_test_reg dp_men_test;/* 0xffc */
};


#endif /* End of file */
