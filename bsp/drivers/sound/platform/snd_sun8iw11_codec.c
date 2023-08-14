/*
 * sound\soc\sunxi\sun8iw11_codec.c
 * (C) Copyright 2022-2027
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * huangxin <huangxin@allwinnertech.com>
 * wolfgang huang <huangjinhui@allwinnertech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/reset.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/pm.h>
#include <linux/regulator/consumer.h>
#include <linux/pinctrl/consumer.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>

#include "snd_sunxi_log.h"
#include "snd_sunxi_pcm.h"
#include "snd_sunxi_common.h"
#include "snd_sun8iw11_codec.h"

#define HLOG		"CODEC"
#define	DRV_NAME	"sunxi-snd-codec"

static unsigned int sunxi_regmap_read_prcm(struct regmap *regmap, unsigned int reg)
{
	unsigned int addr;
	unsigned int reg_val;
	unsigned int write_val = 0x10000000;

	SND_LOG_DEBUG(HLOG, "\n");

	addr = reg - SUNXI_PR_CFG;
	write_val |= (addr<<16);

	regmap_write(regmap, SUNXI_PR_CFG, write_val);
	regmap_read(regmap, SUNXI_PR_CFG, &reg_val);

	reg_val &= 0xff;

	return reg_val;
}

static void sunxi_regmap_write_prcm(struct regmap *regmap, unsigned int reg, unsigned int val)
{
	unsigned int addr;
	unsigned int write_val = 0x11000000;

	SND_LOG_DEBUG(HLOG, "\n");

	addr = reg - SUNXI_PR_CFG;
	val &= 0xff;

	write_val |= (addr<<16);
	write_val |= (val<<8);
	regmap_write(regmap, SUNXI_PR_CFG, write_val);

	write_val &= ~(0x1<<24);
	regmap_write(regmap, SUNXI_PR_CFG, write_val);
}

static void sunxi_regmap_update_prcm_bits(struct regmap *regmap, unsigned int reg,
					  unsigned int mask, unsigned int val)
{
	unsigned int old, new;

	SND_LOG_DEBUG(HLOG, "\n");

	old = sunxi_regmap_read_prcm(regmap, reg);
	new = (old & ~mask) | val;

	sunxi_regmap_write_prcm(regmap, reg, new);
}

static struct audio_reg_label sunxi_reg_labels[] = {
	REG_LABEL(SUNXI_DAC_DPC),
	REG_LABEL(SUNXI_DAC_FIFO_CTR),
	REG_LABEL(SUNXI_DAC_FIFO_STA),
	REG_LABEL(SUNXI_ADC_FIFO_CTR),
	REG_LABEL(SUNXI_ADC_FIFO_STA),
	/* REG_LABEL(SUNXI_ADC_RXDATA), */
	/* REG_LABEL(SUNXI_DAC_TXDATA), */
	REG_LABEL(SUNXI_DAC_CNT),
	REG_LABEL(SUNXI_ADC_CNT),
	REG_LABEL(SUNXI_DAC_DG),
	REG_LABEL(SUNXI_ADC_DG),
	REG_LABEL(SUNXI_HMIC_CTRL),
	REG_LABEL(SUNXI_HMIC_DATA),

	REG_LABEL(SUNXI_HP_VOLC),
	REG_LABEL(SUNXI_LOMIX_SRC),
	REG_LABEL(SUNXI_ROMIX_SRC),
	REG_LABEL(SUNXI_DAC_PA_SRC),
	REG_LABEL(SUNXI_LINEIN_GCTR),
	REG_LABEL(SUNXI_FM_GCTR),
	REG_LABEL(SUNXI_MICIN_GCTR),
	REG_LABEL(SUNXI_PAEN_HP_CTR),
	REG_LABEL(SUNXI_PHONEOUT_CTR),
	REG_LABEL(SUNXI_MIC2G_LINEEN_CTR),
	REG_LABEL(SUNXI_MIC1G_MICBIAS_CTR),
	REG_LABEL(SUNXI_LADCMIX_SRC),
	REG_LABEL(SUNXI_RADCMIX_SRC),

	REG_LABEL(SUNXI_PA_POP_CTR),
	REG_LABEL(SUNXI_ADC_AP_EN),
	REG_LABEL(SUNXI_ADDA_APT0),
	REG_LABEL(SUNXI_ADDA_APT1),
	REG_LABEL(SUNXI_ADDA_APT2),
	REG_LABEL(SUNXI_CHOP_CAL_CTR),
	REG_LABEL(SUNXI_BIAS_DA16_CAL_CTR),
	REG_LABEL(SUNXI_DA16_CALI_DATA),
	REG_LABEL(SUNXI_BIAS_CALI_DATA),
	REG_LABEL(SUNXI_BIAS_CALI_SET),
	REG_LABEL_END,
};

struct sample_rate {
	unsigned int samplerate;
	unsigned int rate_bit;
};

static const struct sample_rate sunxi_sample_rate_conv[] = {
	{44100, 0},
	{48000, 0},
	{8000, 5},
	{32000, 1},
	{22050, 2},
	{24000, 2},
	{16000, 3},
	{11025, 4},
	{12000, 4},
	{192000, 6},
	{96000, 7},
};

static int snd_sunxi_clk_init(struct platform_device *pdev, struct sunxi_codec_clk *clk);
static void snd_sunxi_clk_exit(struct sunxi_codec_clk *clk);
static int snd_sunxi_clk_enable(struct sunxi_codec_clk *clk);
static void snd_sunxi_clk_disable(struct sunxi_codec_clk *clk);

static int snd_sunxi_rglt_init(struct platform_device *pdev, struct sunxi_codec_rglt *rglt);
static void snd_sunxi_rglt_exit(struct sunxi_codec_rglt *rglt);
static int snd_sunxi_rglt_enable(struct sunxi_codec_rglt *rglt);
static void snd_sunxi_rglt_disable(struct sunxi_codec_rglt *rglt);

static const DECLARE_TLV_DB_SCALE(digital_tlv, -7308, 116, 0);
static const DECLARE_TLV_DB_SCALE(headphone_tlv, -6300, 100, 1);
static const DECLARE_TLV_DB_SCALE(linein_tlv, -450, 150, 0);
static const DECLARE_TLV_DB_SCALE(fm_tlv, -450, 150, 0);
static const DECLARE_TLV_DB_SCALE(mic_gain_tlv, -450, 150, 0);
static const DECLARE_TLV_DB_SCALE(phoneout_tlv, -450, 150, 0);
static const DECLARE_TLV_DB_SCALE(adc_gain_tlv, -450, 150, 0);
static const unsigned int sunxi_mic_boost_tlv[] = {
	TLV_DB_RANGE_HEAD(2),
	0, 0, TLV_DB_SCALE_ITEM(0, 0, 0),
	1, 7, TLV_DB_SCALE_ITEM(2400, 300, 0),
};

static int sunxi_codec_get_hub_mode(struct snd_kcontrol *kcontrol,
				    struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;
	unsigned int reg_val;

	regmap_read(regmap, SUNXI_DAC_DPC, &reg_val);
	ucontrol->value.integer.value[0] = ((reg_val & (1<<HUB_EN)) ? 1 : 0);

	return 0;
}

