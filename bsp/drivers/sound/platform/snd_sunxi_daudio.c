/*
 * sound\soc\sunxi\snd_sunxi_daudio.c
 * (C) Copyright 2021-2025
 * AllWinner Technology Co., Ltd. <www.allwinnertech.com>
 * Dby <dby@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/regulator/consumer.h>
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/device.h>
#include <linux/ioport.h>
#include <linux/regmap.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/of_device.h>
#include <sound/soc.h>
#include <sound/tlv.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>

#include "snd_sunxi_log.h"
#include "snd_sunxi_pcm.h"
#include "snd_sunxi_rxsync.h"
#include "snd_sunxi_hdmi.h"
#include "snd_sunxi_daudio.h"

#define HLOG		"DAUDIO"
#define DRV_NAME	"sunxi-snd-plat-daudio"

/* for reg debug */
#define REG_LABEL(constant)	{#constant, constant, 0}
#define REG_LABEL_END		{NULL, 0, 0}

struct audio_reg_label {
	const char *name;
	const unsigned int address;
	unsigned int value;
};

static struct audio_reg_label sunxi_reg_labels[] = {
	REG_LABEL(SUNXI_DAUDIO_CTL),
	REG_LABEL(SUNXI_DAUDIO_FMT0),
	REG_LABEL(SUNXI_DAUDIO_FMT1),
	REG_LABEL(SUNXI_DAUDIO_INTSTA),
	/* REG_LABEL(SUNXI_DAUDIO_RXFIFO), */
	REG_LABEL(SUNXI_DAUDIO_FIFOCTL),
	REG_LABEL(SUNXI_DAUDIO_FIFOSTA),
	REG_LABEL(SUNXI_DAUDIO_INTCTL),
	/* REG_LABEL(SUNXI_DAUDIO_TXFIFO), */
	REG_LABEL(SUNXI_DAUDIO_CLKDIV),
	REG_LABEL(SUNXI_DAUDIO_TXCNT),
	REG_LABEL(SUNXI_DAUDIO_RXCNT),

	REG_LABEL(SUNXI_DAUDIO_CHCFG),
	REG_LABEL(SUNXI_DAUDIO_TX0CHSEL),
	REG_LABEL(SUNXI_DAUDIO_TX1CHSEL),
	REG_LABEL(SUNXI_DAUDIO_TX2CHSEL),
	REG_LABEL(SUNXI_DAUDIO_TX3CHSEL),
	REG_LABEL(SUNXI_DAUDIO_TX0CHMAP0),
	REG_LABEL(SUNXI_DAUDIO_TX0CHMAP1),
	REG_LABEL(SUNXI_DAUDIO_TX1CHMAP0),
	REG_LABEL(SUNXI_DAUDIO_TX1CHMAP1),
	REG_LABEL(SUNXI_DAUDIO_TX2CHMAP0),
	REG_LABEL(SUNXI_DAUDIO_TX2CHMAP1),
	REG_LABEL(SUNXI_DAUDIO_TX3CHMAP0),
	REG_LABEL(SUNXI_DAUDIO_TX3CHMAP1),
	REG_LABEL(SUNXI_DAUDIO_RXCHSEL),
	REG_LABEL(SUNXI_DAUDIO_RXCHMAP0),
	REG_LABEL(SUNXI_DAUDIO_RXCHMAP1),
	REG_LABEL(SUNXI_DAUDIO_RXCHMAP2),
	REG_LABEL(SUNXI_DAUDIO_RXCHMAP3),

	REG_LABEL(SUNXI_DAUDIO_DEBUG),
	REG_LABEL(SUNXI_DAUDIO_REV),
	REG_LABEL_END,
};

static struct audio_reg_label sun8iw11_reg_labels[] = {
	REG_LABEL(SUNXI_DAUDIO_CTL),
	REG_LABEL(SUNXI_DAUDIO_FMT0),
	REG_LABEL(SUNXI_DAUDIO_FMT1),
	REG_LABEL(SUNXI_DAUDIO_INTSTA),
	/* REG_LABEL(SUNXI_DAUDIO_RXFIFO), */
	REG_LABEL(SUNXI_DAUDIO_FIFOCTL),
	REG_LABEL(SUNXI_DAUDIO_FIFOSTA),
	REG_LABEL(SUNXI_DAUDIO_INTCTL),
	/* REG_LABEL(SUNXI_DAUDIO_TXFIFO), */
	REG_LABEL(SUNXI_DAUDIO_CLKDIV),
	REG_LABEL(SUNXI_DAUDIO_TXCNT),
	REG_LABEL(SUNXI_DAUDIO_RXCNT),

	REG_LABEL(SUNXI_DAUDIO_8SLOT_CHCFG),
	REG_LABEL(SUNXI_DAUDIO_8SLOT_TX0CHSEL),
	REG_LABEL(SUNXI_DAUDIO_8SLOT_TX1CHSEL),
	REG_LABEL(SUNXI_DAUDIO_8SLOT_TX2CHSEL),
	REG_LABEL(SUNXI_DAUDIO_8SLOT_TX3CHSEL),
	REG_LABEL(SUNXI_DAUDIO_8SLOT_TX0CHMAP),
	REG_LABEL(SUNXI_DAUDIO_8SLOT_TX1CHMAP),
	REG_LABEL(SUNXI_DAUDIO_8SLOT_TX2CHMAP),
	REG_LABEL(SUNXI_DAUDIO_8SLOT_TX3CHMAP),
	REG_LABEL(SUNXI_DAUDIO_8SLOT_RXCHSEL),
	REG_LABEL(SUNXI_DAUDIO_8SLOT_RXCHMAP),
	REG_LABEL_END,
};

static struct regmap_config g_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.max_register = SUNXI_DAUDIO_MAX_REG,
	.cache_type = REGCACHE_NONE,
};

static int snd_sunxi_save_reg(struct regmap *regmap, struct audio_reg_label *reg_labels);
static int snd_sunxi_echo_reg(struct regmap *regmap, struct audio_reg_label *reg_labels);
static int snd_sunxi_rglt_init(struct platform_device *pdev, struct sunxi_daudio_rglt *rglt);
static void snd_sunxi_rglt_exit(struct platform_device *pdev, struct sunxi_daudio_rglt *rglt);
static int snd_sunxi_rglt_enable(struct platform_device *pdev, struct sunxi_daudio_rglt *rglt);
static void snd_sunxi_rglt_disable(struct platform_device *pdev, struct sunxi_daudio_rglt *rglt);

static void sunxi_rx_sync_enable(void *data, bool enable);

static int sunxi_daudio_set_ch_en(struct sunxi_daudio *daudio, int stream, unsigned int channels)
{
	struct regmap *regmap = daudio->mem.regmap;
	unsigned int channels_en_slot[16] = {
		0x0001, 0x0003, 0x0007, 0x000f, 0x001f, 0x003f, 0x007f, 0x00ff,
		0x01ff, 0x03ff, 0x07ff, 0x0fff, 0x1fff, 0x3fff, 0x7fff, 0xffff
	};

	if (IS_ERR_OR_NULL(daudio) || channels < 1 || channels > 16)
		return -EINVAL;

	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		regmap_update_bits(regmap, SUNXI_DAUDIO_CHCFG, 0xF << TX_SLOT_NUM,
				   (channels - 1) << TX_SLOT_NUM);

		regmap_update_bits(regmap, SUNXI_DAUDIO_TX0CHSEL, 0xF << TX_CHSEL,
				   (channels - 1) << TX_CHSEL);
		regmap_update_bits(regmap, SUNXI_DAUDIO_TX1CHSEL, 0xF << TX_CHSEL,
				   (channels - 1) << TX_CHSEL);
		regmap_update_bits(regmap, SUNXI_DAUDIO_TX2CHSEL, 0xF << TX_CHSEL,
				   (channels - 1) << TX_CHSEL);
		regmap_update_bits(regmap, SUNXI_DAUDIO_TX3CHSEL, 0xF << TX_CHSEL,
				   (channels - 1) << TX_CHSEL);

		regmap_update_bits(regmap, SUNXI_DAUDIO_TX0CHSEL, 0xFFFF << TX_CHEN,
				   channels_en_slot[channels - 1] << TX_CHEN);
		regmap_update_bits(regmap, SUNXI_DAUDIO_TX1CHSEL, 0xFFFF << TX_CHEN,
				   channels_en_slot[channels - 1] << TX_CHEN);
		regmap_update_bits(regmap, SUNXI_DAUDIO_TX2CHSEL, 0xFFFF << TX_CHEN,
				   channels_en_slot[channels - 1] << TX_CHEN);
		regmap_update_bits(regmap, SUNXI_DAUDIO_TX3CHSEL, 0xFFFF << TX_CHEN,
				   channels_en_slot[channels - 1] << TX_CHEN);
	} else {
		regmap_update_bits(regmap, SUNXI_DAUDIO_CHCFG, 0xF << RX_SLOT_NUM,
				   (channels - 1) << RX_SLOT_NUM);
		regmap_update_bits(regmap, SUNXI_DAUDIO_RXCHSEL, 0xF << RX_CHSEL,
				   (channels - 1) << RX_CHSEL);
	}

	return 0;
}

