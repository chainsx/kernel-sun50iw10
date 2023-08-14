/*
 * Copyright (c) 2007-2019 Allwinnertech Co., Ltd.
 * Author: zhengwanyu <zhengwanyu@allwinnertech.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include "sunxi_hdmi20.h"

static struct drv_model_info *hdmi20_drv;
static struct sunxi_hdmi *hwhdmi20;
static struct device_access reg_access;
struct system_functions low_functions;
u32 hdmi_printf;

#define DDC_PIN_STATE_ACTIVE "ddc_active"
#define DDC_PIN_STATE_SLEEP "ddc_sleep"
#define CEC_PIN_STATE_ACTIVE "cec_active"
#define CEC_PIN_STATE_SLEEP "cec_sleep"

#define ESM_REG_BASE_OFFSET 0x8000
#define HDCP22_FIRMWARE_SIZE	(1024 * 256)
#define HDCP22_DATA_SIZE	(1024 * 128)

static struct sunxi_hdmi *sunxi_hdmi20_get_hdmi(void)
{
	if (hwhdmi20)
		return hwhdmi20;
	return NULL;
}

struct sunxi_hdmi_funcs *sunxi_hdmi_get_funcs(void)
{
	struct sunxi_hdmi *hdmi = sunxi_hdmi20_get_hdmi();

	return (struct sunxi_hdmi_funcs *)hdmi->funcs;
}

void register_func_to_drm(struct hdmi_dev_func func)
{
	struct sunxi_hdmi *hdmi = sunxi_hdmi20_get_hdmi();

	memcpy(&hdmi->dev_funcs, &func,
			sizeof(func));
}

void hdmi20_write(uintptr_t addr, u32 data)
{
	asm volatile("dsb st");
	*((volatile u8 *)(hwhdmi20->reg_base + (addr >> 2))) = data;
}

u32 hdmi20_read(uintptr_t addr)
{
	return *((volatile u8 *)(hwhdmi20->reg_base + (addr >> 2)));
}

static int
sunxi_hdmi20_video_dts_parse(struct platform_device *pdev,
		struct sunxi_hdmi *hdmi)
{
	struct sunxi_hdmi_work_mode *params = &hdmi->init_params;

	if (of_property_read_u32(pdev->dev.of_node,
			"dvi_hdmi", &params->hdmi_mode)) {
		HDMI_ERR("can NOT get dvi_hdmi\n");
		return -1;
	}

	if (of_property_read_u32(pdev->dev.of_node,
			"color_format", &params->color_fmt)) {
		HDMI_ERR("can NOT get color_format\n");
		return -1;
	}

	if (of_property_read_u32(pdev->dev.of_node,
			"color_depth", &params->color_depth)) {
		HDMI_ERR("can NOT get color_depth\n");
		return -1;
	}

	if (of_property_read_u32(pdev->dev.of_node,
			"color_space", &params->color_space)) {
		HDMI_ERR("can NOT get color_space\n");
		return -1;
	}

	if (of_property_read_u32(pdev->dev.of_node,
			"color_range", &params->color_range)) {
		HDMI_ERR("can NOT get color_range\n");
		return -1;
	}

	if (of_property_read_u32(pdev->dev.of_node,
			"eotf", &params->eotf)) {
		HDMI_ERR("can NOT get color_eotf\n");
		return -1;
	}

	if (of_property_read_u32(pdev->dev.of_node,
			"aspect_ratio", &params->aspect_ratio)) {
		HDMI_ERR("can NOT get aspect_ratio\n");
		return -1;
	}

	return 0;
}

static int
sunxi_hdmi20_hdcp_dts_parse(struct platform_device *pdev,
	struct sunxi_hdmi *hdmi)
{
	struct device_node *esm_np;
	u32 dts_esm_buff_phy_addr = 0, dts_esm_size_phy_addr = 0;
	void *dts_esm_buff_vir_addr = NULL, *dts_esm_size_vir_addr = NULL;
	u8 *esm_firm_vir_addr = NULL, *esm_data_vir_addr = NULL;

	hdcpParams_t *hdcp = &hdmi->hdcp;

/* Parse if we use hdcp functions in hdmi driver */
	if (of_property_read_u32_array(pdev->dev.of_node,
			"hdmi_hdcp_enable", (u32 *)&hdcp->use_hdcp, 1)) {
		HDMI_INFO("WARN:can NOT get hdmi_hdcp_enable\n");
		return 0;
	}

	if (hdcp->use_hdcp) {
		DRM_INFO("NOT use hdcp\n");
		return 0;
	}

	if (of_property_read_u32_array(pdev->dev.of_node,
			"hdmi_hdcp22_enable", (u32 *)&hdcp->use_hdcp22, 1)) {
		HDMI_INFO("WARN:can NOT get hdmi_hdcp22_enable\n");
		return 0;
	}

/* parse hdcp2.2 */
	if (!hdcp->use_hdcp22) {
		HDMI_INFO("NOT use hdcp2.2\n");
		return 0;
	}

/* allocate dma memory for esm as firmware storage ram and ram for running programs */
	esm_firm_vir_addr = dma_alloc_coherent(&pdev->dev,
		HDCP22_FIRMWARE_SIZE, &hdcp->esm_firm_phy_addr,
		GFP_KERNEL | __GFP_ZERO);
	if (!esm_firm_vir_addr) {
		HDMI_ERR("dma_alloc_coherent for esm firmware failed\n");
		goto failed;
	}

	hdcp->esm_firm_vir_addr = (unsigned long)esm_firm_vir_addr;
	hdcp->esm_firm_size = HDCP22_FIRMWARE_SIZE;

	esm_data_vir_addr = dma_alloc_coherent(&pdev->dev,
		HDCP22_FIRMWARE_SIZE, &hdcp->esm_data_phy_addr,
		GFP_KERNEL | __GFP_ZERO);
	if (!esm_firm_vir_addr) {
		HDMI_ERR("dma_alloc_coherent for esm firmware failed\n");
		goto failed;
	}
	hdcp->esm_data_vir_addr = (unsigned long)esm_data_vir_addr;
	hdcp->esm_data_size = HDCP22_DATA_SIZE;

