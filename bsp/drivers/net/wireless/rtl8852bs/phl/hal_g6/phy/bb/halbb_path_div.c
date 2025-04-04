/******************************************************************************
 *
 * Copyright(c) 2007 - 2020  Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHPATHABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * The full GNU General Public License is included in this distribution in the
 * file called LICENSE.
 *
 * Contact Information:
 * wlanfae <wlanfae@realtek.com>
 * Realtek Corporation, No. 2, Innovation Road II, Hsinchu Science Park,
 * Hsinchu 300, Taiwan.
 *
 * Larry Finger <Larry.Finger@lwfinger.net>
 *
 *****************************************************************************/
#include "halbb_precomp.h"

#ifdef HALBB_PATH_DIV_SUPPORT

void halbb_pathdiv_reset_stat(struct bb_info *bb)
{
	struct bb_pathdiv_info *bb_path_div = &bb->bb_path_div_i;
	struct bb_pathdiv_rssi_info *bb_rssi_i = &bb_path_div->bb_rssi_i;

	halbb_mem_set(bb, bb_rssi_i, 0, sizeof(struct bb_pathdiv_rssi_info));
}

void halbb_pathdiv_reset(struct bb_info *bb)
{
	/* Reset stat */
	halbb_pathdiv_reset_stat(bb);
}

void halbb_pathdiv_reg_init(struct bb_info *bb)
{
#ifdef BB_8852A_CAV_SUPPORT
#endif
#ifdef HALBB_COMPILE_AP_SERIES
#endif
}

void halbb_pathdiv_init(struct bb_info *bb)
{
	struct bb_pathdiv_info *bb_path_div = &bb->bb_path_div_i;
	struct bb_pathdiv_rssi_info *rssi = &bb_path_div->bb_rssi_i;
	u32 i = 0;

	if (!(bb->support_ability & BB_PATH_DIV))
		return;

	if ((bb->phl_com->phy_cap[0].txss > 1) || (bb->phl_com->phy_cap[0].tx_path_num < 2)){
		BB_DBG(bb, DBG_PATH_DIV, "Txss > 1 or 1T, Not support path diversity\n");
          	bb->support_ability &= ~BB_PATH_DIV;
		return;
	}

	BB_DBG(bb, DBG_PATH_DIV, "%s ======>\n", __func__);

	for (i = 0; i < PHL_MAX_STA_NUM; i++) {
		/*BB_PATH_AB is a invalid value used for init state*/
		bb_path_div->fix_path_en[i] = false;
		bb_path_div->path_sel[i] = BB_PATH_NON;
		bb_path_div->fix_path_sel[i] = BB_PATH_A;
		bb_path_div->path_rssi_gap = 4; /*2dB, u(8,1) RSSI*/
	}

	halbb_pathdiv_reset(bb);

	BB_DBG(bb, DBG_INIT, "Init path_diversity");
}

void halbb_set_cctrl_tbl(struct bb_info *bb, u8 macid, u16 cfg)
{
	struct hal_txmap_cfg txmap_cfg;

	halbb_mem_set(bb, &txmap_cfg, 0, sizeof(struct hal_txmap_cfg));

	txmap_cfg.macid = macid;
	txmap_cfg.n_tx_en = cfg & 0x0f;
	txmap_cfg.map_a = ((cfg>>4) & 0x03);
	txmap_cfg.map_b = ((cfg>>6) & 0x03);
	txmap_cfg.map_c = ((cfg>>8) & 0x03);
	txmap_cfg.map_d = ((cfg>>10) & 0x03);

	if (rtw_hal_mac_tx_path_map_cfg(bb->hal_com, &txmap_cfg))
		BB_DBG(bb, DBG_PATH_DIV, "halbb_set_cctrl_tbl failed\n");
	else
		BB_DBG(bb, DBG_PATH_DIV, "halbb_set_cctrl_tbl success\n");
}

