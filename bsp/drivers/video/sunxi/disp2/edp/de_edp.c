/* de_edp.c
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

#include "de_edp.h"

char fp_tx_buf[16];
char fp_rx_buf[16];

int training_aux_rd_interval_CR, training_aux_rd_interval_EQ;
int sink_ssc_flag;

unsigned int glb_lane_cnt;
unsigned long long glb_bit_rate;
static unsigned int glb_swing_lv, glb_pre_lv, glb_postcur2_lv;

unsigned int lane_cnt;
unsigned long long ls_clk;

int g_lane_cfg[2];

static struct tx_current rbr_tbl[] = {
	/* mc,tap1,tap2,tap1_op,tap2_op */

	/* swing lv0, pre l0~l3 */
	{0xffff, 0x03030303, 0x0000, 0x00000000, 0x0000},
	{0xffff, 0x0d0d0d0d, 0x0000, 0x05050505, 0x0000},
	{0x7777, 0x1f1f1f1f, 0x0000, 0x0a0a0a0a, 0x0000},
	{0xffff, 0x1f1f1f1f, 0xbbbb, 0x14141414, 0x0000},
	/* swing lv1, pre l0~l2 */
	{0xffff, 0x0d0d0d0d, 0x0000, 0x00000000, 0x0000},
	{0xcccc, 0x1f1f1f1f, 0x0000, 0x08080808, 0x0000},
	{0xffff, 0x1f1f1f1f, 0xbbbb, 0x0f0f0f0f, 0x0000},
	{0xffff, 0x1f1f1f1f, 0xbbbb, 0x0f0f0f0f, 0x0000}, /* not allowed */
	/* swing lv2, pre l0~l1 */
	{0x7777, 0x1f1f1f1f, 0x0000, 0x00000000, 0x0000},
	{0xffff, 0x1f1f1f1f, 0xbbbb, 0x0a0a0a0a, 0x0000},
	{0xffff, 0x1f1f1f1f, 0xbbbb, 0x0a0a0a0a, 0x0000}, /* not allowed */
	{0xffff, 0x1f1f1f1f, 0xbbbb, 0x0a0a0a0a, 0x0000}, /* not allowed */
	/* swing l3, pre l0 */
	{0xffff, 0x1f1f1f1f, 0xbbbb, 0x00000000, 0x0000}, /* optional */
	{0xffff, 0x1f1f1f1f, 0xbbbb, 0x00000000, 0x0000}, /* not allowed */
	{0xffff, 0x1f1f1f1f, 0xbbbb, 0x00000000, 0x0000}, /* not allowed */
	{0xffff, 0x1f1f1f1f, 0xbbbb, 0x00000000, 0x0000}, /* not allowed */
};

static struct tx_current hbr_tbl[] = {
	/* mc,tap1,tap2,tap1_op,tap2_op */

	/* swing lv0, pre l0~l3 */
	{0xffff, 0x05050505, 0x0000, 0x02020202, 0x0000},
	{0xffff, 0x0f0f0f0f, 0x0000, 0x06060606, 0x0000},
	{0x9999, 0x1f1f1f1f, 0x0000, 0x0b0b0b0b, 0x0000},
	{0xffff, 0x1f1f1f1f, 0xdddd, 0x15151515, 0x0000},
	/* swing lv1, pre l0~l2 */
	{0xffff, 0x0f0f0f0f, 0x0000, 0x02020202, 0x0000},
	{0xeeee, 0x1f1f1f1f, 0x0000, 0x09090909, 0x0000},
	{0xffff, 0x1f1f1f1f, 0xdddd, 0x10101010, 0x0000},
	{0xffff, 0x1f1f1f1f, 0xdddd, 0x10101010, 0x0000}, /* not allowed */
	/* swing lv2, pre l0~l1 */
	{0x9999, 0x1f1f1f1f, 0x0000, 0x02020202, 0x0000},
	{0xffff, 0x1f1f1f1f, 0xffff, 0x0d0d0d0d, 0x0000},
	{0xffff, 0x1f1f1f1f, 0xffff, 0x0d0d0d0d, 0x0000}, /* not allowed */
	{0xffff, 0x1f1f1f1f, 0xffff, 0x0d0d0d0d, 0x0000}, /* not allowed */
	/* swing l3, pre l0 */
	{0xffff, 0x1f1f1f1f, 0xffff, 0x04040404, 0x0000}, /* optional */
	{0xffff, 0x1f1f1f1f, 0xffff, 0x04040404, 0x0000}, /* not allowed */
	{0xffff, 0x1f1f1f1f, 0xffff, 0x04040404, 0x0000}, /* not allowed */
	{0xffff, 0x1f1f1f1f, 0xffff, 0x04040404, 0x0000}, /* not allowed */
};

