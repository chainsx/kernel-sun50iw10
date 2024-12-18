/******************************************************************************
 *
 * Copyright(c) 2016 - 2017 Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 *****************************************************************************/
/*****************************************************************************
 *	Copyright(c) 2009,  RealTEK Technology Inc. All Right Reserved.
 *
 * Module:	__INC_HAL8188FREG_H
 *
 *
 * Note:	1. Define Mac register address and corresponding bit mask map
 *
 *
 * Export:	Constants, macro, functions(API), global variables(None).
 *
 * Abbrev:
 *
 * History:
 *		data		Who		Remark
 *
 *****************************************************************************/
#ifndef __INC_HAL8188FREG_H
#define __INC_HAL8188FREG_H

/* ************************************************************
 *
 * ************************************************************ */

/* -----------------------------------------------------
 *
 *	0x0000h ~ 0x00FFh	System Configuration
 *
 * ----------------------------------------------------- */
#define REG_SYS_ISO_CTRL_8188F 0x0000 /* 2 Byte */
#define REG_SYS_FUNC_EN_8188F 0x0002 /* 2 Byte */
#define REG_APS_FSMCO_8188F 0x0004 /* 4 Byte */
#define REG_SYS_CLKR_8188F 0x0008 /* 2 Byte */
#define REG_9346CR_8188F 0x000A /* 2 Byte */
#define REG_EE_VPD_8188F 0x000C /* 2 Byte */
#define REG_AFE_MISC_8188F 0x0010 /* 1 Byte */
#define REG_SPS0_CTRL_8188F 0x0011 /* 7 Byte */
#define REG_SPS_OCP_CFG_8188F 0x0018 /* 4 Byte */
#define REG_RSV_CTRL_8188F 0x001C /* 3 Byte */
#define REG_RF_CTRL_8188F 0x001F /* 1 Byte */
#define REG_LPLDO_CTRL_8188F 0x0023 /* 1 Byte */
#define REG_AFE_XTAL_CTRL_8188F 0x0024 /* 4 Byte */
#define REG_AFE_PLL_CTRL_8188F 0x0028 /* 4 Byte */
#define REG_MAC_PLL_CTRL_EXT_8188F 0x002c /* 4 Byte */
#define REG_EFUSE_CTRL_8188F 0x0030
#define REG_EFUSE_TEST_8188F 0x0034
#define REG_PWR_DATA_8188F 0x0038
#define REG_CAL_TIMER_8188F 0x003C
#define REG_ACLK_MON_8188F 0x003E
#define REG_GPIO_MUXCFG_8188F 0x0040
#define REG_GPIO_IO_SEL_8188F 0x0042
#define REG_MAC_PINMUX_CFG_8188F 0x0043
#define REG_GPIO_PIN_CTRL_8188F 0x0044
#define REG_GPIO_INTM_8188F 0x0048
#define REG_LEDCFG0_8188F 0x004C
#define REG_LEDCFG1_8188F 0x004D
#define REG_LEDCFG2_8188F 0x004E
#define REG_LEDCFG3_8188F 0x004F
#define REG_FSIMR_8188F 0x0050
#define REG_FSISR_8188F 0x0054
#define REG_HSIMR_8188F 0x0058
#define REG_HSISR_8188F 0x005c
#define REG_GPIO_EXT_CTRL 0x0060
#define REG_MULTI_FUNC_CTRL_8188F 0x0068
#define REG_GPIO_STATUS_8188F 0x006C
#define REG_SDIO_CTRL_8188F 0x0070
#define REG_OPT_CTRL_8188F 0x0074
#define REG_AFE_XTAL_CTRL_EXT_8188F 0x0078
#define REG_MCUFWDL_8188F 0x0080
#define REG_FW_DBG_STATUS_8188F 0x0088
#define REG_FW_DBG_CTRL_8188F 0x008F
#define REG_WLLPS_CTRL_8188F 0x0090
#define REG_HIMR0_8188F 0x00B0
#define REG_HISR0_8188F 0x00B4
#define REG_HIMR1_8188F 0x00B8
#define REG_HISR1_8188F 0x00BC
#define REG_PMC_DBG_CTRL2_8188F 0x00CC
#define REG_EFUSE_BURN_GNT_8188F 0x00CF
#define REG_HPON_FSM_8188F 0x00EC
#define REG_SYS_CFG_8188F 0x00F0
#define REG_SYS_CFG1_8188F 0x00FC
#define REG_ROM_VERSION 0x00FD

/* -----------------------------------------------------
 *
 *	0x0100h ~ 0x01FFh	MACTOP General Configuration
 *
 * ----------------------------------------------------- */
#define REG_CR_8188F 0x0100
#define REG_PBP_8188F 0x0104
#define REG_PKT_BUFF_ACCESS_CTRL_8188F 0x0106
#define REG_TRXDMA_CTRL_8188F 0x010C
#define REG_TRXFF_BNDY_8188F 0x0114
#define REG_TRXFF_STATUS_8188F 0x0118
#define REG_RXFF_PTR_8188F 0x011C
#define REG_CPWM_8188F 0x012F
#define REG_FWIMR_8188F 0x0130
#define REG_FWISR_8188F 0x0134
#define REG_FTIMR_8188F 0x0138
#define REG_PKTBUF_DBG_CTRL_8188F 0x0140
#define REG_RXPKTBUF_CTRL_8188F 0x0142
#define REG_PKTBUF_DBG_DATA_L_8188F 0x0144
#define REG_PKTBUF_DBG_DATA_H_8188F 0x0148

