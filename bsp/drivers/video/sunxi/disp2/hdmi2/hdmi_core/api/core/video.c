/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */


#include "video.h"
#include "fc_video.h"
#include "packets.h"


int videoParams_GetCeaVicCode(int hdmi_vic_code)
{
	switch (hdmi_vic_code) {
	case 1:
		return 95;
		break;
	case 2:
		return 94;
		break;
	case 3:
		return 93;
		break;
	case 4:
		return 98;
		break;
	default:
		return -1;
		break;
	}
	return -1;
}

int videoParams_GetHdmiVicCode(int cea_code)
{
	switch (cea_code) {
	case 95:
		return 1;
		break;
	case 94:
		return 2;
		break;
	case 93:
		return 3;
		break;
	case 98:
		return 4;
		break;
	default:
		return -1;
		break;
	}
	return -1;
}

void video_params_reset(hdmi_tx_dev_t *dev, videoParams_t *params)
{
	dtd_t dtd;

	LOG_TRACE();
	dtd_fill(dev, &dtd, 1, 60000);

	params->mHdmi = DVI;
	params->mEncodingOut = RGB;
	params->mEncodingIn = RGB;
	params->mColorResolution = COLOR_DEPTH_8;
	params->mPixelRepetitionFactor = 0;
	params->mRgbQuantizationRange = 0;
	params->mPixelPackingDefaultPhase = 0;
	params->mColorimetry = 0;
	params->mScanInfo = 0;
	params->mActiveFormatAspectRatio = 8;
	params->mNonUniformScaling = 0;
	params->mExtColorimetry = ~0;
	params->mItContent = 0;
	params->mEndTopBar = ~0;
	params->mStartBottomBar = ~0;
	params->mEndLeftBar = ~0;
	params->mStartRightBar = ~0;
	params->mCscFilter = 0;
	params->mHdmiVideoFormat = 0;
	params->m3dStructure = 0;
	params->m3dExtData = 0;
	params->mHdmiVic = 0;
	params->mHdmi20 = 0;

	memcpy(&params->mDtd, &dtd, sizeof(dtd_t));

/* params->mDtd.mCode = 0; */
/* params->mDtd.mLimitedToYcc420 = 0xFF; */
/* params->mDtd.mYcc420 = 0xFF; */
/* params->mDtd.mPixelRepetitionInput = 0xFF; */
/* params->mDtd.mPixelClock = 0; */
/* params->mDtd.mInterlaced = 0xFF; */
/* params->mDtd.mHActive = 0; */
/* params->mDtd.mHBlanking = 0; */
/* params->mDtd.mHBorder = 0xFFFF; */
/* params->mDtd.mHImageSize = 0; */
/* params->mDtd.mHSyncOffset = 0; */
/* params->mDtd.mHSyncPulseWidth = 0; */
/* params->mDtd.mHSyncPolarity = 0xFF; */
/* params->mDtd.mVActive = 0; */
/* params->mDtd.mVBlanking = 0; */
/* params->mDtd.mVBorder = 0xFFFF; */
/* params->mDtd.mVImageSize = 0; */
/* params->mDtd.mVSyncOffset = 0; */
/* params->mDtd.mVSyncPulseWidth = 0; */
/* params->mDtd.mVSyncPolarity = 0xFF; */
}


/* [KHz] */
u32 videoParams_GetPixelClock(hdmi_tx_dev_t *dev, videoParams_t *params)
{
	if (params->mHdmiVideoFormat == 2) {
		if (params->m3dStructure == 0)
			return 2 * params->mDtd.mPixelClock;
	}
	return params->mDtd.mPixelClock;
}

/* 0.01 */
unsigned videoParams_GetRatioClock(hdmi_tx_dev_t *dev, videoParams_t *params)
{
	unsigned ratio = 100;

	if (params->mEncodingOut != YCC422) {
		if (params->mColorResolution == 8)
			ratio = 100;
		else if (params->mColorResolution == 10)
			ratio = 125;
		else if (params->mColorResolution == 12)
			ratio = 150;
		else if (params->mColorResolution == 16)
			ratio = 200;
	}
	return ratio * (params->mPixelRepetitionFactor + 1);
}

