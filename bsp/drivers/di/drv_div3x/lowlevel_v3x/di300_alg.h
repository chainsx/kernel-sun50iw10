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

#ifndef _DI300_ALG_H_
#define _DI300_ALG_H_

#include <linux/types.h>
#include "../di_client.h"

enum FILMMODE {
	FM_22 = 0,
	FM_32,
	FM_2332,
	FM_2224,
	FM_32322,
	FM_55,
	FM_64,
	FM_87,
	FM_NULL = 10
};

/* array */
#define PROD22 2
#define PROD32 5
#define PROD2332 10
#define PROD2224 10
#define PROD32322 12
#define PROD55 5
#define PROD64 10
#define PROD87 15

/* software algorithm parameters */
struct __fod_alg_para {
	u32 fod_alg_en;	/* enable field order detection alg */

	s32 R_kick;
	s32 R_rev_10;		/* R_rev * 10 */
	s32 T_rev;
	u32 T_rev_time;
};

struct __fmd_alg_para {
	u32 fmd_alg_en;	/* enable film mode detection alg */

	u32 fmd_55_en;
	u32 fmd_64_en;
	u32 fmd_87_en;
	u32 fmd_2332_en;
	u32 fmd_2224_en;
	u32 fmd_32322_en;
	u32 fmd_32_en;
	u32 fmd_22_en;

	/* non-22 detect */
	u32 r_sc;
	s32 hl_ratio;
	u32 r_sigma3;
	s32 t_mlb;
	s32 t_value;
	u32 r_lowcad;
	u32 r_fb_sc;
	s32 t_sigma3_pixel_d10;
	u32 fid_trace_en;
	u32 t_p1diff_trace_pixel;

	/* 22 detect */
	u32 t_p1diffthrl_pixel;
	u32 t_p1diffthrh_pixel;
	u32 avgns_pixel;
	u32 r_film22rel;
	u32 r_film22rel2;
	s32 sc_trace_fnum;	/* sc_trace_fieldnum */

	/* scene change */
	u32 t_p1diff_pixel;
	u32 t_p1diff_u_pixel;

	/* lock para */
	s32 period22_x0;	/* required period to detect 2-2 */
	s32 period22_10;	/* required period to detect 2-2 */

	s32 period32;		/* required period to detect 3-2 */
	s32 period2224;		/* required period to detect 2-2-2-4 */
	s32 period2332;		/* required period to detect 2-3-3-2 */
	s32 period32322;	/* required period to detect 3-2-3-2-2 */
	s32 period55;		/* required period to detect 5-5 */
	s32 period64;		/* required period to detect 6-4 */
	s32 period87;		/* required period to detect 8-7 */
};

struct __vof_alg_para {
	u32 vof_alg_en;	/* enable Video-On-Film detection alg */

	/* Fade out Field number for Video disappear */
	u32 fade_out_video_field_num;
	/* 5 field max video row number threshold */
	u32 video_field_th0;
	/* sum of 5 field max video row number threshold */
	u32 video_field_th1;
	/* 10 field max video row number threshold */
	u32 video_field_th2;

	/* Fade in Field number for Text appear */
	u32 fade_in_text_field_num;
	/* Fade out Field number for Text disappear */
	u32 fade_out_text_field_num;
	/* N field sum of max text row number threshold */
	u32 text_field_th0;
	/* N field max text row number threshold */
	u32 text_field_th1;
	/* N field text row number threshold */
	u32 text_field_th2;
	/* Up vertical position threshold for text */
	u32 text_field_pos_u;
	/* Down vertical position threshold for text */
	u32 text_field_pos_l;
};

struct __itd_alg_para {
	u32 itd_alg_en;
	/* TODO: add interlace detection alg parameter here! */
	/* non-22 detect */
	u32 r_sc;

