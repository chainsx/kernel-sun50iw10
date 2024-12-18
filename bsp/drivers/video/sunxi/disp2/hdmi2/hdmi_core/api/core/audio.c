/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include "audio.h"
#include "fc_audio.h"


typedef union iec {
	u32 frequency;
	u8 sample_size;
} iec_t;

typedef struct iec_sampling_freq {
	iec_t iec;
	u8 value;
} iec_params_t;

typedef struct channel_count {
	unsigned char channel_allocation;
	unsigned char channel_count;
} channel_count_t;

typedef struct audio_n_computation {
	u32 pixel_clock;/* KHZ */
	u32 n;
} audio_n_computation_t;

/* sampling frequency: unit:Hz */
static iec_params_t iec_original_sampling_freq_values[] = {
		{{.frequency = 44100}, 0xF},
		{{.frequency = 88200}, 0x7},
		{{.frequency = 22050}, 0xB},
		{{.frequency = 176400}, 0x3},
		{{.frequency = 48000}, 0xD},
		{{.frequency = 96000}, 0x5},
		{{.frequency = 24000}, 0x9},
		{{.frequency = 192000}, 0x1},
		{{.frequency =  8000}, 0x6},
		{{.frequency = 11025}, 0xA},
		{{.frequency = 12000}, 0x2},
		{{.frequency = 32000}, 0xC},
		{{.frequency = 16000}, 0x8},
		{{.frequency = 0},      0x0}
};

static iec_params_t iec_sampling_freq_values[] = {
		{{.frequency = 22050}, 0x4},
		{{.frequency = 44100}, 0x0},
		{{.frequency = 88200}, 0x8},
		{{.frequency = 176400}, 0xC},
		{{.frequency = 24000}, 0x6},
		{{.frequency = 48000}, 0x2},
		{{.frequency = 96000}, 0xA},
		{{.frequency = 192000}, 0xE},
		{{.frequency = 32000}, 0x3},
		{{.frequency = 768000}, 0x9},
		{{.frequency = 0},      0x0}
};

iec_params_t iec_word_length[] = {
		{{.sample_size = 16}, 0x2},
		{{.sample_size = 17}, 0xC},
		{{.sample_size = 18}, 0x4},
		{{.sample_size = 19}, 0x8},
		{{.sample_size = 20}, 0x3},
		{{.sample_size = 21}, 0xD},
		{{.sample_size = 22}, 0x5},
		{{.sample_size = 23}, 0x9},
		{{.sample_size = 24}, 0xB},
		{{.sample_size = 0},  0x0}
};

static channel_count_t channel_cnt[] = {
		{0x00, 1},
		{0x01, 2},
		{0x02, 2},
		{0x04, 2},
		{0x03, 3},
		{0x05, 3},
		{0x06, 3},
		{0x08, 3},
		{0x14, 3},
		{0x07, 4},
		{0x09, 4},
		{0x0A, 4},
		{0x0C, 4},
		{0x15, 4},
		{0x16, 4},
		{0x18, 4},
		{0x0B, 5},
		{0x0D, 5},
		{0x0E, 5},
		{0x10, 5},
		{0x17, 5},
		{0x19, 5},
		{0x1A, 5},
		{0x1C, 5},
		{0x20, 5},
		{0x22, 5},
		{0x24, 5},
		{0x26, 5},
		{0x0F, 6},
		{0x11, 6},
		{0x12, 6},
		{0x1B, 6},
		{0x1D, 6},
		{0x1E, 6},
		{0x21, 6},
		{0x23, 6},
		{0x25, 6},
		{0x27, 6},
		{0x28, 6},
		{0x2A, 6},
		{0x2C, 6},
		{0x2E, 6},
		{0x30, 6},
		{0x13, 7},
		{0x1F, 7},
		{0x29, 7},
		{0x2B, 7},
		{0x2D, 7},
		{0x2F, 7},
		{0x31, 7},
		{0, 0},
};