/* parse esm */
	esm_np = of_find_node_by_name(NULL, "esm");
	if (!esm_np) {
		HDMI_ERR("can NOT get esm device node\n");
		goto failed;
	}

/* obtain esm firmware size */
	/* get physical address of esm firmware size */
	if (of_property_read_u32_array(esm_np,
				"esm_img_size_addr",
				&dts_esm_size_phy_addr,
				1) && !dts_esm_size_phy_addr) {
		HDMI_ERR("read esm_img_size_addr form esm node failed\n");
		goto failed;
	}
	/* get esm firmware size */
	dts_esm_size_vir_addr = __va(dts_esm_size_phy_addr);
	memcpy((void *)(&hdcp->esm_firm_size), dts_esm_size_vir_addr, 4);

/* obtain esm firmware */
	/* get physical address of esm firmware */
	if (of_property_read_u32_array(esm_np,
				"esm_img_buff_addr",
				&dts_esm_buff_phy_addr,
				1) && !dts_esm_buff_phy_addr) {
		HDMI_ERR("read esm_img_buff_addr form esm node failed\n");
		goto failed;
	}
	/* get esm firmware */
	dts_esm_buff_vir_addr = __va(dts_esm_buff_phy_addr);
	if (hdcp->esm_firm_size <= HDCP22_FIRMWARE_SIZE) {
		memcpy(esm_firm_vir_addr, dts_esm_buff_vir_addr,
						hdcp->esm_firm_size);
		return 0;
	}
	HDMI_ERR("get esm firmware failed\n");

failed:
	if (esm_firm_vir_addr)
		dma_free_coherent(&pdev->dev, HDCP22_FIRMWARE_SIZE,
		esm_firm_vir_addr, hdcp->esm_firm_phy_addr);
	if (esm_data_vir_addr)
		dma_free_coherent(&pdev->dev, HDCP22_DATA_SIZE,
		esm_data_vir_addr, hdcp->esm_data_phy_addr);
	return -1;
}



static int
sunxi_hdmi20_cec_dts_parse(struct platform_device *pdev,
	struct sunxi_hdmi *hdmi)
{
	struct cec_params *cec = &hdmi->cec;

	/* get cec config */
	if (of_property_read_u32(pdev->dev.of_node,
						   "hdmi_cec_support",
						   &cec->support)) {
		HDMI_INFO("WARN:can NOT get hdmi_cec_support\n");
		return 0;
	}

	if (!cec->support) {
		DRM_INFO("WARN:NOT support cec\n");
		return 0;
	}

	if (of_property_read_u32(pdev->dev.of_node,
				"hdmi_cec_super_standby",
				&cec->support_super_standby)) {
		HDMI_INFO("WARN:can NOT get hdmi_cec_super_standby\n");
		return 0;
	}

	return 0;
}

static int sunxi_hdmi20_hdcp_init(void)
{
	struct sunxi_hdmi *hdmi = sunxi_hdmi20_get_hdmi();
	hdcpParams_t *hdcp = &hdmi->hdcp;

	if (!hdcp->use_hdcp)
		return 0;

	if (!hdcp->use_hdcp22)
		return 0;

	hdcp->hdcp_on = 0;
	hdcp->mEnable11Feature = -1;
	hdcp->mRiCheck = -1;
	hdcp->mI2cFastMode = -1;
	hdcp->mEnhancedLinkVerification = -1;
	hdcp->maxDevices = 0;
	hdcp->mKsvListBuffer = NULL;
	hdcp->mAksv = NULL;
	hdcp->mKeys = NULL;
	hdcp->mSwEncKey = NULL;

	hdcp->esm_hpi_base = (unsigned long)hdmi->reg_base
		+ ESM_REG_BASE_OFFSET;
	return 0;
}

static int sunxi_hdmi20_audio_init(void)
{
	struct sunxi_hdmi *hdmi = sunxi_hdmi20_get_hdmi();
	audioParams_t *audio = &hdmi->audio;

	audio->mInterfaceType = I2S;
	audio->mCodingType = PCM;
	audio->mSamplingFrequency = 44100;
	audio->mChannelAllocation = 0;
	audio->mChannelNum = 2;
	audio->mSampleSize = 16;
	audio->mClockFsFactor = 64;
	audio->mPacketType = PACKET_NOT_DEFINED;
	audio->mDmaBeatIncrement = DMA_NOT_DEFINED;

	return 0;
}

static bool hdmi20_clk_enable;
int sunxi_hdmi20_clk_enable(struct sunxi_hdmi *hdmi)
{
	int ret;
	unsigned long tcon_tv_rate;
	struct clk *tcon_tv_clk;

	tcon_tv_clk = clk_get(NULL, "tcon_tv");
	tcon_tv_rate = clk_get_rate(tcon_tv_clk);
	clk_set_rate(hdmi->mclk, tcon_tv_rate);

	if (hdmi20_clk_enable)
		return 0;
	hdmi20_clk_enable = true;

	ret = clk_prepare_enable(hdmi->mclk);
	if (ret < 0) {
		HDMI_ERR("fail to enable hdmi mclk\n");
		return -1;
	}

	ret = clk_prepare_enable(hdmi->ddc_clk);
	if (ret < 0) {
		HDMI_ERR("fail to enable hdmi ddc clk\n");
		return -1;
	}

	if (hdmi->hdcp.use_hdcp) {
		ret = clk_prepare_enable(hdmi->hdcp_clk);
		if (ret < 0) {
			HDMI_ERR("fail to enable hdmi hdcp clk\n");
			return -1;
		}
	}

	if (hdmi->cec.support) {
		ret = clk_prepare_enable(hdmi->cec_clk);
		if (ret < 0) {
			HDMI_ERR("fail to enable hdmi cec clk\n");
			return -1;
		}
	}

	return 0;
}

