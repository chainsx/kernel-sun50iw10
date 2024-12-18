/*
 * Copyright (c) 2007-2018 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef _DI300_REG_H_
#define _DI300_REG_H_

#include <linux/types.h>

#define MD_FLAG_ALIGN 32

#define DI_VOF_BUF_REG_ADDR_A 0x10000
#define DI_VOF_BUF_REG_ADDR_B 0x10140
#define DI_VOF_BUF_REG_ADDR_C 0x10280
#define DI_VOF_BUF_REG_ADDR_D 0x103C0
#define DI_VOF_BUF_SIZE_MAX 0x140



union di_reset_reg {
	u32 dwval;
	struct {
		u32 res0:31;
		u32 reset:1;
	} bits;
};

union di_func_vsn_reg {
	u32 dwval;
	struct {
		u32 res0:16;
		u32 overlay_exist:2;
		u32 online_support:2;
		u32 offline_support:2;
		u32 dns_exist:2;
		u32 tnr_exist:2;
		u32 fmd_exist:2;
		u32 res1:4;
	} bits;
};

union di_start_reg {
	u32 dwval;
	struct {
		u32 start:1;
		u32 res0:31;
	} bits;
};

union di_int_ctl_reg {
	u32 dwval;
	struct {
		u32 finish_int_en:1;
		u32 res0:31;
	} bits;
};

union di_status_reg {
	u32 dwval;
	struct {
		u32 finish_flag:1;
		u32 res0:7;
		u32 busy:1;
		u32 res1:23;
	} bits;
};

union di_func_en_reg {
	u32 dwval;
	struct {
		u32 dit_en:1;
		u32 md_en:1;
		u32 tnr_en:1;
		u32 fmd_en:1;
		u32 res0:28;
	} bits;
};

union di_dma_ctl_reg {
	u32 dwval;
	struct {
		u32 dma_c:1; /* in_fb2 gating */
		u32 res0:3;
		u32 dma_p:1; /* in_fb1 gating */
		u32 res1:3;
		u32 dma_di:1; /* in_fb0 gating */
		u32 res2:3;
		u32 dmafr:1; /* pre-frame motion info gating */
		u32 res3:3;
		u32 diw0:1; /* out_fb0 gating */
		u32 res4:3;
		u32 diw1:1; /* out_fb1 gating */
		u32 res5:3;
		u32 tnrw:1; /* out_fbtnr gating */
		u32 res6:3;
		u32 dmafw:1; /* cur_frame motion info gating */
		u32 res7:2;
		u32 mclk_gate:1;
	} bits;
};

union di_rdma_cmd_ctl_reg {
	u32 dwval;
	struct {
		u32 dma_di_len:8;
		u32 dma_p_len:8;
		u32 dma_c_len:8;
		u32 dmafr_len:8;
	} bits;
};

union di_wdma_cmd_ctl_reg {
	u32 dwval;
	struct {
		u32 diw0_len:8;
		u32 diw1_len:8;
		u32 tnrw_len:8;
		u32 dmafw_len:8;
	} bits;
};

union di_size_reg {
	u32 dwval;
	struct {
		u32 width:11;
		u32 res0:5;
		u32 height:11;
		u32 res1:5;
	} bits;
};

/*
* format:
* 0:planner YUV420   1:UV combined YUV420
* 2:planner YUV422   3:UV combined YUV422
*/
union di_fmt_reg {
	u32 dwval;
	struct {
		u32 in0fmt:2; /* in_fb0 format */
		u32 res0:2;
		u32 in1fmt:2; /* in_fb1 format */
		u32 res1:2;
		u32 in2fmt:2; /* in_fb2 format */
		u32 res2:2;
		u32 tnrfmt:2; /* tnrout_fb format */
		u32 res3:2;
		u32 ditfmt:2; /* ditout_fb format */
		u32 res4:10;
		u32 uvseq:1; /* uv sequence: 0:vuvuvu  1:uvuvuv */
		u32 res5:3;
	} bits;
};

union di_field_order_reg {
	u32 dwval;
	struct {
		u32 bff:1; /* bottom field fist */
		u32 res0:31;
	} bits;
};

union di_2pitch_reg {
	u32 dwval;
	struct {
		u32 f01:16; /* field 0/1 */
		u32 f23:16; /* field 2/3 */
	} bits;
};

union di_pitch_reg {
	u32 dwval;
	struct {
		u32 val:16;
		u32 res0:16;
	} bits;
};

