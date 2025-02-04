/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include "fc_audio.h"

static void fc_packet_layout(hdmi_tx_dev_t *dev, u8 bit);
static void fc_validity_right(hdmi_tx_dev_t *dev, u8 bit, unsigned channel);
static void fc_validity_left(hdmi_tx_dev_t *dev, u8 bit, unsigned channel);
static void fc_user_right(hdmi_tx_dev_t *dev, u8 bit, unsigned channel);
static void fc_user_left(hdmi_tx_dev_t *dev, u8 bit, unsigned channel);
static void fc_iec_cgmsA(hdmi_tx_dev_t *dev, u8 value);
static void fc_iec_copyright(hdmi_tx_dev_t *dev, u8 bit);
static void fc_iec_category_code(hdmi_tx_dev_t *dev, u8 value);
static void fc_iec_pcm_mode(hdmi_tx_dev_t *dev, u8 value);
static void fc_iec_source(hdmi_tx_dev_t *dev, u8 value);
static void fc_iec_channel_right(hdmi_tx_dev_t *dev, u8 value,
							unsigned channel);
static void fc_iec_channel_left(hdmi_tx_dev_t *dev, u8 value,
							unsigned channel);
static void fc_iec_clock_accuracy(hdmi_tx_dev_t *dev, u8 value);
static void fc_iec_sampling_freq(hdmi_tx_dev_t *dev, u8 value);
static void fc_iec_original_sampling_freq(hdmi_tx_dev_t *dev, u8 value);
static void fc_iec_word_length(hdmi_tx_dev_t *dev, u8 value);


static void fc_channel_count(hdmi_tx_dev_t *dev, u8 noOfChannels)
{
	LOG_TRACE1(noOfChannels);
	dev_write_mask(dev, FC_AUDICONF0, FC_AUDICONF0_CC_MASK, noOfChannels);
}

static void fc_sample_freq(hdmi_tx_dev_t *dev, u8 sf)
{
	LOG_TRACE1(sf);
	dev_write_mask(dev, FC_AUDICONF1, FC_AUDICONF1_SF_MASK, sf);
}

static void fc_allocate_channels(hdmi_tx_dev_t *dev, u8 ca)
{
	LOG_TRACE1(ca);
	dev_write(dev, FC_AUDICONF2, ca);
}

static void fc_level_shift_value(hdmi_tx_dev_t *dev, u8 lsv)
{
	LOG_TRACE1(lsv);
	dev_write_mask(dev, FC_AUDICONF3, FC_AUDICONF3_LSV_MASK, lsv);
}

static void fc_down_mix_inhibit(hdmi_tx_dev_t *dev, u8 prohibited)
{
	LOG_TRACE1(prohibited);
	dev_write_mask(dev, FC_AUDICONF3, FC_AUDICONF3_DM_INH_MASK,
							(prohibited ? 1 : 0));
}

static void fc_coding_type(hdmi_tx_dev_t *dev, u8 codingType)
{
	LOG_TRACE1(codingType);
	dev_write_mask(dev, FC_AUDICONF0, FC_AUDICONF0_CT_MASK, codingType);
}

static void fc_sampling_size(hdmi_tx_dev_t *dev, u8 ss)
{
	LOG_TRACE1(ss);
	dev_write_mask(dev, FC_AUDICONF1, FC_AUDICONF1_SS_MASK, ss);
}

void fc_audio_info_config(hdmi_tx_dev_t *dev, audioParams_t *audio)
{
	u8 channel_count = audio_channel_count(dev, audio);

	LOG_TRACE();

	fc_channel_count(dev, channel_count);
	fc_allocate_channels(dev, audio->mChannelAllocation);
	fc_level_shift_value(dev, audio->mLevelShiftValue);
	fc_down_mix_inhibit(dev, audio->mDownMixInhibitFlag);

	AUDIO_INF("DEBUG:Audio channel count = %d\n", channel_count);
	AUDIO_INF("DEBUG:Audio channel allocation = %d\n",
						audio->mChannelAllocation);
	AUDIO_INF("DEBUG:Audio level shift = %d\n",
						audio->mLevelShiftValue);

	if ((audio->mCodingType == ONE_BIT_AUDIO) ||
				(audio->mCodingType == DST)) {
		u32 sampling_freq = audio->mSamplingFrequency;

		/* Audio InfoFrame sample frequency when OBA or DST */
		if (sampling_freq == 32000)
			fc_sample_freq(dev, 1);
		else if (sampling_freq == 44100)
			fc_sample_freq(dev, 2);
		else if (sampling_freq == 48000)
			fc_sample_freq(dev, 3);
		else if (sampling_freq == 88200)
			fc_sample_freq(dev, 4);
		else if (sampling_freq == 96000)
			fc_sample_freq(dev, 5);
		else if (sampling_freq == 176400)
			fc_sample_freq(dev, 6);
		else if (sampling_freq == 192000)
			fc_sample_freq(dev, 7);
		else
			fc_sample_freq(dev, 0);

	} else {
		fc_sample_freq(dev, 0);/* otherwise refer to stream header (0) */
	}

	fc_coding_type(dev, 0);	/* for HDMI refer to stream header  (0) */
	fc_sampling_size(dev, 0);/* for HDMI refer to stream header  (0) */
}



