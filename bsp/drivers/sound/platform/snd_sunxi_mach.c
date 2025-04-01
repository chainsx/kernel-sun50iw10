/*
 * sound\soc\sunxi\snd_sunxi_mach.c
 * (C) Copyright 2021-2025
 * AllWinner Technology Co., Ltd. <www.allwinnertech.com>
 * Dby <dby@allwinnertech.com>
 *
 * based on ${LINUX}/sound/soc/generic/simple-card.c
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */
#include <linux/module.h>
#include <sound/soc.h>
#include <sound/jack.h>

#include "snd_sunxi_log.h"
#include "snd_sunxi_jack.h"
#include "snd_sunxi_mach.h"

#define HLOG		"MACH"
#define DAI		"sound-dai"
#define CELL		"#sound-dai-cells"
#define PREFIX		"soundcard-mach,"

#define DRV_NAME	"sunxi-snd-mach"

static void asoc_simple_shutdown(struct snd_pcm_substream *substream)
{
}

static int asoc_simple_startup(struct snd_pcm_substream *substream)
{
	return 0;
}

static int asoc_simple_hw_params(struct snd_pcm_substream *substream,
				 struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = asoc_rtd_to_codec(rtd, 0);
	struct snd_soc_dai *cpu_dai = asoc_rtd_to_cpu(rtd, 0);
	struct asoc_simple_priv *priv = snd_soc_card_get_drvdata(rtd->card);
	struct snd_soc_dai_link *dai_link = simple_priv_to_link(priv, rtd->num);
	struct simple_dai_props *dai_props = simple_priv_to_props(priv, rtd->num);
	struct asoc_simple_dai *dais = priv->dais;
	unsigned int mclk;
	unsigned int cpu_pll_clk, codec_pll_clk;
	unsigned int cpu_bclk_ratio, codec_bclk_ratio;
	unsigned int freq_point;
	int cpu_clk_div, codec_clk_div;
	int ret = 0;

	switch (params_rate(params)) {
	case 8000:
	case 12000:
	case 16000:
	case 24000:
	case 32000:
	case 48000:
	case 64000:
	case 96000:
	case 192000:
		freq_point = 24576000;
		break;
	case 11025:
	case 22050:
	case 44100:
	case 88200:
	case 176400:
		freq_point = 22579200;
		break;
	default:
		SND_LOG_ERR(HLOG, "Invalid rate %d\n", params_rate(params));
		return -EINVAL;
	}

	/* for cpudai pll clk */
	cpu_pll_clk	= freq_point * dai_props->cpu_pll_fs;
	codec_pll_clk	= freq_point * dai_props->codec_pll_fs;
	cpu_clk_div	= cpu_pll_clk / params_rate(params);
	codec_clk_div	= codec_pll_clk / params_rate(params);
	SND_LOG_DEBUG(HLOG, "freq point   : %u\n", freq_point);
	SND_LOG_DEBUG(HLOG, "cpu pllclk   : %u\n", cpu_pll_clk);
	SND_LOG_DEBUG(HLOG, "codec pllclk : %u\n", codec_pll_clk);
	SND_LOG_DEBUG(HLOG, "cpu clk_div  : %u\n", cpu_clk_div);
	SND_LOG_DEBUG(HLOG, "codec clk_div: %u\n", codec_clk_div);

	if (cpu_dai->driver->ops && cpu_dai->driver->ops->set_pll) {
		ret = snd_soc_dai_set_pll(cpu_dai, substream->stream, 0,
					  cpu_pll_clk, cpu_pll_clk);
		if (ret) {
			SND_LOG_ERR(HLOG, "cpu_dai set pllclk failed\n");
			return ret;
		}
	} else if (cpu_dai->component->driver->set_pll) {
		ret = snd_soc_component_set_pll(cpu_dai->component, substream->stream, 0,
						cpu_pll_clk, cpu_pll_clk);
		if (ret) {
			SND_LOG_ERR(HLOG, "cpu_dai set pllclk failed\n");
			return ret;
		}
	}
	if (codec_dai->driver->ops && codec_dai->driver->ops->set_pll) {
		ret = snd_soc_dai_set_pll(codec_dai, substream->stream, 0,
					  codec_pll_clk, codec_pll_clk);
		if (ret) {
			SND_LOG_ERR(HLOG, "codec_dai set pllclk failed\n");
			return ret;
		}
	} else if (codec_dai->component->driver->set_pll) {
		ret = snd_soc_component_set_pll(codec_dai->component, substream->stream, 0,
						codec_pll_clk, codec_pll_clk);
		if (ret) {
			SND_LOG_ERR(HLOG, "codec_dai set pllclk failed\n");
			return ret;
		}
	}

	if (cpu_dai->driver->ops && cpu_dai->driver->ops->set_clkdiv) {
		ret = snd_soc_dai_set_clkdiv(cpu_dai, 0, cpu_clk_div);
		if (ret) {
			SND_LOG_ERR(HLOG, "cpu_dai set clk_div failed\n");
			return ret;
		}
	}
	if (codec_dai->driver->ops && codec_dai->driver->ops->set_clkdiv) {
		ret = snd_soc_dai_set_clkdiv(codec_dai, 0, codec_clk_div);
		if (ret) {
			SND_LOG_ERR(HLOG, "cadec_dai set clk_div failed.\n");
			return ret;
		}
	}

	/* use for i2s/pcm only */
	if (!(dais->slots && dais->slot_width))
		return 0;

	/* for cpudai & codecdai mclk */
	if (dai_props->mclk_fp)
		mclk = (freq_point >> 1) * dai_props->mclk_fs;
	else
		mclk = params_rate(params) * dai_props->mclk_fs;
	cpu_bclk_ratio = cpu_pll_clk / (params_rate(params) * dais->slot_width * dais->slots);
	codec_bclk_ratio = codec_pll_clk / (params_rate(params) * dais->slot_width * dais->slots);
	SND_LOG_DEBUG(HLOG, "mclk            : %u\n", mclk);
	SND_LOG_DEBUG(HLOG, "cpu_bclk_ratio  : %u\n", cpu_bclk_ratio);
	SND_LOG_DEBUG(HLOG, "codec_bclk_ratio: %u\n", codec_bclk_ratio);

	if (cpu_dai->driver->ops && cpu_dai->driver->ops->set_sysclk) {
		ret = snd_soc_dai_set_sysclk(cpu_dai, 0, mclk, SND_SOC_CLOCK_OUT);
		if (ret) {
			SND_LOG_ERR(HLOG, "cpu_dai set sysclk(mclk) failed\n");
			return ret;
		}
	}
	if (codec_dai->driver->ops && codec_dai->driver->ops->set_sysclk) {
		ret = snd_soc_dai_set_sysclk(codec_dai, 0, mclk, SND_SOC_CLOCK_IN);
		if (ret) {
			SND_LOG_ERR(HLOG, "cadec_dai set sysclk(mclk) failed\n");
			return ret;
		}
	}

	if (cpu_dai->driver->ops && cpu_dai->driver->ops->set_bclk_ratio) {
		ret = snd_soc_dai_set_bclk_ratio(cpu_dai, cpu_bclk_ratio);
		if (ret) {
			SND_LOG_ERR(HLOG, "cpu_dai set bclk failed\n");
			return ret;
		}
	}
	if (codec_dai->driver->ops && codec_dai->driver->ops->set_bclk_ratio) {
		ret = snd_soc_dai_set_bclk_ratio(codec_dai, codec_bclk_ratio);
		if (ret) {
			SND_LOG_ERR(HLOG, "codec_dai set bclk failed\n");
			return ret;
		}
	}

	if (cpu_dai->driver->ops && cpu_dai->driver->ops->set_fmt) {
		ret = snd_soc_dai_set_fmt(cpu_dai, dai_link->dai_fmt);
		if (ret) {
			SND_LOG_ERR(HLOG, "cpu dai set fmt failed\n");
			return ret;
		}
	}
	if (codec_dai->driver->ops && codec_dai->driver->ops->set_fmt) {
		ret = snd_soc_dai_set_fmt(codec_dai, dai_link->dai_fmt);
		if (ret) {
			SND_LOG_ERR(HLOG, "codec dai set fmt failed\n");
			return ret;
		}
	}

	if (cpu_dai->driver->ops && cpu_dai->driver->ops->set_tdm_slot) {
		ret = snd_soc_dai_set_tdm_slot(cpu_dai, 0, 0, dais->slots, dais->slot_width);
		if (ret) {
			SND_LOG_ERR(HLOG, "cpu dai set tdm slot failed\n");
			return ret;
		}
	}
	if (codec_dai->driver->ops && codec_dai->driver->ops->set_tdm_slot) {
		ret = snd_soc_dai_set_tdm_slot(codec_dai, 0, 0, dais->slots, dais->slot_width);
		if (ret) {
			SND_LOG_ERR(HLOG, "codec dai set tdm slot failed\n");
			return ret;
		}
	}

	return 0;
}

