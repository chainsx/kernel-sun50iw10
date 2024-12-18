/* drivers/video/sunxi/disp2/disp/de/lowlevel_v3x/de_edp_hal.c
 *
 * Copyright (c) 2017 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
 * edp lowlevel interface
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#include "de_edp_config.h"
#include "de_edp_type.h"
#include "de_edp_hal.h"

static volatile struct dp_dev_t *dp_dev[2];

void edp_hal_set_reg_base(u32 sel, uintptr_t base)
{
	dp_dev[sel] = (struct dp_dev_t *)(base);
}

uintptr_t edp_hal_get_reg_base(u32 sel)
{
	return (uintptr_t)dp_dev[sel];
}

/**
 * @name       :edp_hal_video_timing_cfg
 * @brief      :video timing setting
 * @param[IN]  :sel: index of edp
 * @param[IN]  :tmp: timing struct
 * @return     :none
 */
void edp_hal_video_timing_cfg(u32 sel, struct disp_video_timings *tmg)
{
	dp_dev[sel]->dp_hwidth.bits.hwidth = tmg->x_res;
	    dp_dev[sel]->dp_vheight.bits.vheight = tmg->y_res;
	dp_dev[sel]->dp_htotal.bits.htotal = tmg->hor_total_time;
	dp_dev[sel]->dp_vtotal.bits.vtotal = tmg->ver_total_time;
	dp_dev[sel]->dp_hstart.bits.hstart = tmg->hor_back_porch;
	dp_dev[sel]->dp_vstart.bits.vstart = tmg->ver_back_porch;
	/* Synchronization signal is low valid */
	dp_dev[sel]->dp_hsp_hsw.bits.hsync_polarity = 0;
	dp_dev[sel]->dp_vsp_vsw.bits.vsync_polarity = 0;
	dp_dev[sel]->dp_hsp_hsw.bits.hsync_width = tmg->hor_sync_time;
	dp_dev[sel]->dp_vsp_vsw.bits.vsync_width = tmg->ver_sync_time;
}

/**
 * @name       :edp_hal_video_para_cfg
 * @brief      :video display param setting
 * @param[IN]  :src: Video Data Source Select
 * @param[IN]  :dly: Video Start Delay
 * @param[IN]  :total_symbol_per_line: Video Frame Timing
 * @return     :none
 */
void edp_hal_video_para_cfg(u32 sel, struct video_para *para, u32 color_level,
		       u32 src, u32 dly, u32 total_symbol_per_line)
{
	if (color_level == 6)
		dp_dev[sel]->dp_vid_ctrl.bits.vid_color_mode = 0;
	else if (color_level == 8)
		dp_dev[sel]->dp_vid_ctrl.bits.vid_color_mode = 1;

	dp_dev[sel]->dp_vid_ctrl.bits.vid_src_sel = src;
	dp_dev[sel]->dp_start_dly.bits.start_dly = dly;

	dp_dev[sel]->dp_vid_idle_ctrl.bits.idle_pattern_count = 3;

	dp_dev[sel]->dp_misc.bits.misc1 = VSC_SDP;
	dp_dev[sel]->dp_misc.bits.misc0 =
	    (((COLOR_FORMAT << 4) << 1) | CLOCK_MODE);

	dp_dev[sel]->dp_mvid.bits.mvid = para->mvid;
	dp_dev[sel]->dp_nvid.bits.nvid = para->nvid;
	/* Transfer Unit Cfg */
	dp_dev[sel]->dp_vid_tu_ctrl.bits.tu_size = TU_SIZE;
	dp_dev[sel]->dp_vid_tu_ctrl.bits.tu_fill_len = para->fill;

	dp_dev[sel]->dp_vid_frame_ctrl.bits.htotal_in_symbol_per_lane =
	    total_symbol_per_line;
}

/**
 * @name       :edp_hal_phy_drive_ctrl_cfg
 * @brief      :DP Phy Configure --- Drive Control Register
 * @param[IN]  :mc: Parameter --- dp_drv_ctrl3
 * @param[IN]  :tap1: Parameter --- dp_drv_ctrl5
 * @param[IN]  :tap1_op: Parameter --- dp_drv_ctrl4
 * @param[IN]  :tap2_op: Parameter --- dp_drv_ctrl2
 * @param[IN]  :glb_lane_cnt: the real number of lane
 * @return     :none
 */