union di_haddr_reg {
	u32 dwval;
	struct {
		u32 addr0:8;
		u32 addr1:8;
		u32 addr2:8;
		u32 res0:8;
	} bits;
};

struct di_addr {
	u32 laddr[3];
	union di_haddr_reg haddr;
};

struct di_in_addr {
	struct di_addr top;
	struct di_addr bot;
};

struct di_field_addr_reg {
	u32 laddr[3];
	union di_haddr_reg haddr;
};

union di_flag_haddr_reg {
	u32 dwval;
	struct {
		u32 in_addr:8;
		u32 out_addr:8;
		u32 res0:16;
	} bits;
};

union di_bist_ctl_reg {
	u32 dwval;
	struct {
		u32 bist_start:1;
		u32 bist_busy:1;
		u32 res0:2;
		u32 bist_mode_en:1;
		u32 res1:11;
		u32 bist_finish:1;
		u32 bist_pass:1;
		u32 res2:14;
	} bits;
};

union di_bist_rand_reg {
	u32 dwval;
	struct {
		u32 bist_rand_seed:31;
		u32 bist_rand_en:1;
	} bits;
};

union di_bist_ext_addr_reg {
	u32 dwval;
	struct {
		u32 bist_ext_addr:16;
		u32 res0:15;
		u32 sram_clk_gating_mode:1;
	} bits;
};

union di_mclk_bist_ext_addr_reg {
	u32 dwval;
	struct {
		u32 sram_domain_sel:1;
		u32 res0:30;
		u32 sram_clk_gating_mode:1;
	} bits;
};

/* MD */
union md_para_reg {
	u32 dwval;
	struct {
		u32 min_luma_th:8;
		u32 max_luma_th:8;
		u32 avg_luma_shifter:4;
		u32 pix_static_th:2;
		u32 res0:2;
		u32 th_shift:4;
		u32 mov_fac_nonedge:2;
		u32 res1:2;
	} bits;
};

/* DIT */
union dit_setting_reg {
	u32 dwval;
	struct {
		/* Y mode, 0:weave mode  1:motion adaptive */
		u32 dit_mode_luma:1;
		u32 res0:3;
		/* Y, motion blending function */
		u32 motion_blend_luma_en:1;
		/* diagonal interpolation function enable */
		u32 diag_intp_en:1;
		u32 res1:2;
		u32 ela_demo_win_en:1;
		u32 field_weave_demo_win_en:1;
		u32 video_blend_demo_win_en:1;
		u32 res2:5;
		/* U/V mode, 0:weave mode  1:motion adaptive */
		u32 dit_mode_chroma:1;
		u32 res3:3;
		u32 motion_blend_chroma_en:1;
		u32 res4:3;
		/* 0:2-frame mode  1:1-frame mode(Only base on field2) */
		u32 output_mode:1;
		u32 res5:7;
	} bits;
};

union dit_chroma_md_para0_reg {
	u32 dwval;
	struct {
		u32 res0:8;
		u32 spatial_th:8;
		u32 diff_th:8;
		u32 res1:4;
		u32 pix_static_th:2;
		u32 res2:2;
	} bits;
};

union dit_chroma_md_para1_reg {
	u32 dwval;
	struct {
		u32 blend_mode:4;
		u32 res0:4;
		u32 font_protect_en:1;
		u32 res1:7;
		u32 font_protect_th:8;
		u32 font_protect_fac:5;
		u32 res2:3;
	} bits;
};

union dit_intra_intp_reg {
	u32 dwval;
	struct {
		u32 ela_a:3;
		u32 ela_en:1;
		u32 ela_c:4;
		u32 ela_cmax:8;
		u32 ela_maxrat:2;
		u32 res0:2;
		u32 angle_limit:5;
		u32 res1:3;
		u32 angle_const_th:3;
		u32 res2:1;
	} bits;
};

union dit_inter_intp_reg {
	u32 dwval;
	struct {
		u32 field_weave_f1:1;/* disable/enable weave for filed1 */
		u32 video_exist_f1:1; /* disable/enable video bleanding when field_weave_f1 = 1 */
		u32 field_weave_chroma_f1:1;;/* isable/enable weave for chroma channel of filed1 */
		u32 res0:1;
		u32 field_weave_phase_f1:2;
		u32 res1:2;
		u32 luma_cur_fac_f1:2;
		u32 res2:2;
		u32 chroma_cur_fac_f1:2;
		u32 res3:2;
		u32 field_weave_f2:1;
		u32 video_exist_f2:1;
		u32 field_weave_chroma_f2:1;
		u32 res4:1;
		u32 field_weave_phase_f2:2;
		u32 res5:2;
		u32 luma_cur_fac_f2:2;
		u32 res6:2;
		u32 chroma_cur_fac_f2:2;
		u32 res7:2;
	} bits;
};

