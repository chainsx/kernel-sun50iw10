/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include "fc_video.h"

void fc_video_hdcp_keepout(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, FC_INVIDCONF, FC_INVIDCONF_HDCP_KEEPOUT_MASK, bit);
}

static void fc_video_VSyncPolarity(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, FC_INVIDCONF,
			FC_INVIDCONF_VSYNC_IN_POLARITY_MASK, bit);
}

static void fc_video_HSyncPolarity(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, FC_INVIDCONF,
			FC_INVIDCONF_HSYNC_IN_POLARITY_MASK, bit);
}

static void fc_video_DataEnablePolarity(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, FC_INVIDCONF,
			FC_INVIDCONF_DE_IN_POLARITY_MASK, bit);
}

void fc_video_DviOrHdmi(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	/* 1: HDMI; 0: DVI */
	dev_write_mask(dev, FC_INVIDCONF, FC_INVIDCONF_DVI_MODEZ_MASK, bit);
}

static void fc_video_VBlankOsc(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, FC_INVIDCONF,
			FC_INVIDCONF_R_V_BLANK_IN_OSC_MASK, bit);
}

/* for some video modes that have fractional Vblank */
static void fc_video_Interlaced(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, FC_INVIDCONF, FC_INVIDCONF_IN_I_P_MASK, bit);
}

static void fc_video_HActive(hdmi_tx_dev_t *dev, u16 value)
{
	LOG_TRACE1(value);
	/* 12-bit width */
	dev_write(dev, (FC_INHACTIV0), (u8) (value));
	dev_write_mask(dev, FC_INHACTIV1, FC_INHACTIV1_H_IN_ACTIV_MASK |
			FC_INHACTIV1_H_IN_ACTIV_12_MASK, (u8)(value >> 8));
}

static void fc_video_HBlank(hdmi_tx_dev_t *dev, u16 value)
{
	LOG_TRACE1(value);
	/* 10-bit width */
	dev_write(dev, (FC_INHBLANK0), (u8) (value));
	dev_write_mask(dev, FC_INHBLANK1, FC_INHBLANK1_H_IN_BLANK_MASK |
			FC_INHBLANK1_H_IN_BLANK_12_MASK, (u8)(value >> 8));
}

static void fc_video_VActive(hdmi_tx_dev_t *dev, u16 value)
{
	LOG_TRACE1(value);
	/* Vactive value holds 11-bit width in two register */
	dev_write(dev, (FC_INVACTIV0), (u8) (value));
	dev_write_mask(dev, FC_INVACTIV1, FC_INVACTIV1_V_IN_ACTIV_MASK |
			FC_INVACTIV1_V_IN_ACTIV_12_11_MASK, (u8)(value >> 8));
}

static void fc_video_VBlank(hdmi_tx_dev_t *dev, u16 value)
{
	LOG_TRACE1(value);
	/* 8-bit width */
	dev_write(dev, (FC_INVBLANK), (u8) (value));
}

/* Setting Frame Composer Input Video HSync Front Porch */
static void fc_video_HSyncEdgeDelay(hdmi_tx_dev_t *dev, u16 value)
{
	LOG_TRACE1(value);
	/* 11-bit width */
	dev_write(dev, (FC_HSYNCINDELAY0), (u8) (value));
	dev_write_mask(dev, FC_HSYNCINDELAY1, FC_HSYNCINDELAY1_H_IN_DELAY_MASK |
			FC_HSYNCINDELAY1_H_IN_DELAY_12_MASK, (u8)(value >> 8));
}

static void fc_video_HSyncPulseWidth(hdmi_tx_dev_t *dev, u16 value)
{
	LOG_TRACE1(value);
	/* 9-bit width */
	dev_write(dev, (FC_HSYNCINWIDTH0), (u8) (value));
	dev_write_mask(dev, FC_HSYNCINWIDTH1, FC_HSYNCINWIDTH1_H_IN_WIDTH_MASK,
							(u8)(value >> 8));
}