static struct snd_soc_ops simple_ops = {
	.startup = asoc_simple_startup,
	.shutdown = asoc_simple_shutdown,
	.hw_params = asoc_simple_hw_params,
};

static int asoc_simple_dai_init(struct snd_soc_pcm_runtime *rtd)
{
	int i;
	struct snd_soc_dai *codec_dai = asoc_rtd_to_codec(rtd, 0);
	struct snd_soc_component *component = codec_dai->component;
	struct snd_soc_dapm_context *dapm = &component->dapm;
	struct snd_soc_card *card = rtd->card;
	const struct snd_kcontrol_new *controls = card->controls;

	for (i = 0; i < card->num_controls; i++)
		if (controls[i].info == snd_soc_dapm_info_pin_switch)
			snd_soc_dapm_disable_pin(dapm, (const char *)controls[i].private_value);

	if (card->num_controls)
		snd_soc_dapm_sync(dapm);

	/* snd_soc_dai_set_sysclk(); */
	/* snd_soc_dai_set_tdm_slot(); */

	return 0;
}

static int simple_dai_link_of(struct device_node *node, struct asoc_simple_priv *priv)
{
	struct device *dev = simple_priv_to_dev(priv);
	struct snd_soc_dai_link *dai_link = simple_priv_to_link(priv, 0);
	struct simple_dai_props *dai_props = simple_priv_to_props(priv, 0);
	struct device_node *top_np = NULL;
	struct device_node *cpu = NULL;
	struct device_node *plat = NULL;
	struct device_node *codec = NULL;
	char prop[128];
	char *prefix = "";
	int ret, single_cpu;

	prefix = PREFIX;
	top_np = node;

	snprintf(prop, sizeof(prop), "%scpu", prefix);
	cpu = of_get_child_by_name(top_np, prop);
	if (!cpu) {
		ret = -EINVAL;
		SND_LOG_ERR(HLOG, "Can't find %s DT node\n", prop);
		goto dai_link_of_err;
	}
	snprintf(prop, sizeof(prop), "%splat", prefix);
	plat = of_get_child_by_name(top_np, prop);

	snprintf(prop, sizeof(prop), "%scodec", prefix);
	codec = of_get_child_by_name(top_np, prop);
	if (!codec) {
		ret = -EINVAL;
		SND_LOG_ERR(HLOG, "Can't find %s DT node\n", prop);
		goto dai_link_of_err;
	}

	ret = asoc_simple_parse_daifmt(top_np, codec, prefix, &dai_link->dai_fmt);
	if (ret < 0)
		goto dai_link_of_err;
	/* sunxi: parse stream direction
	 * ex1)
	 * top_node {
	 *	PREFIXplayback-only;
	 * }
	 * ex2)
	 * top_node {
	 *	PREFIXcapture-only;
	 * }
	 */
	ret = asoc_simple_parse_daistream(top_np, prefix, dai_link);
	if (ret < 0)
		goto dai_link_of_err;
	/* sunxi: parse slot-num & slot-width
	 * ex)
	 * top_node {
	 *	PREFIXplayslot-num	= <x>;
	 *	PREFIXplayslot-width	= <x>;
	 * }
	 */
	ret = asoc_simple_parse_tdm_slot(top_np, prefix, priv->dais);
	if (ret < 0)
		goto dai_link_of_err;

	ret = asoc_simple_parse_cpu(cpu, dai_link, DAI, CELL, &single_cpu);
	if (ret < 0)
		goto dai_link_of_err;
	ret = asoc_simple_parse_codec(codec, dai_link, DAI, CELL);
	if (ret < 0) {
		if (ret == -EPROBE_DEFER)
			goto dai_link_of_err;
		dai_link->codecs->name = "snd-soc-dummy";
		dai_link->codecs->dai_name = "snd-soc-dummy-dai";
		/* dai_link->codecs->name = "sunxi-dummy-codec"; */
		/* dai_link->codecs->dai_name = "sunxi-dummy-codec-dai"; */
		SND_LOG_DEBUG(HLOG, "use dummy codec for simple card.\n");
	}
	ret = asoc_simple_parse_platform(plat, dai_link, DAI, CELL);
	if (ret < 0)
		goto dai_link_of_err;

	/* sunxi: parse pll-fs & mclk-fs
	 * ex)
	 * top_node {
	 *	PREFIXcpu {
	 *		PREFIXpll-fs	= <x>;
	 *		PREFIXmclk-fs	= <x>;
	 *	}
	 * }
	 */
	ret = asoc_simple_parse_tdm_clk(cpu, codec, prefix, dai_props);
	if (ret < 0)
		goto dai_link_of_err;

	ret = asoc_simple_set_dailink_name(dev, dai_link,
					   "%s-%s",
					   dai_link->cpus->dai_name,
					   dai_link->codecs->dai_name);
	if (ret < 0)
		goto dai_link_of_err;

	dai_link->ops = &simple_ops;
	dai_link->init = asoc_simple_dai_init;

	SND_LOG_DEBUG(HLOG, "name   : %s\n", dai_link->stream_name);
	SND_LOG_DEBUG(HLOG, "format : %x\n", dai_link->dai_fmt);
	SND_LOG_DEBUG(HLOG, "cpu    : %s\n", dai_link->cpus->name);
	SND_LOG_DEBUG(HLOG, "codec  : %s\n", dai_link->codecs->name);

	asoc_simple_canonicalize_cpu(dai_link, single_cpu);
	asoc_simple_canonicalize_platform(dai_link);

dai_link_of_err:
	of_node_put(cpu);
	of_node_put(plat);
	of_node_put(codec);

	return ret;
}