#define REG_TC0_CTRL_8188F 0x0150
#define REG_TC1_CTRL_8188F 0x0154
#define REG_TC2_CTRL_8188F 0x0158
#define REG_TC3_CTRL_8188F 0x015C
#define REG_TC4_CTRL_8188F 0x0160
#define REG_TCUNIT_BASE_8188F 0x0164
#define REG_RSVD3_8188F 0x0168
#define REG_32K_CAL_REG1_8188F 0x0198
#define REG_C2HEVT_MSG_NORMAL_8188F 0x01A0
#define REG_C2HEVT_CMD_SEQ_88XX 0x01A1
#define reg_c2h_evt_cmd_content_88xx 0x01A2
#define REG_C2HEVT_CMD_LEN_88XX 0x01AE
#define REG_C2HEVT_CLEAR_8188F 0x01AF
#define REG_MCUTST_1_8188F 0x01C0
#define REG_MCUTST_2_8188F 0x01C4
#define REG_MCUTST_WOWLAN_8188F 0x01C7
#define REG_FMETHR_8188F 0x01C8
#define REG_HMETFR_8188F 0x01CC
#define REG_HMEBOX_0_8188F 0x01D0
#define REG_HMEBOX_1_8188F 0x01D4
#define REG_HMEBOX_2_8188F 0x01D8
#define REG_HMEBOX_3_8188F 0x01DC
#define REG_LLT_INIT_8188F 0x01E0
#define REG_HMEBOX_EXT0_8188F 0x01F0
#define REG_HMEBOX_EXT1_8188F 0x01F4
#define REG_HMEBOX_EXT2_8188F 0x01F8
#define REG_HMEBOX_EXT3_8188F 0x01FC

/* -----------------------------------------------------
 *
 *	0x0200h ~ 0x027Fh	TXDMA Configuration
 *
 * ----------------------------------------------------- */
#define REG_RQPN_8188F 0x0200
#define REG_FIFOPAGE_8188F 0x0204
#define REG_TDECTRL_8188F 0x0208
#define REG_DWBCN0_CTRL_8188F REG_TDECTRL
#define REG_TXDMA_OFFSET_CHK_8188F 0x020C
#define REG_TXDMA_STATUS_8188F 0x0210
#define REG_RQPN_NPQ_8188F 0x0214
#define REG_AUTO_LLT_8188F 0x0224
#define REG_TDECTRL1_8188F 0x0228
#define REG_DWBCN1_CTRL_8188F 0x0228

/* -----------------------------------------------------
 *
 *	0x0280h ~ 0x02FFh	RXDMA Configuration
 *
 * ----------------------------------------------------- */
#define REG_RXDMA_AGG_PG_TH_8188F 0x0280
#define REG_FW_UPD_RDPTR_8188F 0x0284 /* FW shall update this register before FW write RXPKT_RELEASE_POLL to 1 */
#define REG_RXDMA_CONTROL_8188F 0x0286 /* Control the RX DMA. */
#define REG_RXPKT_NUM_8188F 0x0287 /* The number of packets in RXPKTBUF. */
#define REG_RXDMA_STATUS_8188F 0x0288
#define REG_RXDMA_PRO_8188F 0x0290
#define REG_EARLY_MODE_CONTROL_8188F 0x02BC
#define REG_RSVD5_8188F 0x02F0
#define REG_RSVD6_8188F 0x02F4

/* -----------------------------------------------------
 *
 *	0x0300h ~ 0x03FFh	PCIe
 *
 * ----------------------------------------------------- */
#define REG_PCIE_CTRL_REG_8188F 0x0300
#define REG_INT_MIG_8188F 0x0304 /* Interrupt Migration */
#define REG_BCNQ_DESA_8188F 0x0308 /* TX Beacon Descriptor Address */
#define REG_HQ_DESA_8188F 0x0310 /* TX High Queue Descriptor Address */
#define REG_MGQ_DESA_8188F 0x0318 /* TX Manage Queue Descriptor Address */
#define REG_VOQ_DESA_8188F 0x0320 /* TX VO Queue Descriptor Address */
#define REG_VIQ_DESA_8188F 0x0328 /* TX VI Queue Descriptor Address */
#define REG_BEQ_DESA_8188F 0x0330 /* TX BE Queue Descriptor Address */
#define REG_BKQ_DESA_8188F 0x0338 /* TX BK Queue Descriptor Address */
#define REG_RX_DESA_8188F 0x0340 /* RX Queue	Descriptor Address */
#define REG_DBI_WDATA_8188F 0x0348 /* DBI Write data */
#define REG_DBI_RDATA_8188F 0x034C /* DBI Read data */
#define REG_DBI_ADDR_8188F 0x0350 /* DBI Address */
#define REG_DBI_FLAG_8188F 0x0352 /* DBI Read/Write Flag */
#define REG_MDIO_WDATA_8188F 0x0354 /* MDIO for Write PCIE PHY */
#define REG_MDIO_RDATA_8188F 0x0356 /* MDIO for Reads PCIE PHY */
#define REG_MDIO_CTL_8188F 0x0358 /* MDIO for Control */
#define REG_DBG_SEL_8188F 0x0360 /* Debug Selection Register */
#define REG_PCIE_HRPWM_8188F 0x0361 /* PCIe RPWM */
#define REG_PCIE_HCPWM_8188F 0x0363 /* PCIe CPWM */
#define REG_PCIE_MULTIFET_CTRL_8188F 0x036A /* PCIE Multi-Fethc Control */

