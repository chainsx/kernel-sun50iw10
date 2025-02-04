/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef HDCP_H
#define HDCP_H

#define HDCP_TIMER_PERIOD 					10

/* HDCP Transmiter Link State */
typedef enum {
	A0_Wait_for_Active_Rx,
	A1_Exchange_KSVs,
	A2_Computations,
	A3_Validate_Receiver,
	A4_Authenticated,
	A5_Link_Integrity_Check,
	A6_Test_for_Repeater,
	A8_Wait_for_READY,
	A9_Read_KSV_List
} HDCP_STATE;

/* ----------------------------------------------------------------------------- */
#define HDCP_ERROR_BKSV									0x80
#define HDCP_ERROR_AKSV									0x40
#define HDCP_ERROR_R0									0x20
#define HDCP_ERROR_Ri									0x10
#define HDCP_ERROR_RepeaterRdy							0x08
#define HDCP_ERROR_RepeaterSHA							0x04
#define HDCP_ERROR_RSEN									0x02
#define HDCP_ERROR_RepeaterMax							0x01

/* ----------------------------------------------------------------------------- */
extern HDCP_STATE HDCP_Authentication_Task(unsigned char ReceiverRdy);
extern void HDCP_Stop(void);
extern unsigned char HDCP_Get_Status(void);
extern void HDCP_Timer(void);
extern void HDCP_Ext_Ri_Trigger(void);

/* Special Functions */
extern void HDCP_Assign_RKSV_List(unsigned char *pRevocationList,
				  unsigned char ListNumber);
extern void HDCP_Extract_BKSV_BCAPS3(unsigned char *pBKSV_BCaps3);
extern void HDCP_Extract_FIFO(unsigned char *pFIFO, unsigned char ListNumber);
extern void HDCP_Extract_SHA_M0(unsigned char *pSha_M0);

/* ----------------------------------------------------------------------------- */
#endif /* HDCP_H */