int videoParams_IsColorSpaceConversion(hdmi_tx_dev_t *dev,
						videoParams_t *params)
{
	return params->mEncodingIn != params->mEncodingOut;
}

int videoParams_IsColorSpaceDecimation(hdmi_tx_dev_t *dev,
						videoParams_t *params)
{
	return params->mEncodingOut == YCC422 && (params->mEncodingIn == RGB
			|| params->mEncodingIn ==
					YCC444);
}

int videoParams_IsColorSpaceInterpolation(hdmi_tx_dev_t *dev,
						videoParams_t *params)
{
	return params->mEncodingIn == YCC422 && (params->mEncodingOut == RGB
			|| params->mEncodingOut ==
					YCC444);
}

int videoParams_IsPixelRepetition(hdmi_tx_dev_t *dev, videoParams_t *params)
{
	return (params->mPixelRepetitionFactor > 0) ||
			(params->mDtd.mPixelRepetitionInput > 0);
}

void videoParams_UpdateCscCoefficients(hdmi_tx_dev_t *dev,
						videoParams_t *params)
{
	u16 i = 0;

	if (!videoParams_IsColorSpaceConversion(dev, params)) {
		for (i = 0; i < 4; i++) {
			params->mCscA[i] = 0;
			params->mCscB[i] = 0;
			params->mCscC[i] = 0;
		}
		params->mCscA[0] = 0x2000;
		params->mCscB[1] = 0x2000;
		params->mCscC[2] = 0x2000;
		params->mCscScale = 1;
	} else if (videoParams_IsColorSpaceConversion(dev, params) &&
		params->mEncodingOut == RGB) {
		if (params->mColorimetry == ITU601) {
			params->mCscA[0] = 0x2000;
			params->mCscA[1] = 0x6926;
			params->mCscA[2] = 0x74fd;
			params->mCscA[3] = 0x010e;

			params->mCscB[0] = 0x2000;
			params->mCscB[1] = 0x2cdd;
			params->mCscB[2] = 0x0000;
			params->mCscB[3] = 0x7e9a;

			params->mCscC[0] = 0x2000;
			params->mCscC[1] = 0x0000;
			params->mCscC[2] = 0x38b4;
			params->mCscC[3] = 0x7e3b;

			params->mCscScale = 1;
		} else if (params->mColorimetry == ITU709) {
			params->mCscA[0] = 0x2000;
			params->mCscA[1] = 0x7106;
			params->mCscA[2] = 0x7a02;
			params->mCscA[3] = 0x00a7;

			params->mCscB[0] = 0x2000;
			params->mCscB[1] = 0x3264;
			params->mCscB[2] = 0x0000;
			params->mCscB[3] = 0x7e6d;

			params->mCscC[0] = 0x2000;
			params->mCscC[1] = 0x0000;
			params->mCscC[2] = 0x3b61;
			params->mCscC[3] = 0x7e25;

			params->mCscScale = 1;
		}
	} else if (videoParams_IsColorSpaceConversion(dev, params) &&
		params->mEncodingIn == RGB) {
		if (params->mColorimetry == ITU601) {
			params->mCscA[0] = 0x2591;
			params->mCscA[1] = 0x1322;
			params->mCscA[2] = 0x074b;
			params->mCscA[3] = 0x0000;

			params->mCscB[0] = 0x6535;
			params->mCscB[1] = 0x2000;
			params->mCscB[2] = 0x7acc;
			params->mCscB[3] = 0x0200;

			params->mCscC[0] = 0x6acd;
			params->mCscC[1] = 0x7534;
			params->mCscC[2] = 0x2000;
			params->mCscC[3] = 0x0200;

			params->mCscScale = 0;
		} else if (params->mColorimetry == ITU709) {
			params->mCscA[0] = 0x2dc5;
			params->mCscA[1] = 0x0d9b;
			params->mCscA[2] = 0x049e;
			params->mCscA[3] = 0x0000;

			params->mCscB[0] = 0x62f0;
			params->mCscB[1] = 0x2000;
			params->mCscB[2] = 0x7d11;
			params->mCscB[3] = 0x0200;

			params->mCscC[0] = 0x6756;
			params->mCscC[1] = 0x78ab;
			params->mCscC[2] = 0x2000;
			params->mCscC[3] = 0x0200;

			params->mCscScale = 0;
		}
	}
	/* else use user coefficients */
}