static audio_n_computation_t n_values_32[] = {
	{0,      4096},
	{25175, 4576},
	{25200, 4096},
	{27000, 4096},
	{27027, 4096},
	{31500, 4096},/* add */
	{33750, 4096},/* add */
	{54000, 4096},
	{54054, 4096},
	{67500, 4096},/* add */
	{74176, 11648},
	{74250, 4096},
	{928125, 8192},/* add */
	{148352, 11648},
	{148500, 4096},
	{185625, 4096},/* add */
	{296703, 5824},
	{297000, 3072},
	{371250, 6144},/* add */
	{5940000, 3072},/* add */
	{0, 0}
};

static audio_n_computation_t n_values_44p1[] = {
	{0,      6272},
	{25175, 7007},
	{25200, 6272},
	{27000, 6272},
	{27027, 6272},
	{31500, 6272},/* add */
	{33750, 6272},/* add */
	{54000, 6272},
	{54054, 6272},
	{67500, 6272},/* add */
	{74176, 17836},
	{74250, 6272},
	{92812, 6272},/* add */
	{148352, 8918},
	{148500, 6272},
	{185625, 6272},/* add */
	{296703, 4459},
	{297000, 4704},
	{371250, 4704},/* add */
	{594000, 9048},/* add */
	{0, 0}
};

static audio_n_computation_t n_values_48[] = {
	{0,      6144},
	{25175, 6864},
	{25200, 6144},
	{27000, 6144},
	{27027, 6144},
	{31500, 6144},/* add */
	{33750, 6144},/* add */
	{54000, 6144},
	{54054, 6144},
	{67500, 6144},/* add */
	{74176, 11648},
	{74250, 6144},
	{928125, 12288},/* add */
	{148352, 5824},
	{148500, 6144},
	{185625, 6144},/* add */
	{296703, 5824},
	{297000, 5120},
	{371250, 5120},/* add */
	{594000, 6144},/* add */
	{0, 0}
};


u8 audio_channel_count(hdmi_tx_dev_t *dev, audioParams_t *params)
{
	int i = 0;

	for (i = 0; channel_cnt[i].channel_count != 0; i++) {
		if (channel_cnt[i].channel_allocation ==
					params->mChannelAllocation) {
			return channel_cnt[i].channel_count;
		}
	}

	return 0;
}

u8 audio_iec_original_sampling_freq(hdmi_tx_dev_t *dev, audioParams_t *params)
{
	int i = 0;

	for (i = 0; iec_original_sampling_freq_values[i].iec.frequency != 0; i++) {
		if (params->mSamplingFrequency ==
			iec_original_sampling_freq_values[i].iec.frequency) {
			u8 value = iec_original_sampling_freq_values[i].value;
			return value;
		}
	}

	/* Not indicated */
	return 0x0;
}

u8 audio_iec_sampling_freq(hdmi_tx_dev_t *dev, audioParams_t *params)
{
	int i = 0;

	for (i = 0; iec_sampling_freq_values[i].iec.frequency != 0; i++) {
		if (params->mSamplingFrequency ==
			iec_sampling_freq_values[i].iec.frequency) {
			u8 value = iec_sampling_freq_values[i].value;
			return value;
		}
	}

	/* Not indicated */
	return 0x1;
}

u8 audio_iec_word_length(hdmi_tx_dev_t *dev, audioParams_t *params)
{
	int i = 0;

	for (i = 0; iec_word_length[i].iec.sample_size != 0; i++) {
		if (params->mSampleSize == iec_word_length[i].iec.sample_size)
			return iec_word_length[i].value;
	}

	return 0x0;
}

