/*
* for modules The decoder's video format module.
*
* Copyright (C) 2016 	NEXTCHIP Inc. All rights reserved.
*
* Authors:
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*/

#ifndef _NVP6134_VIDEO_HI_
#define _NVP6134_VIDEO_HI_
#include "common.h"
#include "acp.h"
#include "eq.h"
#include "type.h"
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/videodev2.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mediabus.h>

#include "../camera.h"
#include "../sensor_helper.h"
#include "csi_dev_nvp6134.h"

/********************************************************************
 *  define and enum
 ********************************************************************/
typedef enum _nvp6134_vi_mode {
	NVP6134_VI_720H = 0x00,       /* 720x576i(480) */
	NVP6134_VI_960H,	      /* 960x576i(480) */
	NVP6134_VI_1280H,	     /* 1280x576i(480) */
	NVP6134_VI_1440H,	     /* 1440x576i(480) */
	NVP6134_VI_1920H,	     /* 1920x576i(480) */
	NVP6134_VI_960H2EX,	   /* 3840x576i(480) */
	NVP6134_VI_720P_2530 = 0x10,  /* 1280x720@25p(30) */
	NVP6134_VI_HDEX = 0x14,       /* 2560x720@25p(30) */
	NVP6134_VI_720P_5060 = 0x18,  /* 1280x720@50p(60) */
	NVP6134_VI_1080P_2530 = 0x20, /* 1920x1080@25p(30) */
	NVP6134_VI_1080P_NRT,	 /* 1920x1080@12.5p(15) */
	NVP6134_VI_1080P_NOVIDEO,     /* 1920x1080@25p(30) */
	NVP6134_VI_3M_NRT = 0x30,     /* 2048x1536@18p */
	NVP6134_VI_3M,		      /* 2048x1536@25p(30p) */
	NVP6134_VI_4M_NRT = 0x40,     /* 2560x1440@NRT */
	NVP6134_VI_4M,		      /* 2560x1440@25p(30) */
	NVP6134_VI_5M_NRT = 0x50,     /* 2592x1944@12.5P */
	NVP6134_VI_5M,		      /* 2592x1944@25P */
	NVP6134_VI_5M_20P = 0x56,     /* 2592x1944@20P */
	NVP6134_VI_8M_NRT = 0x80,     /* reserved */
	NVP6134_VI_8M,		      /* reserved */
	NVP6134_VI_UHD_X = 0x86,      /* reserved */
	NVP6134_VI_BUTT
} NVP6134_VI_MODE;

typedef enum _nvp6134_outmode_sel {
	NVP6134_OUTMODE_1MUX_SD = 0,
	NVP6134_OUTMODE_1MUX_HD,
	NVP6134_OUTMODE_1MUX_HD5060,
	NVP6134_OUTMODE_1MUX_FHD,
	NVP6134_OUTMODE_2MUX_SD,
	NVP6134_OUTMODE_2MUX_HD_X,
	NVP6134_OUTMODE_2MUX_HD,
	NVP6134_OUTMODE_2MUX_FHD_X,
	NVP6134_OUTMODE_4MUX_SD,
	NVP6134_OUTMODE_4MUX_HD_X,
	NVP6134_OUTMODE_4MUX_HD,
	NVP6134_OUTMODE_2MUX_FHD,
	NVP6134_OUTMODE_1MUX_HD_X,
	NVP6134_OUTMODE_1MUX_FHD_X,
	NVP6134_OUTMODE_4MUX_FHD_X,
	NVP6134_OUTMODE_4MUX_MIX,
	NVP6134_OUTMODE_2MUX_MIX,
	NVP6134_OUTMODE_1MUX_BT1120S, /* */
	NVP6134_OUTMODE_2MUX_BT1120S,
	NVP6134_OUTMODE_4MUX_BT1120S,
	NVP6134_OUTMODE_1MUX_3M_RT,
	NVP6134_OUTMODE_1MUX_4M_NRT,
	NVP6134_OUTMODE_BUTT
} NVP6134_OUTMODE_SEL;

/********************************************************************
 *  structure
 ********************************************************************/

/********************************************************************
 *  external api
 ********************************************************************/
void nvp6134_common_init(unsigned char chip);
int nvp6134_set_portmode(const unsigned char chip, const unsigned char portsel,
			 const unsigned char portmode,
			 const unsigned char chid);