char *getEncodingString(encoding_t encoding)
{
	switch (encoding) {
	case	RGB:
		return "RGB";
	case	YCC444:
		return "YCbCr-444";
	case	YCC422:
		return "YCbCr-422";
	case	YCC420:
		return "YCbCr-420";
	default:
		break;
	}
	return "Undefined";
}

void halVideoSampler_InternalDataEnableGenerator(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, TX_INVID0,
			TX_INVID0_INTERNAL_DE_GENERATOR_MASK, (bit ? 1 : 0));
}

void halVideoSampler_VideoMapping(hdmi_tx_dev_t *dev, u8 value)
{
	LOG_TRACE1(value);
	dev_write_mask(dev, TX_INVID0, TX_INVID0_VIDEO_MAPPING_MASK, value);
}

void halVideoSampler_StuffingGy(hdmi_tx_dev_t *dev, u16 value)
{
	LOG_TRACE1(value);
	dev_write(dev, (TX_GYDATA0), (u8) (value >> 0));
	dev_write(dev, (TX_GYDATA1), (u8) (value >> 8));
	dev_write_mask(dev, TX_INSTUFFING,
			TX_INSTUFFING_GYDATA_STUFFING_MASK, 1);
}

void halVideoSampler_StuffingRcr(hdmi_tx_dev_t *dev, u16 value)
{
	LOG_TRACE1(value);
	dev_write(dev, (TX_RCRDATA0), (u8) (value >> 0));
	dev_write(dev, (TX_RCRDATA1), (u8) (value >> 8));
	dev_write_mask(dev, TX_INSTUFFING,
			TX_INSTUFFING_RCRDATA_STUFFING_MASK, 1);
}

void halVideoSampler_StuffingBcb(hdmi_tx_dev_t *dev, u16 value)
{
	LOG_TRACE1(value);
	dev_write(dev, (TX_BCBDATA0), (u8) (value >> 0));
	dev_write(dev, (TX_BCBDATA1), (u8) (value >> 8));
	dev_write_mask(dev, TX_INSTUFFING,
			TX_INSTUFFING_BCBDATA_STUFFING_MASK, 1);
}

void video_sampler_config(hdmi_tx_dev_t *dev, u8 map_code)
{
	halVideoSampler_InternalDataEnableGenerator(dev, 0);
	halVideoSampler_VideoMapping(dev, map_code);
	halVideoSampler_StuffingGy(dev, 0);
	halVideoSampler_StuffingRcr(dev, 0);
	halVideoSampler_StuffingBcb(dev, 0);
}

u8 vp_PixelPackingPhase(hdmi_tx_dev_t *dev)
{
	LOG_TRACE();
	return (u8)(dev_read(dev, VP_STATUS) & 0xF);
}

void vp_ColorDepth(hdmi_tx_dev_t *dev, u8 value)
{
	LOG_TRACE1(value);
	/* color depth */
	dev_write_mask(dev, VP_PR_CD, VP_PR_CD_COLOR_DEPTH_MASK, value);
}

void vp_PixelPackingDefaultPhase(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, VP_STUFF, VP_STUFF_IDEFAULT_PHASE_MASK, bit);
}

/* be used if the user wants to do pixel repetition using H13TCTRL controller */
void vp_PixelRepetitionFactor(hdmi_tx_dev_t *dev, u8 value)
{
	LOG_TRACE1(value);
	/* desired factor */
	dev_write_mask(dev, VP_PR_CD, VP_PR_CD_DESIRED_PR_FACTOR_MASK, value);
	/* enable stuffing */
	dev_write_mask(dev, VP_STUFF, VP_STUFF_PR_STUFFING_MASK, 1);
	/* enable block */
	dev_write_mask(dev, VP_CONF, VP_CONF_PR_EN_MASK, (value > 1) ? 1 : 0);
	/* bypass block */
	dev_write_mask(dev, VP_CONF, VP_CONF_BYPASS_SELECT_MASK,
						(value > 1) ? 0 : 1);
}

void vp_Ycc422RemapSize(hdmi_tx_dev_t *dev, u8 value)
{
	LOG_TRACE1(value);
	dev_write_mask(dev, VP_REMAP, VP_REMAP_YCC422_SIZE_MASK, value);
}