u8 audio_is_channel_en(hdmi_tx_dev_t *dev, audioParams_t *params, u8 channel)
{
	switch (channel) {
	case 0:
	case 1:
		AUDIO_INF("channal %d is enable\n", channel);
		return 1;
	case 2:
		AUDIO_INF("channal %d is enable\n", channel);
		return params->mChannelAllocation & BIT(0);
	case 3:
		AUDIO_INF("channal %d is enable\n", channel);
		return (params->mChannelAllocation & BIT(1)) >> 1;
	case 4:
		if (((params->mChannelAllocation > 0x03) &&
			(params->mChannelAllocation < 0x14)) ||
			((params->mChannelAllocation > 0x17) &&
			(params->mChannelAllocation < 0x20))) {
			AUDIO_INF("channal %d is enable\n", channel);
			return 1;
		} else {
			return 0;
		}
	case 5:
		if (((params->mChannelAllocation > 0x07) &&
			(params->mChannelAllocation < 0x14)) ||
			((params->mChannelAllocation > 0x1C) &&
			(params->mChannelAllocation < 0x20))) {
			AUDIO_INF("channal %d is enable\n", channel);
			return 1;
		} else {
			return 0;
		}
	case 6:
		if ((params->mChannelAllocation > 0x0B) &&
			(params->mChannelAllocation < 0x20)) {
			AUDIO_INF("channal %d is enable\n", channel);
			return 1;
		} else {
			return 0;
		}
	case 7:
		AUDIO_INF("channal %d is enable\n", channel);
		return (params->mChannelAllocation & BIT(4)) >> 4;
	default:
		return 0;
	}
}


/**********************************************
 * Internal functions
 */
static void _audio_clock_n(hdmi_tx_dev_t *dev, u32 value)
{
	/* LOG_TRACE(); */
	/* 19-bit width */
	dev_write(dev, AUD_N1, (u8)(value >> 0));
	dev_write(dev, AUD_N2, (u8)(value >> 8));
	dev_write_mask(dev, AUD_N3, AUD_N3_AUDN_MASK, (u8)(value >> 16));
	/* no shift */
	dev_write_mask(dev, AUD_CTS3, AUD_CTS3_N_SHIFT_MASK, 0);
}

static void _audio_clock_cts(hdmi_tx_dev_t *dev, u32 value)
{
	/* LOG_TRACE1(value); */
	if (value > 0) {
		/* 19-bit width */
		dev_write(dev, AUD_CTS1, (u8)(value >> 0));
		dev_write(dev, AUD_CTS2, (u8)(value >> 8));
		dev_write_mask(dev, AUD_CTS3, AUD_CTS3_AUDCTS_MASK,
							(u8)(value >> 16));
		dev_write_mask(dev, AUD_CTS3, AUD_CTS3_CTS_MANUAL_MASK, 1);
	} else {
		/* Set to automatic generation of CTS values */
		dev_write_mask(dev, AUD_CTS3, AUD_CTS3_CTS_MANUAL_MASK, 0);
	}
}

static void _audio_clock_f(hdmi_tx_dev_t *dev, u8 value)
{
	/* LOG_TRACE(); */
	dev_write_mask(dev, AUD_INPUTCLKFS, AUD_INPUTCLKFS_IFSFACTOR_MASK,
									value);
}

static u8 get_audio_clock_f(hdmi_tx_dev_t *dev)
{
	return dev_read_mask(dev, AUD_INPUTCLKFS, AUD_INPUTCLKFS_IFSFACTOR_MASK);
}

static void _audio_i2s_reset_fifo(hdmi_tx_dev_t *dev)
{
	LOG_TRACE();
	dev_write_mask(dev, AUD_CONF0, AUD_CONF0_SW_AUDIO_FIFO_RST_MASK, 1);
}

static void _audio_i2s_data_enable(hdmi_tx_dev_t *dev, u8 value)
{
	LOG_TRACE1(value);
	dev_write_mask(dev, AUD_CONF0, AUD_CONF0_I2S_IN_EN_MASK, value);
}

static void _audio_i2s_data_mode(hdmi_tx_dev_t *dev, u8 value)
{
	LOG_TRACE1(value);
	dev_write_mask(dev, AUD_CONF1, AUD_CONF1_I2S_MODE_MASK, value);
}