void edp_hal_phy_drive_ctrl_cfg(u32 sel, u32 mc, u32 tap1, u32 tap2,
				u32 tap1_op, u32 tap2_op, u32 glb_lane_cnt)
{
	u32 temp;
	/* mc */
	dp_dev[sel]->dp_drv_ctrl3.dwval &= (~0xffff0000);
	temp = (mc >> ((MAX_LANE_NUM - glb_lane_cnt) << 2)) << 16;
	dp_dev[sel]->dp_drv_ctrl3.dwval |= temp;

	/* tap1 */
	dp_dev[sel]->dp_drv_ctrl5.dwval &= (~0xffffffff);
	temp = tap1 >> ((MAX_LANE_NUM - glb_lane_cnt) << 3);
	dp_dev[sel]->dp_drv_ctrl5.dwval |= temp;

	/* tap2 */
	dp_dev[sel]->dp_drv_ctrl2.dwval &= (~0x0000ffff);
	temp = tap2 >> ((MAX_LANE_NUM - glb_lane_cnt) << 2);
	dp_dev[sel]->dp_drv_ctrl2.dwval |= temp;

	/* tap1 option */
	dp_dev[sel]->dp_drv_ctrl4.dwval &= (~0xffffffff);
	temp = tap1_op >> ((MAX_LANE_NUM - glb_lane_cnt) << 3);
	dp_dev[sel]->dp_drv_ctrl4.dwval |= temp;

	/* tap2 option */
	dp_dev[sel]->dp_drv_ctrl2.dwval &= (~0xffff0000);
	temp = tap2_op >> ((MAX_LANE_NUM - glb_lane_cnt) << 2) << 16;
	dp_dev[sel]->dp_drv_ctrl2.dwval |= temp;

	/* tap1 enable */
	dp_dev[sel]->dp_drv_ctrl3.dwval &= (~0x0000000f);
	temp = 0xf >> (MAX_LANE_NUM - glb_lane_cnt);
	dp_dev[sel]->dp_drv_ctrl3.dwval |= temp;

	/* tap2 enable */
	dp_dev[sel]->dp_drv_ctrl3.dwval &= (~0x000000f0);
	temp = (0xf >> (MAX_LANE_NUM - glb_lane_cnt)) << 4;
	dp_dev[sel]->dp_drv_ctrl3.dwval |= temp;

	/* tx */
	dp_dev[sel]->dp_drv_ctrl0.dwval &= (~0x000f0000);
	temp = (0xf >> (MAX_LANE_NUM - glb_lane_cnt)) << 16; /* EN P2S */
	dp_dev[sel]->dp_drv_ctrl0.dwval |= temp;

	dp_dev[sel]->dp_drv_ctrl0.dwval &= (~0x0000001f);
	temp = (0xf >> (MAX_LANE_NUM - glb_lane_cnt)) | 0x10; /* EN BI */
	dp_dev[sel]->dp_drv_ctrl0.dwval |= temp;

	dp_dev[sel]->dp_drv_ctrl0.dwval &= (~0x0f000000);
	temp = (0xf >> (MAX_LANE_NUM - glb_lane_cnt)) << 24; /* EN BV */
	dp_dev[sel]->dp_drv_ctrl0.dwval |= temp;

	dp_dev[sel]->dp_drv_ctrl0.dwval &= (~0x00f00000);
	temp = ((0xf >> (MAX_LANE_NUM - glb_lane_cnt)) << 20); /* EN TX */
	dp_dev[sel]->dp_drv_ctrl0.dwval |= temp;

	dp_dev[sel]->dp_drv_ctrl1.dwval &= (~0x000c0000);
	temp = (0x2 << 18); /* CSMPS */
	dp_dev[sel]->dp_drv_ctrl1.dwval |= temp;
}

/**
 * @name       :edp_hal_phy_pll_set
 * @brief      :dp phy pll setting
 * @param[IN]  :bit_rate:bit rate of lane
 * @return     :none
 */