static int simple_parse_of(struct asoc_simple_priv *priv)
{
	int ret;
	struct device *dev = simple_priv_to_dev(priv);
	struct snd_soc_card *card = simple_priv_to_card(priv);
	struct device_node *top_np = dev->of_node;

	SND_LOG_DEBUG(HLOG, "\n");

	if (!top_np)
		return -EINVAL;

	/* DAPM widgets */
	ret = asoc_simple_parse_widgets(card, PREFIX);
	if (ret < 0) {
		SND_LOG_ERR(HLOG, "asoc_simple_parse_widgets failed\n");
		return ret;
	}

	/* DAPM routes */
	ret = asoc_simple_parse_routing(card, PREFIX);
	if (ret < 0) {
		SND_LOG_ERR(HLOG, "asoc_simple_parse_routing failed\n");
		return ret;
	}

	/* DAPM pin_switches */
	ret = asoc_simple_parse_pin_switches(card, PREFIX);
	if (ret < 0) {
		SND_LOG_ERR(HLOG, "asoc_simple_parse_pin_switches failed\n");
		return ret;
	}

	/* For single DAI link & old style of DT node */
	ret = simple_dai_link_of(top_np, priv);
	if (ret < 0) {
		SND_LOG_ERR(HLOG, "simple_dai_link_of failed\n");
		return ret;
	}

	ret = asoc_simple_parse_card_name(card, PREFIX);
	if (ret < 0) {
		SND_LOG_ERR(HLOG, "asoc_simple_parse_card_name failed\n");
		return ret;
	}

	return 0;
}