static void fc_video_VSyncEdgeDelay(hdmi_tx_dev_t *dev, u16 value)
{
	LOG_TRACE1(value);
	/* 8-bit width */
	dev_write(dev, (FC_VSYNCINDELAY), (u8) (value));
}

static void fc_video_VSyncPulseWidth(hdmi_tx_dev_t *dev, u16 value)
{
	LOG_TRACE1(value);
	dev_write_mask(dev, FC_VSYNCINWIDTH, FC_VSYNCINWIDTH_V_IN_WIDTH_MASK,
								(u8)(value));
}

/* static void fc_video_RefreshRate(hdmi_tx_dev_t *dev, u32 value)
{
	LOG_TRACE1(value);
	20-bit width
	dev_write(dev, (FC_INFREQ0), (u8) (value >> 0));
	dev_write(dev, (FC_INFREQ1), (u8) (value >> 8));
	dev_write_mask(dev, FC_INFREQ2, FC_INFREQ2_INFREQ_MASK,
							(u8)(value >> 16));
} */

static void fc_video_ControlPeriodMinDuration(hdmi_tx_dev_t *dev, u8 value)
{
	LOG_TRACE1(value);
	dev_write(dev, (FC_CTRLDUR), value);
}

static void fc_video_ExtendedControlPeriodMinDuration(hdmi_tx_dev_t *dev,
								u8 value)
{
	LOG_TRACE1(value);
	dev_write(dev, (FC_EXCTRLDUR), value);
}

static void fc_video_ExtendedControlPeriodMaxSpacing(hdmi_tx_dev_t *dev,
								u8 value)
{
	LOG_TRACE1(value);
	dev_write(dev, (FC_EXCTRLSPAC), value);
}

static void fc_video_PreambleFilter(hdmi_tx_dev_t *dev, u8 value,
							unsigned channel)
{
	LOG_TRACE1(value);
	if (channel == 0)
		dev_write(dev, (FC_CH0PREAM), value);
	else if (channel == 1)
		dev_write_mask(dev, FC_CH1PREAM,
				FC_CH1PREAM_CH1_PREAMBLE_FILTER_MASK, value);
	else if (channel == 2)
		dev_write_mask(dev, FC_CH2PREAM,
				FC_CH2PREAM_CH2_PREAMBLE_FILTER_MASK, value);
	else
		pr_err("invalid channel number: %d\n", channel);
}

static void fc_video_PixelRepetitionInput(hdmi_tx_dev_t *dev, u8 value)
{
	LOG_TRACE1(value);
	dev_write_mask(dev, FC_PRCONF,
			FC_PRCONF_INCOMING_PR_FACTOR_MASK, value);
}

void fc_force_audio(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, FC_DBGFORCE, FC_DBGFORCE_FORCEAUDIO_MASK, bit);
}

void fc_force_video(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);

	/* avoid glitches */
	if (bit != 0) {
		dev_write(dev, FC_DBGTMDS2, bit ? 0x00 : 0x00);	/* R */
		dev_write(dev, FC_DBGTMDS1, bit ? 0x00 : 0x00);	/* G */
		dev_write(dev, FC_DBGTMDS0, bit ? 0xFF : 0x00);	/* B */
		dev_write_mask(dev, FC_DBGFORCE,
				FC_DBGFORCE_FORCEVIDEO_MASK, 1);
	} else {
		dev_write_mask(dev, FC_DBGFORCE,
				FC_DBGFORCE_FORCEVIDEO_MASK, 0);
		dev_write(dev, FC_DBGTMDS2, bit ? 0x00 : 0x00);	/* R */
		dev_write(dev, FC_DBGTMDS1, bit ? 0x00 : 0x00);	/* G */
		dev_write(dev, FC_DBGTMDS0, bit ? 0xFF : 0x00);	/* B */
	}
}

void fc_force_output(hdmi_tx_dev_t *dev, int enable)
{
	LOG_TRACE1(enable);
	fc_force_audio(dev, 0);
	fc_force_video(dev, (u8)enable);
}