void halbb_set_tx_path_by_cmac_tbl(struct bb_info *bb, u8 macid, enum bb_path tx_path_sel_1ss)
{
	struct bb_pathdiv_info *bb_path_div = &bb->bb_path_div_i;
	enum bb_path tx_path_sel = tx_path_sel_1ss;
	enum rtw_hal_status hal_status = RTW_HAL_STATUS_FAILURE;
	u16 cfg = 0;

	/*Adv-ctrl mode*/
	if (bb_path_div->fix_path_en[macid]) {
		tx_path_sel = bb_path_div->fix_path_sel[macid];
		BB_DBG(bb, DBG_PATH_DIV, "Fix TX path= %s\n",
		       (tx_path_sel == BB_PATH_A) ? "A" : "B");
	} else {
		tx_path_sel = bb_path_div->path_sel[macid];
	}

	BB_DBG(bb, DBG_PATH_DIV, "STA[%d] : path_sel= [%s]\n", macid,
	       (tx_path_sel == BB_PATH_A) ? "A" : "B");
	/*BB_PATH != RF_PATH*/
	tx_path_sel = (tx_path_sel == BB_PATH_B) ? RF_PATH_B : RF_PATH_A;
	cfg = halbb_cfg_cmac_tx_ant(bb, (enum rf_path)tx_path_sel);

	halbb_set_cctrl_tbl(bb, macid, cfg);

	/* Config Tx RF mode */
	if (tx_path_sel == RF_PATH_A){
			halbb_set_reg(bb, 0x12ac, 0xff0, 0x12);
			halbb_set_reg(bb, 0x32ac, 0xff0, 0x11);
	} else if (tx_path_sel == RF_PATH_B){
			halbb_set_reg(bb, 0x12ac, 0xff0, 0x11);
			halbb_set_reg(bb, 0x32ac, 0xff0, 0x12);
	}
}

void halbb_path_diversity(struct bb_info *bb)
{
	struct bb_pathdiv_info *bb_path_div = &bb->bb_path_div_i;
	struct bb_link_info *bb_link = &bb->bb_link_i;
	struct bb_pathdiv_rssi_info *rssi_stat = &bb_path_div->bb_rssi_i;
	struct rtw_phl_stainfo_t *sta;
	enum bb_path path = BB_PATH_A;
	u8 rssi_a = 0, rssi_b = 0;
	u8 mod_rssi_a = 0, mod_rssi_b = 0;
	u8 i = 0, sta_cnt = 0;
	u8 macid = 0;

	BB_DBG(bb, DBG_PATH_DIV, "%s ======>\n", __func__);

	/* Early return */
	if (bb->phl_com->drv_mode == RTW_DRV_MODE_MP) {
		BB_DBG(bb, DBG_PATH_DIV, "MP mode\n");
		return;
	}

	if (!(bb->support_ability & BB_PATH_DIV)) {
		BB_DBG(bb, DBG_PATH_DIV, "Not support path diversity\n");
		return;
	}

	if (!bb_link->is_linked) {
		BB_DBG(bb, DBG_PATH_DIV, "No Link\n");
		return;
	}

	for (i = 0; i < PHL_MAX_STA_NUM; i++) {
		sta = bb->phl_sta_info[i];
		macid = (u8)sta->macid;
		if (!is_sta_active(sta))
			continue;

		sta_cnt ++;

		/* 2 Caculate RSSI per path */
		rssi_a = (u8)HALBB_DIV(rssi_stat->path_a_rssi_sum[macid],
				       rssi_stat->path_a_pkt_cnt[macid]);
		rssi_b = (u8)HALBB_DIV(rssi_stat->path_b_rssi_sum[macid],
				       rssi_stat->path_b_pkt_cnt[macid]);

		/* 3 Add RSSI GAP per path to prevent damping*/
		if (bb_path_div->path_sel[macid] == BB_PATH_A) {
			mod_rssi_a = rssi_a + bb_path_div->path_rssi_gap;
			mod_rssi_b = rssi_b;
		} else if (bb_path_div->path_sel[macid] == BB_PATH_B){
			mod_rssi_a = rssi_a;
			mod_rssi_b = rssi_b + bb_path_div->path_rssi_gap;
		} else {
			mod_rssi_a = rssi_a;
			mod_rssi_b = rssi_b;
		}

		if (mod_rssi_a == mod_rssi_b)
			path = bb_path_div->path_sel[macid];
		else
			path = (mod_rssi_a > mod_rssi_b) ? BB_PATH_A : BB_PATH_B;

		BB_DBG(bb, DBG_PATH_DIV,
		       "STA[%d] : PathA sum=%d, cnt=%d, avg_rssi=%d\n",
		       macid, rssi_stat->path_a_rssi_sum[macid],
		       rssi_stat->path_a_pkt_cnt[macid], rssi_a >> 1);
		BB_DBG(bb, DBG_PATH_DIV,
		       "STA[%d] : PathB sum=%d, cnt=%d, avg_rssi=%d\n",
		       macid, rssi_stat->path_b_rssi_sum[macid],
		       rssi_stat->path_b_pkt_cnt[macid], rssi_b >> 1);
		BB_DBG(bb, DBG_PATH_DIV,
		       "path_rssi_gap=%d dB\n", bb_path_div->path_rssi_gap >> 1);

		if (!bb_path_div->fix_path_en[macid]) {
			if (bb_path_div->path_sel[macid] != path) {
				bb_path_div->path_sel[macid] = path;
				/* Update Tx path */
				halbb_set_tx_path_by_cmac_tbl(bb, macid, path);
				BB_DBG(bb, DBG_PATH_DIV, "Switch TX path= %s\n",
				       (path == BB_PATH_A) ? "A" : "B");
			} else {
				BB_DBG(bb, DBG_PATH_DIV, "Stay in TX path = %s\n",
				       (path == BB_PATH_A) ? "A" : "B");
			}
		} else {
			BB_DBG(bb, DBG_PATH_DIV, "Fix TX path= %s\n",
			       (bb_path_div->fix_path_sel[macid] == BB_PATH_A) ? "A" : "B");
		}

		rssi_stat->path_a_pkt_cnt[macid] = 0;
		rssi_stat->path_a_rssi_sum[macid] = 0;
		rssi_stat->path_b_pkt_cnt[macid] = 0;
		rssi_stat->path_b_rssi_sum[macid] = 0;

		if (sta_cnt == bb_link->num_linked_client)
			break;
	}
	BB_DBG(bb, DBG_PATH_DIV, "[%s] end\n\n", __func__);
}