static void _audio_i2s_data_width(hdmi_tx_dev_t *dev, u8 value)
{
	LOG_TRACE1(value);
	dev_write_mask(dev, AUD_CONF1, AUD_CONF1_I2S_WIDTH_MASK, value);
}

static void audio_i2s_select(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, AUD_CONF0, AUD_CONF0_I2S_SELECT_MASK, bit);
}

static void audio_i2s_interrupt_mask(hdmi_tx_dev_t *dev, u8 value)
{
	LOG_TRACE1(value);
	dev_write_mask(dev, AUD_INT, AUD_INT_FIFO_FULL_MASK_MASK |
				AUD_INT_FIFO_EMPTY_MASK_MASK, value);
}

static void audio_pcuv_insert_mode(hdmi_tx_dev_t *dev, u8 value)
{
	LOG_TRACE1(value);
	dev_write_mask(dev, AUD_CONF2, AUD_CONF2_INSERT_PCUV_MASK, value);
}

static void audio_nlpcm_select(hdmi_tx_dev_t *dev, u8 value)
{
	LOG_TRACE1(value);
	dev_write_mask(dev, AUD_CONF2, AUD_CONF2_NLPCM_MASK, value);
}

static void audio_hbr_select(hdmi_tx_dev_t *dev, u8 value)
{
	LOG_TRACE1(value);
	dev_write_mask(dev, AUD_CONF2, AUD_CONF2_HBR_MASK, value);
}

static int audio_i2s_configure(hdmi_tx_dev_t *dev, audioParams_t *audio)
{
	/* select i2s interface */
	audio_i2s_select(dev, 1);
	/* i2s data off */
	_audio_i2s_data_enable(dev, 0x0);

	if (audio->mCodingType == PCM) {
		audio_pcuv_insert_mode(dev, 1);
		audio_hbr_select(dev, 0);
		audio_nlpcm_select(dev, 0);
	} else if ((audio->mCodingType == DTS_HD) ||
			(audio->mCodingType == MAT)) {
		/* HBR mode, I2S input is ICE60958 frame,
		you must trans the ICE61937 frame to 60958 frame
		before it be sent to i2s  */
		/* HBR mode: the frame rate>192Khz, the ACR should be fs/4 */
		audio_pcuv_insert_mode(dev, 0);
		audio_hbr_select(dev, 1);
		audio_nlpcm_select(dev, 1);
	} else {
		/* NO-PCM  mode, I2S input is ICE60958 frame, you must trans the
		ICE61937 frame to 60958 frame before it be sent to i2s */
		/* NO-PCM  mode: the frame rate<=192Khz, the ACR should be fs */
		audio_pcuv_insert_mode(dev, 0);
		audio_hbr_select(dev, 0);
		audio_nlpcm_select(dev, 1);

	}

	if (audio->mCodingType != PCM)
		_audio_i2s_data_width(dev, 24);
	else
		_audio_i2s_data_width(dev, audio->mSampleSize);
	/* ATTENTION: fixed I2S data mode (standard) */
	_audio_i2s_data_mode(dev, 0x0);
	audio_i2s_interrupt_mask(dev, 3);
	_audio_i2s_reset_fifo(dev);
	mc_audio_i2s_reset(dev, 0);

	return 0;
}

static int configure_i2s(hdmi_tx_dev_t *dev, audioParams_t *audio)
{
	audio_i2s_configure(dev, audio);

	switch (audio->mClockFsFactor) {
	case 64:
		_audio_clock_f(dev, 4);
		break;
	case 128:
		_audio_clock_f(dev, 0);
		break;
	case 256:
		_audio_clock_f(dev, 1);
		break;
	case 512:
		_audio_clock_f(dev, 2);
		break;
	default:
		_audio_clock_f(dev, 0);
		pr_err("Fs Factor [%d] not supported\n",
					 audio->mClockFsFactor);
		return FALSE;
		break;
	}

	return TRUE;
}