int fc_video_config(hdmi_tx_dev_t *dev, videoParams_t *video)
{
	const dtd_t *dtd = &video->mDtd;
	u16 i = 0;

	LOG_TRACE();

	if ((dev == NULL) || (video == NULL)) {
		pr_err("Error:Invalid arguments: dev=%lx; video=%lx\n",
					(uintptr_t)dev, (uintptr_t)video);
		return FALSE;
	}

	dtd = &video->mDtd;

	fc_video_VSyncPolarity(dev, dtd->mVSyncPolarity);
	fc_video_HSyncPolarity(dev, dtd->mHSyncPolarity);
	fc_video_DataEnablePolarity(dev,
			dev->snps_hdmi_ctrl.data_enable_polarity);
	fc_video_DviOrHdmi(dev, video->mHdmi);

	if (video->mHdmiVideoFormat == 2) {
		if (video->m3dStructure == 0) {
			/* 3d data frame packing
			is transmitted as a progressive format */
			fc_video_VBlankOsc(dev, 0);
			fc_video_Interlaced(dev, 0);

			if (dtd->mInterlaced) {
				fc_video_VActive(dev,
				(dtd->mVActive << 2) + 3 * dtd->mVBlanking + 2);
			} else {
				fc_video_VActive(dev,
				(dtd->mVActive << 1) + dtd->mVBlanking);
			}
		} else {
			fc_video_VBlankOsc(dev, dtd->mInterlaced);
			fc_video_Interlaced(dev, dtd->mInterlaced);
			fc_video_VActive(dev, dtd->mVActive);
		}
	} else {
		fc_video_VBlankOsc(dev, dtd->mInterlaced);
		fc_video_Interlaced(dev, dtd->mInterlaced);
		fc_video_VActive(dev, dtd->mVActive);
	}

	if (video->mEncodingOut == YCC420) {
		VIDEO_INF("Encoding configured to YCC 420\n");
		fc_video_HActive(dev, dtd->mHActive/2);
		fc_video_HBlank(dev, dtd->mHBlanking/2);
		fc_video_HSyncPulseWidth(dev, dtd->mHSyncPulseWidth/2);
		fc_video_HSyncEdgeDelay(dev, dtd->mHSyncOffset/2);
	} else {
		fc_video_HActive(dev, dtd->mHActive);
		fc_video_HBlank(dev, dtd->mHBlanking);
		fc_video_HSyncPulseWidth(dev, dtd->mHSyncPulseWidth);
		fc_video_HSyncEdgeDelay(dev, dtd->mHSyncOffset);
	}

	fc_video_VBlank(dev, dtd->mVBlanking);
	fc_video_VSyncEdgeDelay(dev, dtd->mVSyncOffset);
	fc_video_VSyncPulseWidth(dev, dtd->mVSyncPulseWidth);
	fc_video_ControlPeriodMinDuration(dev, 12);
	fc_video_ExtendedControlPeriodMinDuration(dev, 32);

	/* spacing < 256^2 * config / tmdsClock, spacing <= 50ms
	 * worst case: tmdsClock == 25MHz => config <= 19
	 */
	fc_video_ExtendedControlPeriodMaxSpacing(dev, 1);

	for (i = 0; i < 3; i++)
		fc_video_PreambleFilter(dev, (i + 1) * 11, i);

	fc_video_PixelRepetitionInput(dev, dtd->mPixelRepetitionInput + 1);

		/* due to HDCP and Scrambler */
	if (dev->snps_hdmi_ctrl.hdcp_on ||
		(dev->snps_hdmi_ctrl.tmds_clk > 340000))
		/* Start koopout window generation */
		fc_video_hdcp_keepout(dev, true);
	else
		fc_video_hdcp_keepout(dev, false);

	return TRUE;

}


u8 fc_video_tmdsMode_get(hdmi_tx_dev_t *dev)
{
	LOG_TRACE();
	/* 1: HDMI; 0: DVI */
	return dev_read_mask(dev, FC_INVIDCONF, FC_INVIDCONF_DVI_MODEZ_MASK);
}