static int sunxi_codec_set_hub_mode(struct snd_kcontrol *kcontrol,
				    struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	switch (ucontrol->value.integer.value[0]) {
	case 0:
		regmap_update_bits(regmap, SUNXI_DAC_DPC, (0x1<<HUB_EN), (0x0<<HUB_EN));
		break;
	case 1:
		regmap_update_bits(regmap, SUNXI_DAC_DPC, (0x1<<HUB_EN), (0x1<<HUB_EN));
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sunxi_get_hpl_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	unsigned int reg_val;

	reg_val = sunxi_regmap_read_prcm(codec->mem.regmap, SUNXI_PAEN_HP_CTR);
	if (reg_val & (1<<RTLNMUTE)) {
		ucontrol->value.enumerated.item[0] = 2;
		return 0;
	} else {
		reg_val = sunxi_regmap_read_prcm(codec->mem.regmap, SUNXI_DAC_PA_SRC);
		ucontrol->value.enumerated.item[0] = ((reg_val & (0x1<<LHPIS)) ? 1 : 0);
	}

	return 0;
}

static int sunxi_set_hpl_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);

	switch (ucontrol->value.enumerated.item[0]) {
	case 0:
		sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_PAEN_HP_CTR,
					      0x1<<RTLNMUTE, 0x0<<RTLNMUTE);
		sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_DAC_PA_SRC,
					      0x1<<LHPPAMUTE | 0x1<<LHPIS,
					      0x1<<LHPPAMUTE | 0x0<<LHPIS);
		break;
	case 1:
		sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_PAEN_HP_CTR,
					      0x1<<RTLNMUTE, 0x0<<RTLNMUTE);
		sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_DAC_PA_SRC,
					      0x1<<LHPPAMUTE | 0x1<<LHPIS,
					      0x1<<LHPPAMUTE | 0x1<<LHPIS);
		break;
	case 2:
		sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_PAEN_HP_CTR,
					      0x1<<RTLNMUTE, 0x1<<RTLNMUTE);
		sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_DAC_PA_SRC,
					      0x1<<LHPPAMUTE, 0x0<<LHPPAMUTE);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sunxi_get_hpr_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	unsigned int reg_val;

	reg_val = sunxi_regmap_read_prcm(codec->mem.regmap, SUNXI_PAEN_HP_CTR);
	if (reg_val & (1<<LTRNMUTE)) {
		ucontrol->value.enumerated.item[0] = 2;
		return 0;
	} else {
		reg_val = sunxi_regmap_read_prcm(codec->mem.regmap, SUNXI_DAC_PA_SRC);
		ucontrol->value.enumerated.item[0] = (reg_val & (1<<RHPIS)) ? 1 : 0;
	}

	return 0;
}

static int sunxi_set_hpr_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);

	switch (ucontrol->value.enumerated.item[0]) {
	case 0:
		sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_PAEN_HP_CTR,
					      0x1<<LTRNMUTE, 0x0<<LTRNMUTE);
		sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_DAC_PA_SRC,
					      0x1<<RHPPAMUTE | 0x1<<RHPIS,
					      0x1<<RHPPAMUTE | 0x0<<RHPIS);
		break;
	case 1:
		sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_PAEN_HP_CTR,
					      0x1<<LTRNMUTE, 0x0<<LTRNMUTE);
		sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_DAC_PA_SRC,
					      0x1<<RHPPAMUTE | 0x1<<RHPIS,
					      0x1<<RHPPAMUTE | 0x1<<RHPIS);
		break;
	case 2:
		sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_PAEN_HP_CTR,
					      0x1<<LTRNMUTE, 0x1<<LTRNMUTE);
		sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_DAC_PA_SRC,
					      0x1<<RHPPAMUTE, 0x0<<RHPPAMUTE);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/* Define Text */
static const char *sunxi_switch_text[] = {"Off", "On"};
static const char *sunxi_hpl_mux_text[] = {"DACL", "LOMIX", "HPR"};
static const char *sunxi_hpr_mux_text[] = {"DACR", "ROMIX", "HPL"};

/* Define Enum */
static SOC_ENUM_SINGLE_EXT_DECL(sunxi_codec_hub_mode_enum, sunxi_switch_text);

static int sunxi_playback_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		regmap_update_bits(regmap, SUNXI_DAC_DPC, 0x1<<EN_DAC, 0x1<<EN_DAC);
		break;
	case SND_SOC_DAPM_POST_PMD:
		regmap_update_bits(regmap, SUNXI_DAC_DPC, 0x1<<EN_DAC, 0x0<<EN_DAC);
		break;
	default:
		break;
	}

	return 0;
}

static int sunxi_capture_event(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	switch (event) {
	case	SND_SOC_DAPM_POST_PMU:
		regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTR,
				(0x1<<EN_AD), (0x1<<EN_AD));
		break;
	case	SND_SOC_DAPM_POST_PMD:
		regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTR,
				(0x1<<EN_AD), (0x0<<EN_AD));
		break;
	default:
		break;
	}

	return 0;
}

static int sunxi_mic1_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_MIC1G_MICBIAS_CTR,
					      0x1<<MIC1_AMPEN | 0x1<<MMICBIASEN,
					      0x1<<MIC1_AMPEN | 0x1<<MMICBIASEN);
		break;
	case SND_SOC_DAPM_POST_PMD:
		sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_MIC1G_MICBIAS_CTR,
					      0x1<<MIC1_AMPEN | 0x1<<MMICBIASEN,
					      0x0<<MIC1_AMPEN | 0x0<<MMICBIASEN);
		break;
	default:
		break;
	}

	return 0;
}

static int sunxi_mic2_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_MIC1G_MICBIAS_CTR,
					      0x1<<MMICBIASEN, 0x1<<MMICBIASEN);
		sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_MIC2G_LINEEN_CTR,
					      0x1<<MIC2AMPEN, 0x1<<MIC2AMPEN);
		break;
	case SND_SOC_DAPM_POST_PMD:
		sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_MIC1G_MICBIAS_CTR,
					      0x1<<MMICBIASEN, 0x0<<MMICBIASEN);
		sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_MIC2G_LINEEN_CTR,
					      0x1<<MIC2AMPEN, 0x0<<MIC2AMPEN);
		break;
	default:
		break;
	}

	return 0;
}

static int sunxi_hpout_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		/* Enable HPPALR */
		sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_DAC_PA_SRC,
					      0x1<<RHPPAMUTE | 0x1<<LHPPAMUTE,
					      0x1<<RHPPAMUTE | 0x1<<LHPPAMUTE);
		sunxi_regmap_update_prcm_bits(regmap, SUNXI_PAEN_HP_CTR, 1<<HPPAEN, 1<<HPPAEN);
		/* time delay to wait headphone pa work fine */
		mdelay(codec->dts.pa_sleep_time + 150);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		sunxi_regmap_update_prcm_bits(regmap, SUNXI_PAEN_HP_CTR, 1<<HPPAEN, 0<<HPPAEN);
		sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_DAC_PA_SRC,
					      0x1<<RHPPAMUTE | 0x1<<LHPPAMUTE,
					      0x0<<RHPPAMUTE | 0x0<<LHPPAMUTE);
		break;
	default:
		break;
	}
	return 0;
}

static int sunxi_spk_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		if (codec->dts.spk_gpio_used) {
			snd_sunxi_pa_pin_enable(codec->pa_cfg, codec->pa_pin_max);
			/* time delay to wait spk pa work fine, general setting 50ms */
			mdelay(50);
		}
		break;
	case SND_SOC_DAPM_PRE_PMD:
		if (codec->dts.spk_gpio_used)
			snd_sunxi_pa_pin_disable(codec->pa_cfg, codec->pa_pin_max);
		break;
	default:
		break;
	}

	return 0;
}