static int sunxi_hdmi20_clk_disable(struct sunxi_hdmi *hdmi)
{
	if (!hdmi20_clk_enable)
		return 0;
	hdmi20_clk_enable = false;

	if (__clk_get_enable_count(hdmi->mclk))
		clk_disable_unprepare(hdmi->mclk);

	if (__clk_get_enable_count(hdmi->ddc_clk))
		clk_disable_unprepare(hdmi->ddc_clk);

	if (__clk_get_enable_count(hdmi->hdcp_clk))
		clk_disable_unprepare(hdmi->hdcp_clk);

	if (__clk_get_enable_count(hdmi->cec_clk))
		clk_disable_unprepare(hdmi->cec_clk);

	return 0;
}

static void sunxi_hdmi20_get_dtd(dtd_t *dtd,
				struct disp_video_timings *timing)
{
	dtd->mPixelRepetitionInput = timing->pixel_repeat;
	dtd->mInterlaced = timing->b_interlace;

	dtd->mCode = timing->vic;
	dtd->mPixelClock = timing->pixel_clk * (timing->pixel_repeat + 1)
		* (timing->b_interlace + 1) / 1000;

	dtd->mHActive = timing->x_res * (timing->pixel_repeat + 1);
	dtd->mHBlanking = (timing->hor_total_time- timing->x_res) * (timing->pixel_repeat + 1);
	dtd->mHSyncOffset = timing->hor_front_porch * (timing->pixel_repeat + 1);
	dtd->mHSyncPulseWidth = timing->hor_sync_time * (timing->pixel_repeat + 1);

	dtd->mVActive = timing->y_res / (timing->b_interlace + 1);
	dtd->mVBlanking = timing->ver_total_time- timing->y_res;
	dtd->mVSyncOffset = timing->ver_front_porch;
	dtd->mVSyncPulseWidth = timing->ver_sync_time;

	dtd->mHSyncPolarity = timing->hor_sync_polarity;
	dtd->mVSyncPolarity = timing->ver_sync_polarity;

	DRM_INFO("dtd: vsync:%d hsync:%d       timing: vysnc:%d hsync:%d\n",
		dtd->mVSyncPolarity, dtd->mHSyncPolarity, timing->ver_sync_polarity, timing->hor_sync_polarity);
}

static int sunxi_hdmi20_fill_params(struct sunxi_hdmi *hdmi,
		struct disp_video_timings *timing)
{
	videoParams_t *video = &hdmi->video;
	productParams_t *product = &hdmi->product;
	dtd_t *dtd = &video->mDtd;
	sink_edid_t *sink = &hdmi->sink_caps;

/* get dertailed timings from vic */
	sunxi_hdmi20_get_dtd(dtd, timing);

	video->mCea_code = 0;
	video->mHdmi_code = 0;

	/* hdmi vic or cea vic for 4k */
	if ((timing->x_res == 3840 && timing->y_res == 2160)
		|| (timing->x_res == 4096 && timing->y_res == 2160)) {
		if ((timing->vic <= 4) && (timing->vic >= 1)) {
			video->mCea_code = 0;
			video->mHdmi_code = timing->vic;
		} else {
			video->mCea_code = timing->vic;
			video->mHdmi_code = 0;
		}
	} else {
		video->mCea_code = timing->vic;
		video->mHdmi_code = 0;
	}

	/* if hdmi sink support hdmi2.0 */
	video->mHdmi20 = sink->edid_m20Sink;
	video->scdc_ability = sink->edid_mHdmiForumvsdb.mSCDC_Present;

	if (video->mHdmi_code) {
		product->mVendorPayload[0] = 0x20;
		product->mVendorPayload[1] = video->mHdmi_code;
		product->mVendorPayload[2] = 0;
		product->mVendorPayload[3] = 0;
		product->mVendorPayloadLength = 4;
	} else if (video->mCea_code) {
		product->mVendorPayload[0] = 0;
		product->mVendorPayload[1] = 0;
		product->mVendorPayload[2] = 0;
		product->mVendorPayload[3] = 0;
		product->mVendorPayloadLength = 4;
	}

/* set vendor speciffic info frame: IEEE codes and payload */
	/* hdmi20 video format: 4k50/4k60 */
	if (video->mCea_code == 96 || video->mCea_code == 97
		|| video->mCea_code == 101 || video->mCea_code == 102) {
		product->mOUI = 0xc45dd8;

		product->mVendorPayload[0] = 0x01;
		product->mVendorPayload[1] = 0;
		product->mVendorPayload[2] = 0;
		product->mVendorPayload[3] = 0;
		product->mVendorPayloadLength = 4;
	} else
		product->mOUI = 0x000c03;

/* set info frame for 3d */
	if (timing->b_interlace) {
		video->mHdmiVideoFormat = 0x02;
		video->m3dStructure = 0;
	} else if (video->mHdmi_code) {
		video->mHdmiVideoFormat = 0x01;
		video->m3dStructure = 0;
	} else {
		video->mHdmiVideoFormat = 0x0;
		video->m3dStructure = 0;
	}

	return 0;
}