static void fc_packet_sample_flat(hdmi_tx_dev_t *dev, u8 value)
{
	LOG_TRACE1(value);
	dev_write_mask(dev, FC_AUDSCONF, FC_AUDSCONF_AUD_PACKET_SAMPFLT_MASK,
									value);
}

static u8 get_fc_packet_sample_flat(hdmi_tx_dev_t *dev)
{
	return dev_read_mask(dev, FC_AUDSCONF, FC_AUDSCONF_AUD_PACKET_SAMPFLT_MASK);
}

static void fc_packet_layout(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, FC_AUDSCONF, FC_AUDSCONF_AUD_PACKET_LAYOUT_MASK,
									bit);
}

static void fc_validity_right(hdmi_tx_dev_t *dev, u8 bit, unsigned channel)
{
	LOG_TRACE2(bit, channel);
	if (channel < 4)
		dev_write_mask(dev, FC_AUDSV, (1 << (4 + channel)), bit);
	else
		pr_info("ERROR:invalid channel number\n");
}

static void fc_validity_left(hdmi_tx_dev_t *dev, u8 bit, unsigned channel)
{
	LOG_TRACE2(bit, channel);
	if (channel < 4)
		dev_write_mask(dev, FC_AUDSV, (1 << channel), bit);
	else
		pr_info("ERROR:invalid channel number: %d", channel);
}

static void fc_user_right(hdmi_tx_dev_t *dev, u8 bit, unsigned channel)
{
	LOG_TRACE2(bit, channel);
	if (channel < 4)
		dev_write_mask(dev, FC_AUDSU, (1 << (4 + channel)), bit);
	else
		pr_info("ERROR:invalid channel number: %d\n", channel);
}

static void fc_user_left(hdmi_tx_dev_t *dev, u8 bit, unsigned channel)
{
	LOG_TRACE2(bit, channel);
	if (channel < 4)
		dev_write_mask(dev, FC_AUDSU, (1 << channel), bit);
	else
		pr_info("ERRORinvalid channel number: %d\n", channel);
}

static void fc_iec_cgmsA(hdmi_tx_dev_t *dev, u8 value)
{
	LOG_TRACE1(value);
	dev_write_mask(dev, FC_AUDSCHNL0, FC_AUDSCHNL0_OIEC_CGMSA_MASK, value);
}

static void fc_iec_copyright(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, FC_AUDSCHNL0,
			FC_AUDSCHNL0_OIEC_COPYRIGHT_MASK, bit);
}

static void fc_iec_category_code(hdmi_tx_dev_t *dev, u8 value)
{
	LOG_TRACE1(value);
	dev_write(dev, FC_AUDSCHNL1, value);
}

static void fc_iec_pcm_mode(hdmi_tx_dev_t *dev, u8 value)
{
	LOG_TRACE1(value);
	dev_write_mask(dev, FC_AUDSCHNL2,
				FC_AUDSCHNL2_OIEC_PCMAUDIOMODE_MASK, value);
}

static void fc_iec_source(hdmi_tx_dev_t *dev, u8 value)
{
	LOG_TRACE1(value);
	dev_write_mask(dev, FC_AUDSCHNL2,
			FC_AUDSCHNL2_OIEC_SOURCENUMBER_MASK, value);
}

static void fc_iec_channel_right(hdmi_tx_dev_t *dev, u8 value, unsigned channel)
{
	LOG_TRACE2(value, channel);
	if (channel == 0)
		dev_write_mask(dev, FC_AUDSCHNL3,
				FC_AUDSCHNL3_OIEC_CHANNELNUMCR0_MASK, value);
	else if (channel == 1)
		dev_write_mask(dev, FC_AUDSCHNL3,
				FC_AUDSCHNL3_OIEC_CHANNELNUMCR1_MASK, value);
	else if (channel == 2)
		dev_write_mask(dev, FC_AUDSCHNL4,
				FC_AUDSCHNL4_OIEC_CHANNELNUMCR2_MASK, value);
	else if (channel == 3)
		dev_write_mask(dev, FC_AUDSCHNL4,
				FC_AUDSCHNL4_OIEC_CHANNELNUMCR3_MASK, value);
	else
		pr_err("ERROR:invalid channel number: %d\n", channel);
}

static void fc_iec_channel_left(hdmi_tx_dev_t *dev, u8 value, unsigned channel)
{
	LOG_TRACE2(value, channel);
	if (channel == 0)
		dev_write_mask(dev, FC_AUDSCHNL5,
				FC_AUDSCHNL5_OIEC_CHANNELNUMCL0_MASK, value);
	else if (channel == 1)
		dev_write_mask(dev, FC_AUDSCHNL5,
				FC_AUDSCHNL5_OIEC_CHANNELNUMCL1_MASK, value);
	else if (channel == 2)
		dev_write_mask(dev, FC_AUDSCHNL6,
				FC_AUDSCHNL6_OIEC_CHANNELNUMCL2_MASK, value);
	else if (channel == 3)
		dev_write_mask(dev, FC_AUDSCHNL6,
				FC_AUDSCHNL6_OIEC_CHANNELNUMCL3_MASK, value);
	else
		pr_info("ERROR:invalid channel number: %d", channel);
}