	/* 22 detect */
	u32 t_p1diffthrl_pixel;
	u32 t_p1diffthrh_pixel;
	u32 avgns_pixel;
	u32 r_film22rel;
	u32 r_film22rel2;
	s32 sc_trace_fnum;	/* sc_trace_fieldnum */

	/* scene change */
	u32 t_p1diff_pixel;
	u32 t_p1diff_u_pixel;

	/* lock para */
	s32 period22_x0;	/* required period to detect 2-2 */
	s32 period22_10;	/* required period to detect 2-2 */
};

struct __tnr_alg_para {
	u32 tnr_alg_en;
	/* TODO: add tnr adaptive gain control alg parameter here! */
	s32 tnr_mode;
	u32 tnr_adaptive_gain_th;
	u32 tnr_adaptive_gain_th_l;
	u32 k_max_still_num;
	u32 k_max_still_den;
	u32 k_max_noise_num;
	u32 k_max_noise_den;
};

struct __alg_para_t {
	u32 alg_en;	/* enable software alg */

	struct __fod_alg_para fod_alg_para;
	struct __fmd_alg_para fmd_alg_para;
	struct __vof_alg_para vof_alg_para;
	struct __itd_alg_para itd_alg_para;
	struct __tnr_alg_para tnr_alg_para;
};

/* DI parameters */
struct __di_para_t {
	s32 width;
	s32 height;
	u32 bff;
	u32 blksize;

	struct __alg_para_t alg_para;
};

/* Algorithm Histogram */
struct __fod_alg_hist {
	u32 bff_fix;	/* The detected Field Order */
	u32 is_fieldorderchange;
	u32 rev_cnt_tff;
	u32 rev_cnt_bff;
};

/* determine by the maximum field number for 22 film mode detection */
#define FMD22FIELDNUM 50
/* determine by the maximum field number for all non22 film mode detection */
#define FMD32FIELDNUM 15

struct __itd_alg_hist {
	/* main */
	u32 weave_phase_f3;
	u32 is_temp_di_f3;

	u32 weave_phase_f4;
	u32 is_temp_di_f4;

	u32 is_scenechange_f3;
	u32 is_scenechange_f4;

	/* 2-2 */
	u32 is_progressive_lock;
	u32 weave_phase_22;

	/* other para */
	s32 init_field_cnt_22;
	u32 p1_init_phase;

	u32 cad_low_length;
	u32 sc_length;
	s32 sc_trace_cnt;
	u32 pre_totalfid;	/* field diff of last field */
	u32 pre_totalfid_valid;

	u32 cadence_cur[FMD22FIELDNUM];
	u32 cad_low_array[10];
	u32 sc_array[4];
};

struct __fmd_alg_hist {
	/* main */
	u32 is_fm_lock_f3;
	u32 weave_phase_f3;
	u32 is_temp_di_f3;
	u32 film_mode_f3;

	u32 is_fm_lock_f4;
	u32 weave_phase_f4;
	u32 is_temp_di_f4;
	u32 film_mode_f4;

	u32 is_scenechange_f3;
	u32 is_scenechange_f4;

	/* 2-2 */
	u32 is_22_lock;
	u32 weave_phase_22;
	u32 is_22_temp_di;

	/* non 2-2 */
	u32 is_non22_lock;
	u32 film_mode_non22;
	u32 weave_phase_non22;
	u32 is_non22_temp_di;

	/* other para */
	s32 init_field_cnt_22;
	s32 init_field_cnt_non22;
	u32 p1_init_phase;
	u32 p2_init_phase;

	s32 la_length;
	s32 ha_length;

	u32 cad_low_length;
	u32 sc_length;
	s32 sc_trace_cnt;
	u32 pre_totalfid;	/* field diff of last field */
	u32 pre_totalfid_valid;

	u32 p2_array[16];
	u32 lowhigh[FMD32FIELDNUM];
	u32 cadence_cur[FMD22FIELDNUM];
	u32 cad_low_array[10];
	u32 p2diff_la[16];
	u32 p2diff_ha[16];
	u32 low_index[16];	/* record low cad field number */
	u32 high_index[16];	/* record low cad field number */
	u32 sc_array[4];
	u32 field_index;	/* current field index */
};

