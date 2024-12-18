/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef EP952_IF_H
#define EP952_IF_H

#include "EP952api.h"
#include "EP952RegDef.h"

/* ================================================================================================== */
/*  */
/* Protected Data Member */
/*  */

typedef enum {
	COLORSPACE_601 = 1,
	COLORSPACE_709
} COLORSPACE;

typedef enum {
	SYNCMODE_HVDE = 0,
	SYNCMODE_HV,
	SYNCMODE_Embeded
} SYNCMODE;

typedef enum {
	COLORFORMAT_RGB = 0,
	COLORFORMAT_YCC444,
	COLORFORMAT_YCC422
} COLORFORMAT;

typedef enum {
	AFAR_VideoCode = 0,
	AFAR_4_3,
	AFAR_16_9,
	AFAR_14_9
} AFAR;

typedef enum {
	SCAN_NoData = 0,
	SCAN_OverScan,
	SCAN_UnderScan,
	SCAN_Future
} SCAN_MODE;

/* Video Output Congif Params */
typedef struct _VDO_PARAMS {
/* Which cause Timing Change Reset */
	unsigned char Interface;	/* DK[3:1], DKEN, DSEL, BSEL, EDGE, FMT12 */
	unsigned char VideoSettingIndex;	/* VIC */
	unsigned char HVPol;	/* x, x, x, x, VSO_POL, HSO_POL, x, x */
	SYNCMODE SyncMode;	/* 0 = HVDE, 1 = HV(DE Gen), 2 = Embedded Sync */
	COLORFORMAT FormatIn;	/* 0 = RGB, 1 = YCC444, 2 = YCC422 */
	COLORFORMAT FormatOut;	/* 0 = RGB, 1 = YCC444, 2 = YCC422 */
/* Which don't cause Timing Chage Reset */
	COLORSPACE ColorSpace;	/* 0 = Auto, 1 = 601, 2 = 709 */
	AFAR AFARate;		/* 0 = Auto, 1 = 4:3, 2 = 16:9, 3 = 14:9 */
	SCAN_MODE SCAN;		/* 0 = no data, 1 = overscan, 2 = underscan, 3 = future */
} VDO_PARAMS, *PVDO_PARAMS;

typedef enum {
	ADSFREQ_32000Hz = 0x03,
	ADSFREQ_44100Hz = 0x00,
	ADSFREQ_48000Hz = 0x02,
	ADSFREQ_88200Hz = 0x08,
	ADSFREQ_96000Hz = 0x0A,
	ADSFREQ_176400Hz = 0x0C,
	ADSFREQ_192000Hz = 0x0E
} ADSFREQ;

/* Audio Output Congif Params */
typedef struct _ADO_PARAMS {
	unsigned char Interface;	/* x, x, x, x, IIS, WS_M, WS_POL, SCK_POL */
	unsigned char VideoSettingIndex;	/* VIC */
	unsigned char ChannelNumber;	/* 1 = 2 ch, 2 = 3 ch, ... , 5 = 5.1 ch, 7 = 7.1 ch */
	unsigned char ADSRate;	/* 1 = SF/2, 2 = SF/3, 3 = SF/4 (Down Sample) */
	ADSFREQ InputFrequency;	/* ADSFREQ */
	unsigned char VFS;	/* 0 = 59.94Hz, 1 = 60Hz (Vertical Frequency Shift of Video) */
	unsigned char NoCopyRight;
} ADO_PARAMS, *PADO_PARAMS;

/* ================================================================================================== */
/*  */
/* Public Functions */
/*  */

/* -------------------------------------------------------------------------------------------------- */
/*  */
/* General */
/*  */

/* All Interface Inital */
extern void EP952_IIC_Initial(void);
extern void EP952_Info_Reset(void);

/* -------------------------------------------------------------------------------------------------- */
/*  */
/* HDMI Transmiter Interface */
/*  */

/* Common */
extern void HDMI_Tx_Power_Down(void);
extern void HDMI_Tx_Power_Up(void);
extern void HDMI_Tx_HDMI(void);
extern void HDMI_Tx_DVI(void);
extern unsigned char HDMI_Tx_HTPLG(void);
extern unsigned char HDMI_Tx_RSEN(void);
extern unsigned char HDMI_Tx_hpd_state(void);

/* HDCP */
extern void HDMI_Tx_Mute_Enable(void);
extern void HDMI_Tx_Mute_Disable(void);
extern void HDMI_Tx_HDCP_Enable(void);
extern void HDMI_Tx_HDCP_Disable(void);
extern void HDMI_Tx_RPTR_Set(void);
extern void HDMI_Tx_RPTR_Clear(void);
extern unsigned char HDMI_Tx_RI_RDY(void);
extern void HDMI_Tx_write_AN(unsigned char *pAN);
extern unsigned char HDMI_Tx_AKSV_RDY(void);
extern unsigned char HDMI_Tx_read_AKSV(unsigned char *pAKSV);
extern void HDMI_Tx_write_BKSV(unsigned char *pBKSV);
extern unsigned char HDMI_Tx_read_RI(unsigned char *pRI);
extern void HDMI_Tx_read_M0(unsigned char *pM0);
extern SMBUS_STATUS HDMI_Tx_Get_Key(unsigned char *Key);

/* Special for EP952E */
extern void HDMI_Tx_AMute_Enable(void);
extern void HDMI_Tx_AMute_Disable(void);
extern void HDMI_Tx_VMute_Enable(void);
extern void HDMI_Tx_VMute_Disable(void);
extern void HDMI_Tx_Video_Config(PVDO_PARAMS Params);
extern void HDMI_Tx_Audio_Config(PADO_PARAMS Params);

/* -------------------------------------------------------------------------------------------------- */
/*  */
/* Hardware Interface */
/*  */

/* EP952 */
extern SMBUS_STATUS EP952_Reg_Read(unsigned char ByteAddr, unsigned char *Data,
				   unsigned int Size);
extern SMBUS_STATUS EP952_Reg_Write(unsigned char ByteAddr, unsigned char *Data,
				    unsigned int Size);
extern SMBUS_STATUS EP952_Reg_Set_Bit(unsigned char ByteAddr,
				      unsigned char BitMask);
extern SMBUS_STATUS EP952_Reg_Clear_Bit(unsigned char ByteAddr,
					unsigned char BitMask);

#endif /* EP952_IF_H */