void vp_OutputSelector(hdmi_tx_dev_t *dev, u8 value)
{
	LOG_TRACE1(value);
	if (value == 0) {	/* pixel packing */
		dev_write_mask(dev, VP_CONF, VP_CONF_BYPASS_EN_MASK, 0);
		/* enable pixel packing */
		dev_write_mask(dev, VP_CONF, VP_CONF_PP_EN_MASK, 1);
		dev_write_mask(dev, VP_CONF, VP_CONF_YCC422_EN_MASK, 0);
	} else if (value == 1) {	/* YCC422 */
		dev_write_mask(dev, VP_CONF, VP_CONF_BYPASS_EN_MASK, 0);
		dev_write_mask(dev, VP_CONF, VP_CONF_PP_EN_MASK, 0);
		/* enable YCC422 */
		dev_write_mask(dev, VP_CONF, VP_CONF_YCC422_EN_MASK, 1);
	} else if (value == 2 || value == 3) {	/* bypass */
		/* enable bypass */
		dev_write_mask(dev, VP_CONF, VP_CONF_BYPASS_EN_MASK, 1);
		dev_write_mask(dev, VP_CONF, VP_CONF_PP_EN_MASK, 0);
		dev_write_mask(dev, VP_CONF, VP_CONF_YCC422_EN_MASK, 0);
	} else {
		pr_err("Error:wrong output option: %d\n", value);
		return;
	}

	/* YCC422 stuffing */
	dev_write_mask(dev, VP_STUFF, VP_STUFF_YCC422_STUFFING_MASK, 1);
	/* pixel packing stuffing */
	dev_write_mask(dev, VP_STUFF, VP_STUFF_PP_STUFFING_MASK, 1);

	/* output selector */
	dev_write_mask(dev, VP_CONF, VP_CONF_OUTPUT_SELECTOR_MASK, value);
}


void csc_Interpolation(hdmi_tx_dev_t *dev, u8 value)
{
	LOG_TRACE1(value);
	/* 2-bit width */
	dev_write_mask(dev, CSC_CFG, CSC_CFG_INTMODE_MASK, value);
}

void csc_Decimation(hdmi_tx_dev_t *dev, u8 value)
{
	LOG_TRACE1(value);
	/* 2-bit width */
	dev_write_mask(dev, CSC_CFG, CSC_CFG_DECMODE_MASK, value);
}

void csc_ColorDepth(hdmi_tx_dev_t *dev, u8 value)
{
	LOG_TRACE1(value);
	/* 4-bit width */
	dev_write_mask(dev, CSC_SCALE, CSC_SCALE_CSC_COLOR_DEPTH_MASK, value);
}

void csc_ScaleFactor(hdmi_tx_dev_t *dev, u8 value)
{
	LOG_TRACE1(value);
	/* 2-bit width */
	dev_write_mask(dev, CSC_SCALE, CSC_SCALE_CSCSCALE_MASK, value);
}

void csc_CoefficientA1(hdmi_tx_dev_t *dev, u16 value)
{
	LOG_TRACE1(value);
	/* 15-bit width */
	dev_write(dev, CSC_COEF_A1_LSB, (u8)(value));
	dev_write_mask(dev, CSC_COEF_A1_MSB,
			CSC_COEF_A1_MSB_CSC_COEF_A1_MSB_MASK, (u8)(value >> 8));
}

void csc_CoefficientA2(hdmi_tx_dev_t *dev, u16 value)
{
	LOG_TRACE1(value);
	/* 15-bit width */
	dev_write(dev, CSC_COEF_A2_LSB, (u8)(value));
	dev_write_mask(dev, CSC_COEF_A2_MSB,
			CSC_COEF_A2_MSB_CSC_COEF_A2_MSB_MASK, (u8)(value >> 8));
}

void csc_CoefficientA3(hdmi_tx_dev_t *dev, u16 value)
{
	LOG_TRACE1(value);
	/* 15-bit width */
	dev_write(dev, CSC_COEF_A3_LSB, (u8)(value));
	dev_write_mask(dev, CSC_COEF_A3_MSB,
		CSC_COEF_A3_MSB_CSC_COEF_A3_MSB_MASK, (u8)(value >> 8));
}

