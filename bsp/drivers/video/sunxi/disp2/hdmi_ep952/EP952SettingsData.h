/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef EP952SETTINGDATA_H
#define EP952SETTINGDATA_H

/* ----------------------------------------------------------------------------- */

#define EP952_VDO_Settings_IT_Start 76

/* Definition of H/V Polarity */
#define  VPosHPos               0x00
#define  VPosHNeg               0x04
#define  VNegHPos               0x08
#define  VNegHNeg               0x0C

/* Pixel Freq Type */
typedef enum {
	PIX_FREQ_25175KHz = 0,
	PIX_FREQ_25200KHz,

	PIX_FREQ_27000KHz,
	PIX_FREQ_27027KHz,

	PIX_FREQ_54000KHz,
	PIX_FREQ_54054KHz,

	PIX_FREQ_72000KHz,

	PIX_FREQ_74176KHz,
	PIX_FREQ_74250KHz,

	PIX_FREQ_108000KHz,
	PIX_FREQ_108108KHz,

	PIX_FREQ_148352KHz,
	PIX_FREQ_148500KHz,

	PIX_FREQ_PC
} PIX_FREQ_TYPE;

/*  */
/* Index = [Video Code] */
/*  */
typedef struct _HVRES_TYPE {
	unsigned char HVPol;
	unsigned short Hres;
	unsigned short Vres;
	unsigned short Vprd;
} HVRES_TYPE, *PHVRES_TYPE;

/* DE Generation */
typedef struct _DE_GEN_SETTINGS {
	unsigned short DE_DLY;
	unsigned short DE_CNT;
	unsigned char DE_TOP;
	unsigned short DE_LIN;
} DE_GEN_SETTINGS, *PDE_GEN_SETTINGS;

/* Embeded Sybc */
typedef struct _E_SYNC_SETTINGS {
	unsigned char CTL;
	unsigned short H_DLY;
	unsigned short H_WIDTH;
	unsigned char V_DLY;
	unsigned char V_WIDTH;
	unsigned short V_OFST;
} E_SYNC_SETTINGS, *PE_SYNC_SETTINGS;

/* AVI Settings */
typedef struct _VDO_SETTINGS {
	unsigned char VideoCode;
	HVRES_TYPE HVRes_Type;
	DE_GEN_SETTINGS DE_Gen;
	E_SYNC_SETTINGS E_Sync;	/* (HV_Gen) */
	unsigned char AR_PR;
	PIX_FREQ_TYPE Pix_Freq_Type;
} VDO_SETTINGS, *PVDO_SETTINGS;

/* Index = [Channel Number] */
/* Audio Channel and Allocation */
typedef struct _ADO_SETTINGS {
	unsigned char SpeakerMapping;
	unsigned char Flat;
} ADO_SETTINGS, *PADO_SETTINGS;

/* Index = [Pixel Freq Type] */
/* N and CTS */
typedef struct _N_CTS_SETTINGS {
	unsigned long N;
	unsigned long CTS;	/* Use hardware to calculate the CTS */
} N_CTS_SETTINGS, *PN_CTS_SETTINGS;

extern unsigned char EP952_VDO_Settings_Max;
extern VDO_SETTINGS EP952_VDO_Settings[];
extern ADO_SETTINGS EP952_ADO_Settings[];
extern N_CTS_SETTINGS N_CTS_32K[];
extern N_CTS_SETTINGS N_CTS_44K1[];
extern N_CTS_SETTINGS N_CTS_48K[];

#endif