#define REG_MGQ_TXBD_NUM_8188F 0x0380

/* spec version 11
 * -----------------------------------------------------
 *
 *	0x0400h ~ 0x047Fh	Protocol Configuration
 *
 * ----------------------------------------------------- */
#define REG_VOQ_INFORMATION_8188F 0x0400
#define REG_VIQ_INFORMATION_8188F 0x0404
#define REG_BEQ_INFORMATION_8188F 0x0408
#define REG_BKQ_INFORMATION_8188F 0x040C
#define REG_MGQ_INFORMATION_8188F 0x0410
#define REG_HGQ_INFORMATION_8188F 0x0414
#define REG_BCNQ_INFORMATION_8188F 0x0418
#define REG_TXPKT_EMPTY_8188F 0x041A

#define REG_FWHW_TXQ_CTRL_8188F 0x0420
#define REG_HWSEQ_CTRL_8188F 0x0423
#define REG_TXPKTBUF_BCNQ_BDNY_8188F 0x0424
#define REG_TXPKTBUF_MGQ_BDNY_8188F 0x0425
#define REG_LIFECTRL_CTRL_8188F 0x0426
#define REG_MULTI_BCNQ_OFFSET_8188F 0x0427
#define REG_SPEC_SIFS_8188F 0x0428
#define REG_RL_8188F 0x042A
#define REG_TXBF_CTRL_8188F 0x042C
#define REG_DARFRC_8188F 0x0430
#define REG_RARFRC_8188F 0x0438
#define REG_RRSR_8188F 0x0440
#define REG_ARFR0_8188F 0x0444
#define REG_ARFR1_8188F 0x044C
#define REG_CCK_CHECK_8188F 0x0454
#define REG_AMPDU_MAX_TIME_8188F 0x0456
#define REG_TXPKTBUF_BCNQ_BDNY1_8188F 0x0457

#define REG_AMPDU_MAX_LENGTH_8188F 0x0458
#define REG_TXPKTBUF_WMAC_LBK_BF_HD_8188F 0x045D
#define REG_NDPA_OPT_CTRL_8188F 0x045F
#define REG_FAST_EDCA_CTRL_8188F 0x0460
#define REG_RD_RESP_PKT_TH_8188F 0x0463
#define REG_DATA_SC_8188F 0x0483
#define REG_TXRPT_START_OFFSET 0x04AC
#define REG_POWER_STAGE1_8188F 0x04B4
#define REG_POWER_STAGE2_8188F 0x04B8
#define REG_AMPDU_BURST_MODE_8188F 0x04BC
#define REG_PKT_VO_VI_LIFE_TIME_8188F 0x04C0
#define REG_PKT_BE_BK_LIFE_TIME_8188F 0x04C2
#define REG_STBC_SETTING_8188F 0x04C4
#define REG_HT_SINGLE_AMPDU_8188F 0x04C7
#define REG_PROT_MODE_CTRL_8188F 0x04C8
#define REG_MAX_AGGR_NUM_8188F 0x04CA
#define REG_RTS_MAX_AGGR_NUM_8188F 0x04CB
#define REG_BAR_MODE_CTRL_8188F 0x04CC
#define REG_RA_TRY_RATE_AGG_LMT_8188F 0x04CF
#define REG_MACID_PKT_DROP0_8188F 0x04D0
#define REG_MACID_PKT_SLEEP_8188F 0x04D4

/* -----------------------------------------------------
 *
 *	0x0500h ~ 0x05FFh	EDCA Configuration
 *
 * ----------------------------------------------------- */
#define REG_EDCA_VO_PARAM_8188F 0x0500
#define REG_EDCA_VI_PARAM_8188F 0x0504
#define REG_EDCA_BE_PARAM_8188F 0x0508
#define REG_EDCA_BK_PARAM_8188F 0x050C
#define REG_BCNTCFG_8188F 0x0510
#define REG_PIFS_8188F 0x0512
#define REG_RDG_PIFS_8188F 0x0513
#define REG_SIFS_CTX_8188F 0x0514
#define REG_SIFS_TRX_8188F 0x0516
#define REG_AGGR_BREAK_TIME_8188F 0x051A
#define REG_SLOT_8188F 0x051B
#define REG_TX_PTCL_CTRL_8188F 0x0520
#define REG_TXPAUSE_8188F 0x0522
#define REG_DIS_TXREQ_CLR_8188F 0x0523
#define REG_RD_CTRL_8188F 0x0524
/*
 * Format for offset 540h-542h:
 *	[3:0]:   TBTT prohibit setup in unit of 32us. The time for HW getting beacon content before TBTT.
 *	[7:4]:   Reserved.
 *	[19:8]:  TBTT prohibit hold in unit of 32us. The time for HW holding to send the beacon packet.
 *	[23:20]: Reserved
 * Description:
 *	              |
 * |<--Setup--|--Hold------------>|
 *	--------------|----------------------
 * |
 * TBTT
 * Note: We cannot update beacon content to HW or send any AC packets during the time between Setup and Hold.
 * Described by Designer Tim and Bruce, 2011-01-14.
 *   */