static struct tx_current hbr2_tbl[] = {
	/* mc,tap1,tap2,tap1_op,tap2_op */

	/* swing lv0, pre l0~l3 */
	{0xffff, 0x05050505, 0x0000, 0x02020202, 0x0000},
	{0xffff, 0x0f0f0f0f, 0x0000, 0x06060606, 0x0000},
	{0x9999, 0x1f1f1f1f, 0x0000, 0x0b0b0b0b, 0x0000},
	{0xffff, 0x1f1f1f1f, 0xdddd, 0x15151515, 0x0000},
	/* swing lv1, pre l0~l2 */
	{0xffff, 0x0f0f0f0f, 0x0000, 0x02020202, 0x0000},
	{0xeeee, 0x1f1f1f1f, 0x0000, 0x09090909, 0x0000},
	{0xffff, 0x1f1f1f1f, 0xdddd, 0x10101010, 0x0000},
	{0xffff, 0x1f1f1f1f, 0xdddd, 0x10101010, 0x0000}, /* not allowed */
	/* swing lv2, pre l0~l1 */
	{0x9999, 0x1f1f1f1f, 0x0000, 0x02020202, 0x0000},
	{0xffff, 0x1f1f1f1f, 0xffff, 0x0d0d0d0d, 0x0000},
	{0xffff, 0x1f1f1f1f, 0xffff, 0x0d0d0d0d, 0x0000}, /* not allowed */
	{0xffff, 0x1f1f1f1f, 0xffff, 0x0d0d0d0d, 0x0000}, /* not allowed */
	/* swing l3, pre l0 */
	{0xffff, 0x1f1f1f1f, 0xffff, 0x04040404, 0x0000}, /* optional */
	{0xffff, 0x1f1f1f1f, 0xffff, 0x04040404, 0x0000}, /* not allowed */
	{0xffff, 0x1f1f1f1f, 0xffff, 0x04040404, 0x0000}, /* not allowed */
	{0xffff, 0x1f1f1f1f, 0xffff, 0x04040404, 0x0000}, /* not allowed */
};

/**
 * @name       :phy_cfg
 * @brief      :DP Phy configure
 * @param[IN]  :swing_lv:DP swing level
 * @param[IN]  :preemp_lv:DP pre level
 * @param[IN]  :postcur2_lv
 * @return     :none
 */
void phy_cfg(u32 sel, u8 swing_lv, u8 preemp_lv, u8 postcur2_lv)
{
	u32 mc, tap1, tap2, tap1_op, tap2_op;
	u8 index;

	/* index = (swing_lv + 1) * (preemp_lv + 1) - 1; */
	index = swing_lv * 4 + preemp_lv; /* dlp 20160503 */

	switch (glb_bit_rate) {
	case BR_1P62G:
		mc = rbr_tbl[index].mc;
		tap1 = rbr_tbl[index].tap1;
		tap2 = rbr_tbl[index].tap2;
		tap1_op = rbr_tbl[index].tap1_op;
		tap2_op = rbr_tbl[index].tap2_op;
		break;
	case BR_2P7G:
		mc = hbr_tbl[index].mc;
		tap1 = hbr_tbl[index].tap1;
		tap2 = hbr_tbl[index].tap2;
		tap1_op = hbr_tbl[index].tap1_op;
		tap2_op = hbr_tbl[index].tap2_op;
		break;
	case BR_5P4G:
		mc = hbr2_tbl[index].mc;
		tap1 = hbr2_tbl[index].tap1;
		tap2 = hbr2_tbl[index].tap2;
		tap1_op = hbr2_tbl[index].tap1_op;
		tap2_op = hbr2_tbl[index].tap2_op;
		break;
	default:
		mc = hbr_tbl[index].mc;
		tap1 = hbr_tbl[index].tap1;
		tap2 = hbr_tbl[index].tap2;
		tap1_op = hbr_tbl[index].tap1_op;
		tap2_op = hbr_tbl[index].tap2_op;
		break;
	}

#ifdef EDP_PHY_TEST
	swing_preempha_cal(index, mc, tap1, tap2, tap1_op, tap2_op);
#endif

	edp_hal_phy_drive_ctrl_cfg(sel, mc, tap1, tap2, tap1_op, tap2_op,
			      glb_lane_cnt);
}