/* FMD */
union fmd_diff_th0_reg {
	u32 dwval;
	struct {
		u32 p1_th:8;
		u32 p1_th_u:8;
		u32 p2_th:8;
		u32 p2_th_u:8;
	} bits;
};

union fmd_diff_th1_reg {
	u32 dwval;
	struct {
		u32 p1_th:8;
		u32 p1_th_u:8;
		u32 p2_th:8;
		u32 res0:8;
	} bits;
};

union fmd_diff_th2_reg {
	u32 dwval;
	struct {
		u32 p1_th:8;
		u32 p1_th_u:8;
		u32 p2_th:14;
		u32 res0:2;
	} bits;
};

union fmd_fid_hist_reg {
	u32 dwval;
	struct {
		u32 cnt:30;
		u32 res0:2;
	} bits;
};

union fmd_frd_hist_reg {
	u32 dwval;
	struct {
		u32 cnt:30;
		u32 res0:2;
	} bits;
};


union fmd_feat_th0_reg {
	u32 dwval;
	struct {
		u32 d_th:8;
		u32 highcomb_th:3;
		u32 res0:5;
		u32 mincontr:8;
		u32 vp_th:8;
	} bits;
};

union fmd_feat_th1_reg {
	u32 dwval;
	struct {
		u32 f_th:11;
		u32 res0:5;
		u32 c_th_l:3;
		u32 res1:5;
		u32 c_th_h:3;
		u32 res2:5;
	} bits;
};

union fmd_feat_th2_reg {
	u32 dwval;
	struct {
		u32 ccnt_th_l:5;
		u32 res0:3;
		u32 ccnt_th_m:5;
		u32 res1:3;
		u32 crowcnt_th:2;
		u32 res2:14;
	} bits;
};

union fmd_mot_th_reg {
	u32 dwval;
	struct {
		u32 m_th:8;
		u32 res0:8;
		u32 mcnt_th:5;
		u32 res1:3;
		u32 mrowcnt_th:2;
		u32 res2:6;
	} bits;
};

union fmd_text_th_reg {
	u32 dwval;
	struct {
		u32 t_th:8;
		u32 res0:24;
	} bits;
};

union fmd_blk_th_reg {
	u32 dwval;
	struct {
		u32 blk_video_th:8;
		u32 blk_text_th:8;
		u32 blk_motion_th:8;
		u32 res0:8;
	} bits;
};

union fmd_row_th_reg {
	u32 dwval;
	struct {
		u32 row_video_th:8;
		u32 row_text_th:8;
		u32 row_exit_video_th:8;
		u32 res0:8;
	} bits;
};

union fmd_field_hist_reg {
	u32 dwval;
	struct {
		u32 max_video_num:8;
		u32 max_text_num:8;
		u32 max_text_pos:8;
		u32 text_row_num:8;
	} bits;
};

union fmd_glb_set_reg {
	u32 dwval;
	struct {
		u32 blk_size:1;
		u32 res0:3;
		u32 vof_buf_sel:2;
		u32 res1:25;
		u32 vof_en:1;
	} bits;
};

union tnr_strength_reg {
	u32 dwval;
	struct {
		u32 v:8;
		u32 u:8;
		u32 y:8;
		u32 res0:8;
	} bits;
};

union tnr_dark_th_reg {
	u32 dwval;
	struct {
		u32 v:6;
		u32 res0:2;
		u32 u:6;
		u32 res1:2;
		u32 y:6;
		u32 res2:10;
	} bits;
};


union tnr_dark_protect_reg {
	u32 dwval;
	struct {
		u32 strength_v:3;
		u32 res0:5;
		u32 strength_u:3;
		u32 res1:5;
		u32 strength_y:3;
		u32 res2:13;
	} bits;
};

union tnr_dark_para_reg {
	u32 dwval;
	struct {
		u32 std_shift:8;
		u32 std_slope:8;
		u32 avg_shift:8;
		u32 avg_slope:8;
	} bits;
};