static int simple_soc_probe(struct snd_soc_card *card)
{
	int ret;
	char prop[128];
	struct device *dev = card->dev;
	struct asoc_simple_priv *priv = snd_soc_card_get_drvdata(card);

	SND_LOG_DEBUG(HLOG, "\n");

	snprintf(prop, sizeof(prop), "%s%s", PREFIX, "jack");
	priv->hp_jack.use = of_property_read_bool(dev->of_node, prop);

	if (priv->hp_jack.use) {
		ret = snd_sunxi_jack_register(card);
		if (ret < 0) {
			SND_LOG_ERR(HLOG, "jack init failed\n");
			return ret;
		}
	}

	return 0;
}

static int simple_soc_remove(struct snd_soc_card *card)
{
	struct asoc_simple_priv *priv = snd_soc_card_get_drvdata(card);

	SND_LOG_DEBUG(HLOG, "\n");

	if (priv->hp_jack.use)
		snd_sunxi_jack_unregister(card);

	return 0;
}

static ssize_t asoc_simple_show_i2s_dai_fmt(struct device *dev,
					    struct device_attribute *attr,
					    char *buf)
{
	size_t count = 0;

	count += sprintf(buf + count, "usage get: echo 0 > i2s_dai_fmt\n");
	count += sprintf(buf + count, "usage set: echo {Opt num} {Val} > i2s_dai_fmt\n");

	count += sprintf(buf + count, "  {Opt num}   -> {Val}                           "
			 "-> {note}\n");
	count += sprintf(buf + count, "1 CPUPLL FS   -> 1~n                             "
			 "-> 22.5792 or 24.576 * fs MHz\n");
	count += sprintf(buf + count, "2 CODECPLL FS -> 1~n                             "
			 "-> 22.5792 or 24.576 * fs MHz\n");
	count += sprintf(buf + count, "3 MCLK FP     -> Off On                          "
			 "-> mclk fs flag\n");
	count += sprintf(buf + count, "4 MCLK FS     -> 0~n                             "
			 "-> pcm rate * fs if fp Off, 11.2896 or 12.288 * fs MHz if fp On\n");
	count += sprintf(buf + count, "5 FMT         -> i2s right_j left_j dsp_a dsp_b  "
			 "-> i2s/pcm format config\n");
	count += sprintf(buf + count, "6 MASTER      -> CBM_CFM CBS_CFM CBM_CFS CBS_CFS "
			 "-> bclk&lrck master\n");
	count += sprintf(buf + count, "7 INVERT      -> NB_NF NB_IF IB_NF IB_IF         "
			 "-> bclk&lrck invert\n");
	count += sprintf(buf + count, "8 SLOTS       -> 2~32 (must be 2*n)              "
			 "-> slot number\n");
	count += sprintf(buf + count, "9 SLOT WIDTH  -> 16 24 32                        "
			 "-> slot width\n");

	return count;
}