void edp_hal_phy_pll_set(u32 sel, unsigned long long bit_rate)
{
	dp_dev[sel]->dp_pll_ctrl0.bits.dp_pll_en = 1;
	dp_dev[sel]->dp_pll_ctrl0.bits.cp_en = 1;
	dp_dev[sel]->dp_pll_ctrl0.bits.ldo_en = 1;
	dp_dev[sel]->dp_pll_ctrl0.bits.output_div2 = 1;

	dp_dev[sel]->dp_pll_ctrl0.bits.cp1 = 2;
	dp_dev[sel]->dp_pll_ctrl0.bits.cp = 2;
	dp_dev[sel]->dp_pll_ctrl0.bits.ldo_vset = 4;

	if (bit_rate == BR_5P4G) {
		dp_dev[sel]->dp_pll_ctrl0.bits.sdiv3_5 = 1;
		dp_dev[sel]->dp_pll_ctrl0.bits.N = 0x2d; /* pll 5.4G */
		dp_dev[sel]->dp_drv_ctrl1.bits.reg_ckss = 0;
	} else if (bit_rate == BR_2P7G) {
		dp_dev[sel]->dp_pll_ctrl0.bits.sdiv3_5 = 1;
		dp_dev[sel]->dp_pll_ctrl0.bits.N = 0x2d; /* pll 2.7G */
		dp_dev[sel]->dp_drv_ctrl1.bits.reg_ckss = 2;
	} else if (bit_rate == BR_1P62G) {
		dp_dev[sel]->dp_pll_ctrl0.bits.sdiv3_5 = 0;
		dp_dev[sel]->dp_pll_ctrl0.bits.N = 0x2d; /* pll 1.62G */
		dp_dev[sel]->dp_drv_ctrl1.bits.reg_ckss = 2;
	}
}

/**
 * @name       :edp_hal_phy_res_cal
 * @brief      :dp phy res setting
 * @param[IN]  :sel index of edp
 * @return     :0
 */
int edp_hal_phy_res_cal(u32 sel)
{
	/* dp_dev[sel]->dp_int_mask.dwval = 0x00000000; */
	dp_dev[sel]->dp_drv_ctrl0.dwval = 0x00000000;
	dp_dev[sel]->dp_drv_ctrl0.bits.en_ldo = 1;
	dp_dev[sel]->dp_drv_ctrl0.bits.en_ib = 1;
	dp_dev[sel]->dp_drv_ctrl0.bits.en_cp = 1;
	dp_dev[sel]->dp_drv_ctrl0.bits.en_ck = 1;

	dp_dev[sel]->dp_drv_ctrl0.bits.en_res = 1;
	dp_dev[sel]->dp_drv_ctrl0.bits.en_cala = 1;
	dp_dev[sel]->dp_drv_ctrl0.bits.en_cald = 1;

	dp_dev[sel]->dp_drv_ctrl1.bits.reg_calopt = 1;
	edp_hal_delay_ms(sel, 5);
	dp_dev[sel]->dp_drv_ctrl0.bits.en_cala = 0;

	return 0;
}

/**
 * @name       :edp_hal_phy_exit
 * @brief      :dp phy exit function
 * @param[IN]  :sel index of edp
 * @return     :none
 */
void edp_hal_phy_exit(u32 sel)
{
	/* pd */
	dp_dev[sel]->dp_drv_ctrl0.dwval = 0x00000000;
	dp_dev[sel]->dp_drv_ctrl0.bits.en_cp = 1;
	dp_dev[sel]->dp_drv_ctrl0.bits.en_ldo = 1;
	dp_dev[sel]->dp_drv_ctrl0.bits.en_ib = 1;
	dp_dev[sel]->dp_pll_ctrl0.bits.dp_pll_en = 0;
	dp_dev[sel]->dp_pll_ctrl0.bits.cp_en = 0;
	dp_dev[sel]->dp_pll_ctrl0.bits.ldo_en = 0;
}

/**
 * @name       edp_hal_dp_module_en
 * @brief      exit edp module
 * @param[IN]  sel index of edp
 * @param[IN]  en 1:enable, 0:disable
 * @return     none
 */
