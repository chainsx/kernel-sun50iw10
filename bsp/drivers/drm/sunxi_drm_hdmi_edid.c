/*
 * Copyright (C) 2019 Allwinnertech Co.Ltd
 * Authors: zhengwanyu
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */
#include <drm/drmP.h>
#include <drm/drm_edid.h>
#include <drm/drm_modes.h>

#include "hdmi_core/api/core_api.h"
#include "sunxi_drm_connector.h"
#define DRM_DEBUG 0
#if defined(CONFIG_AW_DRM_HDMI14)
#include "sunxi_device/sunxi_hdmi14.h"
#elif defined(CONFIG_AW_DRM_HDMI20)
#include "sunxi_device/sunxi_hdmi20.h"
#endif

/* static unsigned char sunxi_edid[1024]; */

extern struct sunxi_drm_hdmi_connector *
to_sunxi_hdmi_connector(struct drm_connector *conn);

extern void
sunxi_drm_hdmi_set_working_mode(
		struct sunxi_drm_connector *sconn,
		struct sunxi_hdmi_work_mode *work_mode);

static u8 edid_bit_field(const u16 data, u8 shift, u8 width)
{
	return (data >> shift) & ((((u16) 1) << width) - 1);
}

static u16 edid_concat_bits(u8 bHi, u8 oHi, u8 nHi, u8 bLo, u8 oLo, u8 nLo)
{
	return (edid_bit_field(bHi, oHi, nHi) << nLo) | edid_bit_field(bLo, oLo, nLo);
}

static u16 edid_byte_to_word(const u8 hi, const u8 lo)
{
	return edid_concat_bits(hi, 0, 8, lo, 0, 8);
}

static u32 edid_byte_to_dword(u8 b3, u8 b2, u8 b1, u8 b0)
{
	u32 retval = 0;

	retval |= b0 << (0 * 8);
	retval |= b1 << (1 * 8);
	retval |= b2 << (2 * 8);
	retval |= b3 << (3 * 8);
	return retval;
}

static void speaker_alloc_data_block_reset(
			speakerAllocationDataBlock_t *sadb);
static void video_cap_data_block_reset(
			videoCapabilityDataBlock_t *vcdb);

static int dtd_parse(dtd_t *dtd, u8 data[18])
{
	/*  */

	dtd->mCode = -1;
	dtd->mPixelRepetitionInput = 0;
	dtd->mLimitedToYcc420 = 0;
	dtd->mYcc420 = 0;

	dtd->mPixelClock = 1000 * edid_byte_to_word(data[1], data[0]);/* [10000Hz] */
	if (dtd->mPixelClock < 0x01) {	/* 0x0000 is defined as reserved */
		return false;
	}

	dtd->mHActive = edid_concat_bits(data[4], 4, 4, data[2], 0, 8);
	dtd->mHBlanking = edid_concat_bits(data[4], 0, 4, data[3], 0, 8);
	dtd->mHSyncOffset = edid_concat_bits(data[11], 6, 2, data[8], 0, 8);
	dtd->mHSyncPulseWidth = edid_concat_bits(data[11], 4, 2, data[9], 0, 8);
	dtd->mHImageSize = edid_concat_bits(data[14], 4, 4, data[12], 0, 8);
	dtd->mHBorder = data[15];

	dtd->mVActive = edid_concat_bits(data[7], 4, 4, data[5], 0, 8);
	dtd->mVBlanking = edid_concat_bits(data[7], 0, 4, data[6], 0, 8);
	dtd->mVSyncOffset = edid_concat_bits(data[11], 2, 2, data[10], 4, 4);
	dtd->mVSyncPulseWidth = edid_concat_bits(data[11], 0, 2, data[10], 0, 4);
	dtd->mVImageSize = edid_concat_bits(data[14], 0, 4, data[13], 0, 8);
	dtd->mVBorder = data[16];

	if (edid_bit_field(data[17], 4, 1) != 1) {/* if not DIGITAL SYNC SIGNAL DEF */
		DRM_ERROR("Error:Invalid DTD Parameters\n");
		return false;
	}
	if (edid_bit_field(data[17], 3, 1) != 1) {/* if not DIGITAL SEPATATE SYNC */
		DRM_ERROR("Error:Invalid DTD Parameters\n");
		return false;
	}
	/* no stereo viewing support in HDMI */
	dtd->mInterlaced = edid_bit_field(data[17], 7, 1) == 1;
	dtd->mVSyncPolarity = edid_bit_field(data[17], 2, 1) == 1;
	dtd->mHSyncPolarity = edid_bit_field(data[17], 1, 1) == 1;
	return true;
}

static void monitor_range_limits_reset(monitorRangeLimits_t *mrl)
{
	mrl->mMinVerticalRate = 0;
	mrl->mMaxVerticalRate = 0;
	mrl->mMinHorizontalRate = 0;
	mrl->mMaxHorizontalRate = 0;
	mrl->mMaxPixelClock = 0;
	mrl->mValid = false;
}

static void colorimetry_data_block_reset(colorimetryDataBlock_t *cdb)
{
	cdb->mByte3 = 0;
	cdb->mByte4 = 0;
	cdb->mValid = false;
}


static int colorimetry_data_block_parse(colorimetryDataBlock_t *cdb, u8 *data)
{
	colorimetry_data_block_reset(cdb);
	if ((data != 0) && (edid_bit_field(data[0], 0, 5) == 0x03) &&
		(edid_bit_field(data[0], 5, 3) == 0x07)
			&& (edid_bit_field(data[1], 0, 7) == 0x05)) {
		cdb->mByte3 = data[2];
		cdb->mByte4 = data[3];
		cdb->mValid = true;
		return true;
	}
	return false;
}