static int sunxi_hdmi20_get_connect_status(void)
{
	int status, time_out = 5;
	struct sunxi_hdmi *hdmi = sunxi_hdmi20_get_hdmi();
	struct hdmi_dev_func *dev_funcs = &hdmi->dev_funcs;

	/* read hdmi connected status with anti-shake check */
	/* the connected status should remain unchanged for 20*5 ms */
	status = dev_funcs->dev_hpd_status();
	while (time_out) {
		msleep(20);
		if (status == dev_funcs->dev_hpd_status())
			--time_out;
		else {
			time_out = 5;
			status = dev_funcs->dev_hpd_status();
		}
	}

	return status;
}

static void
sunxi_hdmi20_get_working_mode(struct sunxi_hdmi_work_mode *work_mode)
{
	struct sunxi_hdmi *hdmi = sunxi_hdmi20_get_hdmi();
	videoParams_t *video = &hdmi->video;

	if (video->mHdmi == MODE_UNDEFINED
		|| video->mHdmi == DVI)
			work_mode->hdmi_mode = DISP_DVI;
	else if (video->mHdmi == HDMI)
			work_mode->hdmi_mode = DISP_HDMI;

	if (video->mEncodingIn == ENC_UNDEFINED
		|| video->mEncodingIn == RGB)
		work_mode->color_fmt = DISP_CSC_TYPE_RGB;
	else if (video->mEncodingIn == YCC444)
		work_mode->color_fmt = DISP_CSC_TYPE_YUV444;
	else if (video->mEncodingIn == YCC422)
		work_mode->color_fmt = DISP_CSC_TYPE_YUV422;
	else if (video->mEncodingIn == YCC420)
		work_mode->color_fmt = DISP_CSC_TYPE_YUV420;


	if (video->mColorResolution == COLOR_DEPTH_INVALID
		|| video->mColorResolution == COLOR_DEPTH_8)
		work_mode->color_depth = DISP_DATA_8BITS;
	else if (video->mColorResolution == COLOR_DEPTH_10)
		work_mode->color_depth = DISP_DATA_10BITS;
	else if (video->mColorResolution == COLOR_DEPTH_12)
		work_mode->color_depth = DISP_DATA_12BITS;
	else if (video->mColorResolution == COLOR_DEPTH_16)
		work_mode->color_depth = DISP_DATA_16BITS;

	if (video->mColorimetry == ITU601)
		work_mode->color_space = DISP_BT601;
	else if (video->mColorimetry == ITU709)
		work_mode->color_space = DISP_BT709;
	else if (video->mColorimetry == EXTENDED_COLORIMETRY
		&& video->mExtColorimetry == BT2020_Y_CB_CR)
		work_mode->color_space = DISP_BT2020NC;

	if (video->pb->eotf == SDR_LUMINANCE_RANGE)
		work_mode->eotf = DISP_EOTF_GAMMA22;
	else if (video->pb->eotf == HDR_LUMINANCE_RANGE)
		work_mode->eotf = DISP_EOTF_SMPTE2084;
	else if (video->pb->eotf == HLG)
		work_mode->eotf = DISP_EOTF_ARIB_STD_B67;

	if (video->mRgbQuantizationRange == 0)
		work_mode->color_range = DISP_COLOR_RANGE_DEFAULT;
	else if (video->mRgbQuantizationRange == 2)
		work_mode->color_range = DISP_COLOR_RANGE_0_255;
	else if (video->mRgbQuantizationRange == 1)
		work_mode->color_range = DISP_COLOR_RANGE_16_235;

	work_mode->aspect_ratio = video->mActiveFormatAspectRatio;
}

static int
sunxi_hdmi20_set_working_mode(struct sunxi_hdmi_work_mode *work_mode)
{
	struct sunxi_hdmi *hdmi = sunxi_hdmi20_get_hdmi();
	videoParams_t *video = &hdmi->video;

	video->mHdmi = (work_mode->hdmi_mode == DISP_HDMI) ?
			HDMI : DVI;

	if (work_mode->color_fmt == DISP_CSC_TYPE_RGB) {
		video->mEncodingIn = RGB;
		video->mEncodingOut = RGB;
	} else if (work_mode->color_fmt == DISP_CSC_TYPE_YUV444) {
		video->mEncodingIn = YCC444;
		video->mEncodingOut = YCC444;
	} else if (work_mode->color_fmt == DISP_CSC_TYPE_YUV422) {
		video->mEncodingIn = YCC422;
		video->mEncodingOut = YCC422;
	} else if (work_mode->color_fmt == DISP_CSC_TYPE_YUV420) {
		video->mEncodingIn = YCC420;
		video->mEncodingOut = YCC420;
	}

	if (work_mode->color_depth == DISP_DATA_8BITS)
		video->mColorResolution = COLOR_DEPTH_8;
	else if (work_mode->color_depth == DISP_DATA_10BITS)
		video->mColorResolution = COLOR_DEPTH_10;
	else if (work_mode->color_depth == DISP_DATA_12BITS)
		video->mColorResolution = COLOR_DEPTH_12;
	else if (work_mode->color_depth == DISP_DATA_16BITS)
		video->mColorResolution = COLOR_DEPTH_16;

	if (work_mode->color_space == DISP_BT601)
		video->mColorimetry = ITU601;
	else if (work_mode->color_space == DISP_BT709)
		video->mColorimetry = ITU709;
	else if (work_mode->color_space == DISP_BT2020NC) {
		video->mColorimetry = EXTENDED_COLORIMETRY;
		video->mExtColorimetry = BT2020_Y_CB_CR;
	} else {
		video->mColorimetry = ITU709;
	}

	if (work_mode->eotf == DISP_EOTF_GAMMA22)
		video->pb->eotf = SDR_LUMINANCE_RANGE;
	else if (work_mode->eotf == DISP_EOTF_SMPTE2084)
		video->pb->eotf = HDR_LUMINANCE_RANGE;
	else if (work_mode->eotf == DISP_EOTF_ARIB_STD_B67)
		video->pb->eotf = HLG;
	else
		video->pb->eotf = SDR_LUMINANCE_RANGE;

	video->mRgbQuantizationRange = work_mode->color_range;
	video->mActiveFormatAspectRatio = work_mode->aspect_ratio;

	return 0;
}