static u16 get_audio_fsFactor(hdmi_tx_dev_t *dev)
{
	u8 fac = get_audio_clock_f(dev);
	u16 fsFac = 0;

	switch (fac) {
	case 4:
		fsFac = 64;
		break;
	case 0:
		fsFac = 128;
		break;
	case 1:
		fsFac = 256;
		break;
	case 2:
		fsFac = 512;
		break;
	default:
		pr_err("Error: error audio clock factor\n");
		fsFac = 0;
		break;
	}

	return fsFac;
}

static void audio_spdif_interrupt_mask(hdmi_tx_dev_t *dev, u8 value)
{
	LOG_TRACE1(value);
	dev_write_mask(dev, AUD_SPDIFINT,
			AUD_SPDIFINT_SPDIF_FIFO_FULL_MASK_MASK |
			AUD_SPDIFINT_SPDIF_FIFO_EMPTY_MASK_MASK, value);
}

#if 0
static void halAudioMultistream_MetaDataPacket_Descriptor_X(hdmi_tx_dev_t *dev,
		u8 descrNr, audioMetaDataDescriptor_t *mAudioMetaDataDescriptor)
{
	u8 pb = 0x00;

	dev_write(dev, FC_AMP_PB,
		(u8)(mAudioMetaDataDescriptor->mMultiviewRightLeft & 0x03));
	pb = ((mAudioMetaDataDescriptor->mLC_Valid & 0x01) << 7);
	pb = pb | ((mAudioMetaDataDescriptor->mSuppl_A_Valid & 0x01) << 4);
	pb = pb | ((mAudioMetaDataDescriptor->mSuppl_A_Mixed & 0x01) << 3);
	pb = pb | ((mAudioMetaDataDescriptor->mSuppl_A_Type & 0x03) << 0);
	AUDIO_INF("NOTICE:  pb = 0x%x\n", pb);
	dev_write(dev, (FC_AMP_PB + (((0x05 * descrNr) + 0x01)*4)), (u8)(pb));
	if (mAudioMetaDataDescriptor->mLC_Valid == 1) {
		dev_write(dev, FC_AMP_PB + (((0x05 * descrNr) + 0x02)*4),
		((u8)(mAudioMetaDataDescriptor->mLanguage_Code[0])) & 0xff);
		dev_write(dev, FC_AMP_PB + (((0x05 * descrNr) + 0x03)*4),
		((u8)(mAudioMetaDataDescriptor->mLanguage_Code[1])) & 0xff);
		dev_write(dev, FC_AMP_PB + (((0x05 * descrNr) + 0x04)*4),
		((u8)(mAudioMetaDataDescriptor->mLanguage_Code[2])) & 0xff);
	}
	AUDIO_INF("NOTICE:  body AMP descriptor 0 0x%x\n",
			FC_AMP_PB + (((0x05 * descrNr) + 0x00)*4));
	AUDIO_INF("NOTICE: body AMP descriptor 0 0x%x\n",
			FC_AMP_PB + (((0x05 * descrNr) + 0x01)*4));
	AUDIO_INF("NOTICE:  body AMP descriptor 0 0x%x\n",
			FC_AMP_PB + (((0x05 * descrNr) + 0x02)*4));
	AUDIO_INF("NOTICE:  body AMP descriptor 0 0x%x\n",
			FC_AMP_PB + (((0x05 * descrNr) + 0x03)*4));
	AUDIO_INF("NOTICE:  body AMP descriptor 0 0x%x\n",
			FC_AMP_PB + (((0x05 * descrNr) + 0x04)*4));
}

