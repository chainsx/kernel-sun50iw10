/*
 * sound\soc\sunxi\snd_sun8iw11codec.h
 * (C) Copyright 2022-2027
 * allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * huhaoxin <huhaoxin@allwinnertech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */
#ifndef __SND_SUN8IW11_CODEC_H
#define __SND_SUN8IW11_CODEC_H

#define SUNXI_DAC_DPC		0x00
#define SUNXI_DAC_FIFO_CTR	0x04
#define SUNXI_DAC_FIFO_STA	0x08
/* left blank */
#define	SUNXI_ADC_FIFO_CTR	0x10
#define SUNXI_ADC_FIFO_STA	0x14
#define SUNXI_ADC_RXDATA	0x18
#define SUNXI_DAC_TXDATA	0x20
/* left blank */
#define SUNXI_DAC_CNT		0x40
#define SUNXI_ADC_CNT		0x44
#define SUNXI_DAC_DG		0x48
#define SUNXI_ADC_DG		0x4C
#define SUNXI_HMIC_CTRL		0x50
#define SUNXI_HMIC_DATA		0x54
/* left blank */
#define	SUNXI_DAC_DAP_CTR	0x60
#define	SUNXI_ADC_DAP_CTR	0x70
#define SUNXI_ADC_DRC_HHPFC 	0x200
#define SUNXI_ADC_DRC_LHPFC 	0x204

/* Analog register base - Digital register base */
#define SUNXI_PR_CFG		0x300

#define SUNXI_HP_VOLC		(SUNXI_PR_CFG + 0x00)
#define SUNXI_LOMIX_SRC		(SUNXI_PR_CFG + 0x01)
#define SUNXI_ROMIX_SRC		(SUNXI_PR_CFG + 0x02)
#define SUNXI_DAC_PA_SRC	(SUNXI_PR_CFG + 0x03)
#define SUNXI_LINEIN_GCTR	(SUNXI_PR_CFG + 0x04)
#define SUNXI_FM_GCTR		(SUNXI_PR_CFG + 0x05)
#define SUNXI_MICIN_GCTR	(SUNXI_PR_CFG + 0x06)
#define SUNXI_PAEN_HP_CTR	(SUNXI_PR_CFG + 0x07)
#define SUNXI_PHONEOUT_CTR	(SUNXI_PR_CFG + 0x08)
/* left blank */
#define SUNXI_MIC2G_LINEEN_CTR	(SUNXI_PR_CFG + 0x0A)
#define SUNXI_MIC1G_MICBIAS_CTR	(SUNXI_PR_CFG + 0x0B)
#define SUNXI_LADCMIX_SRC	(SUNXI_PR_CFG + 0x0C)
#define SUNXI_RADCMIX_SRC	(SUNXI_PR_CFG + 0x0D)
#define SUNXI_PA_POP_CTR	(SUNXI_PR_CFG + 0x0E)
#define SUNXI_ADC_AP_EN		(SUNXI_PR_CFG + 0x0F)
#define SUNXI_ADDA_APT0		(SUNXI_PR_CFG + 0x10)
#define SUNXI_ADDA_APT1		(SUNXI_PR_CFG + 0x11)
#define SUNXI_ADDA_APT2		(SUNXI_PR_CFG + 0x12)
#define SUNXI_CHOP_CAL_CTR	(SUNXI_PR_CFG + 0x13)
#define SUNXI_BIAS_DA16_CAL_CTR	(SUNXI_PR_CFG + 0x14)
#define SUNXI_DA16_CALI_DATA	(SUNXI_PR_CFG + 0x15)
/* left blank */
#define SUNXI_BIAS_CALI_DATA	(SUNXI_PR_CFG + 0x17)
#define SUNXI_BIAS_CALI_SET	(SUNXI_PR_CFG + 0x18)

#define SUNXI_AUDIO_MAX_REG	SUNXI_PR_CFG

/* SUNXI_DAC_DPC:0x00 */
#define EN_DAC			31
#define MODQU			25
#define HPF_EN			18
#define DVOL			12
#define HUB_EN			0

/* SUNXI_DAC_FIFO_CTR:0x04 */
#define DAC_FS			29
#define FIR_VER			28
#define SEND_LASAT		26
#define FIFO_MODE		24
#define DAC_DRQ_CLR_CNT		21
#define TX_TRIG_LEVEL		8
#define ADDA_LOOP_EN		7
#define DAC_MONO_EN		6
#define TX_SAMPLE_BITS		5
#define DAC_DRQ_EN		4
#define DAC_IRQ_EN		3
#define FIFO_UNDERRUN_IRQ_EN 	2
#define FIFO_OVERRUN_IRQ_EN	1
#define FIFO_FLUSH		0

/* SUNXI_DAC_FIFO_STA:0x08 */
#define	TX_EMPTY		23
#define	TXE_CNT			8
#define	TXE_INT			3
#define	TXU_INT			2
#define	TXO_INT			1