static const struct snd_kcontrol_new sunxi_codec_controls[] = {
	/* Hub Mode */
	SOC_ENUM_EXT("tx hub mode", sunxi_codec_hub_mode_enum,
		     sunxi_codec_get_hub_mode, sunxi_codec_set_hub_mode),

	/* DAC Digital Vol */
	SOC_SINGLE_TLV("DAC Volume", SUNXI_DAC_DPC, DVOL, 0x3F, 1, digital_tlv),

	/* ADC Analog Gain */
	SOC_SINGLE_TLV("ADC Gain", SUNXI_ADC_AP_EN, ADCG, 0x7, 0, adc_gain_tlv),

	/* MIC Gain */
	SOC_SINGLE_TLV("MIC1 Gain", SUNXI_MIC1G_MICBIAS_CTR, MIC1_BOOST, 0x7, 0, sunxi_mic_boost_tlv),
	SOC_SINGLE_TLV("MIC2 Gain", SUNXI_MIC2G_LINEEN_CTR, MIC2BOOST, 0x7, 0, sunxi_mic_boost_tlv),

	/* Input Stage to output Mixer Gain */
	SOC_SINGLE_TLV("MIC1 to OMIX Gain", SUNXI_MICIN_GCTR, MIC1_GAIN, 0x7, 0, mic_gain_tlv),
	SOC_SINGLE_TLV("MIC2 to OMIX Gain", SUNXI_MICIN_GCTR, MIC2_GAIN, 0x7, 0, mic_gain_tlv),

	SOC_SINGLE_TLV("FMIN to OMIX Gain", SUNXI_FM_GCTR, FMG, 0x7, 0, fm_tlv),
	SOC_SINGLE_TLV("LINEIN to OMIX Gain", SUNXI_FM_GCTR, LINEING, 0x7, 0, linein_tlv),

	/* LINEIN/FMIN to Output Mixer Gain */
	SOC_SINGLE_TLV("LINEINL to ROMIX Gain", SUNXI_LINEIN_GCTR, LINEINLG, 0x7, 0, linein_tlv),
	SOC_SINGLE_TLV("LINEINR to LOMIX Gain", SUNXI_LINEIN_GCTR, LINEINRG, 0x7, 0, linein_tlv),

	/* Phoneout Vol */
	SOC_SINGLE_TLV("Phoneout Gain", SUNXI_PHONEOUT_CTR, PHONEOUTG, 0x7, 0, phoneout_tlv),

	/* Hpout Vol */
	SOC_SINGLE_TLV("HPOUT Volume", SUNXI_HP_VOLC, HPVOL, 0x3F, 0, headphone_tlv),
};

/* Define Mux Controls */
static const struct soc_enum sunxi_hpl_enum =
	SOC_ENUM_SINGLE(SND_SOC_NOPM, 0, 3, sunxi_hpl_mux_text);
static const struct soc_enum sunxi_hpr_enum =
	SOC_ENUM_SINGLE(SND_SOC_NOPM, 0, 3, sunxi_hpr_mux_text);

static const struct snd_kcontrol_new sunxi_hpl_mux =
	SOC_DAPM_ENUM_EXT("HPL Source", sunxi_hpl_enum, sunxi_get_hpl_src, sunxi_set_hpl_src);
static const struct snd_kcontrol_new sunxi_hpr_mux =
	SOC_DAPM_ENUM_EXT("HPR Source", sunxi_hpr_enum, sunxi_get_hpr_src, sunxi_set_hpr_src);

/* Define Mixer Controls */
static const struct snd_kcontrol_new sunxi_left_output_mixer[] = {
	SOC_DAPM_SINGLE("DACL Switch", SUNXI_LOMIX_SRC, LMIX_LDAC, 1, 0),
	SOC_DAPM_SINGLE("DACR Switch", SUNXI_LOMIX_SRC, LMIX_RDAC, 1, 0),
	SOC_DAPM_SINGLE("MIC1 Switch", SUNXI_LOMIX_SRC, LMIX_MIC1_BST, 1, 0),
	SOC_DAPM_SINGLE("MIC2 Switch", SUNXI_LOMIX_SRC, LMIX_MIC2_BST, 1, 0),
	SOC_DAPM_SINGLE("FMINL Switch", SUNXI_LOMIX_SRC, LMIX_FML, 1, 0),
	SOC_DAPM_SINGLE("LINEINL Switch", SUNXI_LOMIX_SRC, LMIX_LINEINL, 1, 0),
	SOC_DAPM_SINGLE("LINEINLR Switch", SUNXI_LOMIX_SRC, LMIX_LINEINLR, 1, 0),
};

static const struct snd_kcontrol_new sunxi_right_output_mixer[] = {
	SOC_DAPM_SINGLE("DACL Switch", SUNXI_ROMIX_SRC, RMIX_LDAC, 1, 0),
	SOC_DAPM_SINGLE("DACR Switch", SUNXI_ROMIX_SRC, RMIX_RDAC, 1, 0),
	SOC_DAPM_SINGLE("MIC1 Switch", SUNXI_ROMIX_SRC, RMIX_MIC1_BST, 1, 0),
	SOC_DAPM_SINGLE("MIC2 Switch", SUNXI_ROMIX_SRC, RMIX_MIC2_BST, 1, 0),
	SOC_DAPM_SINGLE("FMINR Switch", SUNXI_ROMIX_SRC, RMIX_FMR, 1, 0),
	SOC_DAPM_SINGLE("LINEINR Switch", SUNXI_ROMIX_SRC, RMIX_LINEINR, 1, 0),
	SOC_DAPM_SINGLE("LINEINLR Switch", SUNXI_ROMIX_SRC, RMIX_LINEINLR, 1, 0),
};

static const struct snd_kcontrol_new sunxi_left_input_mixer[] = {
	SOC_DAPM_SINGLE("MIC1 Switch", SUNXI_LADCMIX_SRC, LADC_MIC1_BST, 1, 0),
	SOC_DAPM_SINGLE("MIC2 Switch", SUNXI_LADCMIX_SRC, LADC_MIC2_BST, 1, 0),
	SOC_DAPM_SINGLE("FMINL Switch", SUNXI_LADCMIX_SRC, LADC_FML, 1, 0),
	SOC_DAPM_SINGLE("LINEINL Switch", SUNXI_LADCMIX_SRC, LADC_LINEINL, 1, 0),
	SOC_DAPM_SINGLE("LINEINLR Switch", SUNXI_LADCMIX_SRC, LADC_LINEINLR, 1, 0),
	SOC_DAPM_SINGLE("LOMIX Switch", SUNXI_LADCMIX_SRC, LADC_LOUT_MIX, 1, 0),
	SOC_DAPM_SINGLE("ROMIX Switch", SUNXI_LADCMIX_SRC, LADC_ROUT_MIX, 1, 0),
};

static const struct snd_kcontrol_new sunxi_right_input_mixer[] = {
	SOC_DAPM_SINGLE("MIC1 Switch", SUNXI_RADCMIX_SRC, RADC_MIC1_BST, 1, 0),
	SOC_DAPM_SINGLE("MIC2 Switch", SUNXI_RADCMIX_SRC, RADC_MIC2_BST, 1, 0),
	SOC_DAPM_SINGLE("FMINR Switch", SUNXI_RADCMIX_SRC, RADC_FMR, 1, 0),
	SOC_DAPM_SINGLE("LINEINR Switch", SUNXI_RADCMIX_SRC, RADC_LINEINR, 1, 0),
	SOC_DAPM_SINGLE("LINEINLR Switch", SUNXI_RADCMIX_SRC, RADC_LINEINLR, 1, 0),
	SOC_DAPM_SINGLE("LOMIX Switch", SUNXI_RADCMIX_SRC, RADC_LOUT_MIX, 1, 0),
	SOC_DAPM_SINGLE("ROMIX Switch", SUNXI_RADCMIX_SRC, RADC_ROUT_MIX, 1, 0),
};

static const struct snd_kcontrol_new sunxi_phoneout_mixer[] = {
	SOC_DAPM_SINGLE("MIC1 Switch", SUNXI_PHONEOUT_CTR, PHONEOUTS3, 1, 0),
	SOC_DAPM_SINGLE("MIC2 Switch", SUNXI_PHONEOUT_CTR, PHONEOUTS2, 1, 0),
	SOC_DAPM_SINGLE("LOMIX Switch", SUNXI_PHONEOUT_CTR, PHONEOUTS0, 1, 0),
	SOC_DAPM_SINGLE("ROMIX Switch", SUNXI_PHONEOUT_CTR, PHONEOUTS1, 1, 0),
};