static int sunxi_hdmi20_get_edid_block(void *data, unsigned char *buf,
		unsigned int block, size_t len)
{
	struct sunxi_hdmi *hdmi = sunxi_hdmi20_get_hdmi();

	return hdmi->dev_funcs.get_edid_block(buf, block, len);
}

static sink_edid_t *sunxi_hdmi20_get_sink_caps(void)
{
	struct sunxi_hdmi *hdmi = sunxi_hdmi20_get_hdmi();

	return &hdmi->sink_caps;
}

static struct sunxi_hdmi_work_mode *sunxi_hdmi20_get_init_params(void)
{
	struct sunxi_hdmi *hdmi = sunxi_hdmi20_get_hdmi();

	return &hdmi->init_params;
}

static void hdmi20_dump_video_para(videoParams_t *para)
{
	DRM_INFO("hdmi mode:%d  cea_code:%d hdmi_code:%d\n",
		para->mHdmi, para->mCea_code, para->mHdmi_code);
	DRM_INFO("format in:%d out:%d\n", para->mEncodingOut, para->mEncodingIn);
	DRM_INFO("depth:%d\n", para->mColorResolution);
	DRM_INFO("pixel_repeat:%d\n", para->mPixelRepetitionFactor);
	DRM_INFO("mColorimetry:%d\n", para->mColorimetry);
	DRM_INFO("mHdmiVideoFormat:%d\n", para->mHdmiVideoFormat);
	DRM_INFO("m3dStructure:%d\n", para->m3dStructure);
	DRM_INFO("mHdmiVic:%d mHdmi20:%d scdc_ability:%d\n",
		para->mHdmiVic, para->mHdmi20, para->scdc_ability);
	DRM_INFO("mActiveFormatAspectRatio:%d\n", para->mActiveFormatAspectRatio);
	DRM_INFO("mRgbQuantizationRange:%d\n", para->mRgbQuantizationRange);
}

static int
sunxi_hdmi20_enable(struct disp_video_timings *timing)
{
	int ret;
	struct sunxi_hdmi *hdmi = sunxi_hdmi20_get_hdmi();
	struct hdmi_dev_func *dev_funcs = &hdmi->dev_funcs;

	ret = sunxi_hdmi20_fill_params(hdmi, timing);
	if (ret < 0) {
		HDMI_ERR("sunxi_hdmi20_fill_params failed\n");
		return ret;
	}

	hdmi20_dump_video_para(&hdmi->video);
	ret = dev_funcs->main_config(&hdmi->video, &hdmi->audio,
			&hdmi->product, &hdmi->hdcp, 301);
	if (ret < 0) {
		HDMI_ERR("hdmi main_config failed!\n");
		return ret;
	}

	return 0;
}

static int
sunxi_hdmi20_sw_enable(struct disp_video_timings *timing)
{
	return 0;
}

static void sunxi_hdmi20_disable(void)
{
	struct sunxi_hdmi *hdmi = sunxi_hdmi20_get_hdmi();
	struct hdmi_dev_func *dev_funcs = &hdmi->dev_funcs;

	dev_funcs->avmute_enable(1);
	dev_funcs->device_close();

	sunxi_hdmi20_clk_disable(hdmi);
	msleep(5);
	sunxi_hdmi20_clk_enable(hdmi);
	dev_funcs->hpd_enable(1);
}

static const struct sunxi_hdmi_funcs hdmi20_funcs = {
	.get_init_params = sunxi_hdmi20_get_init_params,
	.get_connect_status = sunxi_hdmi20_get_connect_status,

	.set_working_mode = sunxi_hdmi20_set_working_mode,
	.get_working_mode = sunxi_hdmi20_get_working_mode,

	.get_edid_block = sunxi_hdmi20_get_edid_block,
	.get_sink_caps = sunxi_hdmi20_get_sink_caps,

	.enable = sunxi_hdmi20_enable,
	.sw_enable = sunxi_hdmi20_sw_enable,
	.disable = sunxi_hdmi20_disable,
};

static int sunxi_hdmi20_power_init(struct platform_device *pdev,
					 struct sunxi_hdmi *hdmi)
{
	int ret = 0, i;
	char power_name[20];

	if (of_property_read_u32(pdev->dev.of_node,
				"hdmi_power_cnt",
				&hdmi->power_count)) {
		pr_err("ERROR: can not get hdmi_power_cnt\n");
		return -1;
	}

	for (i = 0; i < hdmi->power_count; i++) {
		const char *hdmi_power;

		sprintf(power_name, "hdmi_power%d", i);
		if (of_property_read_string(pdev->dev.of_node,
					 power_name, &hdmi_power)) {
			pr_err("Error: get %s failed\n", power_name);
			ret = -1;
		} else {
			DRM_INFO("Get hdmi_power%d:%s\n", i, hdmi_power);
			memcpy((void *)hdmi->power[i], hdmi_power,
					strlen(hdmi_power) + 1);
			sunxi_drm_sys_power_enable(NULL, hdmi->power[i]); /* fix me */
		}
	}

	return 0;
}

static int sunxi_hdmi20_clk_init(struct platform_device *pdev,
					 struct sunxi_hdmi *hdmi)
{
	int ret;
	int index = 0;