static void fc_iec_clock_accuracy(hdmi_tx_dev_t *dev, u8 value)
{
	LOG_TRACE1(value);
	dev_write_mask(dev, FC_AUDSCHNL7,
			FC_AUDSCHNL7_OIEC_CLKACCURACY_MASK, value);
}

static void fc_iec_sampling_freq(hdmi_tx_dev_t *dev, u8 value)
{
	LOG_TRACE1(value);
	dev_write_mask(dev, FC_AUDSCHNL7,
			FC_AUDSCHNL7_OIEC_SAMPFREQ_MASK, value);
}

static void fc_iec_original_sampling_freq(hdmi_tx_dev_t *dev, u8 value)
{
	LOG_TRACE1(value);
	dev_write_mask(dev, FC_AUDSCHNL8,
			FC_AUDSCHNL8_OIEC_ORIGSAMPFREQ_MASK, value);
}

static void fc_iec_word_length(hdmi_tx_dev_t *dev, u8 value)
{
	LOG_TRACE1(value);
	dev_write_mask(dev, FC_AUDSCHNL8,
			FC_AUDSCHNL8_OIEC_WORDLENGTH_MASK, value);
}

void fc_audio_config(hdmi_tx_dev_t *dev, audioParams_t *audio)
{
	int i = 0;
	u8 channel_count = audio_channel_count(dev, audio);
	/* u8 channel_count = audio->mChannelNum; */
	u8 data = 0;

	/* More than 2 channels => layout 1 else layout 0 */
	if ((channel_count + 1) > 2)
		fc_packet_layout(dev, 1);
	else
		fc_packet_layout(dev, 0);

	/* iec validity and user bits (IEC 60958-1) */
	for (i = 0; i < 4; i++) {
		/* audio_is_channel_en considers left as 1 channel and
		 * right as another (+1), hence the x2 factor in the following */
		/* validity bit is 0 when reliable, which is !IsChannelEn */
		u8 channel_enable = audio_is_channel_en(dev, audio, (2 * i));
		fc_validity_right(dev, !channel_enable, i);

		channel_enable = audio_is_channel_en(dev, audio, (2 * i) + 1);
		fc_validity_left(dev, !channel_enable, i);

		fc_user_right(dev, 1, i);
		fc_user_left(dev, 1, i);
	}

	if (audio->mCodingType != PCM)
		return;
	/* IEC - not needed if non-linear PCM */
	fc_iec_cgmsA(dev, audio->mIecCgmsA);
	fc_iec_copyright(dev, audio->mIecCopyright ? 0 : 1);
	fc_iec_category_code(dev, audio->mIecCategoryCode);
	fc_iec_pcm_mode(dev, audio->mIecPcmMode);
	fc_iec_source(dev, audio->mIecSourceNumber);

	for (i = 0; i < 4; i++) {	/* 0, 1, 2, 3 */
		fc_iec_channel_left(dev, 2 * i + 1, i);	/* 1, 3, 5, 7 */
		fc_iec_channel_right(dev, 2 * (i + 1), i);	/* 2, 4, 6, 8 */
	}

	fc_iec_clock_accuracy(dev, audio->mIecClockAccuracy);

	data = audio_iec_sampling_freq(dev, audio);
	fc_iec_sampling_freq(dev, data);

	data = audio_iec_original_sampling_freq(dev, audio);
	fc_iec_original_sampling_freq(dev, data);

	data = audio_iec_word_length(dev, audio);
	fc_iec_word_length(dev, data);
}

void fc_audio_mute(hdmi_tx_dev_t *dev)
{
	fc_packet_sample_flat(dev, 0xF);
}

void fc_audio_unmute(hdmi_tx_dev_t *dev)
{
	fc_packet_sample_flat(dev, 0);
}

u8 get_fc_audio_mute(hdmi_tx_dev_t *dev)
{
	return get_fc_packet_sample_flat(dev);
}

u8 fc_packet_layout_get(hdmi_tx_dev_t *dev)
{
	LOG_TRACE();
	return dev_read_mask(dev,
		FC_AUDSCONF,
		FC_AUDSCONF_AUD_PACKET_LAYOUT_MASK);
}



u8 fc_channel_count_get(hdmi_tx_dev_t *dev)
{
	LOG_TRACE();
	return dev_read_mask(dev, FC_AUDICONF0, FC_AUDICONF0_CC_MASK);
}

u8 fc_iec_word_length_get(hdmi_tx_dev_t *dev)
{
	LOG_TRACE();
	return dev_read_mask(dev, FC_AUDSCHNL8,
			FC_AUDSCHNL8_OIEC_WORDLENGTH_MASK);
}

u8 fc_iec_sampling_freq_get(hdmi_tx_dev_t *dev)
{
	LOG_TRACE();
	return dev_read_mask(dev,  FC_AUDSCHNL7,
			FC_AUDSCHNL7_OIEC_SAMPFREQ_MASK);
}