static const struct snd_soc_dapm_widget sunxi_codec_dapm_widgets[] = {
	SND_SOC_DAPM_AIF_IN_E("DACL", "Playback", 0, SUNXI_DAC_PA_SRC, DACALEN, 0,
				sunxi_playback_event,
				SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_AIF_IN_E("DACR", "Playback", 0, SUNXI_DAC_PA_SRC, DACAREN, 0,
				sunxi_playback_event,
				SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_AIF_OUT_E("ADCL", "Capture", 0, SUNXI_ADC_AP_EN, ADCLEN, 0,
				sunxi_capture_event,
				SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_AIF_OUT_E("ADCR", "Capture", 0, SUNXI_ADC_AP_EN, ADCREN, 0,
				sunxi_capture_event,
				SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_MIXER("Left Output Mixer", SUNXI_DAC_PA_SRC, LMIXEN, 0,
			   sunxi_left_output_mixer, ARRAY_SIZE(sunxi_left_output_mixer)),
	SND_SOC_DAPM_MIXER("Right Output Mixer", SUNXI_DAC_PA_SRC, RMIXEN, 0,
			   sunxi_right_output_mixer, ARRAY_SIZE(sunxi_right_output_mixer)),
	SND_SOC_DAPM_MIXER("Left Input Mixer", SND_SOC_NOPM, 0, 0,
			   sunxi_left_input_mixer, ARRAY_SIZE(sunxi_left_input_mixer)),
	SND_SOC_DAPM_MIXER("Right Input Mixer", SND_SOC_NOPM, 0, 0,
			   sunxi_right_input_mixer, ARRAY_SIZE(sunxi_right_input_mixer)),
	SND_SOC_DAPM_MIXER("Phoneout Mixer", SUNXI_PHONEOUT_CTR, PHONEOUTEN, 0,
			   sunxi_phoneout_mixer, ARRAY_SIZE(sunxi_phoneout_mixer)),

	SND_SOC_DAPM_MUX("HPL Source", SND_SOC_NOPM, 0, 0, &sunxi_hpl_mux),
	SND_SOC_DAPM_MUX("HPR Source", SND_SOC_NOPM, 0, 0, &sunxi_hpr_mux),

	SND_SOC_DAPM_INPUT("MIC1_PIN"),
	SND_SOC_DAPM_INPUT("MIC2_PIN"),
	SND_SOC_DAPM_INPUT("FMINL_PIN"),
	SND_SOC_DAPM_INPUT("FMINR_PIN"),
	SND_SOC_DAPM_INPUT("LINEINL_PIN"),
	SND_SOC_DAPM_INPUT("LINEINR_PIN"),

	SND_SOC_DAPM_OUTPUT("HPOUTL_PIN"),
	SND_SOC_DAPM_OUTPUT("HPOUTR_PIN"),
	SND_SOC_DAPM_OUTPUT("PHONEOUTP_PIN"),
	SND_SOC_DAPM_OUTPUT("PHONEOUTN_PIN"),

	SND_SOC_DAPM_MIC("MIC1", sunxi_mic1_event),
	SND_SOC_DAPM_MIC("MIC2", sunxi_mic2_event),
	SND_SOC_DAPM_LINE("FMIN", NULL),
	SND_SOC_DAPM_LINE("LINEIN", NULL),
	SND_SOC_DAPM_HP("HPOUT", sunxi_hpout_event),
	SND_SOC_DAPM_HP("Phoneout", NULL),
	SND_SOC_DAPM_SPK("SPK", sunxi_spk_event),
};

static const struct snd_soc_dapm_route sunxi_codec_dapm_routes[] = {
	/* input path */
	{"MIC1_PIN", NULL, "MIC1"},
	{"MIC2_PIN", NULL, "MIC2"},
	{"LINEINL_PIN", NULL, "LINEIN"},
	{"LINEINR_PIN", NULL, "LINEIN"},
	{"FMINL_PIN", NULL, "FMIN"},
	{"FMINR_PIN", NULL, "FMIN"},

	{"Left Input Mixer", "MIC1 Switch", "MIC1_PIN"},
	{"Left Input Mixer", "MIC2 Switch", "MIC2_PIN"},
	{"Left Input Mixer", "FMINL Switch", "FMINL_PIN"},
	{"Left Input Mixer", "LINEINL Switch", "LINEINL_PIN"},
	{"Left Input Mixer", "LINEINLR Switch", "LINEINL_PIN"},
	{"Left Input Mixer", "LINEINLR Switch", "LINEINR_PIN"},
	{"Left Input Mixer", "LOMIX Switch", "Left Output Mixer"},
	{"Left Input Mixer", "ROMIX Switch", "Right Output Mixer"},

	{"Right Input Mixer", "MIC1 Switch", "MIC1_PIN"},
	{"Right Input Mixer", "MIC2 Switch", "MIC2_PIN"},
	{"Right Input Mixer", "FMINR Switch", "FMINR_PIN"},
	{"Right Input Mixer", "LINEINR Switch", "LINEINR_PIN"},
	{"Right Input Mixer", "LINEINLR Switch", "LINEINL_PIN"},
	{"Right Input Mixer", "LINEINLR Switch", "LINEINR_PIN"},
	{"Right Input Mixer", "LOMIX Switch", "Left Output Mixer"},
	{"Right Input Mixer", "ROMIX Switch", "Right Output Mixer"},

	{"ADCL", NULL, "Left Input Mixer"},
	{"ADCR", NULL, "Right Input Mixer"},

	/* output path */
	{"Left Output Mixer", "DACL Switch", "DACL"},
	{"Left Output Mixer", "DACR Switch", "DACR"},
	{"Left Output Mixer", "MIC1 Switch", "MIC1_PIN"},
	{"Left Output Mixer", "MIC2 Switch", "MIC2_PIN"},
	{"Left Output Mixer", "FMINL Switch", "FMINL_PIN"},
	{"Left Output Mixer", "LINEINL Switch", "LINEINL_PIN"},
	{"Left Output Mixer", "LINEINLR Switch", "LINEINL_PIN"},
	{"Left Output Mixer", "LINEINLR Switch", "LINEINR_PIN"},

	{"Right Output Mixer", "DACL Switch", "DACL"},
	{"Right Output Mixer", "DACR Switch", "DACR"},
	{"Right Output Mixer", "MIC1 Switch", "MIC1_PIN"},
	{"Right Output Mixer", "MIC2 Switch", "MIC2_PIN"},
	{"Right Output Mixer", "FMINR Switch", "FMINR_PIN"},
	{"Right Output Mixer", "LINEINR Switch", "LINEINR_PIN"},
	{"Right Output Mixer", "LINEINLR Switch", "LINEINL_PIN"},
	{"Right Output Mixer", "LINEINLR Switch", "LINEINR_PIN"},

	{"HPL Source", "DACL", "DACL"},
	{"HPL Source", "LOMIX", "Left Output Mixer"},
	{"HPL Source", "HPR", "HPR Source"},

	{"HPR Source", "DACR", "DACR"},
	{"HPR Source", "ROMIX", "Right Output Mixer"},
	{"HPR Source", "HPL", "HPL Source"},

	{"Phoneout Mixer", "MIC1 Switch", "MIC1_PIN"},
	{"Phoneout Mixer", "MIC2 Switch", "MIC2_PIN"},
	{"Phoneout Mixer", "LOMIX Switch", "Left Output Mixer"},
	{"Phoneout Mixer", "ROMIX Switch", "Right Output Mixer"},

	{"HPOUTL_PIN", NULL, "HPL Source"},
	{"HPOUTR_PIN", NULL, "HPR Source"},

	{"PHONEOUTP_PIN", NULL, "Phoneout Mixer"},
	{"PHONEOUTN_PIN", NULL, "Phoneout Mixer"},

	/*
	 * {"HPOUT", NULL, "HPOUTL_PIN"},
	 * {"HPOUT", NULL, "HPOUTR_PIN"},
	 * {"Phoneout", NULL, "PHONEOUTP_PIN"},
	 * {"Phoneout", NULL, "PHONEOUTN_PIN"},
	 * {"SPK", NULL, "HPOUTL_PIN"},
	 * {"SPK", NULL, "HPOUTR_PIN"},
	 * {"SPK", NULL, "PHONEOUTP_PIN"},
	 * {"SPK", NULL, "PHONEOUTN_PIN"},
	 */
};

static void sunxi_codec_init(struct snd_soc_component *component)
{
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG(HLOG, "\n");

	/* Disable DRC Function for Playback */
	regmap_write(regmap, SUNXI_DAC_DAP_CTR, 0);
	/* Enable HPF */
	regmap_update_bits(regmap, SUNXI_DAC_DPC, 1<<HPF_EN, 1<<HPF_EN);

	/* Enable ADCFDT to Overcome Niose At the Beginning */
	regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTR, 7<<ADCDFEN, 7<<ADCDFEN);
	/* Enable Hardware ZeroCross Overcomm Volume Update Clicks Noise */
	sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_ADDA_APT2,
				      1<<ZERO_CROSS_EN, 1<<ZERO_CROSS_EN);
	/* Setting the Anti-pop Time(rise wave time) to be 393ms */
	sunxi_regmap_write_prcm(codec->mem.regmap, SUNXI_PA_POP_CTR, 0x2);
	/* Setting the Anti-pop Time(rise wave time) to be 393ms */
	sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_PAEN_HP_CTR,
				      3<<PA_ANTI_POP_CTRL, 2<<PA_ANTI_POP_CTRL);
	sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_PAEN_HP_CTR,
				      1<<HPPAEN, 1<<HPPAEN);

	/* Default setting user config volume */
	sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_HP_VOLC, 0x3F<<HPVOL,
				      codec->dts.headphonevol<<HPVOL);
	/* LINEOUT just disable, so just setting PHONEOUT */
	sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_PHONEOUT_CTR, 0x7<<PHONEOUTG,
				      codec->dts.spkervol<<PHONEOUTG);

	/* Set HPCOM */
	sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_PAEN_HP_CTR,
				      0x3<<HPCOM_FC, 0x3<<HPCOM_FC);
	sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_PAEN_HP_CTR,
				      0x1<<HPCOM_PT, 0x1<<HPCOM_PT);
}