void edp_hal_dp_module_en(u32 sel, u32 en)
{
	dp_dev[sel]->dp_ctrl.bits.module_en = en;
}

/**
 * @name       :edp_hal_dp_scramble_seed
 * @brief      :dp scramble param setting
 * @param[IN]  :seed dp scramble seed set
 * @return     :none
 */
void edp_hal_dp_scramble_seed(u32 sel, u32 seed)
{
	dp_dev[sel]->dp_scramble_seed.bits.seed = seed;
}

/**
 * @name       :edp_hal_dp_framing_mode
 * @brief      :dp Framing mode setting
 * @param[IN]  :mode: 0:default mode; 1:enhanced mode
 * @return     :none
 */
void edp_hal_dp_framing_mode(u32 sel, u32 mode)
{
	dp_dev[sel]->dp_ctrl.bits.framing_mode_en = mode;
}

/**
 * @name       :edp_hal_video_enable
 * @brief      :enable edp
 * @param[IN]  :sel index of edp
 * @return     :none
 */
void edp_hal_video_enable(u32 sel)
{
	dp_dev[sel]->dp_ctrl.bits.vid_en = 1;
	edp_hal_delay_ms(sel, 1);  /* delay 1ms before disable training */
	dp_dev[sel]->dp_training_ctrl.dwval = 0x00;
}

/**
 * @name       :edp_hal_video_disable
 * @brief      :disable edp
 * @param[IN]  :sel index of edp
 * @return     :none
 */
void edp_hal_video_disable(u32 sel)
{
	dp_dev[sel]->dp_ctrl.bits.vid_en = 0;
}

/**
 * @name       :edp_hal_int_enable
 * @brief      :enable dp interrupt
 * @param[IN]  :intterupt: LINE0 or LINE1
 * @return     :none
 */
void edp_hal_int_enable(u32 sel, enum edp_int intterupt)
{
	if (intterupt == LINE0)
		dp_dev[sel]->dp_int_mask.bits.line_triger0_int_mask = 1;
	else if (intterupt == LINE1)
		dp_dev[sel]->dp_int_mask.bits.line_triger1_int_mask = 1;

}

/**
 * @name       :edp_hal_int_disable
 * @brief      :dispable dp interrupt
 * @param[IN]  :intterupt: LINE0 or LINE1
 * @return     :none
 */
void edp_hal_int_disable(u32 sel, enum edp_int intterupt)
{
	if (intterupt == LINE0)
		dp_dev[sel]->dp_int_mask.bits.line_triger0_int_mask = 0;
	else if (intterupt == LINE1)
		dp_dev[sel]->dp_int_mask.bits.line_triger1_int_mask = 0;
}

/**
 * @name       :edp_hal_set_line_cnt
 * @brief      :DP Line Trigger control set --- LINE0/LINE1
 * @param[IN]  :line0_cnt: set to 1 indicates that first line in v blanking
 * @param[IN]  :line1_cnt: set to 1 indicates that first line in v blanking
 * @return     :
 */
void edp_hal_set_line_cnt(u32 sel, unsigned int line0_cnt,
			  unsigned int line1_cnt)
{
	dp_dev[sel]->dp_line_triger_ctrl.bits.line_triger0_num = line0_cnt;
	dp_dev[sel]->dp_line_triger_ctrl.bits.line_triger1_num = line1_cnt;
}

/**
 * @name       :aux_wr
 * @brief      :write data through aux channel
 * @param[IN]  :addr dst addr
 * @param[IN]  :len data length
 * @param[IN]  :buf data buffer addr
 * @return     :0 if success
 */