void new_phy_cfg(u32 sel, u8 sw0, u8 pre0, u8 sw1, u8 pre1, u8 sw2, u8 pre2,
		 u8 sw3, u8 pre3)
{
	u8 index0, index1, index2, index3;
	u32 mc0, tap1_0, tap2_0, tap1_op0, tap2_op0;
	u32 mc1, tap1_1, tap2_1, tap1_op1, tap2_op1;
	u32 mc2, tap1_2, tap2_2, tap1_op2, tap2_op2;
	u32 mc3, tap1_3, tap2_3, tap1_op3, tap2_op3;

	u32 mc, tap1, tap2, tap1_op, tap2_op;

	index0 = sw0 * 4 + pre0;
	index1 = sw1 * 4 + pre1;
	index2 = sw2 * 4 + pre2;
	index3 = sw3 * 4 + pre3;

	switch (glb_bit_rate) {
	case BR_1P62G:
		mc0 = (rbr_tbl[index0].mc) & 0xf;
		tap1_0 = (rbr_tbl[index0].tap1) & 0xff;
		tap2_0 = (rbr_tbl[index0].tap2) & 0xf;
		tap1_op0 = (rbr_tbl[index0].tap1_op) & 0xff;
		tap2_op0 = (rbr_tbl[index0].tap2_op) & 0xf;

		mc1 = (rbr_tbl[index1].mc >> 4) & 0xf;
		tap1_1 = (rbr_tbl[index1].tap1 >> 8) & 0xff;
		tap2_1 = (rbr_tbl[index1].tap2 >> 4) & 0xf;
		tap1_op1 = (rbr_tbl[index1].tap1_op >> 8) & 0xff;
		tap2_op1 = (rbr_tbl[index1].tap2_op >> 4) & 0xf;

		mc2 = (rbr_tbl[index2].mc >> 8) & 0xf;
		tap1_2 = (rbr_tbl[index2].tap1 >> 16) & 0xff;
		tap2_2 = (rbr_tbl[index2].tap2 >> 8) & 0xf;
		tap1_op2 = (rbr_tbl[index2].tap1_op >> 16) & 0xff;
		tap2_op2 = (rbr_tbl[index2].tap2_op >> 8) & 0xf;

		mc3 = (rbr_tbl[index3].mc >> 12) & 0xf;
		tap1_3 = (rbr_tbl[index3].tap1 >> 24) & 0xff;
		tap2_3 = (rbr_tbl[index3].tap2 >> 12) & 0xf;
		tap1_op3 = (rbr_tbl[index3].tap1_op >> 24) & 0xff;
		tap2_op3 = (rbr_tbl[index3].tap2_op >> 12) & 0xf;
		break;
	case BR_2P7G:
		mc0 = (hbr_tbl[index0].mc) & 0xf;
		tap1_0 = (hbr_tbl[index0].tap1) & 0xff;
		tap2_0 = (hbr_tbl[index0].tap2) & 0xf;
		tap1_op0 = (hbr_tbl[index0].tap1_op) & 0xff;
		tap2_op0 = (hbr_tbl[index0].tap2_op) & 0xf;

		mc1 = (hbr_tbl[index1].mc >> 4) & 0xf;
		tap1_1 = (hbr_tbl[index1].tap1 >> 8) & 0xff;
		tap2_1 = (hbr_tbl[index1].tap2 >> 4) & 0xf;
		tap1_op1 = (hbr_tbl[index1].tap1_op >> 8) & 0xff;
		tap2_op1 = (hbr_tbl[index1].tap2_op >> 4) & 0xf;

		mc2 = (hbr_tbl[index2].mc >> 8) & 0xf;
		tap1_2 = (hbr_tbl[index2].tap1 >> 16) & 0xff;
		tap2_2 = (hbr_tbl[index2].tap2 >> 8) & 0xf;
		tap1_op2 = (hbr_tbl[index2].tap1_op >> 16) & 0xff;
		tap2_op2 = (hbr_tbl[index2].tap2_op >> 8) & 0xf;

		mc3 = (hbr_tbl[index3].mc >> 12) & 0xf;
		tap1_3 = (hbr_tbl[index3].tap1 >> 24) & 0xff;
		tap2_3 = (hbr_tbl[index3].tap2 >> 12) & 0xf;
		tap1_op3 = (hbr_tbl[index3].tap1_op >> 24) & 0xff;
		tap2_op3 = (hbr_tbl[index3].tap2_op >> 12) & 0xf;
		break;
	case BR_5P4G:
		mc0 = (hbr2_tbl[index0].mc) & 0xf;
		tap1_0 = (hbr2_tbl[index0].tap1) & 0xff;
		tap2_0 = (hbr2_tbl[index0].tap2) & 0xf;
		tap1_op0 = (hbr2_tbl[index0].tap1_op) & 0xff;
		tap2_op0 = (hbr2_tbl[index0].tap2_op) & 0xf;

		mc1 = (hbr2_tbl[index1].mc >> 4) & 0xf;
		tap1_1 = (hbr2_tbl[index1].tap1 >> 8) & 0xff;
		tap2_1 = (hbr2_tbl[index1].tap2 >> 4) & 0xf;
		tap1_op1 = (hbr2_tbl[index1].tap1_op >> 8) & 0xff;
		tap2_op1 = (hbr2_tbl[index1].tap2_op >> 4) & 0xf;

		mc2 = (hbr2_tbl[index2].mc >> 8) & 0xf;
		tap1_2 = (hbr2_tbl[index2].tap1 >> 16) & 0xff;
		tap2_2 = (hbr2_tbl[index2].tap2 >> 8) & 0xf;
		tap1_op2 = (hbr2_tbl[index2].tap1_op >> 16) & 0xff;
		tap2_op2 = (hbr2_tbl[index2].tap2_op >> 8) & 0xf;

		mc3 = (hbr2_tbl[index3].mc >> 12) & 0xf;
		tap1_3 = (hbr2_tbl[index3].tap1 >> 24) & 0xff;
		tap2_3 = (hbr2_tbl[index3].tap2 >> 12) & 0xf;
		tap1_op3 = (hbr2_tbl[index3].tap1_op >> 24) & 0xff;
		tap2_op3 = (hbr2_tbl[index3].tap2_op >> 12) & 0xf;

		break;
	default:
		mc0 = (hbr_tbl[index0].mc) & 0xf;
		tap1_0 = (hbr_tbl[index0].tap1) & 0xff;
		tap2_0 = (hbr_tbl[index0].tap2) & 0xf;
		tap1_op0 = (hbr_tbl[index0].tap1_op) & 0xff;
		tap2_op0 = (hbr_tbl[index0].tap2_op) & 0xf;

		mc1 = (hbr_tbl[index1].mc >> 4) & 0xf;
		tap1_1 = (hbr_tbl[index1].tap1 >> 8) & 0xff;
		tap2_1 = (hbr_tbl[index1].tap2 >> 4) & 0xf;
		tap1_op1 = (hbr_tbl[index1].tap1_op >> 8) & 0xff;
		tap2_op1 = (hbr_tbl[index1].tap2_op >> 4) & 0xf;

		mc2 = (hbr_tbl[index2].mc >> 8) & 0xf;
		tap1_2 = (hbr_tbl[index2].tap1 >> 16) & 0xff;
		tap2_2 = (hbr_tbl[index2].tap2 >> 8) & 0xf;
		tap1_op2 = (hbr_tbl[index2].tap1_op >> 16) & 0xff;
		tap2_op2 = (hbr_tbl[index2].tap2_op >> 8) & 0xf;

		mc3 = (hbr_tbl[index3].mc >> 12) & 0xf;
		tap1_3 = (hbr_tbl[index3].tap1 >> 24) & 0xff;
		tap2_3 = (hbr_tbl[index3].tap2 >> 12) & 0xf;
		tap1_op3 = (hbr_tbl[index3].tap1_op >> 24) & 0xff;
		tap2_op3 = (hbr_tbl[index3].tap2_op >> 12) & 0xf;
		break;
	}

	mc = mc0 | (mc1 << 4) | (mc2 << 8) | (mc3 << 12);
	tap1 = tap1_0 | (tap1_1 << 8) | (tap1_2 << 16) | (tap1_3 << 24);
	tap2 = tap2_0 | (tap2_1 << 4) | (tap2_2 << 8) | (tap2_3 << 12);
	tap1_op =
	    tap1_op0 | (tap1_op1 << 8) | (tap1_op2 << 16) | (tap1_op3 << 24);
	tap2_op =
	    tap2_op0 | (tap2_op1 << 4) | (tap2_op2 << 8) | (tap2_op3 << 12);

#ifdef EDP_PHY_TEST
	swing_preempha_cal(index, mc, tap1, tap2, tap1_op, tap2_op);
#endif

	edp_hal_phy_drive_ctrl_cfg(sel, mc, tap1, tap2, tap1_op, tap2_op,
				   glb_lane_cnt);
}