#define REG_TBTT_PROHIBIT_8188F 0x0540
#define REG_RD_NAV_NXT_8188F 0x0544
#define REG_NAV_PROT_LEN_8188F 0x0546
#define REG_BCN_CTRL_8188F 0x0550
#define REG_BCN_CTRL_1_8188F 0x0551
#define REG_MBID_NUM_8188F 0x0552
#define REG_DUAL_TSF_RST_8188F 0x0553
#define REG_BCN_INTERVAL_8188F 0x0554
#define REG_DRVERLYINT_8188F 0x0558
#define REG_BCNDMATIM_8188F 0x0559
#define REG_ATIMWND_8188F 0x055A
#define REG_USTIME_TSF_8188F 0x055C
#define REG_BCN_MAX_ERR_8188F 0x055D
#define REG_RXTSF_OFFSET_CCK_8188F 0x055E
#define REG_RXTSF_OFFSET_OFDM_8188F 0x055F
#define REG_TSFTR_8188F 0x0560
#define REG_CTWND_8188F 0x0572
#define REG_SECONDARY_CCA_CTRL_8188F 0x0577
#define REG_PSTIMER_8188F 0x0580
#define REG_TIMER0_8188F 0x0584
#define REG_TIMER1_8188F 0x0588
#define REG_ACMHWCTRL_8188F 0x05C0
#define REG_SCH_TXCMD_8188F 0x05F8

/* -----------------------------------------------------
 *
 *	0x0600h ~ 0x07FFh	WMAC Configuration
 *
 * ----------------------------------------------------- */
#define REG_MAC_CR_8188F 0x0600
#define REG_TCR_8188F 0x0604
#define REG_RCR_8188F 0x0608
#define REG_RX_PKT_LIMIT_8188F 0x060C
#define REG_RX_DLK_TIME_8188F 0x060D
#define REG_RX_DRVINFO_SZ_8188F 0x060F

#define REG_MACID_8188F 0x0610
#define REG_BSSID_8188F 0x0618
#define REG_MAR_8188F 0x0620
#define REG_MBIDCAMCFG_8188F 0x0628

#define REG_USTIME_EDCA_8188F 0x0638
#define REG_MAC_SPEC_SIFS_8188F 0x063A
#define REG_RESP_SIFP_CCK_8188F 0x063C
#define REG_RESP_SIFS_OFDM_8188F 0x063E
#define REG_ACKTO_8188F 0x0640
#define REG_CTS2TO_8188F 0x0641
#define REG_EIFS_8188F 0x0642

#define REG_NAV_UPPER_8188F 0x0652 /* unit of 128 */
#define REG_TRXPTCL_CTL_8188F 0x0668

/* security */
#define REG_CAMCMD_8188F 0x0670
#define REG_CAMWRITE_8188F 0x0674
#define REG_CAMREAD_8188F 0x0678
#define REG_CAMDBG_8188F 0x067C
#define REG_SECCFG_8188F 0x0680

/* Power */
#define REG_WOW_CTRL_8188F 0x0690
#define REG_PS_RX_INFO_8188F 0x0692
#define REG_UAPSD_TID_8188F 0x0693
#define REG_WKFMCAM_CMD_8188F 0x0698
#define REG_WKFMCAM_NUM_8188F 0x0698
#define REG_WKFMCAM_RWD_8188F 0x069C
#define REG_RXFLTMAP0_8188F 0x06A0
#define REG_RXFLTMAP1_8188F 0x06A2
#define REG_RXFLTMAP2_8188F 0x06A4
#define REG_BCN_PSR_RPT_8188F 0x06A8
#define REG_BT_COEX_TABLE_8188F 0x06C0
#define REG_BFMER0_INFO_8188F 0x06E4
#define REG_BFMER1_INFO_8188F 0x06EC
#define REG_CSI_RPT_PARAM_BW20_8188F 0x06F4
#define REG_CSI_RPT_PARAM_BW40_8188F 0x06F8
#define REG_CSI_RPT_PARAM_BW80_8188F 0x06FC

/* Hardware Port 2 */
#define REG_MACID1_8188F 0x0700
#define REG_BSSID1_8188F 0x0708
#define REG_BFMEE_SEL_8188F 0x0714
#define REG_SND_PTCL_CTRL_8188F 0x0718

/* LTE_COEX */
#define REG_LTECOEX_CTRL 0x07C0
#define REG_LTECOEX_WRITE_DATA 0x07C4

/* -----------------------------------------------------
 *
 *	Redifine 8192C register definition for compatibility
 *
 * ----------------------------------------------------- */

/* TODO: use these definition when using REG_xxx naming rule.
 * NOTE: DO NOT Remove these definition. Use later. */
#define EFUSE_CTRL_8188F REG_EFUSE_CTRL_8188F /* E-Fuse Control. */
#define EFUSE_TEST_8188F REG_EFUSE_TEST_8188F /* E-Fuse Test. */
#define MSR_8188F (REG_CR_8188F + 2) /* Media status register */
#define ISR_8188F REG_HISR0_8188F
#define TSFR_8188F REG_TSFTR_8188F /* Timing Sync Function Timer Register. */

#define PBP_8188F REG_PBP_8188F

/* Redifine MACID register, to compatible prior ICs. */
#define IDR0_8188F REG_MACID_8188F /* MAC ID Register, Offset 0x0050-0x0053 */
#define IDR4_8188F (REG_MACID_8188F + 4) /* MAC ID Register, Offset 0x0054-0x0055 */

/*
 * 9. security Control Registers	(Offset:)
 *   */
#define RWCAM_8188F REG_CAMCMD_8188F /* 8190 data Sheet is called CAMcmd */
#define WCAMI_8188F REG_CAMWRITE_8188F /* Software write CAM input content */
#define RCAMO_8188F REG_CAMREAD_8188F /* Software read/write CAM config */
#define CAMDBG_8188F REG_CAMDBG_8188F
#define SECR_8188F REG_SECCFG_8188F /* security Configuration Register */