void csc_CoefficientA4(hdmi_tx_dev_t *dev, u16 value)
{
	LOG_TRACE1(value);
	/* 15-bit width */
	dev_write(dev, CSC_COEF_A4_LSB, (u8)(value));
	dev_write_mask(dev, CSC_COEF_A4_MSB,
		CSC_COEF_A4_MSB_CSC_COEF_A4_MSB_MASK, (u8)(value >> 8));
}

void csc_CoefficientB1(hdmi_tx_dev_t *dev, u16 value)
{
	LOG_TRACE1(value);
	/* 15-bit width */
	dev_write(dev, CSC_COEF_B1_LSB, (u8)(value));
	dev_write_mask(dev, CSC_COEF_B1_MSB,
		CSC_COEF_B1_MSB_CSC_COEF_B1_MSB_MASK, (u8)(value >> 8));
}

void csc_CoefficientB2(hdmi_tx_dev_t *dev, u16 value)
{
	LOG_TRACE1(value);
	/* 15-bit width */
	dev_write(dev, CSC_COEF_B2_LSB, (u8)(value));
	dev_write_mask(dev, CSC_COEF_B2_MSB,
			CSC_COEF_B2_MSB_CSC_COEF_B2_MSB_MASK, (u8)(value >> 8));
}

void csc_CoefficientB3(hdmi_tx_dev_t *dev, u16 value)
{
	LOG_TRACE1(value);
	/* 15-bit width */
	dev_write(dev, CSC_COEF_B3_LSB, (u8)(value));
	dev_write_mask(dev, CSC_COEF_B3_MSB,
		CSC_COEF_B3_MSB_CSC_COEF_B3_MSB_MASK, (u8)(value >> 8));
}

void csc_CoefficientB4(hdmi_tx_dev_t *dev, u16 value)
{
	LOG_TRACE1(value);
	/* 15-bit width */
	dev_write(dev, CSC_COEF_B4_LSB, (u8)(value));
	dev_write_mask(dev, CSC_COEF_B4_MSB,
		CSC_COEF_B4_MSB_CSC_COEF_B4_MSB_MASK, (u8)(value >> 8));
}

void csc_CoefficientC1(hdmi_tx_dev_t *dev, u16 value)
{
	LOG_TRACE1(value);
	/* 15-bit width */
	dev_write(dev, CSC_COEF_C1_LSB, (u8) (value));
	dev_write_mask(dev, CSC_COEF_C1_MSB,
		CSC_COEF_C1_MSB_CSC_COEF_C1_MSB_MASK, (u8)(value >> 8));
}

void csc_CoefficientC2(hdmi_tx_dev_t *dev, u16 value)
{
	LOG_TRACE1(value);
	/* 15-bit width */
	dev_write(dev, CSC_COEF_C2_LSB, (u8) (value));
	dev_write_mask(dev, CSC_COEF_C2_MSB,
			CSC_COEF_C2_MSB_CSC_COEF_C2_MSB_MASK, (u8)(value >> 8));
}

void csc_CoefficientC3(hdmi_tx_dev_t *dev, u16 value)
{
	LOG_TRACE1(value);
	/* 15-bit width */
	dev_write(dev, CSC_COEF_C3_LSB, (u8) (value));
	dev_write_mask(dev, CSC_COEF_C3_MSB,
		CSC_COEF_C3_MSB_CSC_COEF_C3_MSB_MASK, (u8)(value >> 8));
}

void csc_CoefficientC4(hdmi_tx_dev_t *dev, u16 value)
{
	LOG_TRACE1(value);
	dev_write(dev, CSC_COEF_C4_LSB, (u8) (value));
	dev_write_mask(dev, CSC_COEF_C4_MSB,
			CSC_COEF_C4_MSB_CSC_COEF_C4_MSB_MASK, (u8)(value >> 8));
}