static void hdr_metadata_data_block_reset(
		struct hdr_static_metadata_data_block *hdr_metadata)
{
	memset(hdr_metadata, 0, sizeof(*hdr_metadata));
}

static int hdr_static_metadata_block_parse(
		struct hdr_static_metadata_data_block *hdr_metadata, u8 *data)
{
	hdr_metadata_data_block_reset(hdr_metadata);
	if ((data != 0) && (edid_bit_field(data[0], 0, 5) > 1)
		&& (edid_bit_field(data[0], 5, 3) == 0x07)
		&& (data[1] == 0x06)) {
		hdr_metadata->et_n = edid_bit_field(data[2], 0, 5);
		hdr_metadata->sm_n = data[3];

		if (edid_bit_field(data[0], 0, 5) > 3)
			hdr_metadata->dc_max_lum_data = data[4];
		if (edid_bit_field(data[0], 0, 5) > 4)
			hdr_metadata->dc_max_fa_lum_data = data[5];
		if (edid_bit_field(data[0], 0, 5) > 5)
			hdr_metadata->dc_min_lum_data = data[6];

		return true;
	}
	return false;
}

static void hdmiforumvsdb_reset(hdmiforumvsdb_t *forumvsdb)
{
	forumvsdb->mValid = false;
	forumvsdb->mIeee_Oui = 0;
	forumvsdb->mVersion = 0;
	forumvsdb->mMaxTmdsCharRate = 0;
	forumvsdb->mSCDC_Present = false;
	forumvsdb->mRR_Capable = false;
	forumvsdb->mLTS_340Mcs_scramble = false;
	forumvsdb->mIndependentView = false;
	forumvsdb->mDualView = false;
	forumvsdb->m3D_OSD_Disparity = false;
	forumvsdb->mDC_30bit_420 = false;
	forumvsdb->mDC_36bit_420 = false;
	forumvsdb->mDC_48bit_420 = false;
}

static int hdmiforumvsdb_parse(hdmiforumvsdb_t *forumvsdb,
					u8 *data)
{
	u16 blockLength;

	hdmiforumvsdb_reset(forumvsdb);
	if (data == 0)
		return false;

	if (edid_bit_field(data[0], 5, 3) != 0x3) {
		DRM_ERROR("Error:Invalid datablock tag\n");
		return false;
	}
	blockLength = edid_bit_field(data[0], 0, 5);
	if (blockLength < 7) {
		DRM_ERROR("Error:Invalid minimum length\n");
		return false;
	}
	if (edid_byte_to_dword(0x00, data[3], data[2], data[1]) !=
	    0xC45DD8) {
		DRM_ERROR("Error:HDMI IEEE registration identifier not valid\n");
		return false;
	}
	forumvsdb->mVersion = edid_bit_field(data[4], 0, 7);
	forumvsdb->mMaxTmdsCharRate = edid_bit_field(data[5], 0, 7);
	forumvsdb->mSCDC_Present = edid_bit_field(data[6], 7, 1);
	forumvsdb->mRR_Capable = edid_bit_field(data[6], 6, 1);
	forumvsdb->mLTS_340Mcs_scramble = edid_bit_field(data[6], 3, 1);
	forumvsdb->mIndependentView = edid_bit_field(data[6], 2, 1);
	forumvsdb->mDualView = edid_bit_field(data[6], 1, 1);
	forumvsdb->m3D_OSD_Disparity = edid_bit_field(data[6], 0, 1);
	forumvsdb->mDC_30bit_420 = edid_bit_field(data[7], 2, 1);
	forumvsdb->mDC_36bit_420 = edid_bit_field(data[7], 1, 1);
	forumvsdb->mDC_48bit_420 = edid_bit_field(data[7], 0, 1);
	forumvsdb->mValid = true;

#if DRM_DEBUG
	DRM_DEBUG_DRIVER("version %d\n", edid_bit_field(data[4], 0, 7));
	DRM_DEBUG_DRIVER("Max_TMDS_Charater_rate %d\n",
		    edid_bit_field(data[5], 0, 7));
	DRM_DEBUG_DRIVER("SCDC_Present %d\n", edid_bit_field(data[6], 7, 1));
	DRM_DEBUG_DRIVER("RR_Capable %d\n", edid_bit_field(data[6], 6, 1));
	DRM_DEBUG_DRIVER("LTE_340Mcsc_scramble %d\n",
		    edid_bit_field(data[6], 3, 1));
	DRM_DEBUG_DRIVER("Independent_View %d\n", edid_bit_field(data[6], 2, 1));
	DRM_DEBUG_DRIVER("Dual_View %d\n", edid_bit_field(data[6], 1, 1));
	DRM_DEBUG_DRIVER("3D_OSD_Disparity %d\n", edid_bit_field(data[6], 0, 1));
	DRM_DEBUG_DRIVER("DC_48bit_420 %d\n", edid_bit_field(data[7], 2, 1));
	DRM_DEBUG_DRIVER("DC_36bit_420 %d\n", edid_bit_field(data[7], 1, 1));
	DRM_DEBUG_DRIVER("DC_30bit_420 %d\n", edid_bit_field(data[7], 0, 1));
#endif
	return true;
}