struct __vof_alg_hist {
	/* video field detect */
	/* History Max Video Row Number of past fields */
	u32 field_max_video_num_array[10];

	u32 video_field_exist_f3;	/* Video Exist in this field3 */
	u32 video_field_exist_f4;	/* Video Exist in this field4 */

	/* text field detect */
	/* History Max Text Row Number of past field */
	u32 field_max_text_num_array[59];
	/* History Text Row Number of past field */
	u32 field_text_num_array[59];

	u32 text_field_exist_f3;	/* Text Exist in this field3 */
	u32 text_field_exist_f4;	/* Text Exist in this field4 */

};

struct __tnr_alg_hist {
	u32 gain;
};

struct __alg_hist {
	struct __fod_alg_hist fod_alg_hist;
	struct __fmd_alg_hist fmd_alg_hist;
	struct __vof_alg_hist vof_alg_hist;
	struct __itd_alg_hist itd_alg_hist;
	struct __tnr_alg_hist tnr_alg_hist;
};

/* Hardware Histogram */
struct __fmd_hist {
	u32 FMD_FID12;	/* 0x1e0 */
	u32 FMD_FID23;	/* 0x1e4 */
	u32 FOD_FID30;	/* 0x1e8 */
	u32 FOD_FID32;	/* 0x1ec */
	u32 FOD_FID10;	/* 0x1f0 */
	u32 FOD_FID12;	/* 0x1f4 */
	u32 FMD_FRD02;	/* 0x1f8 */
	u32 FMD_FRD13;	/* 0x1fc */

	u32 FIELD_MAX_VIDEO_NUM_F3;	/* 0x21c */
	u32 FIELD_MAX_TEXT_NUM_F3;
	u32 FIELD_MAX_TEXT_POS_F3;
	u32 FIELD_TEXT_ROW_NUM_F3;

	u32 FIELD_MAX_VIDEO_NUM_F4;	/* 0x220 */
	u32 FIELD_MAX_TEXT_NUM_F4;
	u32 FIELD_MAX_TEXT_POS_F4;
	u32 FIELD_TEXT_ROW_NUM_F4;
};

struct __itd_hist {
	u32 FMD_FID12;	/* 0x1e0 */
	u32 FMD_FID23;	/* 0x1e4 */
};

struct __tnr_hist {
	u32 tnr_sum_weight_y;	/* 0x26C */
	u32 tnr_sum_weight_u;	/* 0x270 */
	u32 tnr_sum_weight_v;	/* 0x274 */
	u32 tnr_sum_gain_y;	/* 0x278 */
	u32 tnr_sum_gain_u;	/* 0x27C */
	u32 tnr_sum_gain_v;	/* 0x280 */
	u32 tnr_sum_still_out;	/* 0x284 */
	u32 tnr_sum_weight_y_cnt;	/* 0x288 */
	u32 tnr_sum_gain_y_cnt;	/* 0x28C */
};

struct __hw_hist {
	struct __fmd_hist fmd_hist;
	struct __tnr_hist tnr_hist;
	struct __itd_hist itd_hist;
};

struct di_dev_proc_result {
	/* the part of input-paras by reading reg */
	struct __hw_hist hw_hist;
	/*
	 * output-paras from dealing input-paras.
	 * some of output-paras may become input-paras in next dealing.
	 */
	struct __alg_hist alg_hist;

	struct __di_para_t di_para;
};

void di_alg(struct di_dev_proc_result *proc_rst);
void di_alg_fixed_para(struct di_client *c,
		       struct di_dev_proc_result *proc_rst);
void di_alg_hist_to_hardware(struct di_client *c,
			     struct di_dev_proc_result *proc_rst);

#endif /* _DI300_ALG_H_ */