/* fth: feather */
union tnr_fth_detect_reg {
	u32 dwval;
	struct {
		u32 th_fth3:3;
		u32 res0:5;
		u32 th_fth7:3;
		u32 res1:5;
		u32 th_st_fth:8; /* strong fth */
		u32 res2:8;
	} bits;
};

union tnr_dt_filter_reg {
	u32 dwval;
	struct {
		u32 th_clamp:4;
		u32 res0:12;
		u32 th_max:7;
		u32 res1:9;
	} bits;
};

/* lbound: lower bound */
union tnr_weight_lbound_reg {
	u32 dwval;
	struct {
		u32 th_down3:8;
		u32 th_down2:8;
		u32 th_down1:8;
		u32 th_down0:8;
	} bits;
};

/* abn: abnormal */
union tnr_abn_detect_reg {
	u32 dwval;
	struct {
		u32 th_abn_weight:3;
		u32 res0:5;
		u32 th_fmm:3;/* fmm: film mode movement */
		u32 res1:5;
		u32 th_max_diff:8;
		u32 res2:8;
	} bits;
};

union tnr_th_sum_reg {
	u32 dwval;
	struct {
		u32 th_sum_weight_v:2;
		u32 res0:2;
		u32 th_sum_weight_u:2;
		u32 res1:2;
		u32 th_sum_weight_y:2;
		u32 res2:6;
		u32 th_sum_gain_v:2;
		u32 res3:2;
		u32 th_sum_gain_u:2;
		u32 res4:2;
		u32 th_sum_gain_y:2;
		u32 res5:6;
	} bits;
};

union tnr_th_dark_dither_reg {
	u32 dwval;
	struct {
		u32 th_random_dither_v:4;
		u32 res0:4;
		u32 th_random_dither_u:4;
		u32 res1:4;
		u32 th_random_dither_y:4;
		u32 res2:12;
	} bits;
};

union tnr_random_func_cfg_reg {
	u32 dwval;
	struct {
		u32 rand_num_bits:5;
		u32 res0:3;
		u32 mod_bits:5;
		u32 res1:3;
		u32 rand_para_q:4;
		u32 res2:4;
		u32 rand_para_p:4;
		u32 res3:4;
	} bits;
};

union tnr_md_result_reg {
	u32 dwval;
	struct {
		u32 is_scene_changed:1;
		u32 res0:15;
		u32 is_contain_bad_frame:1;
		u32 res1:15;
	} bits;
};

union tnr_coord_hori_reg {
	u32 dwval;
	struct {
		u32 left:11;
		u32 res0:5;
		u32 right:11;
		u32 res1:5;
	} bits;
};

union tnr_coord_vert_reg {
	u32 dwval;
	struct {
		u32 top:11;
		u32 res0:5;
		u32 bottom:11;
		u32 res1:5;
	} bits;
};

/* window */
union win_horz_reg {
	u32 dwval;
	struct {
		u32 x0:11;
		u32 res0:5;
		u32 x1:11;
		u32 res1:5;
	} bits;
};

union win_vert_reg {
	u32 dwval;
	struct {
		u32 y0:11;
		u32 res0:5;
		u32 y1:11;
		u32 res1:5;
	} bits;
};

/* DEBUG */
union top_debug0_reg {
	u32 dwval;
};

union top_debug1_reg {
	u32 dwval;
};

union top_debug2_reg {
	u32 dwval;
};

union top_debug3_reg {
	u32 dwval;
};

union top_debug4_reg {
	u32 dwval;
};

union top_debug5_reg {
	u32 dwval;
};

union top_debug6_reg {
	u32 dwval;
};

/* RSC */
union top_rand_ctl_reg {
	u32 dwval;
};

union top_rand_clk_reg {
	u32 dwval;
};

struct top_rsc {
	union top_rand_ctl_reg rsc_ctl;
	union top_rand_clk_reg rsc_clk;
};