void halAudioMultistream_MetaDataPacket_Header(hdmi_tx_dev_t *dev,
				audioMetaDataPacket_t *mAudioMetaDataPckt)
{
	dev_write(dev, FC_AMP_HB1,
		(u8)(mAudioMetaDataPckt->mAudioMetaDataHeader.m3dAudio));
	dev_write(dev, FC_AMP_HB2,
		(u8)(((mAudioMetaDataPckt->mAudioMetaDataHeader.mNumViews) |
		(mAudioMetaDataPckt->mAudioMetaDataHeader.mNumAudioStreams) << 2)));
	AUDIO_INF("NOTICE: AMP HB1 0x%x\n", FC_AMP_HB1);
	AUDIO_INF("NOTICE: AMP HB1 data 0x%x\n",
		(u8)(mAudioMetaDataPckt->mAudioMetaDataHeader.m3dAudio));
	AUDIO_INF("NOTICE: AMP HB2 0x%x\n", FC_AMP_HB2);
	AUDIO_INF("NOTICE: AMP HB2 data 0x%x\n",
		(u8)(((mAudioMetaDataPckt->mAudioMetaDataHeader.mNumViews) |
		(mAudioMetaDataPckt->mAudioMetaDataHeader.mNumAudioStreams) << 2)));
}

void halAudioMultistream_MetaDataPacketBody(hdmi_tx_dev_t *dev,
			audioMetaDataPacket_t *mAudioMetaDataPacket)
{
	u8 cnt = 0;

	while (cnt <= (mAudioMetaDataPacket->mAudioMetaDataHeader.mNumAudioStreams + 1)) {
		AUDIO_INF("NOTICE: (mAudioMetaDataPacket->mAudioMetaDataHeader.mNumAudioStreams + 1) = %d\n",
		(mAudioMetaDataPacket->mAudioMetaDataHeader.mNumAudioStreams + 1));
		AUDIO_INF("NOTICE:  audiometadata packet descriptor %d\n", cnt);
		halAudioMultistream_MetaDataPacket_Descriptor_X(dev, 0,
			&(mAudioMetaDataPacket->mAudioMetaDataDescriptor[cnt]));
		cnt++;
	}
}
#endif

/**
 * Mute audio.
 * Stop sending audio packets
 * @param baseAddr base Address of controller
 * @param state:  1 to enable/0 to disable the mute
 * @return TRUE if successful
 */
int audio_mute(hdmi_tx_dev_t *dev, u8 state)
{
	/* audio mute priority: AVMUTE, sample flat, validity */
	/* AVMUTE also mutes video */
	if (state)
		fc_audio_mute(dev);
	else
		fc_audio_unmute(dev);

	return TRUE;
}

/**
 * Initial set up of package and prepare it to be configured.
 * Set audio mute to on.
 * @param baseAddr base Address of controller
 * @return TRUE if successful
 */
int audio_Initialize(hdmi_tx_dev_t *dev)
{
	LOG_TRACE();

	/* Mask all interrupts */
	audio_i2s_interrupt_mask(dev, 0x3);
	audio_spdif_interrupt_mask(dev, 0x3);

	return audio_mute(dev,  1);
}