static int sunxi_codec_dai_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;
	int i = 0;

	SND_LOG_DEBUG(HLOG, "\n");

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTR,
					(3<<FIFO_MODE), (3<<FIFO_MODE));
			regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTR,
				(1<<TX_SAMPLE_BITS), (0<<TX_SAMPLE_BITS));
		} else {
			regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTR,
				(1<<RX_FIFO_MODE), (1<<RX_FIFO_MODE));
			regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTR,
				(1<<RX_SAMPLE_BITS), (0<<RX_SAMPLE_BITS));
		}
		break;
	case SNDRV_PCM_FORMAT_S24_3LE:
	case SNDRV_PCM_FORMAT_S24_LE:
	case SNDRV_PCM_FORMAT_S32_LE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTR,
					(3<<FIFO_MODE), (0<<FIFO_MODE));
			regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTR,
				(1<<TX_SAMPLE_BITS), (1<<TX_SAMPLE_BITS));
		} else {
			regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTR,
				(1<<RX_FIFO_MODE), (0<<RX_FIFO_MODE));
			regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTR,
				(1<<RX_SAMPLE_BITS), (1<<RX_SAMPLE_BITS));
		}
		break;
	default:
		break;
	}

	for (i = 0; i < ARRAY_SIZE(sunxi_sample_rate_conv); i++) {
		if (sunxi_sample_rate_conv[i].samplerate == params_rate(params)) {
			if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
				regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTR,
					(0x7<<DAC_FS),
					(sunxi_sample_rate_conv[i].rate_bit<<DAC_FS));
			} else {
				if (sunxi_sample_rate_conv[i].samplerate > 48000)
					return -EINVAL;
				regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTR,
					(0x7<<ADC_FS),
					(sunxi_sample_rate_conv[i].rate_bit<<ADC_FS));
			}
		}
	}

	if (params_channels(params) == 1) {
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTR,
					(1<<DAC_MONO_EN), 1<<DAC_MONO_EN);
		} else {
			regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTR,
					(1<<ADC_MONO_EN), (1<<ADC_MONO_EN));
		}
	} else {
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTR,
					(1<<DAC_MONO_EN), (0<<DAC_MONO_EN));
		} else {
			regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTR,
					(1<<ADC_MONO_EN), (0<<ADC_MONO_EN));
		}
	}

	return 0;
}

static int sunxi_codec_dai_set_pll(struct snd_soc_dai *dai, int pll_id, int source,
					    unsigned int freq_in, unsigned int freq_out)
{
	struct snd_soc_component *component = dai->component;
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct sunxi_codec_clk *clk = &codec->clk;

	SND_LOG_DEBUG(HLOG, "stream -> %s, freq_in ->%u, freq_out ->%u\n",
		      pll_id ? "IN" : "OUT", freq_in, freq_out);

	if (pll_id == SNDRV_PCM_STREAM_PLAYBACK)
		goto playback;
	else
		goto capture;

playback:
	if (clk_set_rate(clk->clk_pll_audio, freq_in)) {
		SND_LOG_ERR(HLOG, "clk pllaudio set rate failed\n");
		return -EINVAL;
	}

	return 0;

capture:
	if (clk_set_rate(clk->clk_pll_audio, freq_in)) {
		SND_LOG_ERR(HLOG, "clk pllaudio set rate failed\n");
		return -EINVAL;
	}

	return 0;
}

static int sunxi_codec_dai_prepare(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG(HLOG, "\n");

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTR,
				(1<<FIFO_FLUSH), (1<<FIFO_FLUSH));
		regmap_write(regmap, SUNXI_DAC_FIFO_STA,
				(1<<TXE_INT|1<<TXU_INT|1<<TXO_INT));
		regmap_write(regmap, SUNXI_DAC_CNT, 0);
	} else {
		regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTR,
				(1<<FIFO_FLUSH), (1<<FIFO_FLUSH));
		regmap_write(regmap, SUNXI_ADC_FIFO_STA,
				(1<<RXA_INT|1<<RXO_INT));
		regmap_write(regmap, SUNXI_ADC_CNT, 0);
	}
	return 0;
}

static int sunxi_codec_dai_trigger(struct snd_pcm_substream *substream,
				int cmd, struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG(HLOG, "\n");

	switch (cmd) {
	case	SNDRV_PCM_TRIGGER_START:
	case	SNDRV_PCM_TRIGGER_RESUME:
	case	SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTR,
					(1<<DAC_DRQ_EN), (1<<DAC_DRQ_EN));
		else
			regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTR,
					(1<<ADC_DRQ_EN), (1<<ADC_DRQ_EN));
		break;
	case	SNDRV_PCM_TRIGGER_STOP:
	case	SNDRV_PCM_TRIGGER_SUSPEND:
	case	SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTR,
					(1<<DAC_DRQ_EN), (0<<DAC_DRQ_EN));
		else
			regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTR,
					(1<<ADC_DRQ_EN), (0<<ADC_DRQ_EN));
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static const struct snd_soc_dai_ops sunxi_codec_dai_ops = {
	.hw_params	= sunxi_codec_dai_hw_params,
	.set_pll	= sunxi_codec_dai_set_pll,
	.trigger	= sunxi_codec_dai_trigger,
	.prepare	= sunxi_codec_dai_prepare,
};