	/* get hdmi main clk */
	hdmi->mclk = of_clk_get(pdev->dev.of_node, index);
	if (IS_ERR(hdmi->mclk)) {
		 HDMI_ERR("fail to get clk for hdmi\n");
		 return -1;
	}

	hdmi->mclk_parent = clk_get_parent(hdmi->mclk);
	if (IS_ERR(hdmi->mclk_parent)) {
		 HDMI_ERR("fail to get clk parent for hdmi\n");
		 return -1;
	}

	if (__clk_get_enable_count(hdmi->mclk) == 0) {
		 ret = clk_prepare_enable(hdmi->mclk);
		 if (ret < 0) {
			 HDMI_ERR("fail to enable hdmi mclk\n");
			 return -1;
		 }
	}

	index++;
	/* get ddc clk for hdmi ddc function like edid reading
	* and hdcp authentication
	*/
	hdmi->ddc_clk = of_clk_get(pdev->dev.of_node, index);
	if (IS_ERR(hdmi->ddc_clk)) {
		 HDMI_ERR("fail to get clk for hdmi ddc\n");
		 return -1;
	}

	if (__clk_get_enable_count(hdmi->ddc_clk) == 0) {
		 ret = clk_prepare_enable(hdmi->ddc_clk);
		 if (ret < 0) {
			 HDMI_ERR("fail to enable hdmi ddc clk\n");
			 return -1;
		 }
	}

	index++;
	/* get hdcp clk for hdmi hdcp2.2 function */
	hdmi->hdcp_clk = of_clk_get(pdev->dev.of_node, index);
	if (IS_ERR_OR_NULL(hdmi->hdcp_clk)) {
		 HDMI_ERR("fail to get hdmi_cec_clk\n");
		 /* return -1; */
	}

	if (hdmi->hdcp.use_hdcp
		&& __clk_get_enable_count(hdmi->hdcp_clk) == 0) {
		 ret = clk_prepare_enable(hdmi->hdcp_clk);
		 if (ret < 0) {
			 HDMI_ERR("fail to enable hdmi hdcp clk\n");
			 /* return -1; */
		 }
	}

	index++;
	/* get cec clk for hdmi cec function */
	hdmi->cec_clk = of_clk_get(pdev->dev.of_node, index);
	if (IS_ERR_OR_NULL(hdmi->cec_clk)) {
		 HDMI_ERR("fail to get hdmi_cec_clk\n");
		 /* return -1; */
	}

	if (hdmi->cec.support
		&& __clk_get_enable_count(hdmi->cec_clk) == 0) {
		 ret = clk_prepare_enable(hdmi->cec_clk);
		 if (ret < 0) {
			 HDMI_ERR("fail to enable hdmi cec clk\n");
			 /* return -1; */
		 }
	}

	return 0;
}

static int sunxi_hdmi20_clk_exit(struct sunxi_hdmi *hdmi)
{
	return sunxi_hdmi20_clk_disable(hdmi);
}

static int sunxi_hdmi20_pin_active(void)
{
	/* set hdmi pin like ddc pin to active state */
	sunxi_drm_sys_pin_set_state("hdmi", DDC_PIN_STATE_ACTIVE);
	/* set hdmi pin like ddc pin to active state */
	sunxi_drm_sys_pin_set_state("hdmi", CEC_PIN_STATE_ACTIVE);

	return 0;
}

int sunxi_hdmi20_pin_sleep(void)
{
	/* set hdmi pin like ddc pin to sleep state */
	sunxi_drm_sys_pin_set_state("hdmi", DDC_PIN_STATE_SLEEP);
	/* set hdmi pin like ddc pin to sleep state */
	sunxi_drm_sys_pin_set_state("hdmi", CEC_PIN_STATE_SLEEP);

	return 0;
}

static ssize_t hdmi_debug_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	ssize_t n = 0;

	n += sprintf(buf + n, "Current debug=%d\n\n", hdmi_printf);

	n += sprintf(buf + n, "hdmi log debug level:\n");
	n += sprintf(buf + n, "debug = 1, print video log\n");
	n += sprintf(buf + n, "debug = 2, print edid log\n");
	n += sprintf(buf + n, "debug = 3, print audio log\n");
	n += sprintf(buf + n, "debug = 4, print video+edid+audio log\n");
	n += sprintf(buf + n, "debug = 5, print cec log\n");
	n += sprintf(buf + n, "debug = 6, print hdcp log\n");
	n += sprintf(buf + n, "debug = 7, print all of the logs above\n");
	n += sprintf(buf + n, "debug = 8, print all of the logs above and trace log\n");

	return n;
}

static ssize_t hdmi_debug_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	if (count < 1)
		return -EINVAL;

	if (strncmp(buf, "9", 1) == 0)
		hdmi_printf = 9;
	else if (strncmp(buf, "8", 1) == 0)
		hdmi_printf = 8;
	else if (strncmp(buf, "7", 1) == 0)
		hdmi_printf = 7;
	else if (strncmp(buf, "6", 1) == 0)
		hdmi_printf = 6;
	else if (strncmp(buf, "5", 1) == 0)
			hdmi_printf = 5;
	else if (strncmp(buf, "4", 1) == 0)
		hdmi_printf = 4;
	else if (strncmp(buf, "3", 1) == 0)
		hdmi_printf = 3;
	else if (strncmp(buf, "2", 1) == 0)
		hdmi_printf = 2;
	else if (strncmp(buf, "1", 1) == 0)
		hdmi_printf = 1;
	else if (strncmp(buf, "0", 1) == 0)
		hdmi_printf = 0;
	else
		pr_err("Error Input!\n");

	pr_info("debug=%d\n", hdmi_printf);

	return count;
}

static DEVICE_ATTR(debug, 0664, hdmi_debug_show, hdmi_debug_store);