/* Description:	            dp phy init function
 * @param lane_cnt:         number of lane
 * @param bit_rate:         bitrate of lane
 * Return: none
 */
static s32 phy_init(u32 sel, u32 lane_cnt, u64 bit_rate)
{
	if ((lane_cnt > MAX_LANE_NUM) || (lane_cnt == 0))
		return -1;

	/* PHY Resistor Configuration */
	edp_hal_phy_res_cal(sel);
	phy_cfg(sel, 0, 0, 0);
	edp_hal_phy_pll_set(sel, bit_rate);

	return 0;
}

s32 dp_hpd_enable(u32 sel, u32 lane_cnt, u64 bit_rate)
{
	s32 ret = -1;

	phy_init(sel, lane_cnt, bit_rate);
	ret = dp_lane_par(lane_cnt, bit_rate, g_lane_cfg);
	if (ret)
		goto OUT;
	edp_hal_ctrl_init(sel, g_lane_cfg[0], g_lane_cfg[1]);
OUT:
	return ret;
}


s32 dp_init(u32 sel, u32 lane_cnt, u64 bit_rate)
{
	s32 ret;
	struct sink_info sink_inf;
	u32 sink_init_try_cnt = 0;

	memset(&sink_inf, 0, sizeof(struct sink_info));

	ret = dp_hpd_enable(sel, lane_cnt, bit_rate);
	if (ret) {
		edp_wrn("dp_hpd_enable fail:%d!\n", ret);
		goto OUT;
	}
	edp_delay_ms(200);

	edp_hal_bist_clock_sel(sel, BIST_MODE_CLK_SEL);

	edp_dbg("Bist clock select %d!\n", BIST_MODE_CLK_SEL);

	while (dp_sink_init(sel)) {
		++sink_init_try_cnt;
		if (sink_init_try_cnt >= 3) {
			edp_wrn("EDP Sink Initial Fail(%d)!\n",
				sink_init_try_cnt);
			return RET_FAIL;
		}
	}
	ret = dp_get_sink_info(&sink_inf); /* get sink info */
	if (ret) {
		edp_wrn("dp_get_sink_info fail:%d\n", ret);
		goto OUT;
	}
	edp_dbg("dp version:%d\n", sink_inf.dp_rev);
	edp_dbg("dp eDP_capable:%d\n", sink_inf.eDP_capable);
	edp_dbg("dp dp_enhanced_frame_cap:%d\n",
		sink_inf.dp_enhanced_frame_cap);
	edp_dbg("dp dp_max_lane_count:%d\n", sink_inf.dp_max_lane_count);
	edp_dbg("dp dp_max_link_rate:%lld\n", sink_inf.dp_max_link_rate);
	edp_dbg("EDP Sink Initial PASS!\n");
OUT:
	return ret;
}