int aux_wr(u32 sel, int addr, int len, char *buf)
{
	int i, remain_cnt;
	int aux_trans_flag = 0;

	if (len <= 0)
		return RET_FAIL;
	dp_dev[sel]->dp_aux_addr.bits.addr = addr;
	dp_dev[sel]->dp_aux_rw_len.bits.rlen = 0; /* Clear the read len */
	dp_dev[sel]->dp_aux_rw_len.bits.wlen = len - 1;
	dp_dev[sel]->dp_aux_fifo_status.bits.write_fifo_rst = 1;
	dp_dev[sel]->dp_aux_fifo_status.bits.write_fifo_rst = 0;
	for (i = 0; i < len; i++)
		dp_dev[sel]->dp_aux_wfifo.bits.aux_write_fifo = buf[i];
	dp_dev[sel]->dp_aux_ctrl.dwval =
	    0x0; /* Clear the dp_aux_ctrl register value */
	dp_dev[sel]->dp_aux_ctrl.bits.aux_start = 1;
	edp_hal_delay_ms(sel, 1);

	/* This bit will self-clear when aux transition finishes or error */
	/* happens */
	aux_trans_flag = dp_dev[sel]->dp_aux_ctrl.bits.aux_start;
	if (aux_trans_flag)
		return RET_FAIL;

	/* If aux transition error happens, */
	/* the remain_cnt var is not equal to 0 */

	remain_cnt = dp_dev[sel]->dp_aux_fifo_status.bits.write_fifo_valid_cnt;
	return remain_cnt;
}

/**
 * @name       :aux_rd
 * @brief      :read data through aux channel
 * @param[IN]  :addr dst addr
 * @param[IN]  :len data length
 * @param[IN]  :buf data buffer addr
 * @return     :0 if success
 */
int aux_rd(u32 sel, int addr, int len, char *buf)
{
	int i, remain_cnt;
	int aux_trans_flag;

	if (len <= 0)
		return RET_FAIL;

	dp_dev[sel]->dp_aux_addr.bits.addr = addr;
	dp_dev[sel]->dp_aux_rw_len.bits.wlen = 0; /* Clear the write len */
	dp_dev[sel]->dp_aux_rw_len.bits.rlen = len - 1;
	dp_dev[sel]->dp_aux_fifo_status.bits.read_fifo_rst = 1;
	dp_dev[sel]->dp_aux_fifo_status.bits.read_fifo_rst = 0;
	dp_dev[sel]->dp_aux_ctrl.dwval =
	    0x0; /* Clear the dp_aux_ctrl register value */
	dp_dev[sel]->dp_aux_ctrl.bits.native_aux_mode = 1;
	dp_dev[sel]->dp_aux_ctrl.bits.aux_start = 1;
	edp_hal_delay_ms(sel, 1);

	/* This bit will self-clear when aux transition finishes */
	/* or error happens */
	aux_trans_flag = dp_dev[sel]->dp_aux_ctrl.bits.aux_start;
	if (aux_trans_flag)
		return RET_FAIL;

	for (i = 0; i < len; i++)
		buf[i] = dp_dev[sel]->dp_aux_rfifo.bits.aux_read_fifo;

	/* If aux transition error happens, */
	/* the remain_cnt var is not equal to 0 */
	remain_cnt = dp_dev[sel]->dp_aux_fifo_status.bits.read_fifo_valid_cnt;
	return remain_cnt;
}

/**
 * @name       :edp_hal_link_training_ctrl
 * @brief      :dp sink training control
 * @param[IN]  :link_quality_pat: Link Quality Pattern
 * @param[IN]  :link_training_pat:Link Training Pattern
 * @return     :none
 */
void edp_hal_link_training_ctrl(u32 sel, int edp_en, int link_quality_pat,
			int link_training_pat)
{
	dp_dev[sel]->dp_training_ctrl.dwval = 0x0;
	dp_dev[sel]->dp_training_ctrl.bits.link_quality_pattern =
	    link_quality_pat;
	dp_dev[sel]->dp_training_ctrl.bits.link_training_pattern =
	    link_training_pat;
	dp_dev[sel]->dp_training_ctrl.bits.training_en = edp_en;
}

/**
 * @name       :edp_hal_ctrl_init
 * @brief      :dp ctrl module init
 * @param[IN]  :lane_cnt:number of lane
 * @param[OUT] :bit_rate:bit rate of lane
 * @return     :0 if success
 */