static u32 audio_ComputeN(hdmi_tx_dev_t *dev, u32 freq, u32 pixelClk)
{
	int i = 0;
	u32 n = 0;
	audio_n_computation_t *n_values = NULL;
	int multiplier_factor = 1;

	if ((freq == 64000) || (freq == 882000) || (freq == 96000))
		multiplier_factor = 2;
	else if ((freq == 128000) || (freq == 176400) || (freq == 192000))
		multiplier_factor = 4;
	else if ((freq == 256000) || (freq == 3528000) || (freq == 384000))
		multiplier_factor = 8;

	if (32000 == (freq/multiplier_factor)) {
		n_values = n_values_32;
	} else if (44100 == (freq/multiplier_factor)) {
		n_values = n_values_44p1;
	} else if (48000 == (freq/multiplier_factor)) {
		n_values = n_values_48;
	} else {
		pr_err("Error:Could not compute N values\n");
		pr_err("Audio Frequency %dhz\n",  freq/1000);
		pr_err("Pixel Clock %dkhz\n",  pixelClk);
		pr_err("Multiplier factor %d\n", multiplier_factor);
		return FALSE;
	}

	for (i = 0; n_values[i].n != 0; i++) {
		if (pixelClk == n_values[i].pixel_clock) {
			n = n_values[i].n * multiplier_factor;
			AUDIO_INF("DEBUG:Audio N value = %d\n", n);
			return n;
		}
	}

	n = n_values[0].n * multiplier_factor;

	pr_warn("Warn:Audio N value default = %d\n", n);

	return n;
}
int audio_Configure(hdmi_tx_dev_t *dev, audioParams_t *audio)
{
	u32 n = 0;

	LOG_TRACE();

	if ((dev == NULL) || (audio == NULL)) {
		pr_info("ERROR:Improper function arguments\n");
		return FALSE;
	}

	if (dev->snps_hdmi_ctrl.audio_on == 0) {
		pr_info("WARN:audio is not on\n");
		return TRUE;
	}

	if (dev->snps_hdmi_ctrl.hdmi_on == 0) {
		pr_info("WARN:avi mode,audio is not configured\n");
		return TRUE;
	}

	if ((audio->mCodingType == ONE_BIT_AUDIO) || (audio->mCodingType == DST)) {
		pr_info("ERROR: hdmi does not support this coding type:%d\n",
							audio->mCodingType);
		return FALSE;

	}

	if ((dev->snps_hdmi_ctrl.pixel_clock < 74250) &&
		((audio->mChannelNum * audio->mSamplingFrequency) > 384000)) {
		pr_info("ERROR: hdmi do not support this audio framerate on this video format\n");
		return FALSE;
	}
	/* Set PCUV info from external source */
	audio->mGpaInsertPucv = 1;
	audio_mute(dev, 1);

	/* Configure Frame Composer audio parameters */
	fc_audio_config(dev, audio);

	audio->mClockFsFactor = 64;
	if (audio->mInterfaceType == I2S) {
		if (configure_i2s(dev, audio) == FALSE) {
			pr_info("ERROR:Configuring I2S audio\n");
			return FALSE;
		}
	} else if (audio->mInterfaceType != I2S) {
		pr_info("ERROR:the audio interface is not supported\n");
		return FALSE;
	}

	n = audio_ComputeN(dev, audio->mSamplingFrequency,
				dev->snps_hdmi_ctrl.tmds_clk);

	_audio_clock_n(dev, n);
	_audio_clock_cts(dev, 0);

	if (!(audio->mChannelNum % 2))
		_audio_i2s_data_enable(dev, (1 << (audio->mChannelNum / 2)) - 1);
	else
		_audio_i2s_data_enable(dev, (1 << ((audio->mChannelNum + 1) / 2)) - 1);

	audio_mute(dev, 0);
	/* Configure audio info frame packets */
	fc_audio_info_config(dev, audio);
	return TRUE;
}


u32 _audio_clock_n_get(hdmi_tx_dev_t *dev)
{
	return (u32)(dev_read(dev, AUD_N1) |
		(dev_read(dev, AUD_N2) << 8) |
		(dev_read_mask(dev, AUD_N3, AUD_N3_AUDN_MASK) << 16));

}

u32 audio_iec_sampling_freq_get(hdmi_tx_dev_t *dev)
{
	int i = 0;

	for (i = 0; iec_sampling_freq_values[i].iec.frequency != 0; i++) {
		if (fc_iec_sampling_freq_get(dev) ==
			iec_sampling_freq_values[i].value) {
			return iec_sampling_freq_values[i].iec.frequency;
		}
	}

	/* Not indicated */
	return 0;
}



u8 audio_iec_word_length_get(hdmi_tx_dev_t *dev)
{
	int i = 0;

	for (i = 0; iec_word_length[i].iec.sample_size != 0; i++) {
		if (fc_iec_word_length_get(dev) == iec_word_length[i].value)
			return iec_word_length[i].iec.sample_size;
	}

	return 0x0;
}

ssize_t audio_dump(hdmi_tx_dev_t *dev, char *buf)
{
	ssize_t n = 0;

	n += sprintf(buf + n, "audio mute:%d\n\n", get_fc_audio_mute(dev));
	n += sprintf(buf + n, "audio mute:%d\n\n", get_audio_fsFactor(dev));

	return n;
}