void csc_config(hdmi_tx_dev_t *dev, videoParams_t *video,
	unsigned interpolation, unsigned decimation, unsigned color_depth)
{
	csc_Interpolation(dev, interpolation);
	csc_Decimation(dev, decimation);
	csc_CoefficientA1(dev, video->mCscA[0]);
	csc_CoefficientA2(dev, video->mCscA[1]);
	csc_CoefficientA3(dev, video->mCscA[2]);
	csc_CoefficientA4(dev, video->mCscA[3]);
	csc_CoefficientB1(dev, video->mCscB[0]);
	csc_CoefficientB2(dev, video->mCscB[1]);
	csc_CoefficientB3(dev, video->mCscB[2]);
	csc_CoefficientB4(dev, video->mCscB[3]);
	csc_CoefficientC1(dev, video->mCscC[0]);
	csc_CoefficientC2(dev, video->mCscC[1]);
	csc_CoefficientC3(dev, video->mCscC[2]);
	csc_CoefficientC4(dev, video->mCscC[3]);
	csc_ScaleFactor(dev, video->mCscScale);
	csc_ColorDepth(dev, color_depth);
}

int video_Configure(hdmi_tx_dev_t *dev, videoParams_t *video)
{
	LOG_TRACE();

	/* DVI mode does not support pixel repetition */
	if ((video->mHdmi == DVI) &&
		videoParams_IsPixelRepetition(dev, video)) {
		pr_err("Error:DVI mode with pixel repetition:video not transmitted\n");
		return FALSE;
	}

	fc_force_output(dev, 1);

	if (fc_video_config(dev, video) == FALSE)
		return FALSE;
	if (video_VideoPacketizer(dev, video) == FALSE)
		return FALSE;
	if (video_ColorSpaceConverter(dev, video) == FALSE)
		return FALSE;
	if (video_VideoSampler(dev, video) == FALSE)
		return FALSE;

	return TRUE;
}

int video_ColorSpaceConverter(hdmi_tx_dev_t *dev, videoParams_t *video)
{
	unsigned interpolation = 0;
	unsigned decimation = 0;
	unsigned color_depth = 0;

	LOG_TRACE();

	if (videoParams_IsColorSpaceInterpolation(dev, video)) {
		if (video->mCscFilter > 1) {
			pr_err("Error:invalid chroma interpolation filter:%d\n", video->mCscFilter);
			return FALSE;
		}
		interpolation = 1 + video->mCscFilter;
	} else if (videoParams_IsColorSpaceDecimation(dev, video)) {
		if (video->mCscFilter > 2) {
			pr_err("Error:invalid chroma decimation filter:%d\n", video->mCscFilter);
			return FALSE;
		}
		decimation = 1 + video->mCscFilter;
	}

	if ((video->mColorResolution == COLOR_DEPTH_8) ||
		(video->mColorResolution == 0))
		color_depth = 4;
	else if (video->mColorResolution == COLOR_DEPTH_10)
		color_depth = 5;
	else if (video->mColorResolution == COLOR_DEPTH_12)
		color_depth = 6;
	else if (video->mColorResolution == COLOR_DEPTH_16)
		color_depth = 7;
	else {
		pr_err("Error:invalid color depth: %d\n",
					video->mColorResolution);
		return FALSE;
	}

	VIDEO_INF("interpolation:%d  decimation:%d  color_depth:%d\n",
					interpolation, decimation, color_depth);
	csc_config(dev, video, interpolation, decimation, color_depth);

	return TRUE;
}

int video_VideoPacketizer(hdmi_tx_dev_t *dev, videoParams_t *video)
{
	unsigned color_depth = 0;
	unsigned remap_size = 0;
	unsigned output_select = 0;

	LOG_TRACE();
	if ((video->mEncodingOut == RGB) || (video->mEncodingOut == YCC444) ||
		(video->mEncodingOut == YCC420)) {
		if (video->mColorResolution == 0)
			output_select = 3;
		else if (video->mColorResolution == COLOR_DEPTH_8) {
			color_depth = 0;
			output_select = 3;
		} else if (video->mColorResolution == COLOR_DEPTH_10)
			color_depth = 5;
		else if (video->mColorResolution == COLOR_DEPTH_12)
			color_depth = 6;
		else if (video->mColorResolution == COLOR_DEPTH_16)
			color_depth = 7;
		else {
			pr_err("Error:invalid color depth: %d\n",
						video->mColorResolution);
			return FALSE;
		}
	} else if (video->mEncodingOut == YCC422) {
		if ((video->mColorResolution == COLOR_DEPTH_8) ||
			(video->mColorResolution == 0))
			remap_size = 0;
		else if (video->mColorResolution == COLOR_DEPTH_10)
			remap_size = 1;
		else if (video->mColorResolution == COLOR_DEPTH_12)
			remap_size = 2;
		else {
			pr_err("Error:invalid color remap size: %d\n",
						video->mColorResolution);
			return FALSE;
		}
		output_select = 1;
	} else {
		pr_err("Error:invalid output encoding type: %d\n",
							video->mEncodingOut);
		return FALSE;
	}

	vp_PixelRepetitionFactor(dev, video->mPixelRepetitionFactor);
	vp_ColorDepth(dev, color_depth);
	vp_PixelPackingDefaultPhase(dev, video->mPixelPackingDefaultPhase);
	vp_Ycc422RemapSize(dev, remap_size);
	vp_OutputSelector(dev, output_select);
	return TRUE;
}