/**
 * @name       :dp_get_training_info
 * @brief      :get training info
 * @param[OUT] :train_inf:
 * @return     :0 if success
 */
s32 dp_get_training_info(struct training_info *train_inf)
{
	s32 ret = -1;

	if (!train_inf) {
		edp_wrn("null hdl\n");
		goto OUT;
	}
	train_inf->swing_lv = glb_swing_lv;
	train_inf->preemp_lv = glb_pre_lv;
	train_inf->postcur2_lv = glb_postcur2_lv;
	ret = 0;
OUT:
	return ret;
}

s32 dp_training0(u32 sel)
{
	int ret = 0;

	ret = new_dp_tps1_test(sel);

	if (ret != RET_OK) {
		edp_wrn("CR Training Fail!\n");
		return RET_FAIL;
	} else {
		edp_dbg("CR Training OK!\n");
	}

	ret = new_dp_tps2_test(sel);

	if (ret != RET_OK) {
		edp_wrn("EQ Training Fail!\n");
		return RET_FAIL;
	} else {
		edp_dbg("EQ Training OK!\n");
	}
	return ret;
}

s32 dp_training1(u32 sel)
{
	int ret = 0;

	ret = new1_dp_tps1_test(sel);

	if (ret != RET_OK) {
		edp_wrn("CR Training Fail!\n");
		return RET_FAIL;
	} else {
		edp_dbg("CR Training OK!\n");
	}

	ret = new1_dp_tps2_test(sel);

	if (ret != RET_OK) {
		edp_wrn("EQ Training Fail!\n");
		return RET_FAIL;
	} else {
		edp_dbg("EQ Training OK!\n");
	}
	return ret;
}