/* ----------------------------------------------------------------------------
 * 8195 IMR/ISR bits						(offset 0xB0,  8bits)
 * ---------------------------------------------------------------------------- */
#define IMR_DISABLED_8188F 0
/* IMR DW0(0x00B0-00B3) Bit 0-31 */
#define IMR_TIMER2_8188F BIT(31) /* Timeout interrupt 2 */
#define IMR_TIMER1_8188F BIT(30) /* Timeout interrupt 1 */
#define IMR_PSTIMEOUT_8188F BIT(29) /* Power Save Time Out Interrupt */
#define IMR_GTINT4_8188F BIT(28) /* When GTIMER4 expires, this bit is set to 1 */
#define IMR_GTINT3_8188F BIT(27) /* When GTIMER3 expires, this bit is set to 1 */
#define IMR_TXBCN0ERR_8188F BIT(26) /* Transmit Beacon0 Error */
#define IMR_TXBCN0OK_8188F BIT(25) /* Transmit Beacon0 OK */
#define IMR_TSF_BIT32_TOGGLE_8188F BIT(24) /* TSF Timer BIT32 toggle indication interrupt */
#define IMR_BCNDMAINT0_8188F BIT(20) /* Beacon DMA Interrupt 0 */
#define IMR_BCNDERR0_8188F BIT(16) /* Beacon Queue DMA OK0 */
#define IMR_HSISR_IND_ON_INT_8188F BIT(15) /* HSISR Indicator (HSIMR & HSISR is true, this bit is set to 1) */
#define IMR_BCNDMAINT_E_8188F BIT(14) /* Beacon DMA Interrupt Extension for Win7 */
#define IMR_ATIMEND_8188F BIT(12) /* CTWidnow End or ATIM Window End */
#define IMR_C2HCMD_8188F BIT(10) /* CPU to Host Command INT status, Write 1 clear */
#define IMR_CPWM2_8188F BIT(9) /* CPU power mode exchange INT status, Write 1 clear */
#define IMR_CPWM_8188F BIT(8) /* CPU power mode exchange INT status, Write 1 clear */
#define IMR_HIGHDOK_8188F BIT(7) /* High Queue DMA OK */
#define IMR_MGNTDOK_8188F BIT(6) /* Management Queue DMA OK */
#define IMR_BKDOK_8188F BIT(5) /* AC_BK DMA OK */
#define IMR_BEDOK_8188F BIT(4) /* AC_BE DMA OK */
#define IMR_VIDOK_8188F BIT(3) /* AC_VI DMA OK */
#define IMR_VODOK_8188F BIT(2) /* AC_VO DMA OK */
#define IMR_RDU_8188F BIT(1) /* Rx Descriptor Unavailable */
#define IMR_ROK_8188F BIT(0) /* Receive DMA OK */

/* IMR DW1(0x00B4-00B7) Bit 0-31 */
#define IMR_BCNDMAINT7_8188F BIT(27) /* Beacon DMA Interrupt 7 */
#define IMR_BCNDMAINT6_8188F BIT(26) /* Beacon DMA Interrupt 6 */
#define IMR_BCNDMAINT5_8188F BIT(25) /* Beacon DMA Interrupt 5 */
#define IMR_BCNDMAINT4_8188F BIT(24) /* Beacon DMA Interrupt 4 */
#define IMR_BCNDMAINT3_8188F BIT(23) /* Beacon DMA Interrupt 3 */
#define IMR_BCNDMAINT2_8188F BIT(22) /* Beacon DMA Interrupt 2 */
#define IMR_BCNDMAINT1_8188F BIT(21) /* Beacon DMA Interrupt 1 */
#define IMR_BCNDOK7_8188F BIT(20) /* Beacon Queue DMA OK Interrup 7 */
#define IMR_BCNDOK6_8188F BIT(19) /* Beacon Queue DMA OK Interrup 6 */
#define IMR_BCNDOK5_8188F BIT(18) /* Beacon Queue DMA OK Interrup 5 */
#define IMR_BCNDOK4_8188F BIT(17) /* Beacon Queue DMA OK Interrup 4 */
#define IMR_BCNDOK3_8188F BIT(16) /* Beacon Queue DMA OK Interrup 3 */
#define IMR_BCNDOK2_8188F BIT(15) /* Beacon Queue DMA OK Interrup 2 */
#define IMR_BCNDOK1_8188F BIT(14) /* Beacon Queue DMA OK Interrup 1 */
#define IMR_ATIMEND_E_8188F BIT(13) /* ATIM Window End Extension for Win7 */
#define IMR_TXERR_8188F BIT(11) /* Tx Error Flag Interrupt status, write 1 clear. */
#define IMR_RXERR_8188F BIT(10) /* Rx Error Flag INT status, Write 1 clear */
#define IMR_TXFOVW_8188F BIT(9) /* Transmit FIFO Overflow */
#define IMR_RXFOVW_8188F BIT(8) /* Receive FIFO Overflow */