static ssize_t asoc_simple_store_i2s_dai_fmt(struct device *dev,
					     struct device_attribute *attr,
					     const char *buf, size_t count)
{
	struct snd_soc_card *card = dev_get_drvdata(dev);
	struct asoc_simple_priv *priv = snd_soc_card_get_drvdata(card);
	unsigned int dai_fmt		= card->dai_link->dai_fmt;
	bool mclk_fp			= priv->dai_props->mclk_fp;
	unsigned int mclk_fs		= priv->dai_props->mclk_fs;
	unsigned int cpu_pll_fs		= priv->dai_props->cpu_pll_fs;
	unsigned int codec_pll_fs	= priv->dai_props->codec_pll_fs;
	int slots			= priv->dais->slots;
	int slot_width			= priv->dais->slot_width;

	int scanf_cnt;
	unsigned int scanf_num = 0;
	unsigned int dai_fmt_tmp;
	char scanf_str[8] = {0};
	char prop[8] = {0};
	bool set_sync = false;

	scanf_cnt = sscanf(buf, "%u %7s", &scanf_num, scanf_str);
	if (scanf_cnt <= 0)
		goto err;
	if (!((scanf_num == 0 && scanf_cnt == 1) || (scanf_num > 0 && scanf_cnt == 2)))
		goto err;

	switch (scanf_num) {
	case 0: /* show all i2s dai format */
		printk("CPUPLL FS   -> %u\n", cpu_pll_fs);
		printk("CODECPLL FS -> %u\n", codec_pll_fs);
		printk("MCLK FP     -> %s\n", mclk_fp?"On":"Off");
		printk("MCLK FS     -> %u\n", mclk_fs);

		switch (dai_fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
		case SND_SOC_DAIFMT_I2S:
			snprintf(prop, sizeof(prop), "%s", "i2s");
			break;
		case SND_SOC_DAIFMT_RIGHT_J:
			snprintf(prop, sizeof(prop), "%s", "right_j");
			break;
		case SND_SOC_DAIFMT_LEFT_J:
			snprintf(prop, sizeof(prop), "%s", "left_j");
			break;
		case SND_SOC_DAIFMT_DSP_A:
			snprintf(prop, sizeof(prop), "%s", "dsp_a");
			break;
		case SND_SOC_DAIFMT_DSP_B:
			snprintf(prop, sizeof(prop), "%s", "dsp_b");
			break;
		default:
			SND_LOG_ERR(HLOG, "format failed, 0x%x\n", dai_fmt);
			return -EINVAL;
		}
		printk("FMT         -> %s\n", prop);

		switch (dai_fmt & SND_SOC_DAIFMT_MASTER_MASK) {
		case SND_SOC_DAIFMT_CBM_CFM:
			snprintf(prop, sizeof(prop), "%s", "CBM_CFM");
			break;
		case SND_SOC_DAIFMT_CBS_CFM:
			snprintf(prop, sizeof(prop), "%s", "CBS_CFM");
			break;
		case SND_SOC_DAIFMT_CBM_CFS:
			snprintf(prop, sizeof(prop), "%s", "CBM_CFS");
			break;
		case SND_SOC_DAIFMT_CBS_CFS:
			snprintf(prop, sizeof(prop), "%s", "CBS_CFS");
			break;
		default:
			SND_LOG_ERR(HLOG, "master invalid, 0x%x\n", dai_fmt);
			return -EINVAL;
		}
		printk("MASTER      -> %s\n", prop);

		switch (dai_fmt & SND_SOC_DAIFMT_INV_MASK) {
		case SND_SOC_DAIFMT_NB_NF:
			snprintf(prop, sizeof(prop), "%s", "NB_NF");
			break;
		case SND_SOC_DAIFMT_NB_IF:
			snprintf(prop, sizeof(prop), "%s", "NB_IF");
			break;
		case SND_SOC_DAIFMT_IB_NF:
			snprintf(prop, sizeof(prop), "%s", "IB_NF");
			break;
		case SND_SOC_DAIFMT_IB_IF:
			snprintf(prop, sizeof(prop), "%s", "IB_IF");
			break;
		default:
			SND_LOG_ERR(HLOG, "invert invalid, 0x%x\n", dai_fmt);
			return -EINVAL;
		}
		printk("INVERT      -> %s\n", prop);

		printk("SLOTS       -> %u\n", slots);
		printk("SLOT WIDTH  -> %u\n", slot_width);
		break;
	case 1:	/* set cpu_pll_fs */
		cpu_pll_fs = simple_strtoul(scanf_str, NULL, 10);
		if (cpu_pll_fs > 0)
			set_sync = true;
		break;
	case 2: /* set codec_pll_fs */
		codec_pll_fs = simple_strtoul(scanf_str, NULL, 10);
		if (codec_pll_fs > 0)
			set_sync = true;
		break;
	case 3: /* set mclk_fp */
		if (!strncmp(scanf_str, "On", 2)) {
			mclk_fp = true;
			set_sync = true;
		} else if (!strncmp(scanf_str, "Off", 3)) {
			mclk_fp = false;
			set_sync = true;
		}
		break;
	case 4: /* set mclk_fs */
		mclk_fs = simple_strtoul(scanf_str, NULL, 10);
		set_sync = true;
		break;
	case 5: /* set dai_fmt -> FMT */
		dai_fmt_tmp = dai_fmt;
		dai_fmt &= ~SND_SOC_DAIFMT_FORMAT_MASK;
		if (!strncmp(scanf_str, "i2s", 3)) {
			dai_fmt |= SND_SOC_DAIFMT_FORMAT_MASK & SND_SOC_DAIFMT_I2S;
			set_sync = true;
		} else if (!strncmp(scanf_str, "right_j", 7)) {
			dai_fmt |= SND_SOC_DAIFMT_FORMAT_MASK & SND_SOC_DAIFMT_RIGHT_J;
			set_sync = true;
		} else if (!strncmp(scanf_str, "left_j", 6)) {
			dai_fmt |= SND_SOC_DAIFMT_FORMAT_MASK & SND_SOC_DAIFMT_LEFT_J;
			set_sync = true;
		} else if (!strncmp(scanf_str, "dsp_a", 5)) {
			dai_fmt |= SND_SOC_DAIFMT_FORMAT_MASK & SND_SOC_DAIFMT_DSP_A;
			set_sync = true;
		} else if (!strncmp(scanf_str, "dsp_b", 5)) {
			dai_fmt |= SND_SOC_DAIFMT_FORMAT_MASK & SND_SOC_DAIFMT_DSP_B;
			set_sync = true;
		} else {
			dai_fmt = dai_fmt_tmp;
		}
		break;
	case 6: /* set dai_fmt -> MASTER */
		dai_fmt_tmp = dai_fmt;
		dai_fmt &= ~SND_SOC_DAIFMT_MASTER_MASK;
		if (!strncmp(scanf_str, "CBM_CFM", 7)) {
			dai_fmt |= SND_SOC_DAIFMT_MASTER_MASK & SND_SOC_DAIFMT_CBM_CFM;
			set_sync = true;
		} else if (!strncmp(scanf_str, "CBS_CFM", 7)) {
			dai_fmt |= SND_SOC_DAIFMT_MASTER_MASK & SND_SOC_DAIFMT_CBS_CFM;
			set_sync = true;
		} else if (!strncmp(scanf_str, "CBM_CFS", 7)) {
			dai_fmt |= SND_SOC_DAIFMT_MASTER_MASK & SND_SOC_DAIFMT_CBM_CFS;
			set_sync = true;
		} else if (!strncmp(scanf_str, "CBS_CFS", 7)) {
			dai_fmt |= SND_SOC_DAIFMT_MASTER_MASK & SND_SOC_DAIFMT_CBS_CFS;
			set_sync = true;
		} else {
			dai_fmt = dai_fmt_tmp;
		}
		break;
	case 7: /* set dai_fmt -> INVERT */
		dai_fmt_tmp = dai_fmt;
		dai_fmt &= ~SND_SOC_DAIFMT_INV_MASK;
		if (!strncmp(scanf_str, "NB_NF", 5)) {
			dai_fmt |= SND_SOC_DAIFMT_INV_MASK & SND_SOC_DAIFMT_NB_NF;
			set_sync = true;
		} else if (!strncmp(scanf_str, "NB_IF", 5)) {
			dai_fmt |= SND_SOC_DAIFMT_INV_MASK & SND_SOC_DAIFMT_NB_IF;
			set_sync = true;
		} else if (!strncmp(scanf_str, "IB_NF", 5)) {
			dai_fmt |= SND_SOC_DAIFMT_INV_MASK & SND_SOC_DAIFMT_IB_NF;
			set_sync = true;
		} else if (!strncmp(scanf_str, "IB_IF", 5)) {
			dai_fmt |= SND_SOC_DAIFMT_INV_MASK & SND_SOC_DAIFMT_IB_IF;
			set_sync = true;
		} else {
			dai_fmt = dai_fmt_tmp;
		}
		break;
	case 8: /* set slots */
		slots = simple_strtoul(scanf_str, NULL, 10);
		if (slots > 0 && slots < 32 && (slots % 2 == 0))
			set_sync = true;
		break;
	case 9: /* set slot_width */
		slot_width = simple_strtoul(scanf_str, NULL, 10);
		if (slot_width == 16 || slot_width == 24 || slot_width == 32)
			set_sync = true;
		break;
	default:
		SND_LOG_ERR(HLOG, "options invalid, %u\n", scanf_num);
		set_sync = false;
	}

	/* sync setting */
	if (scanf_num > 0) {
		if (!set_sync) {
			pr_err("option val invaild\n");
			return count;
		}
		card->dai_link->dai_fmt		= dai_fmt;
		priv->dai_props->mclk_fp	= mclk_fp;
		priv->dai_props->mclk_fs	= mclk_fs;
		priv->dai_props->cpu_pll_fs	= cpu_pll_fs;
		priv->dai_props->codec_pll_fs	= codec_pll_fs;
		priv->dais->slots		= slots;
		priv->dais->slot_width		= slot_width;
	}

	return count;

err:
	pr_err("please get the usage by -> \"cat i2s_dai_fmt\"\n");
	return count;
}