/* register of di300 */
struct di_reg {
	/* TOP */
	union di_reset_reg reset;                      /* 0x000 */
	u32 res0[2];                                   /* 0x004 */
	union di_func_vsn_reg func_vsn;                /* 0x00C */
	union di_start_reg start;                      /* 0x010 */
	union di_int_ctl_reg int_ctl;                  /* 0x014 */
	union di_status_reg status;                    /* 0x018 */
	u32 ip_version;                                /* 0x01C */
	union di_func_en_reg func_en;                  /* 0x020 */
	union di_dma_ctl_reg dma_ctl;                  /* 0x024 */
	union di_rdma_cmd_ctl_reg rdma_cmd_ctl;        /* 0x028 */
	union di_wdma_cmd_ctl_reg wdma_cmd_ctl;        /* 0x02C */
	union di_size_reg size;                        /* 0x030 */
	union di_fmt_reg fmt;                          /* 0x034 */
	union di_field_order_reg forder;               /* 0x038 */
	u32 res1;                                      /* 0x03C */
	/* f01: frame 0,1 */  /* YUV */
	union di_2pitch_reg in_f01_pitch[3];           /* 0x040 */
	u32 res2;                                      /* 0x04C */
	/* f2: frame 2 */  /* YUV */
	union di_pitch_reg in_f2_pitch[3];             /* 0x050 */
	u32 res3;                                      /* 0x05C */
	  /* YUV */
	union di_pitch_reg out_tnr_pitch[3];           /* 0x060 */
	u32 res4;                                      /* 0x06C */
	  /* YUV */
	union di_pitch_reg out_dit_pitch[3];           /* 0x070 */
	u32 res5;                                      /* 0x07C */
	/* in&out flag itch of motion flag fb */
	union di_pitch_reg flag_pitch;                 /* 0x080 */
	u32 res6[3];                                   /* 0x084 */
	/* in_f0: input frame0 */
	struct di_in_addr in_f0_addr;                  /* 0x090~0x0AC */
	struct di_in_addr in_f1_addr;                  /* 0x0B0~0x0CC */
	struct di_in_addr in_f2_addr;                  /* 0x0D0~0x0EC */
	struct di_addr out_tnr_addr;                   /* 0x0F0~0x0FC */
	struct di_addr out_dit0_addr;                  /* 0x100~0x10C */
	struct di_addr out_dit1_addr;                  /* 0x110~0x11C */
	u32 in_flag_laddr;                             /* 0x120 */
	u32 out_flag_laddr;                            /* 0x124 */
	union di_flag_haddr_reg flag_haddr;            /* 0x128 */
	u32 res7[5];                                   /* 0x12C */
	union di_bist_ctl_reg bist_ctl;                /* 0x140 */
	union di_bist_rand_reg bist_rand;              /* 0x144 */
	u32 bist_data0;                                /* 0x148 */
	u32 bist_data1;                                /* 0x14C */
	u32 bist_data2;                                /* 0x150 */
	u32 bist_sts_cur_addr;                         /* 0x154 */
	union di_bist_ext_addr_reg bist_haddr;         /* 0x158 */
	union di_bist_ctl_reg mbist_ctl;               /* 0x15C */
	union di_bist_rand_reg mbist_rand;             /* 0x160 */
	u32 mbist_data0;                               /* 0x164 */
	u32 mbist_data1;                               /* 0x168 */
	u32 mbist_data2;                               /* 0x16c */
	u32 mbist_sts_cur_addr;                        /* 0x170 */
	union di_mclk_bist_ext_addr_reg mbist_haddr;   /* 0x174 */
	u32 res8[2];                                   /* 0x178 */

	/* MD */
	union md_para_reg md_para;                     /* 0x180 */
	u32 md_res0[3];                                /* 0x184 */
	union win_horz_reg md_croph;                   /* 0x190 */
	union win_vert_reg md_cropv;                   /* 0x194 */
	u32 md_res1[2];                                /* 0x198 */

	/* DIT */
	union dit_setting_reg dit_setting;             /* 0x1a0 */
	union dit_chroma_md_para0_reg dit_chr_para0;   /* 0x1a4 */
	union dit_chroma_md_para1_reg dit_chr_para1;   /* 0x1a8 */
	u32 dit_res0;                                  /* 0x1ac */
	union dit_intra_intp_reg dit_intra_para;       /* 0x1b0 */
	union dit_inter_intp_reg dit_inter_para;       /* 0x1b4 */
	u32 dit_res1[2];                               /* 0x1b8 */
	union win_horz_reg dit_croph;                  /* 0x1c0 */
	union win_vert_reg dit_cropv;                  /* 0x1c4 */
	union win_horz_reg dit_demoh;                  /* 0x1c8 */
	union win_vert_reg dit_demov;                  /* 0x1cc */