static void hdmivsdb_reset(hdmivsdb_t *vsdb)
{
	int i, j = 0;

	vsdb->mPhysicalAddress = 0;
	vsdb->mSupportsAi = false;
	vsdb->mDeepColor30 = false;
	vsdb->mDeepColor36 = false;
	vsdb->mDeepColor48 = false;
	vsdb->mDeepColorY444 = false;
	vsdb->mDviDual = false;
	vsdb->mMaxTmdsClk = 0;
	vsdb->mVideoLatency = 0;
	vsdb->mAudioLatency = 0;
	vsdb->mInterlacedVideoLatency = 0;
	vsdb->mInterlacedAudioLatency = 0;
	vsdb->mId = 0;
	vsdb->mContentTypeSupport = 0;
	vsdb->mHdmiVicCount = 0;
	for (i = 0; i < MAX_HDMI_VIC; i++)
		vsdb->mHdmiVic[i] = 0;

	vsdb->m3dPresent = false;
	for (i = 0; i < MAX_VIC_WITH_3D; i++) {
		for (j = 0; j < MAX_HDMI_3DSTRUCT; j++)
			vsdb->mVideo3dStruct[i][j] = 0;
	}
	for (i = 0; i < MAX_VIC_WITH_3D; i++) {
		for (j = 0; j < MAX_HDMI_3DSTRUCT; j++)
			vsdb->mDetail3d[i][j] = ~0;
	}
	vsdb->mValid = false;
}

static int edid_parser_CeaExtReset(sink_edid_t *edidExt)
{
	unsigned i = 0;

	edidExt->edid_m20Sink = false;
#if 1
	for (i = 0; i < sizeof(edidExt->edid_mMonitorName); i++)
		edidExt->edid_mMonitorName[i] = 0;

	edidExt->edid_mBasicAudioSupport = false;
	edidExt->edid_mUnderscanSupport = false;
	edidExt->edid_mYcc422Support = false;
	edidExt->edid_mYcc444Support = false;
	edidExt->edid_mYcc420Support = false;
	edidExt->edid_mDtdIndex = 0;
	edidExt->edid_mSadIndex = 0;
	edidExt->edid_mSvdIndex = 0;
#endif
	hdmivsdb_reset(&edidExt->edid_mHdmivsdb);
	hdmiforumvsdb_reset(&edidExt->edid_mHdmiForumvsdb);
	monitor_range_limits_reset(&edidExt->edid_mMonitorRangeLimits);
	video_cap_data_block_reset(&edidExt->edid_mVideoCapabilityDataBlock);
	colorimetry_data_block_reset(&edidExt->edid_mColorimetryDataBlock);
	hdr_metadata_data_block_reset(&edidExt->edid_hdr_static_metadata_data_block);
	speaker_alloc_data_block_reset(
				&edidExt->edid_mSpeakerAllocationDataBlock);
	return true;
}


