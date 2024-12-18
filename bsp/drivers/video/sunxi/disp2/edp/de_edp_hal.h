/* de_edp_hal.h
 *
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
 * interface of edp driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef _DE_EDP_HAL_H
#define _DE_EDP_HAL_H

#include "de_edp_param.h"
#include "drv_edp_common.h"

void edp_hal_video_timing_cfg(u32 sel, struct disp_video_timings *tmg);
void edp_hal_video_para_cfg(u32 sel, struct video_para *para, u32 color_level,
			    u32 src, u32 dly, u32 total_symbol_per_line);
void phy_drv_ctrl0_init(u32 sel);
void edp_hal_phy_drive_ctrl_cfg(u32 sel, u32 mc, u32 tap1, u32 tap2,
				u32 tap1_op, u32 tap2_op, u32 glb_lane_cnt);
void edp_hal_phy_pll_set(u32 sel, unsigned long long bit_rate);
int edp_hal_phy_res_cal(u32 sel);
void edp_hal_phy_exit(u32 sel);
void edp_hal_dp_scramble_seed(u32 sel, u32 seed);
void edp_hal_dp_framing_mode(u32 sel, u32 mode);
void edp_hal_video_enable(u32 sel);
void edp_hal_video_disable(u32 sel);
void edp_hal_int_enable(u32 sel, enum edp_int intterupt);
void edp_hal_int_disable(u32 sel, enum edp_int intterupt);
void edp_hal_set_line_cnt(u32 sel, unsigned int line0_cnt,
			  unsigned int line1_cnt);
int aux_wr(u32 sel, int addr, int len, char *buf);
int aux_rd(u32 sel, int addr, int len, char *buf);
void edp_hal_link_training_ctrl(u32 sel, int edp_en, int link_quality_pat,
				int link_training_pat);
void edp_hal_ctrl_init(u32 sel, u32 lane_sel, u32 lane_speed);
void edp_hal_bist_clock_sel(u32 sel, u32 bist_clk_sel);
unsigned int edp_hal_get_hpd_status(u32 sel);
int edp_hal_get_int_status(u32 sel, enum edp_int intterupt);
void edp_hal_clr_int_status(u32 sel, enum edp_int intterupt);
int edp_hal_sram_test(u32 sel);
void edp_hal_ssc_enable_test(u32 sel, u32 bot, u32 frq, u32 m12, u32 stp,
			     u32 mod);
void edp_hal_ssc_disable_test(u32 sel);
void edp_hal_cts_pattern_cfg(u32 sel, unsigned char bit_rate);
void edp_hal_delay_ms(u32 sel, int t);
void edp_hal_set_reg_base(u32 sel, uintptr_t base);
uintptr_t edp_hal_get_reg_base(u32 sel);
u32 edp_hal_get_start_dly(u32 sel);
u32 edp_hal_get_cur_line(u32 sel);
void edp_hal_dp_module(u32 sel);
void edp_hal_dp_module_en(u32 sel, u32 en);
void edp_hal_show_builtin_patten(u32 sel, u32 patten);

#endif /* End of file */