int nvp6134_set_chnmode(const unsigned char ch, const unsigned char vfmt,
			const unsigned char chnmode);
void nvp6134_set_portcontrol(unsigned char chip, unsigned char portsel,
			     unsigned char enclk, unsigned char endata);

void nvp6134_set_common_value(unsigned char ch, int mode);
void nvp6134_setchn_common_cvbs(const unsigned char ch,
				const unsigned char vfmt);
void nvp6134_setchn_common_720p(const unsigned char ch,
				const unsigned char vfmt);
void nvp6134_setchn_common_fhd(const unsigned char ch,
			       const unsigned char vfmt);
void nvp6134_setchn_720h(const unsigned char ch, const unsigned char vfmt);
void nvp6134_setchn_960h(const unsigned char ch, const unsigned char vfmt);
void nvp6134_setchn_1280h(const unsigned char ch, const unsigned char vfmt);
void nvp6134_setchn_1440h(const unsigned char ch, const unsigned char vfmt);
void nvp6134_setchn_1920h(const unsigned char ch, const unsigned char vfmt);
void nvp6134_setchn_3840h(const unsigned char ch, const unsigned char vfmt);
void nvp6134_setchn_ahd_720p(const unsigned char ch, const unsigned char vfmt);
void nvp6134_setchn_ahd_720pex(const unsigned char ch,
			       const unsigned char vfmt);
void nvp6134_setchn_ahd_720p5060(const unsigned char ch,
				 const unsigned char vfmt);
void nvp6134_setchn_ahd_1080p2530(const unsigned char ch,
				  const unsigned char vfmt);
void nvp6134_setchn_ahd_3MNRT(const unsigned char ch, const unsigned char vfmt);
void nvp6134_setchn_ahd_3M(const unsigned char ch, const unsigned char vfmt);
void nvp6134_setchn_ahd_5MNRT(const unsigned char ch, const unsigned char vfmt);
void nvp6134_setchn_ahd_1080p_NRT(const unsigned char ch,
				  const unsigned char vfmt);
void nvp6134_setchn_ahd_1080p_novideo(const unsigned char ch,
				      const unsigned char vfmt);
void nvp6134_setchn_ahd_QHD(const unsigned char ch, const unsigned char vfmt);
void nvp6134_setchn_ahd_QHD_X(const unsigned char ch, const unsigned char vfmt);
void nvp6134_setchn_ahd_QHD_NRT(const unsigned char ch,
				const unsigned char vfmt);
void nvp6134_setchn_ahd_5M_20p(const unsigned char ch,
			       const unsigned char vfmt);

void video_fmt_det(nvp6134_input_videofmt *pvideofmt);
unsigned int nvp6134_getvideoloss(void);
unsigned char video_fmt_debounce(unsigned char ch, unsigned char value);

void nvp6134_video_set_contrast(unsigned int ch, unsigned int value,
				unsigned int v_format);
void nvp6134_video_set_brightness(unsigned int ch, unsigned int value,
				  unsigned int v_format);
void nvp6134_video_set_saturation(unsigned int ch, unsigned int value,
				  unsigned int v_format);
void nvp6134_video_set_hue(unsigned int ch, unsigned int value,
			   unsigned int v_format);
void nvp6134_video_set_sharpness(unsigned int ch, unsigned int value);
void nvp6134_video_set_ugain(unsigned int ch, unsigned int value);
void nvp6134_video_set_vgain(unsigned int ch, unsigned int value);
void nvp6134_hide_ch(unsigned char ch);
void nvp6134_show_ch(unsigned char ch);
int nvp6134_GetAgcLockStatus(unsigned char ch);
int nvp6134_GetFSCLockStatus(unsigned char ch);
void nvp6134_ResetFSCLock(unsigned char ch);
void nvp6134_chn_killcolor(unsigned char ch, unsigned char onoff);
void nvp6134_3M_RT2NRT(unsigned char ch, unsigned char vfmt);
void nvp6134_3M_NRT2RT(unsigned char ch);
unsigned char nvp6134_vfmt_convert(unsigned char vdsts, unsigned char g_ck_fmt);
int isItAHDmode(unsigned char vfmt);
unsigned char trans_ahd_to_chd(unsigned char vfmt);

void msleep(u32 mcount);

#endif /* End of _NVP6134_VIDEO_HI_ */

/********************************************************************
 *  End of file
 ********************************************************************/