static int hdmivsdb_parse(hdmivsdb_t *vsdb, u8 *data)
{
	u8 blockLength = 0;
	unsigned videoInfoStart = 0;
	unsigned hdmi3dStart = 0;
	unsigned hdmiVicLen = 0;
	unsigned hdmi3dLen = 0;
	unsigned spanned3d = 0;
	unsigned i = 0;
	unsigned j = 0;

	hdmivsdb_reset(vsdb);
	if (data == 0)
		return false;

	if (edid_bit_field(data[0], 5, 3) != 0x3) {
		DRM_ERROR("Error:Invalid datablock tag\n");
		return false;
	}
	blockLength = edid_bit_field(data[0], 0, 5);
	if (blockLength < 5) {
		DRM_ERROR("Error:Invalid minimum length\n");
		return false;
	}
	if (edid_byte_to_dword(0x00, data[3], data[2], data[1]) != 0x000C03) {
		DRM_ERROR("Error:HDMI IEEE registration identifier not valid\n");
		return false;
	}
	hdmivsdb_reset(vsdb);
	vsdb->mId = 0x000C03;
	vsdb->mPhysicalAddress = edid_byte_to_word(data[4], data[5]);
	/* parse extension fields if they exist */
	if (blockLength > 5) {
		vsdb->mSupportsAi = edid_bit_field(data[6], 7, 1) == 1;
		vsdb->mDeepColor48 = edid_bit_field(data[6], 6, 1) == 1;
		vsdb->mDeepColor36 = edid_bit_field(data[6], 5, 1) == 1;
		vsdb->mDeepColor30 = edid_bit_field(data[6], 4, 1) == 1;
		vsdb->mDeepColorY444 = edid_bit_field(data[6], 3, 1) == 1;
		vsdb->mDviDual = edid_bit_field(data[6], 0, 1) == 1;
	} else {
		vsdb->mSupportsAi = false;
		vsdb->mDeepColor48 = false;
		vsdb->mDeepColor36 = false;
		vsdb->mDeepColor30 = false;
		vsdb->mDeepColorY444 = false;
		vsdb->mDviDual = false;
	}
	vsdb->mMaxTmdsClk = (blockLength > 6) ? data[7] : 0;
	vsdb->mVideoLatency = 0;
	vsdb->mAudioLatency = 0;
	vsdb->mInterlacedVideoLatency = 0;
	vsdb->mInterlacedAudioLatency = 0;
	if (blockLength > 7) {
		if (edid_bit_field(data[8], 7, 1) == 1) {
			if (blockLength < 10) {
				DRM_ERROR("Error:Invalid length - latencies are not valid\n");
				return false;
			}
			if (edid_bit_field(data[8], 6, 1) == 1) {
				if (blockLength < 12) {
					DRM_ERROR("Error:Invalid length - Interlaced latencies are not valid\n");
					return false;
				} else {
					vsdb->mVideoLatency = data[9];
					vsdb->mAudioLatency = data[10];
					vsdb->mInterlacedVideoLatency
								= data[11];
					vsdb->mInterlacedAudioLatency
								= data[12];
					videoInfoStart = 13;
				}
			} else {
				vsdb->mVideoLatency = data[9];
				vsdb->mAudioLatency = data[10];
				vsdb->mInterlacedVideoLatency = 0;
				vsdb->mInterlacedAudioLatency = 0;
				videoInfoStart = 11;
			}
		} else {	/* no latency data */
			vsdb->mVideoLatency = 0;
			vsdb->mAudioLatency = 0;
			vsdb->mInterlacedVideoLatency = 0;
			vsdb->mInterlacedAudioLatency = 0;
			videoInfoStart = 9;
		}
		vsdb->mContentTypeSupport = edid_bit_field(data[8], 0, 4);
	}
	/* additional video format capabilities are described */
	if (edid_bit_field(data[8], 5, 1) == 1) {
		vsdb->mImageSize = edid_bit_field(data[videoInfoStart], 3, 2);
		hdmiVicLen = edid_bit_field(data[videoInfoStart + 1], 5, 3);
		hdmi3dLen = edid_bit_field(data[videoInfoStart + 1], 0, 5);
		for (i = 0; i < hdmiVicLen; i++)
			vsdb->mHdmiVic[i] = data[videoInfoStart + 2 + i];

		vsdb->mHdmiVicCount = hdmiVicLen;
		if (edid_bit_field(data[videoInfoStart], 7, 1) == 1) {/* 3d present */
			vsdb->m3dPresent = true;
			hdmi3dStart = videoInfoStart + hdmiVicLen + 2;
			/* 3d multi 00 -> both 3d_structure_all
			and 3d_mask_15 are NOT present */
			/* 3d mutli 11 -> reserved */
			if (edid_bit_field(data[videoInfoStart], 5, 2) == 1) {
				/* 3d multi 01 */
				/* 3d_structure_all is present but 3d_mask_15 not present */
				for (j = 0; j < 16; j++) {
					/* j spans 3d structures */
					if (edid_bit_field(data[hdmi3dStart
						+ (j / 8)], (j % 8), 1) == 1) {
						for (i = 0; i < 16; i++)
							vsdb->mVideo3dStruct[i][(j < 8)	? j+8 : j - 8] = 1;
					}
				}
				spanned3d = 2;
				/* hdmi3dStart += 2;
				   hdmi3dLen -= 2; */
			} else if (edid_bit_field(data[videoInfoStart], 5, 2) == 2) {
				/* 3d multi 10 */
				/* 3d_structure_all and 3d_mask_15 are present */
				for (j = 0; j < 16; j++) {
					for (i = 0; i < 16; i++) {
						if (edid_bit_field(data[hdmi3dStart + 2 + (i / 8)], (i % 8), 1) == 1)
							vsdb->mVideo3dStruct[(i < 8) ? i + 8 : i - 8][(j < 8) ? j + 8 : j - 8] = edid_bit_field(data[hdmi3dStart + (j / 8)], (j % 8), 1);
					}
				}
				spanned3d = 4;
			}
			if (hdmi3dLen > spanned3d) {
				hdmi3dStart += spanned3d;
				for (i = 0, j = 0; i < (hdmi3dLen - spanned3d); i++) {
					vsdb->mVideo3dStruct[edid_bit_field(data[hdmi3dStart + i + j], 4, 4)][edid_bit_field(data[hdmi3dStart + i + j], 0, 4)] = 1;
					if (edid_bit_field(data[hdmi3dStart + i + j], 4, 4) > 7) {
						j++;
						vsdb->mDetail3d[edid_bit_field(data[hdmi3dStart + i + j], 4, 4)][edid_bit_field(data[hdmi3dStart + i + j], 4, 4)] = edid_bit_field(data[hdmi3dStart + i + j], 4, 4);
					}
				}
			}
		} else {	/* 3d NOT present */
			vsdb->m3dPresent = false;
		}
	}
	vsdb->mValid = true;
	return true;
}

static void sad_reset(shortAudioDesc_t *sad)
{
	sad->mFormat = 0;
	sad->mMaxChannels = 0;
	sad->mSampleRates = 0;
	sad->mByte3 = 0;
}

static int sad_parse(shortAudioDesc_t *sad, u8 *data)
{
	sad_reset(sad);
	if (data != 0) {
		sad->mFormat = edid_bit_field(data[0], 3, 4);
		sad->mMaxChannels = edid_bit_field(data[0], 0, 3) + 1;
		sad->mSampleRates = edid_bit_field(data[1], 0, 7);
		sad->mByte3 = data[2];
		return true;
	}
	return false;
}

#if DRM_DEBUG
static int sad_support32k(shortAudioDesc_t *sad)
{
	return (edid_bit_field(sad->mSampleRates, 0, 1) == 1) ? true : false;
}

static int sad_support44k1(shortAudioDesc_t *sad)
{
	return (edid_bit_field(sad->mSampleRates, 1, 1) == 1) ? true : false;
}

static int sad_support48k(shortAudioDesc_t *sad)
{
	return (edid_bit_field(sad->mSampleRates, 2, 1) == 1) ? true : false;
}

static int sad_support88k2(shortAudioDesc_t *sad)
{
	return (edid_bit_field(sad->mSampleRates, 3, 1) ==
		1) ? true : false;
}

static int sad_support96k(shortAudioDesc_t *sad)
{
	return (edid_bit_field(sad->mSampleRates, 4, 1) == 1) ? true : false;
}