int video_VideoSampler(hdmi_tx_dev_t *dev, videoParams_t *video)
{
	unsigned map_code = 0;

	LOG_TRACE();

	if (video->mEncodingIn == RGB) {
		if ((video->mColorResolution == COLOR_DEPTH_8) ||
			(video->mColorResolution == 0))
			map_code = 1;
		else if (video->mColorResolution == COLOR_DEPTH_10)
			map_code = 3;
		else if (video->mColorResolution == COLOR_DEPTH_12)
			map_code = 5;
		else if (video->mColorResolution == COLOR_DEPTH_16)
			map_code = 7;
		else {
			pr_err("invalid color depth: %d\n",
						video->mColorResolution);
			return FALSE;
		}
	} else if (video->mEncodingIn == YCC422) {
		/* YCC422 mapping is discontinued - only map 1 is supported */
		if (video->mColorResolution == COLOR_DEPTH_8)
			map_code = 22;
		else if (video->mColorResolution == COLOR_DEPTH_10)
			map_code = 20;
		else if ((video->mColorResolution == COLOR_DEPTH_12) ||
			(video->mColorResolution == 0))
			map_code = 18;
		else {
			pr_err("invalid color remap size: %d\n",
						video->mColorResolution);
			return FALSE;
		}
	} else if (video->mEncodingIn == YCC444) {
		if (video->mColorResolution == COLOR_DEPTH_8)
			map_code = 23;
		else if (video->mColorResolution == COLOR_DEPTH_10)
			map_code = 24;
		else if ((video->mColorResolution == COLOR_DEPTH_12) ||
			(video->mColorResolution == 0))
			map_code = 27;
		else {
			pr_err("invalid color remap size: %d\n",
						video->mColorResolution);
			return FALSE;
		}
	} else if (video->mEncodingIn == YCC420) {
		if (video->mColorResolution == COLOR_DEPTH_8)
			map_code = 9;
		else if (video->mColorResolution == COLOR_DEPTH_10)
			map_code = 11;
		else if ((video->mColorResolution == COLOR_DEPTH_12) ||
			(video->mColorResolution == 0))
			map_code = 13;
		else {
			pr_err("invalid color remap size: %d\n",
						video->mColorResolution);
			return FALSE;
		}
	} else {
		pr_err("invalid input encoding type: %d\n",
							video->mEncodingIn);
		return FALSE;
	}

	video_sampler_config(dev, map_code);

	return TRUE;
}

u8 vp_PixelRepetitionFactor_get(hdmi_tx_dev_t *dev)
{

	LOG_TRACE();
	return dev_read_mask(dev, VP_PR_CD, VP_PR_CD_DESIRED_PR_FACTOR_MASK);
}

u8 vp_ColorDepth_get(hdmi_tx_dev_t *dev)
{
	u8 depth = 0;

	LOG_TRACE();

	switch (dev_read_mask(dev, VP_PR_CD, VP_PR_CD_COLOR_DEPTH_MASK)) {
	case 0x0:
	case 0x4:
		depth = 8;
		break;
	case 0x5:
		depth = 10;
		break;
	case 0x6:
		depth = 12;
		break;
	case 0x7:
		depth = 16;
		break;
	default:
		depth = 8;
		break;
	}
	return depth;

}