static unsigned int reg_read_start, reg_read_end, reg_read_cmd;
static ssize_t sunxi_hdmi20_read_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;
	unsigned int reg;

	if (!reg_read_cmd) {
		n += sprintf(buf + n, "read hdmi register, Usage:\n");
		n += sprintf(buf + n, "echo [reg_offset_start] [reg_offset_end] > read\n");
		n += sprintf(buf + n, "OR:echo [reg_offset_start],[reg_offset_end] > read\n");
		return n;
	}

	for (reg = reg_read_start; reg <= reg_read_end; reg++) {
		if (reg % 16 == 0) {
			n += sprintf(buf + n, "\n");
			n += sprintf(buf + n, "0x%x: ", reg);
		}

		n += sprintf(buf + n, "0x%02x ", hdmi20_read(reg << 2));
	}

	n += sprintf(buf + n, "\n");

	reg_read_cmd = 0;

	return n;
}

static ssize_t sunxi_hdmi20_read_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	char *end;

	reg_read_start = (unsigned int)simple_strtoull(buf, &end, 0);

	if ((*end != ' ') && (*end != ',')) {
		HDMI_ERR("error separator:%c\n", *end);
		return count;
	}

	reg_read_end = (unsigned int)simple_strtoull(end + 1, &end, 0);

	reg_read_cmd = 1;

	return count;
}

static DEVICE_ATTR(read, 0660,
		sunxi_hdmi20_read_show, sunxi_hdmi20_read_store);


static ssize_t sunxi_hdmi20_write_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	char *end;
	unsigned int reg_addr;
	unsigned char value;

	reg_addr = (unsigned int)simple_strtoull(buf, &end, 0);

	if ((*end != ' ') && (*end != ',')) {
		HDMI_ERR("error separator:%c\n", *end);
		return count;
	}

	value = (unsigned char)simple_strtoull(end + 1, &end, 0);

	hdmi20_write(reg_addr << 2, value);

	return count;
}

static DEVICE_ATTR(write, 0660, NULL, sunxi_hdmi20_write_store);

static ssize_t sunxi_hdmi20_source_para_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;
	/* struct sunxi_hdmi *hdmi = sunxi_hdmi20_get_hdmi();
	videoParams_t *video = &hdmi->video;
	audioParams_t *audio = &hdmi->audio;
	productParams_t *product = &hdmi->product;
	hdcpParams_t *hdcp = &hdmi->hdcp;
	struct cec_params *cec = &hdmi->cec; */

	return n;
}

static DEVICE_ATTR(source_para, 0660,
		sunxi_hdmi20_source_para_show, NULL);


static ssize_t sunxi_hdmi20_hdcp_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;
	/* struct sunxi_hdmi *hdmi = sunxi_hdmi20_get_hdmi(); */

	/* n += sprintf(buf + n, "%d", hdmi->video.is_hcts); */

	return n;
}

static ssize_t sunxi_hdmi20_hdcp_enable_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	/* struct sunxi_hdmi *hdmi = sunxi_hdmi20_get_hdmi(); */

	if (count < 1)
		return -EINVAL;

	if (strncasecmp(buf, "on", 2) == 0 || strncasecmp(buf, "1", 1) == 0)
		;
	else if (strncasecmp(buf, "off", 3) == 0 ||
			strncasecmp(buf, "0", 1) == 0)
		;
	else
		return -EINVAL;

	return count;
}

static DEVICE_ATTR(hdcp_enable, 0660,
		sunxi_hdmi20_hdcp_enable_show, sunxi_hdmi20_hdcp_enable_store);

static struct attribute *hdmi20_attributes[] = {
	&dev_attr_hdcp_enable.attr,
	&dev_attr_source_para.attr,
	&dev_attr_read.attr,
	&dev_attr_write.attr,
	&dev_attr_debug.attr,
	NULL
};


static void hdmi20_delay_us(int us)
{
	 udelay(us);
}

static int sunxi_hdmi20_core_init(struct sunxi_hdmi *hdmi)
{
	low_functions.sleep = hdmi20_delay_us;
	reg_access.read = hdmi20_read;
	reg_access.write = hdmi20_write;

	register_system_functions(&low_functions);
	register_bsp_functions(&reg_access);

	/* set initial video timing */
/*	sunxi_hdmi20_get_video_mode_info(&hdmi->video_info, */
/*				 DISP_TV_MOD_720P_50HZ); */

	hdmitx_api_init(&hdmi->hdmi_dev,
		&hdmi->video, &hdmi->audio, &hdmi->hdcp);

	hdmi->dev_funcs.hpd_enable(1);

	hdmi->video.pb = kzalloc(sizeof(fc_drm_pb_t), GFP_KERNEL);
	if (!hdmi->video.pb) {
		HDMI_ERR("kzalloc for fc_drm_pb_t failed\n");
		return -1;
	}

	hdmi->funcs = &hdmi20_funcs;
	return 0;
}