static int sad_support176k4(shortAudioDesc_t *sad)
{
	return (edid_bit_field(sad->mSampleRates, 5, 1) == 1) ? true : false;
}

static int sad_support192k(shortAudioDesc_t *sad)
{
	return (edid_bit_field(sad->mSampleRates, 6, 1) == 1) ? true : false;
}

static int sad_support16bit(shortAudioDesc_t *sad)
{
	if (sad->mFormat == 1)
		return (edid_bit_field(sad->mByte3, 0, 1) == 1) ? true : false;

	DRM_DEBUG_DRIVER("%s:Information is not valid for this format\n", __func__);
	return false;
}

static int sad_support20bit(shortAudioDesc_t *sad)
{
	if (sad->mFormat == 1)
		return (edid_bit_field(sad->mByte3, 1, 1) == 1) ? true : false;
	DRM_DEBUG_DRIVER("%s:Information is not valid for this format\n", __func__);
	return false;
}

static int sad_support24bit(shortAudioDesc_t *sad)
{
	if (sad->mFormat == 1)
		return (edid_bit_field(sad->mByte3, 2, 1) == 1) ? true : false;
	DRM_DEBUG_DRIVER("%s:Information is not valid for this format\n", __func__);
	return false;
}
#endif

static void svd_reset(shortVideoDesc_t *svd)
{
	svd->mNative = false;
	svd->mCode = 0;
}

static int svd_parse(shortVideoDesc_t *svd, u8 data)
{
	svd_reset(svd);
	svd->mNative = (edid_bit_field(data, 7, 1) == 1) ? true : false;
	svd->mCode = edid_bit_field(data, 0, 7);
	svd->mLimitedToYcc420 = 0;
	svd->mYcc420 = 0;
	return true;
}


static void speaker_alloc_data_block_reset(
				speakerAllocationDataBlock_t *sadb)
{
	sadb->mByte1 = 0;
	sadb->mValid = false;
}

static int speaker_alloc_data_block_parse(
				speakerAllocationDataBlock_t *sadb, u8 *data)
{
	speaker_alloc_data_block_reset(sadb);
	if ((data != 0) && (edid_bit_field(data[0], 0, 5) == 0x03) &&
				(edid_bit_field(data[0], 5, 3) == 0x04)) {
		sadb->mByte1 = data[1];
		sadb->mValid = true;
		return true;
	}
	return false;
}

static void video_cap_data_block_reset(videoCapabilityDataBlock_t *vcdb)
{
	vcdb->mQuantizationRangeSelectable = false;
	vcdb->mPreferredTimingScanInfo = 0;
	vcdb->mItScanInfo = 0;
	vcdb->mCeScanInfo = 0;
	vcdb->mValid = false;
}

static int video_cap_data_block_parse(videoCapabilityDataBlock_t *vcdb, u8 *data)
{
	video_cap_data_block_reset(vcdb);
	/* check tag code and extended tag */
	if ((data != 0) && (edid_bit_field(data[0], 5, 3) == 0x7) &&
		(edid_bit_field(data[1], 0, 8) == 0x0) &&
			(edid_bit_field(data[0], 0, 5) == 0x2)) {
		/* so far VCDB is 2 bytes long */
		vcdb->mCeScanInfo = edid_bit_field(data[2], 0, 2);
		vcdb->mItScanInfo = edid_bit_field(data[2], 2, 2);
		vcdb->mPreferredTimingScanInfo = edid_bit_field(data[2], 4, 2);
		vcdb->mQuantizationRangeSelectable =
				(edid_bit_field(data[2], 6, 1) == 1) ? true : false;
		vcdb->mValid = true;
		return true;
	}
	return false;
}

#if DRM_DEBUG
static int _edid_checksum(u8 *edid)
{
	int i, checksum = 0;

	for (i = 0; i < EDID_LENGTH; i++)
		checksum += edid[i];

	return checksum % 256; /* CEA-861 Spec */
}
#endif

static void edid_parser_updateYcc420(sink_edid_t *edidExt,
					u8 Ycc420All, u8 LimitedToYcc420All)
{
	u16 edid_cnt = 0;

	for (edid_cnt = 0; edid_cnt < edidExt->edid_mSvdIndex; edid_cnt++) {
		switch (edidExt->edid_mSvd[edid_cnt].mCode) {
		case 96:
		case 97:
		case 101:
		case 102:
		case 106:
		case 107:
			Ycc420All == 1 ?
				edidExt->edid_mSvd[edid_cnt].mYcc420 = Ycc420All : 0;
			LimitedToYcc420All == 1 ?
				edidExt->edid_mSvd[edid_cnt].mLimitedToYcc420 =
							LimitedToYcc420All : 0;
			break;
		}
	}
}


static int _edid_struture_parser(struct edid *edid,
					sink_edid_t *sink)
{
	int i;
	char *monitorName;

	if (edid->header[0] != 0) {
		DRM_ERROR(":Error:Invalid Header\n");
		return -1;
	}

	DRM_DEBUG_DRIVER("\n\nEDID Block0 detailed discriptor:\n");
	for (i = 0; i < 4; i++) {
		struct detailed_timing *detailed_timing = &(edid->detailed_timings[i]);

		if (detailed_timing->pixel_clock == 0) {
			struct detailed_non_pixel *npixel = &(detailed_timing->data.other_data);

			switch (npixel->type) {
			case EDID_DETAIL_MONITOR_NAME:
				monitorName = (char *) &(npixel->data.str.str);
				DRM_DEBUG_DRIVER("Monitor name: %s\n", monitorName);
				break;
			case EDID_DETAIL_MONITOR_RANGE:
				break;

			}
		} else { /* Detailed Timing Definition */
			struct detailed_pixel_timing *ptiming = &(detailed_timing->data.pixel_data);
			DRM_DEBUG_DRIVER("\npixel_clock:%d\n", detailed_timing->pixel_clock * 10000);
			DRM_DEBUG_DRIVER("hactive * vactive: %d * %d\n",
			(((ptiming->hactive_hblank_hi >> 4) & 0x0f) << 8)
			| ptiming->hactive_lo,
			(((ptiming->vactive_vblank_hi >> 4) & 0x0f) << 8)
			| ptiming->vactive_lo);
		}
	}
	return true;
}