static int sun8iw11_daudio_set_ch_en(struct sunxi_daudio *daudio, int stream,
				      unsigned int channels)
{
	struct regmap *regmap = daudio->mem.regmap;
	unsigned int channels_en_slot[8] = {
		0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff,
	};

	if (IS_ERR_OR_NULL(daudio) || channels < 1 || channels > 8)
		return -EINVAL;

	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		regmap_update_bits(regmap, SUNXI_DAUDIO_CHCFG, 0x7 << 0,
				   (channels - 1) << 0);

		if (daudio->dts.dai_type == SUNXI_DAI_HDMI_TYPE) {
			regmap_update_bits(regmap, SUNXI_DAUDIO_8SLOT_TX0CHSEL,
					   0x7 << 0, 0x1 << 0);
			regmap_update_bits(regmap, SUNXI_DAUDIO_8SLOT_TX1CHSEL,
					   0x7 << 0, 0x1 << 0);
			regmap_update_bits(regmap, SUNXI_DAUDIO_8SLOT_TX2CHSEL,
					   0x7 << 0, 0x1 << 0);
			regmap_update_bits(regmap, SUNXI_DAUDIO_8SLOT_TX3CHSEL,
					   0x7 << 0, 0x1 << 0);

			regmap_update_bits(regmap, SUNXI_DAUDIO_8SLOT_TX0CHSEL, 0xFF << 4, 3 << 4);
			regmap_update_bits(regmap, SUNXI_DAUDIO_8SLOT_TX1CHSEL, 0xFF << 4, 3 << 4);
			regmap_update_bits(regmap, SUNXI_DAUDIO_8SLOT_TX2CHSEL, 0xFF << 4, 3 << 4);
			regmap_update_bits(regmap, SUNXI_DAUDIO_8SLOT_TX3CHSEL, 0xFF << 4, 3 << 4);
		} else {
			regmap_update_bits(regmap, SUNXI_DAUDIO_8SLOT_TX0CHSEL, 0x7 << 0,
					   (channels - 1) << 0);
			regmap_update_bits(regmap, SUNXI_DAUDIO_8SLOT_TX1CHSEL, 0x7 << 0,
					   (channels - 1) << 0);
			regmap_update_bits(regmap, SUNXI_DAUDIO_8SLOT_TX2CHSEL, 0x7 << 0,
					   (channels - 1) << 0);
			regmap_update_bits(regmap, SUNXI_DAUDIO_8SLOT_TX3CHSEL, 0x7 << 0,
					   (channels - 1) << 0);

			regmap_update_bits(regmap, SUNXI_DAUDIO_8SLOT_TX0CHSEL, 0xFF << 4,
					   channels_en_slot[channels - 1] << 4);
			regmap_update_bits(regmap, SUNXI_DAUDIO_8SLOT_TX1CHSEL, 0xFF << 4,
					   channels_en_slot[channels - 1] << 4);
			regmap_update_bits(regmap, SUNXI_DAUDIO_8SLOT_TX2CHSEL, 0xFF << 4,
					   channels_en_slot[channels - 1] << 4);
			regmap_update_bits(regmap, SUNXI_DAUDIO_8SLOT_TX3CHSEL, 0xFF << 4,
					   channels_en_slot[channels - 1] << 4);
		}
	} else {
		regmap_update_bits(regmap, SUNXI_DAUDIO_8SLOT_CHCFG, 0x7 << 4,
				   (channels - 1) << 4);
		regmap_update_bits(regmap, SUNXI_DAUDIO_8SLOT_RXCHSEL, 0x7 << 0,
				   (channels - 1) << 0);
	}

	return 0;
}

static int sunxi_daudio_set_daifmt_fmt(struct sunxi_daudio *daudio, unsigned int format)
{
	struct regmap *regmap = daudio->mem.regmap;
	unsigned int mode, offset;

	if (IS_ERR_OR_NULL(daudio))
		return -EINVAL;

	switch (format) {
	case SND_SOC_DAIFMT_I2S:
		mode = 1;
		offset = 1;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		mode = 2;
		offset = 0;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		mode = 1;
		offset = 0;
		break;
	case SND_SOC_DAIFMT_DSP_A:
		mode = 0;
		offset = 1;
		/* L data MSB after FRM LRC (short frame) */
		regmap_update_bits(regmap, SUNXI_DAUDIO_FMT0, 1 << LRCK_WIDTH, 0 << LRCK_WIDTH);
		break;
	case SND_SOC_DAIFMT_DSP_B:
		mode = 0;
		offset = 0;
		/* L data MSB during FRM LRC (long frame) */
		regmap_update_bits(regmap, SUNXI_DAUDIO_FMT0, 1 << LRCK_WIDTH, 1 << LRCK_WIDTH);
		break;
	default:
		SND_LOG_ERR(HLOG, "format setting failed\n");
		return -EINVAL;
	}

	regmap_update_bits(regmap, SUNXI_DAUDIO_CTL, 3 << MODE_SEL, mode << MODE_SEL);

	regmap_update_bits(regmap, SUNXI_DAUDIO_TX0CHSEL,
			   3 << TX_OFFSET, offset << TX_OFFSET);
	regmap_update_bits(regmap, SUNXI_DAUDIO_TX1CHSEL,
			   3 << TX_OFFSET, offset << TX_OFFSET);
	regmap_update_bits(regmap, SUNXI_DAUDIO_TX2CHSEL,
			   3 << TX_OFFSET, offset << TX_OFFSET);
	regmap_update_bits(regmap, SUNXI_DAUDIO_TX3CHSEL,
			   3 << TX_OFFSET, offset << TX_OFFSET);
	regmap_update_bits(regmap, SUNXI_DAUDIO_RXCHSEL,
			   3 << RX_OFFSET, offset << RX_OFFSET);

	return 0;
}

static int sun8iw11_daudio_set_daifmt_fmt(struct sunxi_daudio *daudio, unsigned int format)
{
	struct regmap *regmap = daudio->mem.regmap;
	unsigned int mode, offset;

	if (IS_ERR_OR_NULL(daudio))
		return -EINVAL;

	switch (format) {
	case SND_SOC_DAIFMT_I2S:
		mode = 1;
		offset = 1;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		mode = 2;
		offset = 0;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		mode = 1;
		offset = 0;
		break;
	case SND_SOC_DAIFMT_DSP_A:
		mode = 0;
		offset = 1;
		/* L data MSB after FRM LRC (short frame) */
		regmap_update_bits(regmap, SUNXI_DAUDIO_FMT0, 1 << LRCK_WIDTH, 0 << LRCK_WIDTH);
		break;
	case SND_SOC_DAIFMT_DSP_B:
		mode = 0;
		offset = 0;
		/* L data MSB during FRM LRC (long frame) */
		regmap_update_bits(regmap, SUNXI_DAUDIO_FMT0, 1 << LRCK_WIDTH, 1 << LRCK_WIDTH);
		break;
	default:
		SND_LOG_ERR(HLOG, "format setting failed\n");
		return -EINVAL;
	}

	regmap_update_bits(regmap, SUNXI_DAUDIO_CTL, 3 << MODE_SEL, mode << MODE_SEL);

	regmap_update_bits(regmap, SUNXI_DAUDIO_8SLOT_TX0CHSEL, 3 << 12, offset << 12);
	regmap_update_bits(regmap, SUNXI_DAUDIO_8SLOT_TX1CHSEL, 3 << 12, offset << 12);
	regmap_update_bits(regmap, SUNXI_DAUDIO_8SLOT_TX2CHSEL, 3 << 12, offset << 12);
	regmap_update_bits(regmap, SUNXI_DAUDIO_8SLOT_TX3CHSEL, 3 << 12, offset << 12);
	regmap_update_bits(regmap, SUNXI_DAUDIO_8SLOT_RXCHSEL, 3 << 12, offset << 12);

	return 0;
}

static int sunxi_daudio_set_ch_map(struct sunxi_daudio *daudio, unsigned int channels)
{
	struct regmap *regmap = daudio->mem.regmap;
	(void)channels;

	if (IS_ERR_OR_NULL(daudio))
		return -EINVAL;

	regmap_write(regmap, SUNXI_DAUDIO_TX0CHMAP0, 0xFEDCBA98);
	regmap_write(regmap, SUNXI_DAUDIO_TX0CHMAP1, 0x76543210);
	regmap_write(regmap, SUNXI_DAUDIO_TX1CHMAP0, 0xFEDCBA98);
	regmap_write(regmap, SUNXI_DAUDIO_TX1CHMAP1, 0x76543210);
	regmap_write(regmap, SUNXI_DAUDIO_TX2CHMAP0, 0xFEDCBA98);
	regmap_write(regmap, SUNXI_DAUDIO_TX2CHMAP1, 0x76543210);
	regmap_write(regmap, SUNXI_DAUDIO_TX3CHMAP0, 0xFEDCBA98);
	regmap_write(regmap, SUNXI_DAUDIO_TX3CHMAP1, 0x76543210);

	regmap_write(regmap, SUNXI_DAUDIO_RXCHMAP0, 0x0F0E0D0C);
	regmap_write(regmap, SUNXI_DAUDIO_RXCHMAP1, 0x0B0A0908);
	regmap_write(regmap, SUNXI_DAUDIO_RXCHMAP2, 0x07060504);
	regmap_write(regmap, SUNXI_DAUDIO_RXCHMAP3, 0x03020100);

	return 0;
}

static int sun8iw11_daudio_set_ch_map(struct sunxi_daudio *daudio, unsigned int channels)
{
	struct regmap *regmap = daudio->mem.regmap;

	if (IS_ERR_OR_NULL(daudio))
		return -EINVAL;

	if (daudio->dts.dai_type == SUNXI_DAI_HDMI_TYPE) {
		regmap_write(regmap, SUNXI_DAUDIO_8SLOT_TX0CHMAP, 0x10);
		if (daudio->hdmi_fmt > HDMI_FMT_PCM) {
			regmap_write(regmap, SUNXI_DAUDIO_8SLOT_TX1CHMAP, 0x32);
			regmap_write(regmap, SUNXI_DAUDIO_8SLOT_TX2CHMAP, 0x54);
			regmap_write(regmap, SUNXI_DAUDIO_8SLOT_TX3CHMAP, 0x76);
		} else {
			if (channels == 4) {
				regmap_write(regmap, SUNXI_DAUDIO_8SLOT_TX1CHMAP, 0x23);
			} else if (channels == 6) {
				regmap_write(regmap, SUNXI_DAUDIO_8SLOT_TX1CHMAP, 0x23);
				regmap_write(regmap, SUNXI_DAUDIO_8SLOT_TX2CHMAP, 0x54);
			} else {
				regmap_write(regmap, SUNXI_DAUDIO_8SLOT_TX1CHMAP, 0x23);
				regmap_write(regmap, SUNXI_DAUDIO_8SLOT_TX2CHMAP, 0x76);
				regmap_write(regmap, SUNXI_DAUDIO_8SLOT_TX3CHMAP, 0x54);
			}
		}
	 } else {
		regmap_write(regmap, SUNXI_DAUDIO_8SLOT_TX0CHMAP, 0x76543210);
		regmap_write(regmap, SUNXI_DAUDIO_8SLOT_TX1CHMAP, 0x76543210);
		regmap_write(regmap, SUNXI_DAUDIO_8SLOT_TX2CHMAP, 0x76543210);
		regmap_write(regmap, SUNXI_DAUDIO_8SLOT_TX3CHMAP, 0x76543210);
	}

	regmap_write(regmap, SUNXI_DAUDIO_8SLOT_RXCHMAP, 0x76543210);

	return 0;
}