void halbb_pathdiv_phy_sts(struct bb_info *bb, struct physts_rxd *desc)
{
	struct bb_physts_rslt_hdr_info	*psts_h = &bb->bb_physts_i.bb_physts_rslt_hdr_i;
	struct bb_cmn_rpt_info	*cmn_rpt = &bb->bb_cmn_rpt_i;
	struct bb_pathdiv_info *bb_path_div = &bb->bb_path_div_i;
	struct bb_pathdiv_rssi_info *rssi = &bb_path_div->bb_rssi_i;
	u8 macid = 0;

	if (cmn_rpt->is_cck_rate)
		return;

	macid = desc->macid_su;
	rssi->path_a_rssi_sum[macid] += psts_h->rssi[0];
	rssi->path_a_pkt_cnt[macid]++;

	rssi->path_b_rssi_sum[macid] += psts_h->rssi[1];
	rssi->path_b_pkt_cnt[macid]++;
}

void halbb_pathdiv_dbg(struct bb_info *bb, char input[][16], u32 *_used,
			      char *output, u32 *_out_len)
{
	struct bb_pathdiv_info *bb_path_div = &bb->bb_path_div_i;

	char help[] = "-h";
	u8 macid = 0;
	u32 var[10] = {0};
	u32 used = *_used;
	u32 out_len = *_out_len;

	if ((_os_strcmp(input[1], help) == 0)) {
		BB_DBG_CNSL(out_len, used, output + used, out_len - used,
			    "Fix CMAC TX path Mode: {1} {en} {macid} {path(1/2)}\n");
		BB_DBG_CNSL(out_len, used, output + used, out_len - used,
			    "RSSI Gap dbg mode: {2} {path rssi gap(1:0.5dB)}\n");
	} else {
		HALBB_SCAN(input[1], DCMD_DECIMAL, &var[0]);

		if (var[0] == 1) {
			HALBB_SCAN(input[2], DCMD_DECIMAL, &var[1]);
			HALBB_SCAN(input[3], DCMD_DECIMAL, &var[2]);
			HALBB_SCAN(input[4], DCMD_DECIMAL, &var[3]);
			bb_path_div->fix_path_en[macid] = (u8)var[1];
			macid = (u8)var[2];
			bb_path_div->fix_path_sel[macid] = (enum bb_path)var[3];
			halbb_set_tx_path_by_cmac_tbl(bb, macid, (enum bb_path)var[3]);
			BB_DBG_CNSL(out_len, used, output + used, out_len - used,
				    "Fix STA[%d] path= %s\n", macid,
				    (bb_path_div->fix_path_sel[macid] == BB_PATH_A) ? "A" : "B");
		} else if (var[0] == 2) {
			HALBB_SCAN(input[2], DCMD_DECIMAL, &var[1]);
			bb_path_div->path_rssi_gap = (u8)var[1];
			BB_DBG_CNSL(out_len, used, output + used, out_len - used,
				    "path rssi gap = %d dB\n", var[1] >> 1);
		}
	}

	*_used = used;
	*_out_len = out_len;
}

void halbb_cr_cfg_pathdiv_init(struct bb_info *bb)
{
	//struct bb_pathdiv_cr_info *cr = &bb->bb_ant_div_i.bb_antdiv_cr_i;

	switch (bb->cr_type) {

	#ifdef BB_8852A_CAV_SUPPORT
	case BB_52AA:
		break;

	#endif
	#ifdef HALBB_COMPILE_AP_SERIES
	case BB_AP:
		break;

	#endif
	#ifdef HALBB_COMPILE_CLIENT_SERIES
	case BB_CLIENT:
		break;
	#endif

	default:
		break;
	}

}
#endif
