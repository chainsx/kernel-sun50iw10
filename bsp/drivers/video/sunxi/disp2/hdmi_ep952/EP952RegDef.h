/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef EP952REGDEF_H
#define EP952REGDEF_H

/* Registers                                                            Word    BitMask */

#define EP952_TX_PHY_Control_0				0x00
#define EP952_TX_PHY_Control_0__TERM_ON				0x80
#define EP952_TX_PHY_Control_0__DSWING				0x30

#define EP952_TX_PHY_Control_1				0x01
#define EP952_TX_PHY_Control_1__SCLK_DLY			0x80
#define EP952_TX_PHY_Control_1__TPHY_RST			0x40
#define EP952_TX_PHY_Control_1__RESN_DIS			0x20
#define EP952_TX_PHY_Control_1__VCO_GAIN			0x0C
#define EP952_TX_PHY_Control_1__PHD_CUR				0x03

#define EP952_TX_EE_SUM						0x02

#define EP952_General_Control_8				0x05
#define EP952_General_Control_8__ADO_SPDIF_IN		0x00
#define EP952_General_Control_8__ADO_IIS_IN			0x10
#define EP952_General_Control_8__COMR_DIS			0x04

#define EP952_SMPRD							0x06	/* 2 Byte */

#define EP952_General_Control_1				0x08
#define EP952_General_Control_1__TSEL_HTP			0x80
#define EP952_General_Control_1__INT_OD				0x40
#define EP952_General_Control_1__INT_POL			0x20
#define EP952_General_Control_1__OSC_PD				0x10
#define EP952_General_Control_1__DSEL				0x08
#define EP952_General_Control_1__BSEL				0x04
#define EP952_General_Control_1__EDGE				0x02
#define EP952_General_Control_1__PU					0x01

#define EP952_General_Control_2				0x09
#define EP952_General_Control_2__RSEN				0x80
#define EP952_General_Control_2__HTPLG				0x40
#define EP952_General_Control_2__PIFE				0x08
#define EP952_General_Control_2__RIFE				0x04
#define EP952_General_Control_2__VIFE				0x02
#define EP952_General_Control_2__MIFE				0x01

#define EP952_General_Control_3				0x0A

#define EP952_Configuration					0x0B

#define EP952_Color_Space_Control			0x0C
#define EP952_Color_Space_Control__422_OUT			0x80
#define EP952_Color_Space_Control__YCC_OUT			0x40
#define EP952_Color_Space_Control__COLOR			0x20
#define EP952_Color_Space_Control__YCC_Range		0x10
#define EP952_Color_Space_Control__VMUTE			0x08
#define EP952_Color_Space_Control__AMUTE			0x04
#define EP952_Color_Space_Control__AVMUTE			0x02

#define EP952_Pixel_Repetition_Control		0x0D
#define EP952_Pixel_Repetition_Control__CS_M		0x80
#define EP952_Pixel_Repetition_Control__CTS_M		0x40
#define EP952_Pixel_Repetition_Control__ADSR		0x30
#define EP952_Pixel_Repetition_Control__VSYNC		0x04
#define EP952_Pixel_Repetition_Control__PR			0x03

#define EP952_General_Control_4				0x0E
#define EP952_General_Control_4__FMT12				0x80
#define EP952_General_Control_4__422_IN				0x40
#define EP952_General_Control_4__YCC_IN				0x20
#define EP952_General_Control_4__E_SYNC				0x10
#define EP952_General_Control_4__VPOL_DET			0x08
#define EP952_General_Control_4__HPOL_DET			0x04
#define EP952_General_Control_4__EESS				0x02
#define EP952_General_Control_4__HDMI				0x01

#define EP952_General_Control_5				0x0F
#define EP952_General_Control_5__AKSV_RDY			0x80
#define EP952_General_Control_5__RPTR				0x10
#define EP952_General_Control_5__RI_RDY				0x02
#define EP952_General_Control_5__ENC_EN				0x01

#define EP952_BKSV							0x10	/* BKSV1-BKSV5 0x10-0x14 */

#define EP952_AN							0x15	/* AN1-AN8 0x15-0x1C */

#define EP952_AKSV							0x1D	/* AKSV1-AKSV5 0x1D-0x21 */

#define EP952_RI							0x22	/* RI1-RI2 0x22-0x23 */

#define EP952_M0							0x25	/* 0x25-0x32 */

#define EP952_DE_DLY						0x32	/* 10 bit */

#define EP952_DE_Control					0x33	/* 10 bit */
#define EP952_DE_Control__DE_GEN					0x40
#define EP952_DE_Control__VSO_POL					0x08
#define EP952_DE_Control__HSO_POL					0x04

#define EP952_DE_TOP						0x34	/* 6 bit */

#define EP952_DE_CNT						0x36	/* 10 bit */

#define EP952_DE_LIN						0x38	/* 10 bit */

#define EP952_H_RES							0x3A	/* 11 bit */

#define EP952_V_RES							0x3C	/* 11 bit */

#define EP952_Audio_Subpacket_Allocation	0x3E	/* Default 0xE4 */

#define EP952_IIS_Control					0x3F	/* Default 0x00 */
#define EP952_IIS_Control__ACR_EN					0x80
#define EP952_IIS_Control__AVI_EN					0x40
#define EP952_IIS_Control__ADO_EN					0x20
#define EP952_IIS_Control__GC_EN					0x10
#define EP952_IIS_Control__AUDIO_EN					0x08
#define EP952_IIS_Control__WS_M						0x04
#define EP952_IIS_Control__WS_POL					0x02
#define EP952_IIS_Control__SCK_POL					0x01

#define EP952_Packet_Control				0x40	/* Default 0x00 */
#define EP952_Packet_Control__FLAT					0x10
#define EP952_Packet_Control__VTX0					0x08
#define EP952_Packet_Control__PKT_RDY				0x01

#define EP952_Data_Packet_Header 			0x41	/* HB0-HB2 0x41-0x43 */
#define EP952_Data_Packet 					0x44	/* PB0-PB27 0x44-0x5F */

#define EP952_CTS_H		 					0x60	/* 20bit (3 Byte) */
#define EP952_CTS_M		 					0x61	/* 20bit (3 Byte) */
#define EP952_CTS_L		 					0x62	/* 20bit (3 Byte) */

#define EP952_N_H			 				0x63	/* 20bit (3 Byte) */
#define EP952_N_M			 				0x64	/* 20bit (3 Byte) */
#define EP952_N_L			 				0x65	/* 20bit (3 Byte) */

#define EP952_AVI_Packet 					0x66	/* 14 Byte 0x66-0x73 */

#define EP952_ADO_Packet 					0x74	/* 6 Byte 0x74-0x79 */

#define EP952_SPDIF_Sampling_Frequency 		0x7A	/* 1 Byte */

#define EP952_Channel_Status		 		0x7B	/* 5 Byte 0x7B-0x7F */

#define EP952_Embedded_Sync_Control	 		0x80	/* Default 0x00 */

#define EP952_H_Delay			 			0x81	/* 10 bit (2 Byte) */

#define EP952_H_Width			 			0x83	/* 10 bit (2 Byte) */

#define EP952_V_Delay			 			0x85	/* 6 bit */

#define EP952_V_Width			 			0x86	/* 6 bit */

#define EP952_V_Off_Set			 			0x87	/* 12 bit (2 Byte) */

#define EP952_Key_Add			 			0xF0	/* 1 Byte */

#define EP952_Key_Data			 			0xF1	/* 7 Byte */

#endif