static DEVICE_ATTR(i2s_dai_fmt, 0644, asoc_simple_show_i2s_dai_fmt, asoc_simple_store_i2s_dai_fmt);

static struct attribute *audio_debug_attrs[] = {
	&dev_attr_i2s_dai_fmt.attr,
	NULL,
};

static struct attribute_group sunxi_debug_attr = {
	.name	= "audio_debug",
	.attrs	= audio_debug_attrs,
};

static int asoc_simple_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *top_np = dev->of_node;
	struct asoc_simple_priv *priv;
	struct snd_soc_card *card;
	int ret;

	/* Allocate the private data and the DAI link array */
	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	card = simple_priv_to_card(priv);
	card->owner		= THIS_MODULE;
	card->dev		= dev;
	card->probe		= simple_soc_probe;
	card->remove		= simple_soc_remove;

	ret = asoc_simple_init_priv(priv);
	if (ret < 0)
		return ret;

	if (top_np && of_device_is_available(top_np)) {
		ret = simple_parse_of(priv);
		if (ret < 0) {
			if (ret != -EPROBE_DEFER)
				SND_LOG_ERR(HLOG, "parse error %d\n", ret);
			goto err;
		}
	} else {
		SND_LOG_ERR(HLOG, "simple card dts available\n");
	}

	snd_soc_card_set_drvdata(card, priv);

	/* asoc_simple_debug_info(priv); */
	ret = devm_snd_soc_register_card(dev, card);
	if (ret < 0)
		goto err;

	ret = snd_sunxi_sysfs_create_group(pdev, &sunxi_debug_attr);
	if (ret) {
		SND_LOG_WARN(HLOG, "sysfs debug create failed\n");
		goto err;
	}

	return 0;