s32 dp_training2(u32 sel)
{
	int ret = 0;

	ret = new2_dp_tps1_test(sel);

	if (ret != RET_OK) {
		edp_wrn("CR Training Fail!\n");
		return RET_FAIL;
	} else {
		edp_dbg("CR Training OK!\n");
	}

	ret = new2_dp_tps2_test(sel);

	if (ret != RET_OK) {
		edp_wrn("EQ Training Fail!\n");
		return RET_FAIL;
	} else {
		edp_dbg("EQ Training OK!\n");
	}
	return ret;
}

/**
 * @name       :dp_training
 * @brief      :dp training call after edp_hal_ctrl_init and dp_sink_init
 * @param[IN]  :init_swing_lv: swing level
 * @return     :0: training successful; -1: training error
 */
s32 dp_training(u32 sel, u32 init_swing_lv)
{
	s32 ret_val;
	u8 is_swing_max, is_pre_max;

	if (init_swing_lv > VOL_SWING_LEVEL_NUM - 1) {
		edp_wrn("Initial Voltage Swing Level is False!\n");
		return RET_FAIL;
	}

	for (glb_pre_lv = 0; glb_pre_lv <= PRE_EMPHASIS_LEVEL_NUM - 1;
	     glb_pre_lv++)
		for (glb_swing_lv = init_swing_lv;
		     glb_swing_lv <= VOL_SWING_LEVEL_NUM - 1; glb_swing_lv++) {
			if (glb_swing_lv == VOL_SWING_LEVEL_NUM - 1)
				is_swing_max = 1;
			else
				is_swing_max = 0;

			if (glb_pre_lv == PRE_EMPHASIS_LEVEL_NUM - 1)
				is_pre_max = 1;
			else
				is_pre_max = 0;

			/* phy_cfg(glb_swing_lv, */
			/* glb_pre_lv, */
			/* glb_postcur2_lv); */
			ret_val = dp_tps1_test(sel, glb_swing_lv, glb_pre_lv,
					       is_swing_max, is_pre_max);

			if (ret_val == 0) {
				edp_dbg("CR phase is OK!\n");
				edp_dbg(
				    "Swing Level:%d, Pre-emphasize Level:%d\n",
				    glb_swing_lv, glb_pre_lv);
				goto train_next;
			} else if (glb_pre_lv < PRE_EMPHASIS_LEVEL_NUM - 1) {
				edp_dbg("CR phase is continue!\n");
			} else {
				edp_wrn("CR phase is FAIL!\n");
				return RET_FAIL;
			}
		}

train_next:
	/* Initial EQ phase is at the configuration of CR phase */
	for (glb_pre_lv = 0; glb_pre_lv <= PRE_EMPHASIS_LEVEL_NUM - 1;
	     glb_pre_lv++)
		for (glb_swing_lv = init_swing_lv;
		     glb_swing_lv <= VOL_SWING_LEVEL_NUM - 1; glb_swing_lv++) {
			if (glb_swing_lv == VOL_SWING_LEVEL_NUM - 1)
				is_swing_max = 1;
			else
				is_swing_max = 0;

			if (glb_pre_lv == PRE_EMPHASIS_LEVEL_NUM - 1)
				is_pre_max = 1;
			else
				is_pre_max = 0;

						/* phy_cfg(glb_swing_lv, */
			/* glb_pre_lv, */
			/* glb_postcur2_lv); */
			ret_val = dp_tps2_test(sel, glb_swing_lv, glb_pre_lv,
					       is_swing_max, is_pre_max);

			if (ret_val == 0) {
				edp_dbg("EQ phase is OK!\n");
				edp_dbg(
				    "Swing Level:%d,Pre-emphasize Level:%d\n",
				    glb_swing_lv, glb_pre_lv);
				return RET_OK;
			} else if (glb_pre_lv < PRE_EMPHASIS_LEVEL_NUM - 1) {
				edp_dbg("EQ phase is continue!\n");
			} else {
				edp_wrn("EQ phase is FAIL!\n");
				return RET_FAIL;
			}
			/* For Compliance with some display device */
		}

	return RET_OK;
}