	/* FMD */ /* FMD difference Threshold */
	union fmd_diff_th0_reg fmd_diff_th0;           /* 0x1d0 */
	union fmd_diff_th1_reg fmd_diff_th1;           /* 0x1d4 */
	union fmd_diff_th2_reg fmd_diff_th2;           /* 0x1d8 */
	u32 fmd_res0;                                  /* 0x1dc */
	union fmd_fid_hist_reg fmd_fid12;              /* 0x1e0 */
	union fmd_fid_hist_reg fmd_fid23;              /* 0x1e4 */
	union fmd_fid_hist_reg fod_fid30;              /* 0x1e8 */
	union fmd_fid_hist_reg fod_fid32;              /* 0x1ec */
	union fmd_fid_hist_reg fod_fid10;              /* 0x1f0 */
	union fmd_fid_hist_reg fod_fid12;              /* 0x1f4 */
	union fmd_frd_hist_reg fmd_frd02;              /* 0x1f8 */
	union fmd_frd_hist_reg fmd_frd13;              /* 0x1fc */
	union fmd_feat_th0_reg fmd_feat_th0;           /* 0x200 */
	union fmd_feat_th1_reg fmd_feat_th1;           /* 0x204 */
	union fmd_feat_th2_reg fmd_feat_th2;           /* 0x208 */
	union fmd_mot_th_reg fmd_mot_th;               /* 0x20c */
	union fmd_text_th_reg fmd_text_th;             /* 0x210 */
	union fmd_blk_th_reg fmd_blk_th;               /* 0x214 */
	union fmd_row_th_reg fmd_row_th;               /* 0x218 */
	union fmd_field_hist_reg fmd_field_hist0;      /* 0x21c */
	union fmd_field_hist_reg fmd_field_hist1;      /* 0x220 */
	union fmd_glb_set_reg fmd_glb;                 /* 0x224 */
	u32 fmd_res1[2];                               /* 0x228 */
	union win_horz_reg fmd_croph;                  /* 0x230 */
	union win_vert_reg fmd_cropv;                  /* 0x234 */
	u32 fmd_res2[2];                               /* 0x238 */

	/* TNR */
	union tnr_strength_reg tnr_strength;           /* 0x240 */
	union tnr_dark_th_reg tnr_dark_th;             /* 0x244 */
	union tnr_dark_protect_reg tnr_dark_protect;   /* 0x248 */
	union tnr_dark_para_reg tnr_dark_para_y;       /* 0x24C */
	union tnr_dark_para_reg tnr_dark_para_u;       /* 0x250 */
	union tnr_dark_para_reg tnr_dark_para_v;       /* 0x254 */
	union tnr_fth_detect_reg tnr_fth_detect;       /* 0x258 */
	union tnr_dt_filter_reg tnr_dt_filter;         /* 0x25C */
	union tnr_weight_lbound_reg tnr_weight_lbound; /* 0x260 */
	union tnr_abn_detect_reg tnr_abn_detect;       /* 0x264 */
	union tnr_th_sum_reg tnr_th_sum;               /* 0x268 */
	u32 tnr_sum_weight_y;                          /* 0x26C */
	u32 tnr_sum_weight_u;                          /* 0x270 */
	u32 tnr_sum_weight_v;                          /* 0x274 */
	u32 tnr_sum_gain_y;                            /* 0x278 */
	u32 tnr_sum_gain_u;                            /* 0x27C */
	u32 tnr_sum_gain_v;                            /* 0x280 */
	u32 tnr_sum_still_out;                         /* 0x284 */
	u32 tnr_sum_weight_y_cnt;                      /* 0x288 */
	u32 tnr_sum_gain_y_cnt;                        /* 0x28C */
	union tnr_th_dark_dither_reg tnr_th_dark_dth;  /* 0x290 */
	union tnr_random_func_cfg_reg tnr_random_cfg;  /* 0x294 */
	u32 tnr_random_gen;                            /* 0x298 */
	union tnr_md_result_reg tnr_md_result;         /* 0x29C */
	union win_horz_reg tnr_croph;                  /* 0x2A0 */
	union win_vert_reg tnr_cropv;                  /* 0x2A4 */
	union win_horz_reg tnr_demoh;                  /* 0x2A8 */
	union win_vert_reg tnr_demov;                  /* 0x2AC */
	u32 tnr_res0[8];                               /* 0x2B0 */

};

#endif /* _DI300_REG_H_ */