err:
	asoc_simple_clean_reference(card);

	return ret;
}

static int asoc_simple_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);

	snd_sunxi_sysfs_remove_group(pdev, &sunxi_debug_attr);
	return asoc_simple_clean_reference(card);
}

static const struct of_device_id snd_soc_sunxi_of_match[] = {
	{ .compatible = "allwinner," DRV_NAME, },
	{},
};
MODULE_DEVICE_TABLE(of, snd_soc_sunxi_of_match);

static struct platform_driver sunxi_soundcard_machine_driver = {
	.driver	= {
		.name		= DRV_NAME,
		.pm		= &snd_soc_pm_ops,
		.of_match_table	= snd_soc_sunxi_of_match,
	},
	.probe	= asoc_simple_probe,
	.remove	= asoc_simple_remove,
};

int __init sunxi_soundcard_machine_dev_init(void)
{
	int ret;

	ret = platform_driver_register(&sunxi_soundcard_machine_driver);
	if (ret != 0) {
		SND_LOG_ERR(HLOG, "platform driver register failed\n");
		return -EINVAL;
	}

	return ret;
}

void __exit sunxi_soundcard_machine_dev_exit(void)
{
	platform_driver_unregister(&sunxi_soundcard_machine_driver);
}

late_initcall(sunxi_soundcard_machine_dev_init);
module_exit(sunxi_soundcard_machine_dev_exit);

MODULE_AUTHOR("Dby@allwinnertech.com");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("sunxi soundcard machine");