static int sunxi_hdmi20_probe(struct platform_device *pdev)
{
	int ret = -1;
	struct sunxi_hdmi *hdmi = NULL;

	hdmi = kmalloc(sizeof(*hdmi), GFP_KERNEL | __GFP_ZERO);
	if (!hdmi) {
		HDMI_ERR("Malloc sunxi_hdmi fail!\n");
		goto OUT;
	}

	hwhdmi20 = hdmi;

	hdmi->pdev = pdev;

	/* iomap */
	hdmi->reg_base = of_iomap(pdev->dev.of_node, 0);
	if (hdmi->reg_base == 0) {
		HDMI_ERR("unable to map hdmi registers\n");
		ret = -EINVAL;
		goto FREE_HDMI;
	}

	ret = sunxi_hdmi20_video_dts_parse(pdev, hdmi);
	if (ret < 0) {
		HDMI_ERR("parse video dts failed\n");
		ret = -EINVAL;
		goto FREE_HDMI;
	}

	ret = sunxi_hdmi20_hdcp_dts_parse(pdev, hdmi);
	if (ret < 0) {
		HDMI_ERR("parse hdcp dts failed\n");
		ret = -EINVAL;
		goto FREE_HDMI;
	}

	ret = sunxi_hdmi20_cec_dts_parse(pdev, hdmi);
	if (ret < 0) {
		HDMI_ERR("parse cec dts failed\n");
		ret = -EINVAL;
		goto FREE_HDMI;
	}

	sunxi_hdmi20_power_init(pdev, hdmi);

	ret = sunxi_hdmi20_clk_init(pdev, hdmi);
	if (ret < 0) {
		HDMI_ERR("sunxi_hdmi_clk_init failed\n");
		goto err_iomap;
	}

	sunxi_hdmi20_pin_active();

	sunxi_hdmi20_hdcp_init();
	sunxi_hdmi20_audio_init();

	ret = sunxi_hdmi20_core_init(hdmi);
	if (ret < 0) {
		HDMI_ERR("sunxi_hdmi20_core_init failed\n");
		goto err_iomap;
	}

	return 0;

err_iomap:
	if (hdmi->reg_base)
		iounmap((char __iomem *)hdmi->reg_base);
FREE_HDMI:
	kfree(hdmi);
OUT:
	return ret;
}

static int sunxi_hdmi20_remove(struct platform_device *pdev)
{
	 int i;
	 struct sunxi_hdmi *hdmi = hwhdmi20;

	 if (!hdmi) {
		 HDMI_ERR("Null pointer!\n");
		 return -1;
	 }

	 sunxi_hdmi20_clk_exit(hdmi);
	 for (i = 0; i < hdmi->power_count; i++)
		 sunxi_drm_sys_power_disable(NULL, hdmi->power[i]); /* fix me */
	 iounmap((char __iomem *)hdmi->reg_base);
	 kfree(hdmi);
	 return 0;
}

static const struct of_device_id sunxi_hdmi20_match[] = {
	 { .compatible = "allwinner,sunxi-hdmi", },

	 {},
};

struct platform_driver sunxi_hdmi20_platform_driver = {
	 .probe = sunxi_hdmi20_probe,
	 .remove = sunxi_hdmi20_remove,
	 .driver = {
			.name = "hdmi",
			.owner = THIS_MODULE,
			.of_match_table = sunxi_hdmi20_match,
	 },
};

static struct attribute_group hdmi20_attribute_group = {
	 .name = "attr",
	 .attrs = hdmi20_attributes,
};

int __init sunxi_hdmi20_module_init(void)
{
	int ret = -1;

	HDMI_INFO(" start\n");
	hdmi20_drv = kmalloc(sizeof(*hdmi20_drv),
				 GFP_KERNEL | __GFP_ZERO);
	if (!hdmi20_drv) {
		 HDMI_ERR("Null drv_model_info pointer\n");
		 goto OUT;
	}
	ret = alloc_chrdev_region(&hdmi20_drv->devid, 0, 1, "hdmi");
	if (ret < 0) {
		 HDMI_ERR("alloc_chrdev_region failed\n");
		 goto FREE_DRV;
	}

	hdmi20_drv->cdev = cdev_alloc();
	if (!hdmi20_drv->cdev) {
		 HDMI_ERR("cdev_alloc failed\n");
		 goto FREE_DRV;
	}

	cdev_init(hdmi20_drv->cdev, NULL);
	hdmi20_drv->cdev->owner = THIS_MODULE;
	ret = cdev_add(hdmi20_drv->cdev, hdmi20_drv->devid, 1);
	if (ret) {
		 HDMI_ERR("cdev_add major number:%d failed\n",
				MAJOR(hdmi20_drv->devid));
		 goto FREE_DRV;
	}

	hdmi20_drv->sysclass = class_create(THIS_MODULE, "hdmi");
	if (IS_ERR(hdmi20_drv->sysclass)) {
		HDMI_ERR("create class error\n");
		goto FREE_DRV;
	}

	hdmi20_drv->dev = device_create(hdmi20_drv->sysclass, NULL,
			 hdmi20_drv->devid, NULL, "hdmi");
	if (!hdmi20_drv->dev) {
		 HDMI_ERR("device_create failed\n");
		 goto FREE_DRV;
	}

	ret = platform_driver_register(&sunxi_hdmi20_platform_driver);
	if (ret) {
		 HDMI_ERR("platform_driver_register failed\n");
		 goto FREE_DEVICE;
	}

	ret = sysfs_create_group(&hdmi20_drv->dev->kobj, &hdmi20_attribute_group);
	if (ret < 0) {
		 HDMI_ERR("sysfs_create_file fail!\n");
		 goto UNREGISTER;
	}

	HDMI_INFO(" end\n");
	return ret;

UNREGISTER:
	 platform_driver_unregister(&sunxi_hdmi20_platform_driver);
FREE_DEVICE:
	 device_destroy(hdmi20_drv->sysclass, hdmi20_drv->devid);
FREE_DRV:
	 kfree(hdmi20_drv);
OUT:
	 HDMI_ERR(" failed\n");
	 return -EINVAL;
}

void __exit sunxi_hdmi20_module_exit(void)
{
	 HDMI_INFO("\n");
	 if (hdmi20_drv) {
		 platform_driver_unregister(&sunxi_hdmi20_platform_driver);

		 device_destroy(hdmi20_drv->sysclass, hdmi20_drv->devid);
		 class_destroy(hdmi20_drv->sysclass);

		 cdev_del(hdmi20_drv->cdev);
		 kfree(hdmi20_drv);
	 }
}