/*===================================================================
=====================================================================
Here the register defines are for 92C. When the define is as same with 92C,
we will use the 92C's define for the consistency
So the following defines for 92C is not entire!!!!!!
=====================================================================
=====================================================================*/
/*
Based on Datasheet V33---090401
Register Summary
Current IOREG MAP
0x0000h ~ 0x00FFh   System Configuration (256 Bytes)
0x0100h ~ 0x01FFh   MACTOP General Configuration (256 Bytes)
0x0200h ~ 0x027Fh   TXDMA Configuration (128 Bytes)
0x0280h ~ 0x02FFh   RXDMA Configuration (128 Bytes)
0x0300h ~ 0x03FFh   PCIE EMAC Reserved Region (256 Bytes)
0x0400h ~ 0x04FFh   Protocol Configuration (256 Bytes)
0x0500h ~ 0x05FFh   EDCA Configuration (256 Bytes)
0x0600h ~ 0x07FFh   WMAC Configuration (512 Bytes)
0x2000h ~ 0x3FFFh   8051 FW Download Region (8196 Bytes)
*/
/* ----------------------------------------------------------------------------
 *		 8195 (TXPAUSE) transmission pause	(Offset 0x522, 8 bits)
 * ----------------------------------------------------------------------------
 *
#define		StopBecon			BIT(6)
#define		StopHigh				BIT(5)
#define		StopMgt				BIT(4)
#define		StopVO				BIT(3)
#define		StopVI				BIT(2)
#define		StopBE				BIT(1)
#define		StopBK				BIT(0)
*/

/* ****************************************************************************
 * 8192C Regsiter Bit and Content definition
 * ****************************************************************************
 * -----------------------------------------------------
 *
 *	0x0000h ~ 0x00FFh	System Configuration
 *
 * ----------------------------------------------------- */
#if 0
	/* 2 SYS_ISO_CTRL */
#define ISO_MD2PP BIT(0)
#define ISO_UA2USB BIT(1)
#define ISO_UD2CORE BIT(2)
#define ISO_PA2PCIE BIT(3)
#define ISO_PD2CORE BIT(4)
#define ISO_IP2MAC BIT(5)
#define ISO_DIOP BIT(6)
#define ISO_DIOE BIT(7)
#define ISO_EB2CORE BIT(8)
#define ISO_DIOR BIT(9)
#define PWC_EV12V BIT(15)


	/* 2 SYS_FUNC_EN */
#define FEN_BBRSTB BIT(0)
#define FEN_BB_GLB_RSTn BIT(1)
#define FEN_USBA BIT(2)
#define FEN_UPLL BIT(3)
#define FEN_USBD BIT(4)
#define FEN_DIO_PCIE BIT(5)
#define FEN_PCIEA BIT(6)
#define FEN_PPLL BIT(7)
#define FEN_PCIED BIT(8)
#define FEN_DIOE BIT(9)
#define FEN_CPUEN BIT(10)
#define FEN_DCORE BIT(11)
#define FEN_ELDR BIT(12)
#define FEN_DIO_RF BIT(13)
#define FEN_HWPDN BIT(14)
#define FEN_MREGEN BIT(15)

	/* 2 APS_FSMCO */
#define PFM_LDALL BIT(0)
#define PFM_ALDN BIT(1)
#define PFM_LDKP BIT(2)
#define PFM_WOWL BIT(3)
#define EnPDN BIT(4)
#define PDN_PL BIT(5)
#define APFM_ONMAC BIT(8)
#define APFM_OFF BIT(9)
#define APFM_RSM BIT(10)
#define AFSM_HSUS BIT(11)
#define AFSM_PCIE BIT(12)
#define APDM_MAC BIT(13)
#define APDM_HOST BIT(14)
#define APDM_HPDN BIT(15)
#define RDY_MACON BIT(16)
#define SUS_HOST BIT(17)
#define ROP_ALD BIT(20)
#define ROP_PWR BIT(21)
#define ROP_SPS BIT(22)
#define SOP_MRST BIT(25)
#define SOP_FUSE BIT(26)
#define SOP_ABG BIT(27)
#define SOP_AMB BIT(28)
#define SOP_RCK BIT(29)
#define SOP_A8M BIT(30)
#define XOP_BTCK BIT(31)

	/* 2 SYS_CLKR */
#define ANAD16V_EN BIT(0)
#define ANA8M BIT(1)
#define MACSLP BIT(4)
#define LOADER_CLK_EN BIT(5)


	/* 2 9346CR */

#define BOOT_FROM_EEPROM BIT(4)
#define EEPROM_EN BIT(5)


	/* 2 RF_CTRL */
#define RF_EN BIT(0)
#define RF_RSTB BIT(1)
#define RF_SDMRSTB BIT(2)

	/* 2 LDOV12D_CTRL */
#define LDV12_EN BIT(0)
#define LDV12_SDBY BIT(1)
#define LPLDO_HSM BIT(2)
#define LPLDO_LSM_DIS BIT(3)
#define _LDV12_VADJ(x) (((x) & 0xF) << 4)


	/* 2 EFUSE_TEST (For RTL8188 partially) */
#define EF_TRPT BIT(7)
#define EF_CELL_SEL (BIT(8) | BIT(9))  /*  00: Wifi Efuse, 01: BT Efuse0, 10: BT Efuse1, 11: BT Efuse2 */
#define LDOE25_EN BIT(31)
#define EFUSE_SEL(x) (((x) & 0x3) << 8)
#define EFUSE_SEL_MASK 0x300
#define EFUSE_WIFI_SEL_0 0x0
#define EFUSE_BT_SEL_0 0x1
#define EFUSE_BT_SEL_1 0x2
#define EFUSE_BT_SEL_2 0x3


	/* 2 8051FWDL */
	/* 2 MCUFWDL */