void edp_hal_ctrl_init(u32 sel, u32 lane_cnt, u32 lane_speed)
{
	dp_dev[sel]->dp_ctrl.dwval = 0x00000000;
	/* dp_dev[sel]->dp_int_mask.dwval = 0xffffffff; */
	dp_dev[sel]->dp_int_mask.dwval = 0x00000000;
	dp_dev[sel]->dp_int_status.dwval = 0xffffffff;
	dp_dev[sel]->dp_line_triger_ctrl.bits.line_triger0_num = 1;
	dp_dev[sel]->dp_ctrl.bits.dp_speed_cfg = lane_speed;
	dp_dev[sel]->dp_ctrl.bits.dp_lane_cfg = lane_cnt;
	dp_dev[sel]->dp_ctrl.bits.module_en = 1;
}

/**
 * @name       edp_hal_bist_clock_sel
 * @brief      DP bist clk source select
 * @param[IN]   bist_clk_sel:0-Vclk??1-DPclk
 * @return
 */
void edp_hal_bist_clock_sel(u32 sel, u32 bist_clk_sel)
{
	dp_dev[sel]->dp_debug3.bits.bist_mode_clk_src = bist_clk_sel;
}

/**
 * @name       :edp_hal_get_hpd_status
 * @brief      :get dp hpd status
 * @param[IN]  :sel index of edp
 * @return     :1:detected;
 */
unsigned int edp_hal_get_hpd_status(u32 sel)
{
	int hpd_flag;

	hpd_flag = dp_dev[sel]->dp_int_status.bits.hpd_flag;
	if (hpd_flag)
		return 1;
	else
		return 0;
}

/**
 * @name       :edp_hal_get_int_status
 * @brief      :get dp interrupt status info
 * @param[IN]  :sel index of edp
 * @param[IN]  :interrupt:intterupt type
 * @return     :-1??int type not exist?? 0:no pending?? 1: pending
 */
int edp_hal_get_int_status(u32 sel, enum edp_int intterupt)
{
	int int_flag = 0;

	switch (intterupt) {
	case LINE0:
		int_flag = dp_dev[sel]->dp_int_status.bits.line_triger0_int;
		break;
	case LINE1:
		int_flag = dp_dev[sel]->dp_int_status.bits.line_triger1_int;
		break;
	case FIFO_EMPTY:
		int_flag = dp_dev[sel]->dp_int_status.bits.rd_empty;
		break;
	default:
		int_flag = -1;
		break;
	}
	return int_flag;
}

/**
 * @name       :edp_hal_clr_int_status
 * @brief      :clear dp interrupt status bit
 * @param[IN]  :sel:index of edp
 * @param[IN]  :interrupt:intterupt type
 * @return     :none
 */
void edp_hal_clr_int_status(u32 sel, enum edp_int intterupt)
{
	switch (intterupt) {
	case LINE0:
		dp_dev[sel]->dp_int_status.bits.line_triger0_int = 1;
		break;
	case LINE1:
		dp_dev[sel]->dp_int_status.bits.line_triger1_int = 1;
		break;
	case FIFO_EMPTY:
		dp_dev[sel]->dp_int_status.bits.rd_empty = 1;
		break;
	default:
		edp_wrn("Not support interrupt!\n");
		break;
	}
}

int edp_hal_sram_test(u32 sel)
{
	u32 tmp, i, j;
	u32 sram_pattern[4] = {0x5a5a5a5a, 0xa5a5a5a5, 0xffffffff, 0x00000000};

	dp_dev[sel]->dp_debug5.bits.start_edgenum = 0x16;
	dp_dev[sel]->dp_debug5.bits.bist_en = 1;

	for (i = 0; i < 4; i += 1) {
		for (j = 0; j < SRAM_SIZE; j += 1) {
			dp_dev[sel]->dp_men_test.bits.sram_pat =
			    sram_pattern[i];
		}

		for (j = 0; j < SRAM_SIZE; j += 1) {
			tmp = dp_dev[sel]->dp_men_test.bits.sram_pat;
			if (tmp != sram_pattern[i])
				return RET_FAIL;
		}
	}

	return RET_OK;
}