/**
 * @name       :enable dp module
 * @brief      :enable dp
 * @param[IN]  :sel index of edp
 * @param[IN]  :para:pointer of edp parameter(fps lane count etc.)
 * @param[IN]  :tmg video timing structure
 * @return     :0 if success
 */
s32 dp_enable(u32 sel, struct edp_para *para, struct disp_video_timings *tmg)
{
	s32 ret = -1;
	struct training_info train_inf;
	u32 try_cnt = 10;

	ret = dp_init(sel, para->edp_lane, para->edp_rate);
	if (ret) {
		edp_wrn("dp init fail\n");
		goto OUT;
	}
	ret = dp_video_set(sel, tmg, para, RGB_INPUT);
	if (ret) {
		edp_wrn("dp_video_set fail:%d\n", ret);
		goto OUT;
	}

	while (try_cnt--) {
		switch (para->edp_training_func) {
		case 0:
			ret = dp_training0(sel);
			break;
		case 1:
			ret = dp_training1(sel);
			break;
		case 2:
			ret = dp_training2(sel);
			break;
		case 3:
			ret = dp_training(sel, 0);
			break;
		default:
			ret = dp_training0(sel);
			break;
		}
		if (ret) {
			edp_wrn("dp_training%d fail:%d after %d times\n",
				para->edp_training_func, ret, try_cnt);
			continue;
		} else
			break;
	}

	dp_get_training_info(&train_inf);

	edp_hal_video_enable(sel);
OUT:
	return ret;
}

s32 dp_disable(u32 sel)
{
	edp_hal_video_disable(sel);
	edp_hal_phy_exit(sel);
	return 0;
}

void dp_moudle_en(u32 sel, u32 en)
{
	edp_hal_dp_module_en(sel, en);
}


s32 dp_get_start_dly(u32 sel)
{
	return edp_hal_get_start_dly(sel);
}

u32 dp_get_cur_line(u32 sel)
{
	return edp_hal_get_cur_line(sel);
}

void dp_show_builtin_patten(u32 sel, u32 patten)
{
	return edp_hal_show_builtin_patten(sel, patten);
}

void dp_set_reg_base(u32 sel, uintptr_t base)
{
	edp_hal_set_reg_base(sel, base);
}

void dp_int_enable(u32 sel, enum edp_int interrupt, bool en)
{
	u32 start_dly = edp_hal_get_start_dly(sel);

	edp_hal_set_line_cnt(sel, 1, start_dly - 2);
	if (en)
		edp_hal_int_enable(sel, interrupt);
	else
		edp_hal_int_disable(sel, interrupt);
	edp_hal_clr_int_status(sel, interrupt);
}

s32 dp_int_query(u32 sel, enum edp_int interrupt)
{
	return edp_hal_get_int_status(sel, interrupt);
}

/**
 * @name       dp_int_clear
 * @brief      clear interrupt
 * @param[IN]  sel index of edp
 * @param[IN]  interrupt type of interrupt
 * @return     none
 */
void dp_int_clear(u32 sel, enum edp_int interrupt)
{
	edp_hal_clr_int_status(sel, interrupt);
}

u32 dp_get_hpd_status(u32 sel)
{
	return edp_hal_get_hpd_status(sel);
}
/* End of File */