/* SUNXI_ADC_FIFO_CTR:0x10 */
#define ADC_FS			29
#define EN_AD			28
#define RX_FIFO_MODE		24
#define ADCFDT			17
#define ADCDFEN			16
#define RX_FIFO_TRG_LEVEL	8
#define ADC_MONO_EN		7
#define RX_SAMPLE_BITS		6
#define ADC_DRQ_EN		4
#define ADC_IRQ_EN		3
#define ADC_OVERRUN_IRQ_EN	1
#define ADC_FIFO_FLUSH		0

/* SUNXI_ADC_FIFO_STA:0x14 */
#define	RXA			23
#define	RXA_CNT			8
#define	RXA_INT			3
#define	RXO_INT			1

/* SUNXI_HMIC_CTRL:0x50 */
#define HMIC_M			28
#define HMIC_N			24
#define HMIC_DATA_IRQ_MODE	23
#define HMIC_TH1_HYSTERESIS	21
#define HMIC_PULLOUT_IRQ	20
#define HMIC_PLUGIN_IRQ		19
#define HMIC_KEYUP_IRQ		18
#define HMIC_KEYDOWN_IRQ	17
#define HMIC_DATA_IRQ_EN	16
#define HMIC_SAMPLE_SELECT	14
#define HMIC_TH2_HYSTERESIS	13
#define HMIC_TH2		8
#define HMIC_SF			6
#define KEYUP_CLEAR		5
#define HMIC_TH1		0

/* SUNXI_HMIC_DATA:0x54 */
#define HMIC_PULLOUT_PEND	20
#define HMIC_PLUGIN_PEND	19
#define HMIC_KEYUP_PEND		18
#define HMIC_KEYDOWN_PEND	17
#define HMIC_DATA_PEND		16
#define HMIC_DATA		0

/* SUNXI_DAC_DAP_CTR:0x60 */
#define	DDAP_EN			31
#define	DDAP_DRC_EN		15
#define	DDAP_HPF_EN		14
#define	RAM_ADDR		0

/* SUNXI_ADC_DAP_CTR:0x70 */
#define	ENADC_DRC		26
#define	ADC_DRC_EN		25
#define	ADC_DRC_HPF_EN		24

/* SUNXI_ADC_DRC_HHPFC:0x200 */
#define ADC_HHPF_CONF		0

/* SUNXI_ADC_DRC_LHPFC:0x204 */
#define ADC_LHPF_CONF		0

/* SUNXI_PR_CFG:0x300 */
#define AC_PR_RST		28
#define AC_PR_RW		24
#define AC_PR_ADDR		16
#define ADDA_PR_WDAT		8
#define ADDA_PR_RDAT		0

/* SUNXI_HP_VOLC:0x00 */
#define PA_CLK_GATING		7
#define HPVOL			0

/* SUNXI_LOMIX_SRC:0x01 */
#define LMIXMUTE		0
#define	LMIX_MIC1_BST		6
#define LMIX_MIC2_BST		5
#define LMIX_LINEINLR		4
#define LMIX_LINEINL		3
#define LMIX_FML		2
#define LMIX_LDAC		1
#define LMIX_RDAC		0

/* SUNXI_ROMIX_SRC:0x02 */
#define RMIXMUTE		0
#define RMIX_MIC1_BST		6
#define RMIX_MIC2_BST		5
#define RMIX_LINEINLR		4
#define RMIX_LINEINR		3
#define RMIX_FMR		2
#define RMIX_RDAC		1
#define RMIX_LDAC		0

/* SUNXI_DAC_PA_SRC:0x03 */
#define DACAREN			7
#define DACALEN			6
#define RMIXEN			5
#define LMIXEN			4
#define RHPPAMUTE		3
#define LHPPAMUTE		2
#define RHPIS			1
#define LHPIS			0

/* SUNXI_LINEIN_GCTR:0x04 */
#define LINEINLG		4
#define LINEINRG		0

/* SUNXI_FM_GCTR:0x05 */
#define FMG			4
#define LINEING			0

/* SUNXI_MICIN_GCTR:0x06 */
#define MIC1_GAIN		4
#define MIC2_GAIN		0

/* SUNXI_PAEN_HP_CTR:0x07 */
#define HPPAEN			7
#define HPCOM_FC		5
#define HPCOM_PT		4
#define PA_ANTI_POP_CTRL	2
#define LTRNMUTE		1
#define RTLNMUTE		0

/* SUNXI_PHONEOUT_CTR:0x08 */
#define PHONEOUTG		5
#define PHONEOUTEN		4
#define PHONEOUTS3		3
#define PHONEOUTS2		2
#define PHONEOUTS1		1
#define PHONEOUTS0		0

/* SUNXI_MIC2G_LINEEN_CTR:0x0A */
#define MIC2AMPEN		7
#define MIC2BOOST		4