static int sunxi_get_i2s_dai_fmt(struct sunxi_i2s_dai_fmt *i2s_dai_fmt,
				 enum SUNXI_I2S_DAI_FMT_SEL dai_fmt_sel,
				 unsigned int *val)
{
	switch (dai_fmt_sel) {
	case SUNXI_I2S_DAI_PLL:
		*val = i2s_dai_fmt->pllclk_freq;
		break;
	case SUNXI_I2S_DAI_MCLK:
		*val = i2s_dai_fmt->moduleclk_freq;
		break;
	case SUNXI_I2S_DAI_FMT:
		*val = i2s_dai_fmt->fmt & SND_SOC_DAIFMT_FORMAT_MASK;
		break;
	case SUNXI_I2S_DAI_MASTER:
		*val = i2s_dai_fmt->fmt & SND_SOC_DAIFMT_MASTER_MASK;
		break;
	case SUNXI_I2S_DAI_INVERT:
		*val = i2s_dai_fmt->fmt & SND_SOC_DAIFMT_INV_MASK;
		break;
	case SUNXI_I2S_DAI_SLOT_NUM:
		*val = i2s_dai_fmt->slots;
		break;
	case SUNXI_I2S_DAI_SLOT_WIDTH:
		*val = i2s_dai_fmt->slot_width;
		break;
	default:
		SND_LOG_ERR(HLOG, "unsupport dai fmt sel %d\n", dai_fmt_sel);
		return -EINVAL;
	}

	return 0;
}

static int sunxi_set_i2s_dai_fmt(struct sunxi_i2s_dai_fmt *i2s_dai_fmt,
				 enum SUNXI_I2S_DAI_FMT_SEL dai_fmt_sel,
				 unsigned int val)
{
	switch (dai_fmt_sel) {
	case SUNXI_I2S_DAI_PLL:
		i2s_dai_fmt->pllclk_freq = val;
		break;
	case SUNXI_I2S_DAI_MCLK:
		i2s_dai_fmt->moduleclk_freq = val;
		break;
	case SUNXI_I2S_DAI_FMT:
		i2s_dai_fmt->fmt &= ~SND_SOC_DAIFMT_FORMAT_MASK;
		i2s_dai_fmt->fmt |= SND_SOC_DAIFMT_FORMAT_MASK & val;
		break;
	case SUNXI_I2S_DAI_MASTER:
		i2s_dai_fmt->fmt &= ~SND_SOC_DAIFMT_MASTER_MASK;
		i2s_dai_fmt->fmt |= SND_SOC_DAIFMT_MASTER_MASK & val;
		break;
	case SUNXI_I2S_DAI_INVERT:
		i2s_dai_fmt->fmt &= ~SND_SOC_DAIFMT_INV_MASK;
		i2s_dai_fmt->fmt |= SND_SOC_DAIFMT_INV_MASK & val;
		break;
	case SUNXI_I2S_DAI_SLOT_NUM:
		i2s_dai_fmt->slots = val;
		break;
	case SUNXI_I2S_DAI_SLOT_WIDTH:
		i2s_dai_fmt->slot_width = val;
		break;
	default:
		SND_LOG_ERR(HLOG, "unsupport dai fmt sel %d\n", dai_fmt_sel);
		return -EINVAL;
	}

	return 0;
}

static void sunxi_sdout_enable(struct regmap *regmap, bool *tx_pin)
{
	/* tx_pin[x] -- x < 4 */
	regmap_update_bits(regmap, SUNXI_DAUDIO_CTL, 1 << (SDO0_EN + 0),
			   tx_pin[0] << (SDO0_EN + 0));
	regmap_update_bits(regmap, SUNXI_DAUDIO_CTL, 1 << (SDO0_EN + 1),
			   tx_pin[1] << (SDO0_EN + 1));
	regmap_update_bits(regmap, SUNXI_DAUDIO_CTL, 1 << (SDO0_EN + 2),
			   tx_pin[2] << (SDO0_EN + 2));
	regmap_update_bits(regmap, SUNXI_DAUDIO_CTL, 1 << (SDO0_EN + 3),
			   tx_pin[3] << (SDO0_EN + 3));
}

static void sunxi_sdout_disable(struct regmap *regmap)
{
	regmap_update_bits(regmap, SUNXI_DAUDIO_CTL, 1 << (SDO0_EN + 0), 0 << (SDO0_EN + 0));
	regmap_update_bits(regmap, SUNXI_DAUDIO_CTL, 1 << (SDO0_EN + 1), 0 << (SDO0_EN + 0));
	regmap_update_bits(regmap, SUNXI_DAUDIO_CTL, 1 << (SDO0_EN + 2), 0 << (SDO0_EN + 0));
	regmap_update_bits(regmap, SUNXI_DAUDIO_CTL, 1 << (SDO0_EN + 3), 0 << (SDO0_EN + 0));
}

static int sunxi_daudio_dai_set_pll(struct snd_soc_dai *dai, int pll_id, int source,
				    unsigned int freq_in, unsigned int freq_out)
{
	struct sunxi_daudio *daudio = snd_soc_dai_get_drvdata(dai);
	struct sunxi_daudio_clk *clk = &daudio->clk;
	struct sunxi_i2s_dai_fmt *i2s_dai_fmt = &daudio->i2s_dai_fmt;
	int ret;

	SND_LOG_DEBUG(HLOG, "\n");

	if (snd_sunxi_clk_rate(clk, freq_in, freq_out)) {
		SND_LOG_ERR(HLOG, "clk set rate failed\n");
		return -EINVAL;
	}

	ret = sunxi_set_i2s_dai_fmt(i2s_dai_fmt, SUNXI_I2S_DAI_PLL, freq_in);
	if (ret < 0)
		return -EINVAL;
	ret = sunxi_set_i2s_dai_fmt(i2s_dai_fmt, SUNXI_I2S_DAI_MCLK, freq_out);
	if (ret < 0)
		return -EINVAL;

	return 0;
}

static int sunxi_daudio_dai_set_sysclk(struct snd_soc_dai *dai, int clk_id,
				       unsigned int freq, int dir)
{
	struct sunxi_daudio *daudio = snd_soc_dai_get_drvdata(dai);
	struct regmap *regmap = daudio->mem.regmap;
	struct sunxi_i2s_dai_fmt *i2s_dai_fmt = &daudio->i2s_dai_fmt;
	unsigned int pllclk_freq, mclk_ratio, mclk_ratio_map;
	int ret;

	SND_LOG_DEBUG(HLOG, "\n");

	if (freq == 0) {
		regmap_update_bits(regmap, SUNXI_DAUDIO_CLKDIV, 1 << MCLKOUT_EN, 0 << MCLKOUT_EN);
		return 0;
	}

	ret = sunxi_get_i2s_dai_fmt(i2s_dai_fmt, SUNXI_I2S_DAI_PLL, &pllclk_freq);
	if (ret < 0)
		return -EINVAL;

	if (pllclk_freq == 0) {
		SND_LOG_ERR(HLOG, "pllclk freq is invalid\n");
		return -ENOMEM;
	}
	mclk_ratio = pllclk_freq / freq;

	switch (mclk_ratio) {
	case 1:
		mclk_ratio_map = 1;
		break;
	case 2:
		mclk_ratio_map = 2;
		break;
	case 4:
		mclk_ratio_map = 3;
		break;
	case 6:
		mclk_ratio_map = 4;
		break;
	case 8:
		mclk_ratio_map = 5;
		break;
	case 12:
		mclk_ratio_map = 6;
		break;
	case 16:
		mclk_ratio_map = 7;
		break;
	case 24:
		mclk_ratio_map = 8;
		break;
	case 32:
		mclk_ratio_map = 9;
		break;
	case 48:
		mclk_ratio_map = 10;
		break;
	case 64:
		mclk_ratio_map = 11;
		break;
	case 96:
		mclk_ratio_map = 12;
		break;
	case 128:
		mclk_ratio_map = 13;
		break;
	case 176:
		mclk_ratio_map = 14;
		break;
	case 192:
		mclk_ratio_map = 15;
		break;
	default:
		regmap_update_bits(regmap, SUNXI_DAUDIO_CLKDIV, 1 << MCLKOUT_EN, 0 << MCLKOUT_EN);
		SND_LOG_ERR(HLOG, "mclk freq div unsupport\n");
		return -EINVAL;
	}

	regmap_update_bits(regmap, SUNXI_DAUDIO_CLKDIV,
			   0xf << MCLK_DIV, mclk_ratio_map << MCLK_DIV);
	regmap_update_bits(regmap, SUNXI_DAUDIO_CLKDIV, 1 << MCLKOUT_EN, 1 << MCLKOUT_EN);

	return 0;
}

static int sunxi_daudio_dai_set_bclk_ratio(struct snd_soc_dai *dai, unsigned int ratio)
{
	struct sunxi_daudio *daudio = snd_soc_dai_get_drvdata(dai);
	struct regmap *regmap = daudio->mem.regmap;
	unsigned int bclk_ratio;

	SND_LOG_DEBUG(HLOG, "\n");

	/* ratio -> cpudai pllclk / pcm rate */
	switch (ratio) {
	case 1:
		bclk_ratio = 1;
		break;
	case 2:
		bclk_ratio = 2;
		break;
	case 4:
		bclk_ratio = 3;
		break;
	case 6:
		bclk_ratio = 4;
		break;
	case 8:
		bclk_ratio = 5;
		break;
	case 12:
		bclk_ratio = 6;
		break;
	case 16:
		bclk_ratio = 7;
		break;
	case 24:
		bclk_ratio = 8;
		break;
	case 32:
		bclk_ratio = 9;
		break;
	case 48:
		bclk_ratio = 10;
		break;
	case 64:
		bclk_ratio = 11;
		break;
	case 96:
		bclk_ratio = 12;
		break;
	case 128:
		bclk_ratio = 13;
		break;
	case 176:
		bclk_ratio = 14;
		break;
	case 192:
		bclk_ratio = 15;
		break;
	default:
		SND_LOG_ERR(HLOG, "bclk freq div unsupport\n");
		return -EINVAL;
	}

	regmap_update_bits(regmap, SUNXI_DAUDIO_CLKDIV, 0xf << BCLK_DIV, bclk_ratio << BCLK_DIV);

	return 0;
}