static int edid_parser_ParseDataBlock(u8 *data, sink_edid_t *edidExt)
{
	u8 c = 0;
	shortAudioDesc_t tmpSad;
	shortVideoDesc_t tmpSvd;
	u8 tmpYcc420All = 0;
	u8 tmpLimitedYcc420All = 0;
	u32 ieeeId = 0;
	u8 extendedTag = 0;
	int i = 0;
	int edid_cnt = 0;
	int svdNr = 0;
	int icnt = 0;
	u8 tag = edid_bit_field(data[0], 5, 3);
	u8 length = edid_bit_field(data[0], 0, 5);

	tmpSvd.mLimitedToYcc420 = 0;
	tmpSvd.mYcc420 = 0;


	switch (tag) {
	case 0x1:		/* Audio Data Block */
		DRM_DEBUG_DRIVER("EDID: Audio datablock parsing\n");
		for (c = 1; c < (length + 1); c += 3) {
			sad_parse(&tmpSad, data + c);
			if (edidExt->edid_mSadIndex < (sizeof(edidExt->edid_mSad) / sizeof(shortAudioDesc_t)))
				edidExt->edid_mSad[edidExt->edid_mSadIndex++] = tmpSad;
			else
				DRM_ERROR("buffer full - SAD ignored\n");
		}
		break;
	case 0x2:		/* Video Data Block */
		DRM_DEBUG_DRIVER("EDID: Video datablock parsing\n");
		for (c = 1; c < (length + 1); c++) {
			svd_parse(&tmpSvd, data[c]);
			if (edidExt->edid_mSvdIndex < (sizeof(edidExt->edid_mSvd) / sizeof(shortVideoDesc_t)))
				edidExt->edid_mSvd[edidExt->edid_mSvdIndex++] = tmpSvd;
			else
				DRM_ERROR("buffer full - SVD ignored\n");
		}
		break;
	case 0x3:		/* Vendor Specific Data Block HDMI or HF */
		DRM_DEBUG_DRIVER("\nEDID: VSDB HDMI and HDMI-F\n ");
		ieeeId = edid_byte_to_dword(0x00, data[3], data[2], data[1]);
		if (ieeeId == 0x000C03) {	/* HDMI */
			if (hdmivsdb_parse(&edidExt->edid_mHdmivsdb, data) != true) {
				DRM_ERROR("Error:HDMI Vendor Specific Data Block corrupt\n");
				break;
			}
			DRM_DEBUG_DRIVER("EDID HDMI VSDB parsed\n");
		} else {
			if (ieeeId == 0xC45DD8) {	/* HDMI-F */
				DRM_DEBUG_DRIVER("Sink is HDMI 2.0 because haves HF-VSDB\n");
				edidExt->edid_m20Sink = true;
				if (hdmiforumvsdb_parse(&edidExt->edid_mHdmiForumvsdb, data) != true) {
					DRM_ERROR("Error:HDMI Vendor Specific Data Block corrupt\n");
					break;
				}
			} else {
				DRM_DEBUG_DRIVER("Vendor Specific Data Block not parsed ieeeId: 0x%x\n",
						ieeeId);
			}
		}
		DRM_DEBUG_DRIVER("\n");
		break;
	case 0x4:		/* Speaker Allocation Data Block */
		DRM_DEBUG_DRIVER("SAD block parsing\n");
		if (speaker_alloc_data_block_parse(&edidExt->edid_mSpeakerAllocationDataBlock, data) != true)
			DRM_ERROR("Error:Speaker Allocation Data Block corrupt\n");
		break;
	case 0x7:{
		DRM_DEBUG_DRIVER("EDID CEA Extended field 0x07\n");
		extendedTag = data[1];
		switch (extendedTag) {
		case 0x00:	/* Video Capability Data Block */
			DRM_DEBUG_DRIVER("Video Capability Data Block\n");
			if (video_cap_data_block_parse(&edidExt->edid_mVideoCapabilityDataBlock, data) != true)
				DRM_DEBUG_DRIVER("Error:Video Capability Data Block corrupt\n");
			break;
		case 0x04:	/* HDMI Video Data Block */
			DRM_DEBUG_DRIVER("HDMI Video Data Block\n");
			break;
		case 0x05:	/* Colorimetry Data Block */
			DRM_DEBUG_DRIVER("Colorimetry Data Block\n");
			if (colorimetry_data_block_parse(&edidExt->edid_mColorimetryDataBlock, data) != true)
				DRM_DEBUG_DRIVER("Error:Colorimetry Data Block corrupt\n");
			break;
		case 0x06:	/* HDR Static Metadata Data Block */
			DRM_DEBUG_DRIVER("HDR Static Metadata Data Block\n");
			if (hdr_static_metadata_block_parse(&edidExt->edid_hdr_static_metadata_data_block, data) != true)
				DRM_DEBUG_DRIVER("HDR Static Metadata Data Block corrupt\n");
			break;
		case 0x12:	/* HDMI Audio Data Block */
			DRM_DEBUG_DRIVER("HDMI Audio Data Block\n");
			break;
		case 0xe:
			/* If it is a YCC420 VDB then VICs can ONLY be displayed in YCC 4:2:0 */
			DRM_DEBUG_DRIVER("YCBCR 4:2:0 Video Data Block\n");
			/* If Sink has YCC Datablocks it is HDMI 2.0 */
			edidExt->edid_m20Sink = true;
			tmpLimitedYcc420All = (edid_bit_field(data[0], 0, 5) == 1 ? 1 : 0);
			edid_parser_updateYcc420(edidExt, tmpYcc420All, tmpLimitedYcc420All);
			for (i = 0; i < (edid_bit_field(data[0], 0, 5) - 1); i++) {
				/* Length includes the tag byte */
				tmpSvd.mCode = data[2 + i];
				tmpSvd.mNative = 0;
				tmpSvd.mLimitedToYcc420 = 1;
				for (edid_cnt = 0; edid_cnt < edidExt->edid_mSvdIndex; edid_cnt++) {
					if (edidExt->edid_mSvd[edid_cnt].mCode == tmpSvd.mCode) {
						edidExt->edid_mSvd[edid_cnt] =	tmpSvd;
						goto concluded;
					}
				}
				if (edidExt->edid_mSvdIndex <
					(sizeof(edidExt->edid_mSvd) /  sizeof(shortVideoDesc_t))) {
					edidExt->edid_mSvd[edidExt->edid_mSvdIndex] = tmpSvd;
					edidExt->edid_mSvdIndex++;
				} else {
					DRM_DEBUG_DRIVER("buffer full - YCC 420 DTD ignored\n");
				}
concluded:;
			}
			break;
		case 0x0f:
			/* If it is a YCC420 CDB then VIC can ALSO be displayed in YCC 4:2:0 */
			edidExt->edid_m20Sink = true;
			DRM_DEBUG_DRIVER("YCBCR 4:2:0 Capability Map Data Block\n");
			/* If YCC420 CMDB is bigger than 1, then there is SVD info to parse */
			if (edid_bit_field(data[0], 0, 5) > 1) {
				for (icnt = 0; icnt < 8; icnt++) {
					/* Lenght includes the tag byte */
					if ((edid_bit_field(data[2], 1 >> icnt, 1) & 0x01)) {
						svdNr = icnt;
						tmpSvd.mCode = edidExt->edid_mSvd[svdNr - 1].mCode;
						tmpSvd.mYcc420 = 1;
						edidExt->edid_mSvd[svdNr - 1] = tmpSvd;
					}
				}
				/* Otherwise, all SVDs present at the Video Data Block support YCC420 */
			} else {
				tmpYcc420All = (edid_bit_field(data[0], 0, 5) == 1 ? 1 : 0);
				edid_parser_updateYcc420(edidExt, tmpYcc420All, tmpLimitedYcc420All);
			}
			break;
		default:
			DRM_ERROR("Error:Extended Data Block not parsed %d\n",
					extendedTag);
			break;
		}
		break;
	}
	default:
		DRM_ERROR("Error:Data Block not parsed %d\n", tag);
		break;
	}
	return length + 1;
}