/* SUNXI_MIC1G_MICBIAS_CTR:0x0B */
#define HMICBIASEN		7
#define MMICBIASEN		6
#define HMICBIASMODE		5
#define MIC2_SS			4
#define MIC1_AMPEN		3
#define MIC1_BOOST		0

/* SUNXI_LADCMIX_SRC:0x0C */
#define LADCMIXMUTE		0
#define LADC_MIC1_BST		6
#define LADC_MIC2_BST		5
#define LADC_LINEINLR		4
#define LADC_LINEINL		3
#define LADC_FML		2
#define LADC_LOUT_MIX		1
#define LADC_ROUT_MIX		0

/* SUNXI_RADCMIX_SRC:0x0D */
#define RADC_MIC1_BST		6
#define RADC_MIC2_BST		5
#define RADC_LINEINLR		4
#define RADC_LINEINR		3
#define RADC_FMR		2
#define RADC_ROUT_MIX		1
#define RADC_LOUT_MIX		0

/* SUNXI_PA_POP_CTR:0x0E */
#define PA_POP_CTRL		0

/* SUNXI_ADC_AP_EN:0x0F */
#define ADCREN			7
#define ADCLEN			6
#define ADCG			0

/* SUNXI_ADDA_APT0:0x10 */
#define	OPDRV_OPCOM_CUR		6
#define	OPADC1_BIAS_CUR		4
#define	OPADC2_BIAS_CUR		2
#define	OPAAF_BIAS_CUR		0

/* SUNXI_ADDA_APT1:0x11 */
#define	OPMIC_BIAS_CUR		6
#define	OPVR_BIAS_CUR		4
#define	OPDAC_BIAS_CUR		2
#define	OPMIX_BIAS_CUR		0

/* SUNXI_ADDA_APT2:0x12 */
#define	ZERO_CROSS_EN		7
#define	ZERO_CROSS_TIMEOUT	6
#define	PTDBS			4
#define PA_SLOPE_SELECT		3
#define	USB_BIAS_CUR		0

/* SUNXI_CHOP_CAL_CTR:0x13 */
#define MMIC_BIASCHOPEN		7
#define	MMIC_BIAS_CHOP_SRC	5
#define DITHER			4
#define	DITHER_CLK_SEL		2
#define	BIHE_CTRL		0

/* SUNXI_BIAS_DA16_CAL_CTR:0x14 */
#define PA_SPEED_SELECT		7
#define	CUR_TEST_SEL		6
#define	BIAS_DA16_CLK_SEL	4
#define	BIAS_CAL_MODE_SEL	3
#define	BIAS_DA16_CAL_CTRL	2
#define	BIAS_CAL_VER		1

struct sunxi_codec_mem {
	struct resource res;
	void __iomem *membase;
	struct resource *memregion;
	struct regmap *regmap;
};

struct sunxi_codec_clk {
	struct reset_control *clk_rst;
	struct clk *clk_bus_audio;

	struct clk *clk_pll_audio;
	struct clk *clk_module_audio;
};

struct sunxi_codec_rglt {
	u32 avcc_vol;
	u32 dvcc_vol;
	bool avcc_external;
	bool dvcc_external;
	struct regulator *avcc;
	struct regulator *dvcc;
};

struct sunxi_codec_dts {
	u32 headphonevol;
	u32 maingain;
	u32 spkervol;
	u32 spk_gpio;
	u32 pa_sleep_time;
	bool spk_gpio_used;
};

struct sunxi_codec {
	struct platform_device *pdev;

	struct sunxi_codec_mem mem;
	struct sunxi_codec_clk clk;
	struct sunxi_codec_rglt rglt;
	struct sunxi_codec_dts dts;

#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_JACK_CODEC)
	struct sunxi_jack_codec_priv jack_codec_priv;
#elif IS_ENABLED(CONFIG_SND_SOC_SUNXI_JACK_EXTCON)
	struct sunxi_jack_extcon_priv jack_extcon_priv;
#endif

	unsigned int pa_pin_max;
	struct pa_config *pa_cfg;
};

#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_DEBUG)
static inline int snd_sunxi_sysfs_create_group(struct platform_device *pdev,
					       struct attribute_group *attr)
{
	return sysfs_create_group(&pdev->dev.kobj, attr);
}

static inline void snd_sunxi_sysfs_remove_group(struct platform_device *pdev,
						struct attribute_group *attr)
{
	sysfs_remove_group(&pdev->dev.kobj, attr);
}
#else
static inline int snd_sunxi_sysfs_create_group(struct platform_device *pdev,
					       struct attribute_group *attr)
{
	return 0;
}

static inline void snd_sunxi_sysfs_remove_group(struct platform_device *pdev,
						struct attribute_group *attr)
{
	return;
}
#endif

#endif /* __SND_SUN8IW11_CODEC_H */