static int sunxi_daudio_dai_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct sunxi_daudio *daudio = snd_soc_dai_get_drvdata(dai);
	const struct sunxi_daudio_quirks *quirks = daudio->quirks;
	struct regmap *regmap = daudio->mem.regmap;
	struct sunxi_i2s_dai_fmt *i2s_dai_fmt = &daudio->i2s_dai_fmt;
	unsigned int lrck_polarity, bclk_polarity;
	int ret;

	SND_LOG_DEBUG(HLOG, "dai fmt -> 0x%x\n", fmt);

	ret = sunxi_set_i2s_dai_fmt(i2s_dai_fmt, SUNXI_I2S_DAI_FMT, fmt);
	if (ret < 0)
		return -EINVAL;
	ret = sunxi_set_i2s_dai_fmt(i2s_dai_fmt, SUNXI_I2S_DAI_MASTER, fmt);
	if (ret < 0)
		return -EINVAL;
	ret = sunxi_set_i2s_dai_fmt(i2s_dai_fmt, SUNXI_I2S_DAI_INVERT, fmt);
	if (ret < 0)
		return -EINVAL;

	/* set TDM format */
	ret = quirks->set_daifmt_format(daudio, fmt & SND_SOC_DAIFMT_FORMAT_MASK);
	if (ret < 0) {
		SND_LOG_ERR(HLOG, "set daifmt format failed\n");
		return -EINVAL;
	}

	/* set lrck & bclk polarity */
	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		lrck_polarity = 0;
		bclk_polarity = 0;
		break;
	case SND_SOC_DAIFMT_NB_IF:
		lrck_polarity = 1;
		bclk_polarity = 0;
		break;
	case SND_SOC_DAIFMT_IB_NF:
		lrck_polarity = 0;
		bclk_polarity = 1;
		break;
	case SND_SOC_DAIFMT_IB_IF:
		lrck_polarity = 1;
		bclk_polarity = 1;
		break;
	default:
		SND_LOG_ERR(HLOG, "invert clk setting failed\n");
		return -EINVAL;
	}
	regmap_update_bits(regmap, SUNXI_DAUDIO_FMT0,
			   1 << LRCK_POLARITY,
			   lrck_polarity << LRCK_POLARITY);
	regmap_update_bits(regmap, SUNXI_DAUDIO_FMT0,
			   1 << BCLK_POLARITY,
			   bclk_polarity << BCLK_POLARITY);

	/* set master/slave */
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	/* bclk & lrck dir input */
	case SND_SOC_DAIFMT_CBM_CFM:
		regmap_update_bits(regmap, SUNXI_DAUDIO_CTL, 1 << BCLK_OUT, 0 << BCLK_OUT);
		regmap_update_bits(regmap, SUNXI_DAUDIO_CTL, 1 << LRCK_OUT, 0 << LRCK_OUT);
		break;
	case SND_SOC_DAIFMT_CBS_CFM:
		regmap_update_bits(regmap, SUNXI_DAUDIO_CTL, 1 << BCLK_OUT, 1 << BCLK_OUT);
		regmap_update_bits(regmap, SUNXI_DAUDIO_CTL, 1 << LRCK_OUT, 0 << LRCK_OUT);
		break;
	case SND_SOC_DAIFMT_CBM_CFS:
		regmap_update_bits(regmap, SUNXI_DAUDIO_CTL, 1 << BCLK_OUT, 0 << BCLK_OUT);
		regmap_update_bits(regmap, SUNXI_DAUDIO_CTL, 1 << LRCK_OUT, 1 << LRCK_OUT);
		break;
	case SND_SOC_DAIFMT_CBS_CFS:
		regmap_update_bits(regmap, SUNXI_DAUDIO_CTL, 1 << BCLK_OUT, 1 << BCLK_OUT);
		regmap_update_bits(regmap, SUNXI_DAUDIO_CTL, 1 << LRCK_OUT, 1 << LRCK_OUT);
		break;
	default:
		SND_LOG_ERR(HLOG, "unknown master/slave format\n");
		return -EINVAL;
	}

	return 0;
}

static int sunxi_daudio_dai_set_tdm_slot(struct snd_soc_dai *dai,
					 unsigned int tx_mask, unsigned int rx_mask,
					 int slots, int slot_width)
{
	struct sunxi_daudio *daudio = snd_soc_dai_get_drvdata(dai);
	struct regmap *regmap = daudio->mem.regmap;
	struct sunxi_i2s_dai_fmt *i2s_dai_fmt = &daudio->i2s_dai_fmt;
	unsigned int slot_width_map, lrck_width_map;
	unsigned int dai_fmt_get;
	int ret;

	SND_LOG_DEBUG(HLOG, "\n");

	switch (slot_width) {
	case 8:
		slot_width_map = 1;
		break;
	case 12:
		slot_width_map = 2;
		break;
	case 16:
		slot_width_map = 3;
		break;
	case 20:
		slot_width_map = 4;
		break;
	case 24:
		slot_width_map = 5;
		break;
	case 28:
		slot_width_map = 6;
		break;
	case 32:
		slot_width_map = 7;
		break;
	default:
		SND_LOG_ERR(HLOG, "unknown slot width\n");
		return -EINVAL;
	}
	regmap_update_bits(regmap, SUNXI_DAUDIO_FMT0,
			   7 << SLOT_WIDTH, slot_width_map << SLOT_WIDTH);

	/* bclk num of per channel
	 * I2S/RIGHT_J/LEFT_J	-> lrck long total is lrck_width_map * 2
	 * DSP_A/DSP_B		-> lrck long total is lrck_width_map * 1
	 */
	ret = sunxi_get_i2s_dai_fmt(i2s_dai_fmt, SUNXI_I2S_DAI_FMT, &dai_fmt_get);
	if (ret < 0)
		return -EINVAL;
	switch (dai_fmt_get) {
	case SND_SOC_DAIFMT_I2S:
	case SND_SOC_DAIFMT_RIGHT_J:
	case SND_SOC_DAIFMT_LEFT_J:
		lrck_width_map = (slots / 2) * slot_width - 1;
		break;
	case SND_SOC_DAIFMT_DSP_A:
	case SND_SOC_DAIFMT_DSP_B:
		lrck_width_map = slots * slot_width - 1;
		break;
	default:
		SND_LOG_ERR(HLOG, "unsupoort format\n");
		return -EINVAL;
	}
	regmap_update_bits(regmap, SUNXI_DAUDIO_FMT0,
			   0x3ff << LRCK_PERIOD, lrck_width_map << LRCK_PERIOD);

	ret = sunxi_set_i2s_dai_fmt(i2s_dai_fmt, SUNXI_I2S_DAI_SLOT_NUM, slots);
	if (ret < 0)
		return -EINVAL;
	ret = sunxi_set_i2s_dai_fmt(i2s_dai_fmt, SUNXI_I2S_DAI_SLOT_WIDTH, slot_width);
	if (ret < 0)
		return -EINVAL;

	return 0;
}

static int sunxi_daudio_dai_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct sunxi_daudio *daudio = snd_soc_dai_get_drvdata(dai);

	SND_LOG_DEBUG(HLOG, "\n");

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		snd_soc_dai_set_dma_data(dai, substream, &daudio->playback_dma_param);
	else
		snd_soc_dai_set_dma_data(dai, substream, &daudio->capture_dma_param);

	return 0;
}

static void sunxi_daudio_dai_shutdown(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct sunxi_daudio *daudio = snd_soc_dai_get_drvdata(dai);

	SND_LOG_DEBUG(HLOG, "\n");

	if (daudio->dts.dai_type == SUNXI_DAI_HDMI_TYPE) {
		snd_sunxi_hdmi_shutdown();
	}

	return;
}

static int sunxi_daudio_dai_hw_params(struct snd_pcm_substream *substream,
				      struct snd_pcm_hw_params *params,
				      struct snd_soc_dai *dai)
{
	struct sunxi_daudio *daudio = snd_soc_dai_get_drvdata(dai);
	const struct sunxi_daudio_quirks *quirks = daudio->quirks;
	struct regmap *regmap = daudio->mem.regmap;
	int ret;

	SND_LOG_DEBUG(HLOG, "\n");

	if (daudio->dts.dai_type == SUNXI_DAI_HDMI_TYPE) {
		daudio->hdmi_fmt = snd_sunxi_hdmi_get_fmt();
		SND_LOG_DEBUG(HLOG, "hdmi fmt -> %d\n", daudio->hdmi_fmt);
	}

	/* set bits */
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			if (daudio->dts.dai_type == SUNXI_DAI_HDMI_TYPE &&
			    daudio->hdmi_fmt > HDMI_FMT_PCM) {
				regmap_update_bits(regmap, SUNXI_DAUDIO_FMT0,
						   0x7 << DAUDIO_SAMPLE_RESOLUTION,
						   0x5 << DAUDIO_SAMPLE_RESOLUTION);
				regmap_update_bits(regmap, SUNXI_DAUDIO_FIFOCTL, 0x1 << TXIM, 0x0 << TXIM);
			} else {
				regmap_update_bits(regmap, SUNXI_DAUDIO_FMT0,
						   0x7 << DAUDIO_SAMPLE_RESOLUTION,
						   0x3 << DAUDIO_SAMPLE_RESOLUTION);
				regmap_update_bits(regmap, SUNXI_DAUDIO_FIFOCTL, 0x1 << TXIM, 0x1 << TXIM);
			}
		} else {
			regmap_update_bits(regmap, SUNXI_DAUDIO_FMT0,
					   0x7 << DAUDIO_SAMPLE_RESOLUTION,
					   0x3 << DAUDIO_SAMPLE_RESOLUTION);
			regmap_update_bits(regmap, SUNXI_DAUDIO_FIFOCTL,
					   0x3 << RXOM,
					   0x1 << RXOM);
		}
		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
	case SNDRV_PCM_FORMAT_S24_LE:
	case SNDRV_PCM_FORMAT_S24_3LE:
		regmap_update_bits(regmap, SUNXI_DAUDIO_FMT0,
				   0x7 << DAUDIO_SAMPLE_RESOLUTION,
				   0x5 << DAUDIO_SAMPLE_RESOLUTION);

		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			regmap_update_bits(regmap, SUNXI_DAUDIO_FIFOCTL, 0x1 << TXIM, 0x1 << TXIM);
		} else {
			regmap_update_bits(regmap, SUNXI_DAUDIO_FIFOCTL,
					   0x3 << RXOM,
					   0x1 << RXOM);
		}
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
		regmap_update_bits(regmap, SUNXI_DAUDIO_FMT0,
				   0x7 << DAUDIO_SAMPLE_RESOLUTION,
				   0x7 << DAUDIO_SAMPLE_RESOLUTION);

		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			regmap_update_bits(regmap, SUNXI_DAUDIO_FIFOCTL, 0x1 << TXIM, 0x1 << TXIM);
		} else {
			regmap_update_bits(regmap, SUNXI_DAUDIO_FIFOCTL,
					   0x3 << RXOM,
					   0x1 << RXOM);
		}
		break;
	default:
		SND_LOG_ERR(HLOG, "unrecognized format\n");
		return -EINVAL;
	}

	/* set channels map */
	ret = quirks->set_channels_map(daudio, params_channels(params));

	/* set channels */
	ret = quirks->set_channel_enable(daudio, substream->stream, params_channels(params));
	if (ret < 0) {
		SND_LOG_ERR(HLOG, "set channel enable failed\n");
		return -EINVAL;
	}

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		if (daudio->dts.dai_type == SUNXI_DAI_HDMI_TYPE) {
			ret = snd_sunxi_hdmi_hw_params(params, daudio->hdmi_fmt);
			if (ret) {
				SND_LOG_ERR(HLOG, "hdmi audio hw_params set failed\n");
				return ret;
			}
		}
	}

	return 0;
}