static struct snd_soc_dai_driver sunxi_codec_dai = {
		.playback = {
			.stream_name = "Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates	= SNDRV_PCM_RATE_8000_192000
				| SNDRV_PCM_RATE_KNOT,
			.formats = SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S24_3LE
				| SNDRV_PCM_FMTBIT_S32_LE,
		},
		.capture = {
			.stream_name = "Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_48000
				| SNDRV_PCM_RATE_KNOT,
			.formats = SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S24_3LE
				| SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &sunxi_codec_dai_ops,
};

static int sunxi_codec_component_probe(struct snd_soc_component *component)
{
	SND_LOG_DEBUG(HLOG, "\n");

	sunxi_codec_init(component);
	return 0;
}

static void sunxi_codec_component_remove(struct snd_soc_component *component)
{
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);

	SND_LOG_DEBUG(HLOG, "\n");

	sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_PAEN_HP_CTR, 1<<HPPAEN, 0<<HPPAEN);
}

static int sunxi_codec_component_suspend(struct snd_soc_component *component)
{
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);

	SND_LOG_DEBUG(HLOG, "\n");

	pr_debug("Enter %s\n", __func__);

	snd_sunxi_pa_pin_disable(codec->pa_cfg, codec->pa_pin_max);

	snd_sunxi_rglt_disable(&codec->rglt);
	snd_sunxi_clk_disable(&codec->clk);

	pr_debug("End %s\n", __func__);

	return 0;
}

static int sunxi_codec_component_resume(struct snd_soc_component *component)
{
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	int ret;

	pr_debug("Enter %s\n", __func__);

	snd_sunxi_rglt_enable(&codec->rglt);
	ret = snd_sunxi_clk_enable(&codec->clk);
	if (ret) {
		SND_LOG_ERR(HLOG, "clk enable failed\n");
		return ret;
	}

	snd_sunxi_pa_pin_disable(codec->pa_cfg, codec->pa_pin_max);

	sunxi_codec_init(component);
	pr_debug("End %s\n", __func__);

	return 0;
}

static unsigned int sunxi_codec_component_read(struct snd_soc_component *component,
					       unsigned int reg)
{
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	unsigned int reg_val;

	SND_LOG_DEBUG(HLOG, "\n");

	if (reg >= SUNXI_PR_CFG)
		return sunxi_regmap_read_prcm(codec->mem.regmap, reg);
	else
		regmap_read(codec->mem.regmap, reg, &reg_val);

	return reg_val;
}

static int sunxi_codec_component_write(struct snd_soc_component *component,
				       unsigned int reg, unsigned int val)
{
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);

	SND_LOG_DEBUG(HLOG, "\n");

	if (reg >= SUNXI_PR_CFG)
		sunxi_regmap_write_prcm(codec->mem.regmap, reg, val);
	else
		regmap_write(codec->mem.regmap, reg, val);

	return 0;
}

static struct snd_soc_component_driver sunxi_codec_dev = {
	.name		= DRV_NAME,
	.probe		= sunxi_codec_component_probe,
	.remove		= sunxi_codec_component_remove,
	.suspend	= sunxi_codec_component_suspend,
	.resume		= sunxi_codec_component_resume,
	.read		= sunxi_codec_component_read,
	.write		= sunxi_codec_component_write,
	.controls		= sunxi_codec_controls,
	.num_controls		= ARRAY_SIZE(sunxi_codec_controls),
	.dapm_widgets		= sunxi_codec_dapm_widgets,
	.num_dapm_widgets	= ARRAY_SIZE(sunxi_codec_dapm_widgets),
	.dapm_routes		= sunxi_codec_dapm_routes,
	.num_dapm_routes	= ARRAY_SIZE(sunxi_codec_dapm_routes),
};

/*******************************************************************************
 * *** kernel source ***
 * @1 regmap
 * @2 clk
 * @3 rglt
 * @4 pa pin
 * @5 dts params
 * @6 sysfs
 ******************************************************************************/
static struct regmap_config sunxi_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.max_register = SUNXI_AUDIO_MAX_REG,
	.cache_type = REGCACHE_NONE,
};

static int snd_sunxi_mem_init(struct platform_device *pdev, struct sunxi_codec_mem *mem)
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

	mem->regmap = devm_regmap_init_mmio(&pdev->dev, mem->membase, &sunxi_regmap_config);
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

static void snd_sunxi_mem_exit(struct platform_device *pdev, struct sunxi_codec_mem *mem)
{
	SND_LOG_DEBUG(HLOG, "\n");

	devm_iounmap(&pdev->dev, mem->membase);
	devm_release_mem_region(&pdev->dev, mem->memregion->start, resource_size(mem->memregion));
}

static int snd_sunxi_clk_init(struct platform_device *pdev, struct sunxi_codec_clk *clk)
{
	int ret = 0;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG(HLOG, "\n");

	/* get rst clk */
	clk->clk_rst = devm_reset_control_get(&pdev->dev, NULL);
	if (IS_ERR_OR_NULL(clk->clk_rst)) {
		SND_LOG_ERR(HLOG, "clk rst get failed\n");
		ret =  PTR_ERR(clk->clk_rst);
		goto err_get_clk_rst;
	}

	/* get bus clk */
	clk->clk_bus_audio = of_clk_get_by_name(np, "clk_bus_audio");
	if (IS_ERR_OR_NULL(clk->clk_bus_audio)) {
		SND_LOG_ERR(HLOG, "clk bus get failed\n");
		ret = PTR_ERR(clk->clk_bus_audio);
		goto err_get_clk_bus;
	}

	clk->clk_pll_audio = of_clk_get_by_name(np, "clk_pll_audio");
	if (IS_ERR_OR_NULL(clk->clk_pll_audio)) {
		SND_LOG_ERR(HLOG, "clk pll get failed\n");
		ret = PTR_ERR(clk->clk_pll_audio);
		goto err_get_clk_pll;
	}

	clk->clk_module_audio = of_clk_get_by_name(np, "clk_ac_digital_gate");
	if (IS_ERR_OR_NULL(clk->clk_module_audio)) {
		SND_LOG_ERR(HLOG, "clk module get failed\n");
		ret = PTR_ERR(clk->clk_module_audio);
		goto err_get_clk_module;
	}

	if (clk_set_parent(clk->clk_module_audio, clk->clk_pll_audio)) {
		SND_LOG_ERR(HLOG, "set parent of clk_pll_audio to clk_module_audio failed\n");
		ret = -EINVAL;
		goto err_set_parent;
	}

	ret = snd_sunxi_clk_enable(clk);
	if (ret) {
		SND_LOG_ERR(HLOG, "clk enable failed\n");
		ret = -EINVAL;
		goto err_clk_enable;
	}


	return 0;

err_clk_enable:
err_set_parent:
	clk_put(clk->clk_module_audio);
err_get_clk_module:
	clk_put(clk->clk_pll_audio);
err_get_clk_pll:
	clk_put(clk->clk_bus_audio);
err_get_clk_bus:
err_get_clk_rst:
	return ret;
}

static void snd_sunxi_clk_exit(struct sunxi_codec_clk *clk)
{
	SND_LOG_DEBUG(HLOG, "\n");

	snd_sunxi_clk_disable(clk);
	clk_put(clk->clk_module_audio);
	clk_put(clk->clk_pll_audio);
	clk_put(clk->clk_bus_audio);
}

static int snd_sunxi_clk_enable(struct sunxi_codec_clk *clk)
{
	int ret = 0;

	SND_LOG_DEBUG(HLOG, "\n");

	if (reset_control_deassert(clk->clk_rst)) {
		SND_LOG_ERR(HLOG, "clk rst deassert failed\n");
		ret = -EINVAL;
		goto err_deassert_rst;
	}

	if (clk_prepare_enable(clk->clk_bus_audio)) {
		SND_LOG_ERR(HLOG, "clk bus enable failed\n");
		goto err_enable_clk_bus;
	}

	if (clk_prepare_enable(clk->clk_pll_audio)) {
		SND_LOG_ERR(HLOG, "clk pll enable failed\n");
		goto err_enable_clk_pll;
	}

	if (clk_prepare_enable(clk->clk_module_audio)) {
		SND_LOG_ERR(HLOG, "clk module enable failed\n");
		goto err_enable_clk_module;
	}

	return 0;

err_enable_clk_module:
	clk_disable_unprepare(clk->clk_pll_audio);
err_enable_clk_pll:
	clk_disable_unprepare(clk->clk_bus_audio);
err_enable_clk_bus:
	reset_control_assert(clk->clk_rst);
err_deassert_rst:
	return ret;
}

