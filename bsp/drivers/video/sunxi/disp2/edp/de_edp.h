/* de_edp.h
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
#ifndef _DE_EDP_H
#define _DE_EDP_H

#include "drv_edp_common.h"
#include "de_edp_hal.h"
#include "de_edp_param.h"
#include "de_edp_core.h"
#include "de_edp_config.h"

/**
 * @name       :dp_init
 * @brief      :init edp module
 * @param[IN]  :sel index of edp
 * @param[IN]  :lane_cnt number of lane
 * @param[IN]  :bit_rate bit rate of lane
 * @return     :0 if success
 */
s32 dp_init(u32 sel, u32 lane_cnt, u64 bit_rate);

/**
 * @name       :dp_get_training_info
 * @brief      :get training info
 * @param[OUT] :train_inf:
 * @return     :0 if success
 */
s32 dp_get_training_info(struct training_info *train_inf);

/**
 * @name       :dp_training
 * @brief      :dp training call after edp_hal_ctrl_init and dp_sink_init
 * @param[IN]  :init_swing_lv: swing level
 * @return     :0: training successful; -1: training error
 */
s32 dp_training(u32 sel, u32 init_swing_lv);

s32 dp_enable(u32 sel, struct edp_para *para, struct disp_video_timings *tmg);

s32 dp_disable(u32 sel);

s32 dp_get_start_dly(u32 sel);

u32 dp_get_cur_line(u32 sel);

void dp_set_reg_base(u32 sel, uintptr_t base);

void phy_cfg(u32 sel, u8 swing_lv, u8 preemp_lv, u8 postcur2_lv);
void new_phy_cfg(u32 sel, u8 sw0, u8 pre0, u8 sw1, u8 pre1, u8 sw2, u8 pre2,
		 u8 sw3, u8 pre3);

void dp_int_enable(u32 sel, enum edp_int interrupt, bool en);

s32 dp_int_query(u32 sel, enum edp_int interrupt);

void dp_int_clear(u32 sel, enum edp_int interrupt);

u32 dp_get_hpd_status(u32 sel);

void dp_moudle_en(u32 sel, u32 en);
s32 dp_hpd_enable(u32 sel, u32 lane_cnt, u64 bit_rate);
void dp_show_builtin_patten(u32 sel, u32 patten);

extern int training_aux_rd_interval_CR;
extern int training_aux_rd_interval_EQ;
extern int sink_ssc_flag;
extern char fp_tx_buf[16];
extern char fp_rx_buf[16];
extern unsigned int glb_lane_cnt;
extern unsigned long long glb_bit_rate;
extern unsigned int lane_cnt;
extern unsigned long long ls_clk;
extern int g_lane_cfg[2];

#endif /* End of file */