static int sunxi_daudio_dai_prepare(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct sunxi_daudio *daudio = snd_soc_dai_get_drvdata(dai);
	struct regmap *regmap = daudio->mem.regmap;
	unsigned int i, ret;

	SND_LOG_DEBUG(HLOG, "\n");

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		for (i = 0 ; i < 10 ; i++) {
			regmap_update_bits(regmap, SUNXI_DAUDIO_FIFOCTL,
					   1 << FIFO_CTL_FTX, 1 << FIFO_CTL_FTX);
			mdelay(1);
		}
		regmap_write(regmap, SUNXI_DAUDIO_TXCNT, 0);

		if (daudio->dts.dai_type == SUNXI_DAI_HDMI_TYPE) {
			ret = snd_sunxi_hdmi_prepare();
			if (ret) {
				SND_LOG_ERR(HLOG, "hdmi audio prepare failed\n");
				return ret;
			}
		}
	} else {
		regmap_update_bits(regmap, SUNXI_DAUDIO_FIFOCTL,
				   1 << FIFO_CTL_FRX, 1 << FIFO_CTL_FRX);
		regmap_write(regmap, SUNXI_DAUDIO_RXCNT, 0);
	}

	return 0;
}

static void sunxi_daudio_dai_tx_route(struct sunxi_daudio *daudio, bool enable)
{
	struct sunxi_daudio_dts *dts = &daudio->dts;
	struct regmap *regmap = daudio->mem.regmap;
	unsigned int reg_val;

	if (enable) {
		regmap_update_bits(regmap, SUNXI_DAUDIO_INTCTL, 1 << TXDRQEN, 1 << TXDRQEN);
		sunxi_sdout_enable(regmap, dts->tx_pin);
		regmap_update_bits(regmap, SUNXI_DAUDIO_CTL, 1 << CTL_TXEN, 1 << CTL_TXEN);
	} else {
		regmap_update_bits(regmap, SUNXI_DAUDIO_INTCTL, 1 << TXDRQEN, 0 << TXDRQEN);

		/* add this to avoid the i2s pop */
		while (1) {
			regmap_update_bits(regmap, SUNXI_DAUDIO_FIFOCTL,
					   1 << FIFO_CTL_FTX, 1 << FIFO_CTL_FTX);
			regmap_write(regmap, SUNXI_DAUDIO_TXCNT, 0);
			regmap_read(regmap, SUNXI_DAUDIO_FIFOSTA, &reg_val);
			reg_val = ((reg_val & 0xFF0000) >> 16);
			if (reg_val == 0x80)
				break;
		}
		udelay(250);

		regmap_update_bits(regmap, SUNXI_DAUDIO_CTL, 1 << CTL_TXEN, 0 << CTL_TXEN);
		sunxi_sdout_disable(regmap);
	}
}

static void sunxi_daudio_dai_rx_route(struct sunxi_daudio *daudio, bool enable)
{
	struct regmap *regmap = daudio->mem.regmap;

	if (enable) {
		regmap_update_bits(regmap, SUNXI_DAUDIO_CTL, 1 << CTL_RXEN, 1 << CTL_RXEN);
		regmap_update_bits(regmap, SUNXI_DAUDIO_INTCTL, 1 << RXDRQEN, 1 << RXDRQEN);
	} else {
		regmap_update_bits(regmap, SUNXI_DAUDIO_INTCTL, 1 << RXDRQEN, 0 << RXDRQEN);
		regmap_update_bits(regmap, SUNXI_DAUDIO_CTL, 1 << CTL_RXEN, 0 << CTL_RXEN);
	}
}