static void snd_sunxi_clk_disable(struct sunxi_codec_clk *clk)
{
	SND_LOG_DEBUG(HLOG, "\n");

	clk_disable_unprepare(clk->clk_module_audio);
	clk_disable_unprepare(clk->clk_pll_audio);
	clk_disable_unprepare(clk->clk_bus_audio);
	reset_control_assert(clk->clk_rst);
}

static int snd_sunxi_rglt_init(struct platform_device *pdev, struct sunxi_codec_rglt *rglt)
{
	int ret = 0;
	unsigned int temp_val;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG(HLOG, "\n");

	rglt->avcc_external = of_property_read_bool(np, "avcc-external");
	ret = of_property_read_u32(np, "avcc-vol", &temp_val);
	if (ret < 0)
		rglt->avcc_vol = 1800000;	/* default avcc voltage: 1.8v */
	else
		rglt->avcc_vol = temp_val;

	rglt->dvcc_external = of_property_read_bool(np, "dvcc-external");
	ret = of_property_read_u32(np, "dvcc-vol", &temp_val);
	if (ret < 0)
		rglt->dvcc_vol = 1800000;	/* default dvcc voltage: 1.8v */
	else
		rglt->dvcc_vol = temp_val;

	if (rglt->avcc_external) {
		SND_LOG_DEBUG(HLOG, "use external avcc\n");
		rglt->avcc = regulator_get(&pdev->dev, "avcc");
		if (IS_ERR_OR_NULL(rglt->avcc)) {
			SND_LOG_DEBUG(HLOG, "unused external pmu\n");
		} else {
			ret = regulator_set_voltage(rglt->avcc, rglt->avcc_vol, rglt->avcc_vol);
			if (ret < 0) {
				SND_LOG_ERR(HLOG, "set avcc voltage failed\n");
				ret = -EFAULT;
				goto err_rglt;
			}
			ret = regulator_enable(rglt->avcc);
			if (ret < 0) {
				SND_LOG_ERR(HLOG, "enable avcc failed\n");
				ret = -EFAULT;
				goto err_rglt;
			}
		}
	} else {
		SND_LOG_ERR(HLOG, "unsupport internal avcc\n");
		ret = -EFAULT;
		goto err_rglt;
	}

	if (rglt->dvcc_external) {
		SND_LOG_DEBUG(HLOG, "use external dvcc\n");
		rglt->dvcc = regulator_get(&pdev->dev, "dvcc");
		if (IS_ERR_OR_NULL(rglt->dvcc)) {
			SND_LOG_DEBUG(HLOG, "unused external pmu\n");
		} else {
			ret = regulator_set_voltage(rglt->dvcc, rglt->dvcc_vol, rglt->dvcc_vol);
			if (ret < 0) {
				SND_LOG_ERR(HLOG, "set dvcc voltage failed\n");
				ret = -EFAULT;
				goto err_rglt;
			}
			ret = regulator_enable(rglt->dvcc);
			if (ret < 0) {
				SND_LOG_ERR(HLOG, "enable dvcc failed\n");
				ret = -EFAULT;
				goto err_rglt;
			}
		}
	} else {
		SND_LOG_ERR(HLOG, "unsupport internal cpvdd for headphone charge pump\n");
		ret = -EFAULT;
		goto err_rglt;
	}

	return 0;

err_rglt:
	snd_sunxi_rglt_exit(rglt);
	return ret;
}

static void snd_sunxi_rglt_exit(struct sunxi_codec_rglt *rglt)
{
	SND_LOG_DEBUG(HLOG, "\n");

	if (rglt->avcc)
		if (!IS_ERR_OR_NULL(rglt->avcc)) {
			regulator_disable(rglt->avcc);
			regulator_put(rglt->avcc);
		}

	if (rglt->dvcc)
		if (!IS_ERR_OR_NULL(rglt->dvcc)) {
			regulator_disable(rglt->dvcc);
			regulator_put(rglt->dvcc);
		}
}

static int snd_sunxi_rglt_enable(struct sunxi_codec_rglt *rglt)
{
	int ret;

	SND_LOG_DEBUG(HLOG, "\n");

	if (rglt->avcc)
		if (!IS_ERR_OR_NULL(rglt->avcc)) {
			ret = regulator_enable(rglt->avcc);
			if (ret) {
				SND_LOG_ERR(HLOG, "enable avcc failed\n");
				return -1;
			}
		}

	if (rglt->dvcc)
		if (!IS_ERR_OR_NULL(rglt->dvcc)) {
			ret = regulator_enable(rglt->dvcc);
			if (ret) {
				SND_LOG_ERR(HLOG, "enable dvcc failed\n");
				return -1;
			}
		}

	return 0;
}

static void snd_sunxi_rglt_disable(struct sunxi_codec_rglt *rglt)
{
	SND_LOG_DEBUG(HLOG, "\n");

	if (rglt->avcc)
		if (!IS_ERR_OR_NULL(rglt->avcc))
			regulator_disable(rglt->avcc);

	if (rglt->dvcc)
		if (!IS_ERR_OR_NULL(rglt->dvcc))
			regulator_disable(rglt->dvcc);
}

static void snd_sunxi_dts_params_init(struct platform_device *pdev, struct sunxi_codec_dts *dts)
{
	int ret = 0;
	unsigned int temp_val;
	struct device_node *np = pdev->dev.of_node;
	struct sunxi_codec *codec = dev_get_drvdata(&pdev->dev);

	SND_LOG_DEBUG(HLOG, "\n");

	ret = of_property_read_u32(np, "headphonevol", &temp_val);
	if (ret < 0) {
		SND_LOG_WARN(HLOG, "headphone volume get failed\n");
		codec->dts.headphonevol = 0;
	} else {
		codec->dts.headphonevol = temp_val;
	}

	ret = of_property_read_u32(np, "spkervol", &temp_val);
	if (ret < 0) {
		SND_LOG_WARN(HLOG, "speaker volume get failed\n");
		codec->dts.spkervol = 0;
	} else {
		codec->dts.spkervol = temp_val;
	}

	ret = of_property_read_u32(np, "maingain", &temp_val);
	if (ret < 0) {
		SND_LOG_WARN(HLOG, "main gain get failed\n");
		codec->dts.maingain = 0;
	} else {
		codec->dts.maingain = temp_val;
	}

	ret = of_property_read_u32(np, "pa_sleep_time", &temp_val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "pa_sleep_time get failed\n");
		codec->dts.pa_sleep_time = 350;
	} else {
		codec->dts.pa_sleep_time = temp_val;
	}

	SND_LOG_DEBUG(HLOG, "headphonevol:%d, spkervol:%d, maingain:%d, pa_sleep_time:%d\n",
		      codec->dts.headphonevol,
		      codec->dts.spkervol,
		      codec->dts.maingain,
		      codec->dts.pa_sleep_time
	);
}

static ssize_t snd_sunxi_debug_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct sunxi_codec *codec = dev_get_drvdata(dev);
	int count = 0, i = 0;
	int reg_group = 1;
	unsigned int reg_val;

	count += sprintf(buf, "dump audio reg:\n");

	while (sunxi_reg_labels[i].name != NULL) {
		if (sunxi_reg_labels[i].address == SUNXI_PR_CFG)
			reg_group++;

		if (reg_group == 1) {
			regmap_read(codec->mem.regmap, sunxi_reg_labels[i].address, &reg_val);
			count += sprintf(buf + count, "%s 0x%x: 0x%08x\n",
			sunxi_reg_labels[i].name, (sunxi_reg_labels[i].address), reg_val);
		} else if (reg_group == 2) {
			reg_val = sunxi_regmap_read_prcm(codec->mem.regmap,
							 sunxi_reg_labels[i].address);
			count += sprintf(buf + count, "%s 0x%x: 0x%x\n", sunxi_reg_labels[i].name,
					 sunxi_reg_labels[i].address, reg_val);
		}
		i++;
	}

	return count;
}