void edp_hal_ssc_enable_test(u32 sel, u32 bot, u32 frq, u32 m12, u32 stp,
			     u32 mod)
{
	u32 temp;

	temp = dp_dev[sel]->dp_pll_ctrl0.bits.N;

	dp_dev[sel]->dp_pll_ctrl2.bits.sdm_bot = bot;
	dp_dev[sel]->dp_pll_ctrl2.bits.sdm_frq = frq;
	dp_dev[sel]->dp_pll_ctrl2.bits.sdm_m12 = m12;
	dp_dev[sel]->dp_pll_ctrl2.bits.sdm_stp = stp;
	dp_dev[sel]->dp_pll_ctrl2.bits.sdm_mode = mod;
	dp_dev[sel]->dp_pll_ctrl2.bits.sdm_en = 1;

	if (temp != 44) {
		/* 24*44*3/2=1584M */
		/* 24M*44*5/2;24M*44*5 */
		dp_dev[sel]->dp_pll_ctrl0.bits.N = 44;
		dp_dev[sel]->dp_pll_ctrl0.bits.sdm_en = 1; /* Enable SDM */
	}
}

void edp_hal_ssc_disable_test(u32 sel)
{
	u32 temp;

	temp = dp_dev[sel]->dp_pll_ctrl0.bits.N;

	if (temp != 45) {
		dp_dev[sel]->dp_pll_ctrl0.bits.sdm_en = 0; /* Disable SDM */
		dp_dev[sel]->dp_pll_ctrl0.bits.N = 45;     /* 24*45*3/2=1620M */

		dp_dev[sel]->dp_pll_ctrl2.bits.sdm_en = 0;
		dp_dev[sel]->dp_pll_ctrl2.dwval = 0x00;
	}
}

void edp_hal_cts_pattern_cfg(u32 sel, unsigned char bit_rate)
{
	if ((bit_rate == 0) || (bit_rate == 1)) {
		dp_dev[sel]->dp_training_ctrl.bits.link_quality_pattern =
		    3; /* PRBS7 */
	}

	if (bit_rate == 2) {
		dp_dev[sel]->dp_80b_test_ctrl0.bits.custom_80b_31_0 =
		    0xc1f07c1f;
		dp_dev[sel]->dp_80b_test_ctrl1.bits.custom_80b_63_32 =
		    0xf07c1f07;
		dp_dev[sel]->dp_80b_test_ctrl2.bits.custom_80b_79_64 = 0x07c1;

		dp_dev[sel]->dp_training_ctrl.bits.link_quality_pattern =
		    4; /* 80 bit custom pattern */
	}

	dp_dev[sel]->dp_training_ctrl.bits.training_en = 1;
}

/**
 * @name       :edp_hal_delay_ms
 * @brief      :use dp's timer to delay
 * @param[IN]  :t: number of ms to delay clk=732.4hz
 * @return     :none
 */
void edp_hal_delay_ms(u32 sel, int t)
{
	int timer_flag;

	dp_dev[sel]->dp_ctrl.bits.module_en = 1;
	dp_dev[sel]->dp_general_timer.dwval = 0x00000000;
	dp_dev[sel]->dp_int_status.dwval = 0xffffffff;
	dp_dev[sel]->dp_general_timer.bits.N = 15;
	dp_dev[sel]->dp_general_timer.bits.preload_val = (0x10000 - t);
	dp_dev[sel]->dp_general_timer.bits.start = 1;

	do {
		timer_flag = dp_dev[sel]->dp_int_status.bits.timer_over_flow;
	} while (timer_flag == 0);
	dp_dev[sel]->dp_general_timer.dwval = 0x00000000; /* close timer */
}

/**
 * @name       :dp_get_start_dly
 * @brief      :get start delay(unit:line)
 * @param[IN]  :sel:edp index
 * @return     :start delay number
 */
u32 edp_hal_get_start_dly(u32 sel)
{
	return dp_dev[sel]->dp_start_dly.bits.start_dly;
}

/**
 * @name       edp_hal_get_cur_line
 * @brief      get current line number
 * @param[IN]  sel:index of edp
 * @return     line number
 */
u32 edp_hal_get_cur_line(u32 sel)
{
	return dp_dev[sel]->dp_dbg_status0.bits.cur_line;
}

void edp_hal_show_builtin_patten(u32 sel, u32 patten)
{
	dp_dev[sel]->dp_vid_ctrl.bits.vid_src_sel = patten;
}