static int sunxi_daudio_dai_trigger(struct snd_pcm_substream *substream,
				    int cmd, struct snd_soc_dai *dai)
{
	struct sunxi_daudio *daudio = snd_soc_dai_get_drvdata(dai);
	const struct sunxi_daudio_quirks *quirks = daudio->quirks;
	struct sunxi_daudio_dts *dts = &daudio->dts;

	SND_LOG_DEBUG(HLOG, "\n");

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			sunxi_daudio_dai_tx_route(daudio, true);
		} else {
			sunxi_daudio_dai_rx_route(daudio, true);
			if (dts->rx_sync_en && dts->rx_sync_ctl && quirks->rx_sync_en)
				sunxi_rx_sync_control(dts->rx_sync_domain, dts->rx_sync_id, true);
		}
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			sunxi_daudio_dai_tx_route(daudio, false);
		} else {
			sunxi_daudio_dai_rx_route(daudio, false);
			if (dts->rx_sync_en && dts->rx_sync_ctl && quirks->rx_sync_en)
				sunxi_rx_sync_control(dts->rx_sync_domain, dts->rx_sync_id, false);
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static const struct snd_soc_dai_ops sunxi_daudio_dai_ops = {
	/* call by machine */
	.set_pll	= sunxi_daudio_dai_set_pll,		/* set pllclk */
	.set_sysclk	= sunxi_daudio_dai_set_sysclk,		/* set mclk */
	.set_bclk_ratio	= sunxi_daudio_dai_set_bclk_ratio,	/* set bclk freq */
	.set_fmt	= sunxi_daudio_dai_set_fmt,		/* set tdm fmt */
	.set_tdm_slot	= sunxi_daudio_dai_set_tdm_slot,	/* set slot num and width */
	/* call by asoc */
	.startup	= sunxi_daudio_dai_startup,
	.hw_params	= sunxi_daudio_dai_hw_params,
	.prepare	= sunxi_daudio_dai_prepare,
	.trigger	= sunxi_daudio_dai_trigger,
	.shutdown	= sunxi_daudio_dai_shutdown,
};

static int sunxi_daudio_init(struct sunxi_daudio *daudio)
{
	const struct sunxi_daudio_quirks *quirks = daudio->quirks;
	struct regmap *regmap = daudio->mem.regmap;

	regmap_update_bits(regmap, SUNXI_DAUDIO_FMT1, 1 << TX_MLS, 0 << TX_MLS);
	regmap_update_bits(regmap, SUNXI_DAUDIO_FMT1, 1 << RX_MLS, 0 << RX_MLS);
	regmap_update_bits(regmap, SUNXI_DAUDIO_FMT1, 3 << SEXT, 0 << SEXT);
	regmap_update_bits(regmap, SUNXI_DAUDIO_FMT1, 3 << TX_PDM, 0 << TX_PDM);
	regmap_update_bits(regmap, SUNXI_DAUDIO_FMT1, 3 << RX_PDM, 0 << RX_PDM);

	if (quirks->rx_sync_en)
		regmap_update_bits(regmap, SUNXI_DAUDIO_CTL, 1 << RX_SYNC_EN, 0 << RX_SYNC_EN);

	regmap_update_bits(regmap, SUNXI_DAUDIO_CTL, 1 << GLOBAL_EN, 1 << GLOBAL_EN);

	return 0;
}

static int sunxi_daudio_dai_probe(struct snd_soc_dai *dai)
{
	struct sunxi_daudio *daudio = snd_soc_dai_get_drvdata(dai);

	SND_LOG_DEBUG(HLOG, "\n");

	/* pcm_new will using the dma_param about the cma and fifo params. */
	snd_soc_dai_init_dma_data(dai,
				  &daudio->playback_dma_param,
				  &daudio->capture_dma_param);

	sunxi_daudio_init(daudio);

	return 0;
}

static int sunxi_daudio_dai_remove(struct snd_soc_dai *dai)
{
	struct sunxi_daudio *daudio = snd_soc_dai_get_drvdata(dai);
	struct regmap *regmap = daudio->mem.regmap;

	SND_LOG_DEBUG(HLOG, "\n");

	regmap_update_bits(regmap, SUNXI_DAUDIO_CTL, 0x1 << GLOBAL_EN, 0x0 << GLOBAL_EN);

	return 0;
}

static struct snd_soc_dai_driver sunxi_daudio_dai = {
	.probe		= sunxi_daudio_dai_probe,
	.remove		= sunxi_daudio_dai_remove,
	.playback = {
		.stream_name	= "Playback",
		.channels_min	= 1,
		.channels_max	= 16,
		.rates		= SNDRV_PCM_RATE_8000_192000
				| SNDRV_PCM_RATE_KNOT,
		.formats	= SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S20_3LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S24_3LE
				| SNDRV_PCM_FMTBIT_S32_LE,
	},
	.capture = {
		.stream_name	= "Capture",
		.channels_min	= 1,
		.channels_max	= 16,
		.rates		= SNDRV_PCM_RATE_8000_192000
			| SNDRV_PCM_RATE_KNOT,
		.formats	= SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S20_3LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S24_3LE
				| SNDRV_PCM_FMTBIT_S32_LE,
	},
	.ops = &sunxi_daudio_dai_ops,
};

/*******************************************************************************
 * *** sound card & component function source ***
 * @0 sound card probe
 * @1 component function kcontrol register
 ******************************************************************************/
static void sunxi_rx_sync_enable(void *data, bool enable)
{
	struct sunxi_daudio *daudio = data;
	struct regmap *regmap = daudio->mem.regmap;

	SND_LOG_DEBUG(HLOG, "%s\n", enable ? "on" : "off");

	if (enable) {
		regmap_update_bits(regmap, SUNXI_DAUDIO_CTL,
				   0x1 << RX_SYNC_EN_START, 0x1 << RX_SYNC_EN_START);
	} else {
		regmap_update_bits(regmap, SUNXI_DAUDIO_CTL,
				   0x1 << RX_SYNC_EN_START, 0x0 << RX_SYNC_EN_START);
	}

	return;
}

static int sunxi_get_tx_hub_mode(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_daudio *daudio = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = daudio->mem.regmap;

	unsigned int reg_val;

	regmap_read(regmap, SUNXI_DAUDIO_FIFOCTL, &reg_val);

	ucontrol->value.integer.value[0] = ((reg_val & (0x1 << HUB_EN)) ? 1 : 0);

	return 0;
}

static int sunxi_set_tx_hub_mode(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_daudio *daudio = snd_soc_component_get_drvdata(component);
	struct sunxi_daudio_dts *dts = &daudio->dts;
	struct regmap *regmap = daudio->mem.regmap;

	switch (ucontrol->value.integer.value[0]) {
	case 0:
		regmap_update_bits(regmap, SUNXI_DAUDIO_CTL, 1 << CTL_TXEN, 0 << CTL_TXEN);
		sunxi_sdout_disable(regmap);
		regmap_update_bits(regmap, SUNXI_DAUDIO_FIFOCTL, 1 << HUB_EN, 0 << HUB_EN);
		break;
	case 1:
		regmap_update_bits(regmap, SUNXI_DAUDIO_FIFOCTL, 1 << HUB_EN, 1 << HUB_EN);
		regmap_update_bits(regmap, SUNXI_DAUDIO_CTL, 1 << CTL_TXEN, 1 << CTL_TXEN);
		sunxi_sdout_enable(regmap, dts->tx_pin);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sunxi_get_rx_sync_mode(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_daudio *daudio = snd_soc_component_get_drvdata(component);
	struct sunxi_daudio_dts *dts = &daudio->dts;

	ucontrol->value.integer.value[0] = dts->rx_sync_ctl;

	return 0;
}

static int sunxi_set_rx_sync_mode(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_daudio *daudio = snd_soc_component_get_drvdata(component);
	struct sunxi_daudio_dts *dts = &daudio->dts;
	struct regmap *regmap = daudio->mem.regmap;

	switch (ucontrol->value.integer.value[0]) {
	case 0:
		dts->rx_sync_ctl = 0;
		regmap_update_bits(regmap, SUNXI_DAUDIO_CTL, 1 << RX_SYNC_EN, 0 << RX_SYNC_EN);
		sunxi_rx_sync_shutdown(dts->rx_sync_domain, dts->rx_sync_id);
		break;
	case 1:
		sunxi_rx_sync_startup(dts->rx_sync_domain, dts->rx_sync_id,
				      (void *)regmap, sunxi_rx_sync_enable);
		regmap_update_bits(regmap, SUNXI_DAUDIO_CTL, 1 << RX_SYNC_EN, 1 << RX_SYNC_EN);
		dts->rx_sync_ctl = 1;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static const char *sunxi_switch_text[] = {"Off", "On"};

static SOC_ENUM_SINGLE_EXT_DECL(sunxi_tx_hub_mode_enum, sunxi_switch_text);
static SOC_ENUM_SINGLE_EXT_DECL(sunxi_rx_sync_mode_enum, sunxi_switch_text);
static const struct snd_kcontrol_new sunxi_tx_hub_controls[] = {
	SOC_ENUM_EXT("tx hub mode", sunxi_tx_hub_mode_enum,
		     sunxi_get_tx_hub_mode, sunxi_set_tx_hub_mode),
};
static const struct snd_kcontrol_new sunxi_rx_sync_controls[] = {
	SOC_ENUM_EXT("rx sync mode", sunxi_rx_sync_mode_enum,
		     sunxi_get_rx_sync_mode, sunxi_set_rx_sync_mode),
};
static const struct snd_kcontrol_new sunxi_daudio_controls[] = {
	SOC_SINGLE("loopback debug", SUNXI_DAUDIO_CTL, LOOP_EN, 1, 0),
};

static int sunxi_daudio_component_probe(struct snd_soc_component *component)
{
	struct sunxi_daudio *daudio = snd_soc_component_get_drvdata(component);
	const struct sunxi_daudio_quirks *quirks = daudio->quirks;
	struct sunxi_daudio_dts *dts = &daudio->dts;
	int ret;

	SND_LOG_DEBUG(HLOG, "\n");

	/* for hdmi */
	if (dts->dai_type == SUNXI_DAI_HDMI_TYPE) {
		ret = snd_sunxi_hdmi_init();
		if (ret) {
			SND_LOG_ERR(HLOG, "hdmi audio init failed\n");
			return -1;
		}
		ret = snd_sunxi_hdmi_add_controls(component);
		if (ret) {
			SND_LOG_ERR(HLOG, "add hdmiaudio kcontrols failed\n");
			return -1;
		}
		daudio->hdmi_fmt = snd_sunxi_hdmi_get_fmt();
	} else {
		daudio->hdmi_fmt = HDMI_FMT_NULL;
	}

	/* component kcontrols -> tx_hub */
	if (dts->tx_hub_en) {
		ret = snd_soc_add_component_controls(component, sunxi_tx_hub_controls,
						     ARRAY_SIZE(sunxi_tx_hub_controls));
		if (ret)
			SND_LOG_ERR(HLOG, "add tx_hub kcontrols failed\n");
	}

	/* component kcontrols -> rx_sync */
	if (dts->rx_sync_en && quirks->rx_sync_en) {
		ret = snd_soc_add_component_controls(component, sunxi_rx_sync_controls,
						     ARRAY_SIZE(sunxi_rx_sync_controls));
		if (ret)
			SND_LOG_ERR(HLOG, "add rx_sync kcontrols failed\n");
	}

	return 0;
}

static int sunxi_daudio_component_suspend(struct snd_soc_component *component)
{
	struct sunxi_daudio *daudio = snd_soc_component_get_drvdata(component);
	const struct sunxi_daudio_quirks *quirks = daudio->quirks;
	struct sunxi_daudio_clk *clk = &daudio->clk;
	struct sunxi_daudio_rglt *rglt = &daudio->rglt;
	struct regmap *regmap = daudio->mem.regmap;

	SND_LOG_DEBUG(HLOG, "\n");

	/* save reg value */
	snd_sunxi_save_reg(regmap, quirks->reg_labels);

	/* disable clk & regulator */
	snd_sunxi_rglt_disable(daudio->pdev, rglt);
	snd_sunxi_clk_disable(clk);

	return 0;
}

static int sunxi_daudio_component_resume(struct snd_soc_component *component)
{
	struct sunxi_daudio *daudio = snd_soc_component_get_drvdata(component);
	const struct sunxi_daudio_quirks *quirks = daudio->quirks;
	struct sunxi_daudio_clk *clk = &daudio->clk;
	struct sunxi_daudio_rglt *rglt = &daudio->rglt;
	struct regmap *regmap = daudio->mem.regmap;
	int ret;
	int i;

	SND_LOG_DEBUG(HLOG, "\n");

	ret = snd_sunxi_clk_enable(clk);
	if (ret) {
		SND_LOG_ERR(HLOG, "clk enable failed\n");
		return ret;
	}
	ret = snd_sunxi_rglt_enable(daudio->pdev, rglt);
	if (ret) {
		SND_LOG_ERR(HLOG, "regulator enable failed\n");
		return ret;
	}

	/* for daudio init */
	sunxi_daudio_init(daudio);

	/* resume reg value */
	snd_sunxi_echo_reg(regmap, quirks->reg_labels);

	/* for clear TX fifo */
	for (i = 0 ; i < 10 ; i++) {
		regmap_update_bits(regmap, SUNXI_DAUDIO_FIFOCTL,
				   1 << FIFO_CTL_FTX, 1 << FIFO_CTL_FTX);
		mdelay(1);
	}
	regmap_write(regmap, SUNXI_DAUDIO_TXCNT, 0);

	/* for clear RX fifo */
	regmap_update_bits(regmap, SUNXI_DAUDIO_FIFOCTL, 1 << FIFO_CTL_FRX, 1 << FIFO_CTL_FRX);
	regmap_write(regmap, SUNXI_DAUDIO_RXCNT, 0);

	return 0;
}

static struct snd_soc_component_driver sunxi_daudio_dev = {
	.name		= DRV_NAME,
	.probe		= sunxi_daudio_component_probe,
	.suspend	= sunxi_daudio_component_suspend,
	.resume		= sunxi_daudio_component_resume,
	.controls	= sunxi_daudio_controls,
	.num_controls	= ARRAY_SIZE(sunxi_daudio_controls),
};

/*******************************************************************************
 * *** kernel source ***
 * @1 regmap
 * @2 clk
 * @3 regulator
 * @4 dts params
 * @5 dma params
 * @6 pinctrl
 * @7 reg debug
 ******************************************************************************/
static int snd_sunxi_save_reg(struct regmap *regmap, struct audio_reg_label *reg_labels)
{
	int i = 0;

	SND_LOG_DEBUG(HLOG, "\n");

	while (reg_labels[i].name != NULL) {
		regmap_read(regmap, reg_labels[i].address, &(reg_labels[i].value));
		i++;
	}

	return i;
}

static int snd_sunxi_echo_reg(struct regmap *regmap, struct audio_reg_label *reg_labels)
{
	int i = 0;

	SND_LOG_DEBUG(HLOG, "\n");

	while (reg_labels[i].name != NULL) {
		regmap_write(regmap, reg_labels[i].address, reg_labels[i].value);
		i++;
	}

	return i;
}

static int snd_sunxi_mem_init(struct platform_device *pdev, struct sunxi_daudio_mem *mem)
{
	int ret = 0;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG(HLOG, "\n");

	ret = of_address_to_resource(np, 0, &mem->res);
	if (ret) {
		SND_LOG_ERR(HLOG, "parse device node resource failed\n");
		ret = -EINVAL;
		goto err_of_addr_to_resource;
	}

	mem->memregion = devm_request_mem_region(&pdev->dev, mem->res.start,
						 resource_size(&mem->res),
						 DRV_NAME);
	if (IS_ERR_OR_NULL(mem->memregion)) {
		SND_LOG_ERR(HLOG, "memory region already claimed\n");
		ret = -EBUSY;
		goto err_devm_request_region;
	}

	mem->membase = devm_ioremap(&pdev->dev, mem->memregion->start,
				    resource_size(mem->memregion));
	if (IS_ERR_OR_NULL(mem->membase)) {
		SND_LOG_ERR(HLOG, "ioremap failed\n");
		ret = -EBUSY;
		goto err_devm_ioremap;
	}

	mem->regmap = devm_regmap_init_mmio(&pdev->dev, mem->membase, &g_regmap_config);
	if (IS_ERR_OR_NULL(mem->regmap)) {
		SND_LOG_ERR(HLOG, "regmap init failed\n");
		ret = -EINVAL;
		goto err_devm_regmap_init;
	}

	return 0;

err_devm_regmap_init:
	devm_iounmap(&pdev->dev, mem->membase);
err_devm_ioremap:
	devm_release_mem_region(&pdev->dev, mem->memregion->start, resource_size(mem->memregion));
err_devm_request_region:
err_of_addr_to_resource:
	return ret;
}

static void snd_sunxi_mem_exit(struct platform_device *pdev, struct sunxi_daudio_mem *mem)
{
	SND_LOG_DEBUG(HLOG, "\n");

	devm_iounmap(&pdev->dev, mem->membase);
	devm_release_mem_region(&pdev->dev, mem->memregion->start, resource_size(mem->memregion));
}

static int snd_sunxi_rglt_init(struct platform_device *pdev, struct sunxi_daudio_rglt *rglt)
{
	int ret = 0;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG(HLOG, "\n");

	rglt->rglt_name = NULL;
	ret = of_property_read_string(np, "daudio-regulator", &rglt->rglt_name);
	if (ret < 0) {
		SND_LOG_DEBUG(HLOG, "regulator missing or invalid\n");
		rglt->daudio_rglt = NULL;
		return 0;
	}

	rglt->daudio_rglt = regulator_get(NULL, rglt->rglt_name);
	if (IS_ERR_OR_NULL(rglt->daudio_rglt)) {
		SND_LOG_ERR(HLOG, "get duaido vcc-pin failed\n");
		ret = -EFAULT;
		goto err_regulator_get;
	}
	ret = regulator_set_voltage(rglt->daudio_rglt, 3300000, 3300000);
	if (ret < 0) {
		SND_LOG_ERR(HLOG, "set duaido voltage failed\n");
		ret = -EFAULT;
		goto err_regulator_set_vol;
	}
	ret = regulator_enable(rglt->daudio_rglt);
	if (ret < 0) {
		SND_LOG_ERR(HLOG, "enable duaido vcc-pin failed\n");
		ret = -EFAULT;
		goto err_regulator_enable;
	}

	return 0;

err_regulator_enable:
err_regulator_set_vol:
	if (rglt->daudio_rglt)
		regulator_put(rglt->daudio_rglt);
err_regulator_get:
	return ret;
}

static void snd_sunxi_rglt_exit(struct platform_device *pdev, struct sunxi_daudio_rglt *rglt)
{
	SND_LOG_DEBUG(HLOG, "\n");

	if (rglt->daudio_rglt)
		if (!IS_ERR_OR_NULL(rglt->daudio_rglt)) {
			regulator_disable(rglt->daudio_rglt);
			regulator_put(rglt->daudio_rglt);
		}
}

static int snd_sunxi_rglt_enable(struct platform_device *pdev, struct sunxi_daudio_rglt *rglt)
{
	int ret;

	SND_LOG_DEBUG(HLOG, "\n");

	if (rglt->daudio_rglt)
		if (!IS_ERR_OR_NULL(rglt->daudio_rglt)) {
			ret = regulator_enable(rglt->daudio_rglt);
			if (ret) {
				SND_LOG_ERR(HLOG, "enable daudio_rglt failed\n");
				return -1;
			}
		}

	return 0;
}

static void snd_sunxi_rglt_disable(struct platform_device *pdev, struct sunxi_daudio_rglt *rglt)
{
	SND_LOG_DEBUG(HLOG, "\n");

	if (rglt->daudio_rglt)
		if (!IS_ERR_OR_NULL(rglt->daudio_rglt))
			regulator_disable(rglt->daudio_rglt);
}

static void snd_sunxi_dts_params_init(struct platform_device *pdev, struct sunxi_daudio_dts *dts)
{
	int i;
	int ret = 0;
	unsigned int temp_val;
	unsigned int tx_pin_size, rx_pin_size;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG(HLOG, "\n");

	/* get dma params */
	ret = of_property_read_u32(np, "playback-cma", &temp_val);
	if (ret < 0) {
		dts->playback_cma = SUNXI_AUDIO_CMA_MAX_KBYTES;
		SND_LOG_WARN(HLOG, "playback-cma missing, using default value\n");
	} else {
		if (temp_val		> SUNXI_AUDIO_CMA_MAX_KBYTES)
			temp_val	= SUNXI_AUDIO_CMA_MAX_KBYTES;
		else if (temp_val	< SUNXI_AUDIO_CMA_MIN_KBYTES)
			temp_val	= SUNXI_AUDIO_CMA_MIN_KBYTES;

		dts->playback_cma = temp_val;
	}
	ret = of_property_read_u32(np, "capture-cma", &temp_val);
	if (ret != 0) {
		dts->capture_cma = SUNXI_AUDIO_CMA_MAX_KBYTES;
		SND_LOG_WARN(HLOG, "capture-cma missing, using default value\n");
	} else {
		if (temp_val		> SUNXI_AUDIO_CMA_MAX_KBYTES)
			temp_val	= SUNXI_AUDIO_CMA_MAX_KBYTES;
		else if (temp_val	< SUNXI_AUDIO_CMA_MIN_KBYTES)
			temp_val	= SUNXI_AUDIO_CMA_MIN_KBYTES;

		dts->capture_cma = temp_val;
	}
	ret = of_property_read_u32(np, "tx-fifo-size", &temp_val);
	if (ret != 0) {
		dts->playback_fifo_size = SUNXI_AUDIO_FIFO_SIZE;
		SND_LOG_WARN(HLOG, "tx-fifo-size miss, using default value\n");
	} else {
		dts->playback_fifo_size = temp_val;
	}
	ret = of_property_read_u32(np, "rx-fifo-size", &temp_val);
	if (ret != 0) {
		dts->capture_fifo_size = SUNXI_AUDIO_FIFO_SIZE;
		SND_LOG_WARN(HLOG, "rx-fifo-size miss,using default value\n");
	} else {
		dts->capture_fifo_size = temp_val;
	}

	ret = of_property_read_u32(np, "tdm-num", &temp_val);
	if (ret < 0) {
		SND_LOG_WARN(HLOG, "tdm-num config missing\n");
		dts->tdm_num = 0;
	} else {
		dts->tdm_num = temp_val;
	}

	tx_pin_size = of_property_count_elems_of_size(np, "tx-pin", 4);
	for (i = 0; i < tx_pin_size; i++) {
		ret = of_property_read_u32_index(np, "tx-pin", i, &temp_val);
		if (temp_val > 3) {
			SND_LOG_WARN(HLOG, "tx-pin[%d] config invalid\n", i);
			continue;
		}
		if (ret < 0) {
			dts->tx_pin[temp_val] = false;
			SND_LOG_WARN(HLOG, "tx-pin[%d] config missing\n", i);
		} else {
			dts->tx_pin[temp_val] = true;
		}
	}

	rx_pin_size = of_property_count_elems_of_size(np, "rx-pin", 4);
	for (i = 0; i < rx_pin_size; i++) {
		ret = of_property_read_u32_index(np, "rx-pin", i, &temp_val);
		if (temp_val > 3) {
			SND_LOG_WARN(HLOG, "rx-pin[%d] config invalid\n", i);
			continue;
		}
		if (ret < 0) {
			dts->rx_pin[temp_val] = false;
			SND_LOG_WARN(HLOG, "rx-pin[%d] config missing\n", i);
		} else {
			dts->rx_pin[temp_val] = true;
		}
	}

	SND_LOG_DEBUG(HLOG, "playback-cma : %zu\n", dts->playback_cma);
	SND_LOG_DEBUG(HLOG, "capture-cma  : %zu\n", dts->capture_cma);
	SND_LOG_DEBUG(HLOG, "tx-fifo-size : %zu\n", dts->playback_fifo_size);
	SND_LOG_DEBUG(HLOG, "rx-fifo-size : %zu\n", dts->capture_fifo_size);
	SND_LOG_DEBUG(HLOG, "tx-pin       : %u\n", dts->tx_pin[0]);
	SND_LOG_DEBUG(HLOG, "rx-pin       : %u\n", dts->rx_pin[0]);

	/* tx_hub */
	dts->tx_hub_en = of_property_read_bool(np, "tx-hub-en");

	/* components func -> rx_sync */
	dts->rx_sync_en = of_property_read_bool(np, "rx-sync-en");
	if (dts->rx_sync_en) {
		dts->rx_sync_ctl = false;
		dts->rx_sync_domain = RX_SYNC_SYS_DOMAIN;
		dts->rx_sync_id = sunxi_rx_sync_probe(dts->rx_sync_domain);
		if (dts->rx_sync_id < 0) {
			SND_LOG_ERR(HLOG, "sunxi_rx_sync_probe failed\n");
		} else {
			SND_LOG_DEBUG(HLOG, "sunxi_rx_sync_probe successful. domain=%d, id=%d\n",
				      dts->rx_sync_domain, dts->rx_sync_id);
		}
	}

	/* dai-type */
	ret = snd_sunxi_hdmi_get_dai_type(np, &dts->dai_type);
	if (ret)
		dts->dai_type = SUNXI_DAI_I2S_TYPE;
}

static int snd_sunxi_pin_init(struct platform_device *pdev, struct sunxi_daudio_pinctl *pin)
{
	int ret = 0;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG(HLOG, "\n");

	if (of_property_read_bool(np, "pinctrl-used")) {
		pin->pinctrl_used = 1;
	} else {
		pin->pinctrl_used = 0;
		SND_LOG_DEBUG(HLOG, "unused pinctrl\n");
		return 0;
	}

	pin->pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR_OR_NULL(pin->pinctrl)) {
		SND_LOG_ERR(HLOG, "pinctrl get failed\n");
		ret = -EINVAL;
		return ret;
	}
	pin->pinstate = pinctrl_lookup_state(pin->pinctrl, PINCTRL_STATE_DEFAULT);
	if (IS_ERR_OR_NULL(pin->pinstate)) {
		SND_LOG_ERR(HLOG, "pinctrl default state get fail\n");
		ret = -EINVAL;
		goto err_loopup_pinstate;
	}
	pin->pinstate_sleep = pinctrl_lookup_state(pin->pinctrl, PINCTRL_STATE_SLEEP);
	if (IS_ERR_OR_NULL(pin->pinstate_sleep)) {
		SND_LOG_ERR(HLOG, "pinctrl sleep state get failed\n");
		ret = -EINVAL;
		goto err_loopup_pin_sleep;
	}
	ret = pinctrl_select_state(pin->pinctrl, pin->pinstate);
	if (ret < 0) {
		SND_LOG_ERR(HLOG, "daudio set pinctrl default state fail\n");
		ret = -EBUSY;
		goto err_pinctrl_select_default;
	}

	return 0;

err_pinctrl_select_default:
err_loopup_pin_sleep:
err_loopup_pinstate:
	devm_pinctrl_put(pin->pinctrl);
	return ret;
}

static void snd_sunxi_dma_params_init(struct sunxi_daudio *daudio)
{
	struct resource *res = &daudio->mem.res;
	struct sunxi_daudio_dts *dts = &daudio->dts;

	SND_LOG_DEBUG(HLOG, "\n");

	daudio->playback_dma_param.src_maxburst = 8;
	daudio->playback_dma_param.dst_maxburst = 8;
	daudio->playback_dma_param.dma_addr = res->start + SUNXI_DAUDIO_TXFIFO;
	daudio->playback_dma_param.cma_kbytes = dts->playback_cma;
	daudio->playback_dma_param.fifo_size = dts->playback_fifo_size;

	daudio->capture_dma_param.src_maxburst = 8;
	daudio->capture_dma_param.dst_maxburst = 8;
	daudio->capture_dma_param.dma_addr = res->start + SUNXI_DAUDIO_RXFIFO;
	daudio->capture_dma_param.cma_kbytes = dts->capture_cma;
	daudio->capture_dma_param.fifo_size = dts->capture_fifo_size;
};

static void snd_sunxi_pin_exit(struct platform_device *pdev, struct sunxi_daudio_pinctl *pin)
{
	SND_LOG_DEBUG(HLOG, "\n");

	if (pin->pinctrl_used)
		devm_pinctrl_put(pin->pinctrl);
}

/* sysfs debug */
static ssize_t snd_sunxi_debug_show_reg(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	size_t count = 0;

	count += sprintf(buf + count, "usage->read : echo [num] > audio_reg\n");

	count += sprintf(buf + count, "usage->write: echo [reg] [value] > audio_reg\n");

	count += sprintf(buf + count, "num: 0.all\n");

	return count;
}

static ssize_t snd_sunxi_debug_store_reg(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t count)
{
	struct sunxi_daudio *daudio = dev_get_drvdata(dev);
	const struct sunxi_daudio_quirks *quirks = daudio->quirks;
	struct regmap *regmap = daudio->mem.regmap;
	int scanf_cnt;
	unsigned int num = 0, i = 0;
	unsigned int output_reg_val;
	unsigned int input_reg_val;
	unsigned int input_reg_offset;
	unsigned int size = quirks->reg_labels_size;

	if (buf[1] == 'x')
		scanf_cnt = sscanf(buf, "0x%x 0x%x", &input_reg_offset, &input_reg_val);
	else
		scanf_cnt = sscanf(buf, "%x", &num);

	if (scanf_cnt <= 0 || num != 0) {
		pr_err("please get the usage by\"cat audio_reg\"\n");
		return count;
	}

	if (scanf_cnt == 1) {
		while ((i < size) && (quirks->reg_labels[i].name != NULL)) {
			regmap_read(regmap, quirks->reg_labels[i].address, &output_reg_val);
			pr_info("%-32s [0x%03x]: 0x%8x :0x%x\n",
				quirks->reg_labels[i].name,
				quirks->reg_labels[i].address, output_reg_val,
				quirks->reg_labels[i].value);
			i++;
		}
		return count;
	} else if (scanf_cnt == 2) {
		if (input_reg_offset > quirks->reg_max) {
			pr_err("reg offset > audio max reg[0x%x]\n", quirks->reg_max);
			return count;
		}

		regmap_read(regmap, input_reg_offset, &output_reg_val);
		pr_info("reg[0x%03x]: 0x%x (old)\n", input_reg_offset, output_reg_val);
		regmap_write(regmap, input_reg_offset, input_reg_val);
		regmap_read(regmap, input_reg_offset, &output_reg_val);
		pr_info("reg[0x%03x]: 0x%x (new)\n", input_reg_offset, output_reg_val);
	}

	return count;
}

static DEVICE_ATTR(audio_reg, 0644, snd_sunxi_debug_show_reg, snd_sunxi_debug_store_reg);

static struct attribute *audio_debug_attrs[] = {
	&dev_attr_audio_reg.attr,
	NULL,
};

static struct attribute_group debug_attr = {
	.name	= "audio_debug",
	.attrs	= audio_debug_attrs,
};

static int sunxi_daudio_dev_probe(struct platform_device *pdev)
{
	int ret;
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	struct sunxi_daudio *daudio;
	struct sunxi_daudio_mem *mem;
	struct sunxi_daudio_clk *clk;
	struct sunxi_daudio_pinctl *pin;
	struct sunxi_daudio_dts *dts;
	struct sunxi_daudio_rglt *rglt;
	const struct sunxi_daudio_quirks *quirks;

	SND_LOG_DEBUG(HLOG, "\n");

	/* sunxi daudio */
	daudio = devm_kzalloc(dev, sizeof(*daudio), GFP_KERNEL);
	if (IS_ERR_OR_NULL(daudio)) {
		SND_LOG_ERR(HLOG, "alloc sunxi_daudio failed\n");
		ret = -ENOMEM;
		goto err_devm_kzalloc;
	}
	dev_set_drvdata(dev, daudio);

	daudio->pdev = pdev;
	mem = &daudio->mem;
	clk = &daudio->clk;
	pin = &daudio->pin;
	dts = &daudio->dts;
	rglt = &daudio->rglt;

	ret = snd_sunxi_mem_init(pdev, mem);
	if (ret) {
		SND_LOG_ERR(HLOG, "remap init failed\n");
		ret = -EINVAL;
		goto err_snd_sunxi_mem_init;
	}

	ret = snd_sunxi_clk_init(pdev, clk);
	if (ret) {
		SND_LOG_ERR(HLOG, "clk init failed\n");
		ret = -EINVAL;
		goto err_snd_sunxi_clk_init;
	}

	ret = snd_sunxi_rglt_init(pdev, rglt);
	if (ret) {
		SND_LOG_ERR(HLOG, "regulator init failed\n");
		ret = -ENOMEM;
		goto err_snd_sunxi_rglt_init;
	}

	snd_sunxi_dts_params_init(pdev, dts);
	snd_sunxi_dma_params_init(daudio);

	ret = snd_sunxi_pin_init(pdev, pin);
	if (ret) {
		SND_LOG_ERR(HLOG, "pinctrl init failed\n");
		ret = -EINVAL;
		goto err_snd_sunxi_pin_init;
	}

	ret = snd_soc_register_component(&pdev->dev, &sunxi_daudio_dev, &sunxi_daudio_dai, 1);
	if (ret) {
		SND_LOG_ERR(HLOG, "component register failed\n");
		ret = -ENOMEM;
		goto err_snd_soc_register_component;
	}

	if (dts->dai_type == SUNXI_DAI_HDMI_TYPE)
		ret = snd_sunxi_hdmi_platform_register(&pdev->dev);
	else
		ret = snd_sunxi_dma_platform_register(&pdev->dev);
	if (ret) {
		SND_LOG_ERR(HLOG, "register ASoC platform failed\n");
		ret = -ENOMEM;
		goto err_snd_sunxi_platform_register;
	}

	quirks = of_device_get_match_data(&pdev->dev);
	if (quirks == NULL) {
		SND_LOG_ERR(HLOG, "quirks get failed\n");
		return -ENODEV;
	}
	daudio->quirks = quirks;

	ret = snd_sunxi_sysfs_create_group(pdev, &debug_attr);
	if (ret)
		SND_LOG_WARN(HLOG, "sysfs debug create failed\n");

	SND_LOG_DEBUG(HLOG, "register daudio platform success\n");

	return 0;

err_snd_sunxi_platform_register:
	snd_soc_unregister_component(&pdev->dev);
err_snd_soc_register_component:
	snd_sunxi_pin_exit(pdev, pin);
err_snd_sunxi_pin_init:
	snd_sunxi_rglt_exit(pdev, rglt);
err_snd_sunxi_rglt_init:
	snd_sunxi_clk_exit(clk);
err_snd_sunxi_clk_init:
	snd_sunxi_mem_exit(pdev, mem);
err_snd_sunxi_mem_init:
	devm_kfree(dev, daudio);
err_devm_kzalloc:
	of_node_put(np);

	return ret;
}

static int sunxi_daudio_dev_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	struct sunxi_daudio *daudio = dev_get_drvdata(dev);
	struct sunxi_daudio_mem *mem = &daudio->mem;
	struct sunxi_daudio_clk *clk = &daudio->clk;
	struct sunxi_daudio_pinctl *pin = &daudio->pin;
	struct sunxi_daudio_dts *dts = &daudio->dts;
	struct sunxi_daudio_rglt *rglt = &daudio->rglt;

	SND_LOG_DEBUG(HLOG, "\n");

	/* remove components */
	snd_sunxi_sysfs_remove_group(pdev, &debug_attr);
	if (dts->rx_sync_en)
		sunxi_rx_sync_remove(dts->rx_sync_domain);

	if (dts->dai_type == SUNXI_DAI_HDMI_TYPE)
		snd_sunxi_hdmi_platform_unregister(dev);
	else
		snd_sunxi_dma_platform_unregister(&pdev->dev);

	snd_soc_unregister_component(&pdev->dev);

	snd_sunxi_pin_exit(pdev, pin);
	snd_sunxi_clk_exit(clk);
	snd_sunxi_mem_exit(pdev, mem);
	snd_sunxi_rglt_exit(pdev, rglt);

	devm_kfree(dev, daudio);
	of_node_put(np);

	SND_LOG_DEBUG(HLOG, "unregister daudio platform success\n");

	return 0;
}

static const struct sunxi_daudio_quirks sunxi_daudio_quirks = {
	.reg_labels		= sunxi_reg_labels,
	.reg_labels_size	= ARRAY_SIZE(sunxi_reg_labels),
	.reg_max		= SUNXI_DAUDIO_MAX_REG,
	.rx_sync_en = true,
	.set_channel_enable	= sunxi_daudio_set_ch_en,
	.set_daifmt_format	= sunxi_daudio_set_daifmt_fmt,
	.set_channels_map	= sunxi_daudio_set_ch_map,
};

static const struct sunxi_daudio_quirks sun8iw11_daudio_quirks = {
	.reg_labels		= sun8iw11_reg_labels,
	.reg_labels_size	= ARRAY_SIZE(sun8iw11_reg_labels),
	.reg_max		= SUN8IW11_DAUDIO_MAX_REG,
	.rx_sync_en = false,
	.set_channel_enable	= sun8iw11_daudio_set_ch_en,
	.set_daifmt_format	= sun8iw11_daudio_set_daifmt_fmt,
	.set_channels_map	= sun8iw11_daudio_set_ch_map,
};

static const struct of_device_id sunxi_daudio_of_match[] = {
	{
		.compatible = "allwinner," DRV_NAME,
		.data = &sunxi_daudio_quirks,
	},
	{
		.compatible = "allwinner,sun8iw11-daudio",
		.data = &sun8iw11_daudio_quirks,
	},
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_daudio_of_match);

static struct platform_driver sunxi_daudio_driver = {
	.driver	= {
		.name		= DRV_NAME,
		.owner		= THIS_MODULE,
		.of_match_table	= sunxi_daudio_of_match,
	},
	.probe	= sunxi_daudio_dev_probe,
	.remove	= sunxi_daudio_dev_remove,
};

int __init sunxi_daudio_dev_init(void)
{
	int ret;

	ret = platform_driver_register(&sunxi_daudio_driver);
	if (ret != 0) {
		SND_LOG_ERR(HLOG, "platform driver register failed\n");
		return -EINVAL;
	}

	return ret;
}

void __exit sunxi_daudio_dev_exit(void)
{
	platform_driver_unregister(&sunxi_daudio_driver);
}

late_initcall(sunxi_daudio_dev_init);
module_exit(sunxi_daudio_dev_exit);

MODULE_AUTHOR("Dby@allwinnertech.com");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("sunxi soundcard platform of daudio");