#define MCUFWDL_EN BIT(0)
#define MCUFWDL_RDY BIT(1)
#define FWDL_ChkSum_rpt BIT(2)
#define MACINI_RDY BIT(3)
#define BBINI_RDY BIT(4)
#define RFINI_RDY BIT(5)
#define WINTINI_RDY BIT(6)
#define RAM_DL_SEL BIT(7)
#define ROM_DLEN BIT(19)
#define CPRST BIT(23)



	/* 2 REG_SYS_CFG */
#define XCLK_VLD BIT(0)
#define ACLK_VLD BIT(1)
#define UCLK_VLD BIT(2)
#define PCLK_VLD BIT(3)
#define PCIRSTB BIT(4)
#define V15_VLD BIT(5)
#define TRP_B15V_EN BIT(7)
#define SIC_IDLE BIT(8)
#define BD_MAC2 BIT(9)
#define BD_MAC1 BIT(10)
#define IC_MACPHY_MODE BIT(11)
#define CHIP_VER (BIT(12) | BIT(13) | BIT(14) | BIT(15))
#define BT_FUNC BIT(16)
#define VENDOR_ID BIT(19)
#define PAD_HWPD_IDN BIT(22)
#define TRP_VAUX_EN BIT(23)	 /*  RTL ID */
#define TRP_BT_EN BIT(24)
#define BD_PKG_SEL BIT(25)
#define BD_HCI_SEL BIT(26)
#define TYPE_ID BIT(27)

#define CHIP_VER_RTL_MASK 0xF000	 /* Bit 12 ~ 15 */
#define CHIP_VER_RTL_SHIFT 12

#endif
/* -----------------------------------------------------
 *
 *	0x0100h ~ 0x01FFh	MACTOP General Configuration
 *
 * ----------------------------------------------------- */
#if 0

	/* 2 Function Enable Registers */
	/* 2 CR 0x0100-0x0103 */

#define HCI_TXDMA_EN BIT(0)
#define HCI_RXDMA_EN BIT(1)
#define TXDMA_EN BIT(2)
#define RXDMA_EN BIT(3)
#define PROTOCOL_EN BIT(4)
#define SCHEDULE_EN BIT(5)
#define MACTXEN BIT(6)
#define MACRXEN BIT(7)
#define ENSWBCN BIT(8)
#define ENSEC BIT(9)
#define CALTMR_EN BIT(10)	 /*  32k CAL TMR enable */

	/*  Network type */
#define _NETTYPE(x) (((x) & 0x3) << 16)
#define MASK_NETTYPE 0x30000
#define NT_NO_LINK 0x0
#define NT_LINK_AD_HOC 0x1
#define NT_LINK_AP 0x2
#define NT_AS_AP 0x3


	/* 2 PBP - Page Size Register 0x0104 */
#define GET_RX_PAGE_SIZE(value) ((value) & 0xF)
#define GET_TX_PAGE_SIZE(value) (((value) & 0xF0) >> 4)
#define _PSRX_MASK 0xF
#define _PSTX_MASK 0xF0
#define _PSRX(x) (x)
#define _PSTX(x) ((x) << 4)

#define PBP_64 0x0
#define PBP_128 0x1
#define PBP_256 0x2
#define PBP_512 0x3
#define PBP_1024 0x4


	/* 2 TX/RXDMA 0x010C */
#define RXDMA_ARBBW_EN BIT(0)
#define RXSHFT_EN BIT(1)
#define RXDMA_AGG_EN BIT(2)
#define QS_VO_QUEUE BIT(8)
#define QS_VI_QUEUE BIT(9)
#define QS_BE_QUEUE BIT(10)
#define QS_BK_QUEUE BIT(11)
#define QS_MANAGER_QUEUE BIT(12)
#define QS_HIGH_QUEUE BIT(13)

#define HQSEL_VOQ BIT(0)
#define HQSEL_VIQ BIT(1)
#define HQSEL_BEQ BIT(2)
#define HQSEL_BKQ BIT(3)
#define HQSEL_MGTQ BIT(4)
#define HQSEL_HIQ BIT(5)

	/*  For normal driver, 0x10C */
#define _TXDMA_HIQ_MAP(x) (((x) & 0x3) << 14)
#define _TXDMA_MGQ_MAP(x) (((x) & 0x3) << 12)
#define _TXDMA_BKQ_MAP(x) (((x) & 0x3) << 10)
#define _TXDMA_BEQ_MAP(x) (((x) & 0x3) << 8)
#define _TXDMA_VIQ_MAP(x) (((x) & 0x3) << 6)
#define _TXDMA_VOQ_MAP(x) (((x) & 0x3) << 4)

#define QUEUE_LOW 1
#define QUEUE_NORMAL 2
#define QUEUE_HIGH 3


	/* 2 REG_C2HEVT_CLEAR 0x01AF */
#define C2H_EVT_HOST_CLOSE 0x00	 /*  Set by driver and notify FW that the driver has read the C2H command message */
#define C2H_EVT_FW_CLOSE 0xFF		 /*  Set by FW indicating that FW had set the C2H command message and it's not yet read by driver. */



	/* 2 LLT_INIT 0x01E0 */
#define _LLT_NO_ACTIVE 0x0
#define _LLT_WRITE_ACCESS 0x1
#define _LLT_READ_ACCESS 0x2

#define _LLT_INIT_DATA(x) ((x) & 0xFF)
#define _LLT_INIT_ADDR(x) (((x) & 0xFF) << 8)
#define _LLT_OP(x) (((x) & 0x3) << 30)
#define _LLT_OP_VALUE(x) (((x) >> 30) & 0x3)