/* ex:
 * param 1: 0 read;1 write
 * param 2: 1 digital reg; 2 analog reg
 * param 3: reg value;
 * param 4: write value;
 *	read:
 *		echo 0,1,0x00> audio_reg
 *		echo 0,2,0x00> audio_reg
 *	write:
 *		echo 1,1,0x00,0xa > audio_reg
 *		echo 1,2,0x00,0xff > audio_reg
 */
static ssize_t snd_sunxi_debug_store(struct device *dev, struct device_attribute *attr,
				     const char *buf, size_t count)
{
	int ret;
	int rw_flag;
	int input_reg_val = 0;
	int input_reg_group = 0;
	int input_reg_offset = 0;
	struct sunxi_codec *codec = dev_get_drvdata(dev);

	ret = sscanf(buf, "%d,%d,0x%x,0x%x", &rw_flag, &input_reg_group,
		     &input_reg_offset, &input_reg_val);
	dev_info(dev, "ret:%d, reg_group:%d, reg_offset:%d, reg_val:0x%x\n",
		 ret, input_reg_group, input_reg_offset, input_reg_val);

	if (!(input_reg_group == 1 || input_reg_group == 2)) {
		pr_err("not exist reg group\n");
		ret = count;
		goto out;
	}
	if (!(rw_flag == 1 || rw_flag == 0)) {
		pr_err("not rw_flag\n");
		ret = count;
		goto out;
	}
	if (input_reg_group == 1) {
		if (rw_flag) {
			regmap_write(codec->mem.regmap, input_reg_offset, input_reg_val);
		} else {
			regmap_read(codec->mem.regmap, input_reg_offset, &input_reg_val);
			dev_info(dev, "\n\n Reg[0x%x] : 0x%08x\n\n",
					input_reg_offset, input_reg_val);
		}
	} else if (input_reg_group == 2) {
		if (rw_flag) {
			sunxi_regmap_write_prcm(codec->mem.regmap, input_reg_offset + SUNXI_PR_CFG,
						input_reg_val);
		} else {
			 input_reg_val = sunxi_regmap_read_prcm(codec->mem.regmap,
								input_reg_offset + SUNXI_PR_CFG);
			 dev_info(dev, "\n\n Reg[0x%02x] : 0x%02x\n\n",
					input_reg_offset, input_reg_val);
		}
	}
	ret = count;

out:
	return ret;
}

/* About Sysfs */
static DEVICE_ATTR(audio_reg, 0644, snd_sunxi_debug_show, snd_sunxi_debug_store);

static struct attribute *audio_debug_attrs[] = {
	&dev_attr_audio_reg.attr,
	NULL,
};

static struct attribute_group debug_attr = {
	.name	= "audio_debug",
	.attrs	= audio_debug_attrs,
};

/* Platform Driver */
static int  sunxi_codec_dev_probe(struct platform_device *pdev)
{
	int ret;
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	struct sunxi_codec *codec;
	struct sunxi_codec_mem *mem;
	struct sunxi_codec_clk *clk;
	struct sunxi_codec_rglt *rglt;
	struct sunxi_codec_dts *dts;

	SND_LOG_DEBUG(HLOG, "\n");

	/* sunxi codec info */
	codec = devm_kzalloc(dev, sizeof(*codec), GFP_KERNEL);
	if (!codec) {
		SND_LOG_ERR(HLOG, "can't allocate sunxi codec memory\n");
		ret = -ENOMEM;
		goto err_devm_kzalloc;
	}
	dev_set_drvdata(dev, codec);
	mem = &codec->mem;
	clk = &codec->clk;
	rglt = &codec->rglt;
	dts = &codec->dts;
	codec->pdev = pdev;

	/* memio init */
	ret = snd_sunxi_mem_init(pdev, mem);
	if (ret) {
		SND_LOG_ERR(HLOG, "mem init failed\n");
		ret = -ENOMEM;
		goto err_mem_init;
	}

	/* clk init */
	ret = snd_sunxi_clk_init(pdev, clk);
	if (ret) {
		SND_LOG_ERR(HLOG, "clk init failed\n");
		ret = -ENOMEM;
		goto err_clk_init;
	}

	/* rglt init */
	ret = snd_sunxi_rglt_init(pdev, rglt);
	if (ret) {
		SND_LOG_ERR(HLOG, "rglt init failed\n");
		ret = -ENOMEM;
		goto err_rglt_init;
	}

	/* dts_params init */
	snd_sunxi_dts_params_init(pdev, dts);

	/* pa_pin init */
	codec->pa_cfg = snd_sunxi_pa_pin_init(pdev, &codec->pa_pin_max);

	/* alsa component register */
	ret = snd_soc_register_component(dev, &sunxi_codec_dev, &sunxi_codec_dai, 1);
	if (ret) {
		SND_LOG_ERR(HLOG, "internal-codec component register failed\n");
		ret = -ENOMEM;
		goto err_register_component;
	}

	ret = snd_sunxi_sysfs_create_group(pdev, &debug_attr);
	if (ret)
		SND_LOG_WARN(HLOG, "sysfs debug create failed\n");

	return 0;

err_register_component:
	snd_sunxi_rglt_exit(rglt);
err_rglt_init:
	snd_sunxi_clk_exit(clk);
err_clk_init:
	snd_sunxi_mem_exit(pdev, mem);
err_mem_init:
	devm_kfree(dev, codec);
err_devm_kzalloc:
	of_node_put(np);

	return ret;
}

static int sunxi_codec_dev_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct sunxi_codec *codec = dev_get_drvdata(dev);
	struct sunxi_codec_mem *mem = &codec->mem;
	struct sunxi_codec_clk *clk = &codec->clk;
	struct sunxi_codec_rglt *rglt = &codec->rglt;

	SND_LOG_DEBUG(HLOG, "\n");

	/* remove components */
	snd_sunxi_sysfs_remove_group(pdev, &debug_attr);

	snd_soc_unregister_component(dev);

	snd_sunxi_mem_exit(pdev, mem);
	snd_sunxi_clk_exit(clk);
	snd_sunxi_rglt_exit(rglt);
	if (codec->dts.spk_gpio_used)
		snd_sunxi_pa_pin_exit(pdev, codec->pa_cfg, codec->pa_pin_max);

	devm_kfree(dev, codec);
	of_node_put(pdev->dev.of_node);

	SND_LOG_DEBUG(HLOG, "unregister internal-codec codec success\n");

	return 0;
}

static const struct of_device_id sunxi_codec_of_match[] = {
	{ .compatible = "allwinner," DRV_NAME,},
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_codec_of_match);

static struct platform_driver sunxi_codec_driver = {
	.driver = {
		.name		= DRV_NAME,
		.owner		= THIS_MODULE,
		.of_match_table = sunxi_codec_of_match,
	},
	.probe	= sunxi_codec_dev_probe,
	.remove = sunxi_codec_dev_remove,
};

int __init sunxi_codec_dev_init(void)
{
	int ret;

	ret = platform_driver_register(&sunxi_codec_driver);
	if (ret != 0) {
		SND_LOG_ERR(HLOG, "platform driver register failed\n");
		return -EINVAL;
	}

	return ret;
}

void __exit sunxi_codec_dev_exit(void)
{
	platform_driver_unregister(&sunxi_codec_driver);
}

late_initcall(sunxi_codec_dev_init);
module_exit(sunxi_codec_dev_exit);

MODULE_DESCRIPTION("sunxi soundcard codec of internal-codec");
MODULE_AUTHOR("huhaoxin@allwinnertech.com");
MODULE_LICENSE("GPL");