static int _edid_cea_extension_parser(u8 *buffer, sink_edid_t *edidExt)
{
	int i = 0;
	int c = 0;
	dtd_t tmpDtd;
	u8 offset = buffer[2];

	if (buffer[1] < 0x03) {
		DRM_ERROR("Error:Invalid version for CEA Extension block,only rev 3 or higher is supported");
		return -1;
	}

	edidExt->edid_mYcc422Support = edid_bit_field(buffer[3], 4, 1) == 1;
	edidExt->edid_mYcc444Support = edid_bit_field(buffer[3], 5, 1) == 1;
	edidExt->edid_mBasicAudioSupport = edid_bit_field(buffer[3], 6, 1) == 1;
	edidExt->edid_mUnderscanSupport = edid_bit_field(buffer[3], 7, 1) == 1;
	if (offset != 4) {
		for (i = 4; i < offset; i += edid_parser_ParseDataBlock(buffer + i, edidExt))
			;
	}

	memcpy(edidExt->detailed_timings, buffer + 84, sizeof(edidExt->detailed_timings));

	/* last is checksum */
	for (i = offset, c = 0; i < (sizeof(buffer) - 1) && c < 6; i += 12, c++) {
		if (dtd_parse(&tmpDtd, buffer + i) == true) {
			if (edidExt->edid_mDtdIndex < ((sizeof(edidExt->edid_mDtd) / sizeof(dtd_t)))) {
				edidExt->edid_mDtd[edidExt->edid_mDtdIndex++] = tmpDtd;
				DRM_DEBUG_DRIVER("edid_mDtd code %d\n", edidExt->edid_mDtd[edidExt->edid_mDtdIndex].mCode);
				DRM_DEBUG_DRIVER("edid_mDtd limited to Ycc420? %d\n", edidExt->edid_mDtd[edidExt->edid_mDtdIndex].mLimitedToYcc420);
				DRM_DEBUG_DRIVER("edid_mDtd supports Ycc420? %d\n", edidExt->edid_mDtd[edidExt->edid_mDtdIndex].mYcc420);
			} else {
				DRM_DEBUG_DRIVER("buffer full - DTD ignored\n");
			}
		}
	}
	return true;
}

