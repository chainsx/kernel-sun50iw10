/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef EP952CONTROLLER_H
#define EP952CONTROLLER_H

#include "EP952_If.h"
#include "EP952api.h"

#if Enable_HDCP
#include "HDCP.h"
#endif

#define VERSION_MAJOR             52
#define VERSION_MINOR              1

#define INT_enable		1
#define INT_disable		0
#define INT_OPEN_DRAIN	1
#define INT_PUSH_PULL	0
#define INT_High		1
#define INT_Low			0

typedef enum {
	TXS_Search_EDID,
	TXS_Wait_Upstream,
	TXS_Stream,
	TXS_HDCP
} TX_STATE;

typedef struct _EP952C_REGISTER_MAP {

	unsigned char System_Status;

#if Enable_HDCP
	unsigned char HDCP_Status;
	unsigned char HDCP_State;
	unsigned char HDCP_AKSV[5];
	unsigned char HDCP_BKSV[5];
	unsigned char HDCP_BCAPS3[3];
	unsigned char HDCP_KSV_FIFO[5 * 16];
	unsigned char HDCP_SHA[20];
	unsigned char HDCP_M0[8];
#endif

	unsigned char Readed_EDID[256];
	unsigned char EDID_ASFreq;
	unsigned char EDID_AChannel;
	/* unsigned char         EDID_VideoDataAddr; */
	/* unsigned char         EDID_AudioDataAddr; */
	/* unsigned char         EDID_SpeakerDataAddr; */
	/* unsigned char         EDID_VendorDataAddr; */

	unsigned char System_Configuration;

	unsigned char Video_Interface[2];	/*  */
	unsigned char Video_Input_Format[2];	/*  */
	unsigned char Video_Output_Format;	/*  */

	unsigned char Audio_Interface;	/*  */
	unsigned char Audio_Input_Format;	/*  */

	unsigned char Video_change;
	unsigned char Audio_change;

} EP952C_REGISTER_MAP, *PEP952C_REGISTER_MAP;

/* ----------------------------------------------------------------------------- */
/* ----------------------------------------------------------------------------- */

typedef void (*EP952C_CALLBACK) (void);

void EP952Controller_Initial(PEP952C_REGISTER_MAP pEP952C_RegMap);

void EP952Controller_Task(void);

void EP952Controller_Timer(void);

void EP952_Audio_reg_set(void);
void EP952_Video_reg_set(void);

void EP952_EXTINT_init(unsigned char INT_Enable, unsigned char INT_OD,
		       unsigned char INT_POL);
void EP_HDMI_DumpMessage(void);

/* ----------------------------------------------------------------------------- */
#endif