#endif
/* -----------------------------------------------------
 *
 *	0x0200h ~ 0x027Fh	TXDMA Configuration
 *
 * ----------------------------------------------------- */
#if 0
	/* 2 TDECTL 0x0208 */
#define BLK_DESC_NUM_SHIFT 4
#define BLK_DESC_NUM_MASK 0xF


	/* 2 TXDMA_OFFSET_CHK 0x020C */
#define DROP_DATA_EN BIT(9)
#endif
/* -----------------------------------------------------
 *
 *	0x0280h ~ 0x028Bh	RX DMA Configuration
 *
 * ----------------------------------------------------- */
#if 0
	/* 2 REG_RXDMA_CONTROL, 0x0286h */

	/*  Write only. When this bit is set, RXDMA will decrease RX PKT counter by one. Before */
	/*  this bit is polled, FW shall update RXFF_RD_PTR first. This register is write pulse and auto clear. */
#define RXPKT_RELEASE_POLL BIT(0)
	/*  Read only. When RXMA finishes on-going DMA operation, RXMDA will report idle state in */
	/*  this bit. FW can start releasing packets after RXDMA entering idle mode. */
#define RXDMA_IDLE BIT(1)
	/*  When this bit is set, RXDMA will enter this mode after on-going RXDMA packet to host */
	/*  completed, and stop DMA packet to host. RXDMA will then report Default: 0; */
#define RW_RELEASE_EN BIT(2)
#endif
/* -----------------------------------------------------
 *
 *	0x0400h ~ 0x047Fh	Protocol Configuration
 *
 * ----------------------------------------------------- */
#if 0
	/* 2 FWHW_TXQ_CTRL 0x0420 */
#define EN_AMPDU_RTY_NEW BIT(7)


	/* 2 REG_LIFECTRL_CTRL 0x0426 */
#define HAL92C_EN_PKT_LIFE_TIME_BK BIT(3)
#define HAL92C_EN_PKT_LIFE_TIME_BE BIT(2)
#define HAL92C_EN_PKT_LIFE_TIME_VI BIT(1)
#define HAL92C_EN_PKT_LIFE_TIME_VO BIT(0)

#define HAL92C_MSDU_LIFE_TIME_UNIT 128		 /*  in us, said by Tim. */


	/* 2 SPEC SIFS 0x0428 */
#define _SPEC_SIFS_CCK(x) ((x) & 0xFF)
#define _SPEC_SIFS_OFDM(x) (((x) & 0xFF) << 8)

	/* 2 RL 0x042A */
#define RETRY_LIMIT_SHORT_SHIFT 8
#define RETRY_LIMIT_LONG_SHIFT 0

#define _LRL(x) ((x) & 0x3F)
#define _SRL(x) (((x) & 0x3F) << 8)
#endif

/* -----------------------------------------------------
 *
 *	0x0500h ~ 0x05FFh	EDCA Configuration
 *
 * ----------------------------------------------------- */
#if 0
	/* 2 EDCA setting 0x050C */
#define AC_PARAM_TXOP_LIMIT_OFFSET 16
#define AC_PARAM_ECW_MAX_OFFSET 12
#define AC_PARAM_ECW_MIN_OFFSET 8
#define AC_PARAM_AIFS_OFFSET 0


	/* 2 BCN_CTRL 0x0550 */
#define EN_TXBCN_RPT BIT(2)
#define EN_BCN_FUNCTION BIT(3)

	/* 2 TxPause 0x0522 */
#define STOP_BCNQ BIT(6)
#endif

/* 2 ACMHWCTRL 0x05C0 */
#define acm_hw_hw_en_8188f BIT(0)
#define acm_hw_voq_en_8188f BIT(1)
#define acm_hw_viq_en_8188f BIT(2)
#define acm_hw_beq_en_8188f BIT(3)
#define acm_hw_voq_status_8188f BIT(5)
#define acm_hw_viq_status_8188f BIT(6)
#define acm_hw_beq_status_8188f BIT(7)

/* -----------------------------------------------------
 *
 *	0x0600h ~ 0x07FFh	WMAC Configuration
 *
 * ----------------------------------------------------- */
#if 0

	/* 2 TCR 0x0604 */
#define DIS_GCLK BIT(1)
#define PAD_SEL BIT(2)
#define PWR_ST BIT(6)
#define PWRBIT_OW_EN BIT(7)
#define ACRC BIT(8)
#define CFENDFORM BIT(9)
#define ICV BIT(10)
#endif

/* ----------------------------------------------------------------------------
 * 8195 (RCR) Receive Configuration Register	(Offset 0x608, 32 bits)
 * ---------------------------------------------------------------------------- */
#if 0
#define RCR_APPFCS BIT(31)		 /*  WMAC append FCS after pauload */
#define RCR_APP_MIC BIT(30)		 /*  MACRX will retain the MIC at the bottom of the packet. */
#define RCR_APP_ICV BIT(29)        /*  MACRX will retain the ICV at the bottom of the packet. */
#define RCR_APP_PHYST_RXFF BIT(28)        /*  HY status is appended before RX packet in RXFF */
#define RCR_APP_BA_SSN BIT(27)		 /*  SSN of previous TXBA is appended as after original RXDESC as the 4-th DW of RXDESC. */
#define RCR_RSVD_BIT(26) BIT26		 /*  Reserved */
#endif
#define RCR_TCPOFLD_EN BIT(25) /* Enable TCP checksum offload */

#endif /*  #ifndef __INC_HAL8188FREG_H */