static int edid_parser(u8 *buffer, sink_edid_t *edidExt,
							u16 edid_size)
{
	int ret = 0;

	switch (buffer[0]) {
	case 0x00:
		ret = _edid_struture_parser((struct edid *) buffer,
								edidExt);
		break;
	case CEA_EXT:
		ret = _edid_cea_extension_parser(buffer, edidExt);
		break;
	case VTB_EXT:
	case DI_EXT:
	case LS_EXT:
	case MI_EXT:
	default:
		DRM_DEBUG_DRIVER("Block 0x%02x not supported\n", buffer[0]);
	}
	if (ret == true)
		return true;

	return false;
}

void sunxi_edid_parse_cap(struct edid *edid, sink_edid_t *sink)
{
	memset(sink, 0, sizeof(sink_edid_t));

	edid_parser_CeaExtReset(sink);

	if (edid_parser((u8 *)edid, sink, 128) == false) {
		DRM_ERROR("Error:Could not parse EDID\n");
		return;
	}


	if (edid->extensions) {
		int edid_ext_cnt = 1;

		while (edid_ext_cnt <= edid->extensions) {
			DRM_DEBUG_DRIVER("EDID Extension %d\n", edid_ext_cnt);
			if (edid_ext_cnt < 2) {
				if (edid_parser((u8 *)&edid[edid_ext_cnt], sink, 128) == false)
					DRM_ERROR("Could not parse EDID EXTENSIONS:%d\n", edid_ext_cnt);
			}

			edid_ext_cnt++;
		}
	}
}

static bool sunixi_hdmi_edid_is_zero(const u8 *in_edid, int length)
{
	if (memchr_inv(in_edid, 0, length))
		return false;

	return true;
}

struct edid *sunxi_drm_do_get_edid(struct drm_connector *connector)
{
	int i, j = 0, valid_extensions = 0;
	u8 *block, *new;
	struct sunxi_drm_connector *sunxi_con = to_sunxi_connector(connector);
	struct sunxi_hdmi_funcs *hdmi_funcs = sunxi_con->hw_funcs;
	bool print_bad_edid = !connector->bad_edid_counter || (drm_debug & DRM_UT_KMS);

	block = kmalloc(EDID_LENGTH, GFP_KERNEL | __GFP_ZERO);
	if (block == NULL)
		return NULL;
	/* block =  sunxi_edid; */

	/* base block fetch, try maximun 4 times agin */
	for (i = 0; i < 4; i++) {
		if (hdmi_funcs->get_edid_block(NULL, block, 0, EDID_LENGTH)) {
			DRM_ERROR("get_edid_block:0 failed\n");
			goto out;
		}

		if (drm_edid_block_valid(block, 0, print_bad_edid,
					 &connector->edid_corrupt))
			break;

		DRM_ERROR("drm_edid_block0 is INVALID!!!\n");

		if (i == 0 && sunixi_hdmi_edid_is_zero(block, EDID_LENGTH)) {
			connector->null_edid_counter++;
			goto carp;
		}
	}

	if (i == 4) {
		DRM_ERROR("read EDID Failed!!!\n");
		goto carp;
	}

	/* if there's no extensions, we're done */
	if (block[0x7e] == 0) {
		DRM_DEBUG_DRIVER("WARN: Sink's EDID has NO extension block");
		return (struct edid *)block;
	}
	DRM_DEBUG_DRIVER("EDID EXT BLOCK NUM:%d\n", block[0x7e]);

	new = krealloc(block, (block[0x7e] + 1) * EDID_LENGTH,
				GFP_KERNEL | __GFP_ZERO);
	if (!new)
		goto out;
	block = new;

	for (j = 1; j <= block[0x7e]; j++) {
		for (i = 0; i < 4; i++) {
			if (hdmi_funcs->get_edid_block(NULL,
				  block + (valid_extensions + 1) * EDID_LENGTH,
				  j, EDID_LENGTH))
				goto out;
			if (drm_edid_block_valid(block + (valid_extensions + 1)
						 * EDID_LENGTH, j,
						 print_bad_edid,
						 NULL)) {
				valid_extensions++;
				break;
			}
		}

		if (i == 4 && print_bad_edid) {
			dev_warn(connector->dev->dev,
			 "%s: Ignoring invalid EDID block %d.\n",
			 connector->name, j);

			connector->bad_edid_counter++;
		}
	}

	if (valid_extensions != block[0x7e]) {
		block[EDID_LENGTH-1] += block[0x7e] - valid_extensions;
		block[0x7e] = valid_extensions;
		new = krealloc(block, (valid_extensions + 1) * EDID_LENGTH, GFP_KERNEL);
		if (!new)
			goto out;
		block = new;
	}

	return (struct edid *)block;

carp:
	if (print_bad_edid) {
		dev_warn(connector->dev->dev, "%s: EDID block %d invalid.\n",
			 connector->name, j);
	}
	connector->bad_edid_counter++;

out:
	kfree(block);
	return NULL;
}

int sunxi_drm_hdmi_edid_get_modes(
	struct drm_connector *connector, sink_edid_t *sink)
{
	int ret = 0;
	struct edid *edid = NULL;

	/* read edid */
	edid = sunxi_drm_do_get_edid(connector);
	if (edid) {
		/* parse or check edid */
		drm_get_displayid(connector, edid);
		drm_connector_update_edid_property(connector, edid);
		ret = drm_add_edid_modes(connector, edid);
		/* drm_edid_to_eld(connector, edid); */

		sunxi_edid_parse_cap(edid, sink);
	}

	kfree(edid);

	return ret;
}
