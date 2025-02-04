/* sound\soc\sunxi\snd_sunxi_daudio.h
 * (C) Copyright 2021-2025
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Dby <dby@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __SND_SUNXI_DAUDIO_H
#define __SND_SUNXI_DAUDIO_H

/* DAUDIO register definition */
#define SUNXI_DAUDIO_CTL		0x00
#define SUNXI_DAUDIO_FMT0		0x04
#define SUNXI_DAUDIO_FMT1		0x08
#define SUNXI_DAUDIO_INTSTA		0x0C
#define SUNXI_DAUDIO_RXFIFO		0x10
#define SUNXI_DAUDIO_FIFOCTL		0x14
#define SUNXI_DAUDIO_FIFOSTA		0x18
#define SUNXI_DAUDIO_INTCTL		0x1C
#define SUNXI_DAUDIO_TXFIFO		0x20
#define SUNXI_DAUDIO_CLKDIV		0x24
#define SUNXI_DAUDIO_TXCNT		0x28
#define SUNXI_DAUDIO_RXCNT		0x2C
#define SUNXI_DAUDIO_CHCFG		0x30
#define SUNXI_DAUDIO_TX0CHSEL		0x34
#define SUNXI_DAUDIO_TX1CHSEL		0x38
#define SUNXI_DAUDIO_TX2CHSEL		0x3C
#define SUNXI_DAUDIO_TX3CHSEL		0x40
#define SUNXI_DAUDIO_TX0CHMAP0		0x44
#define SUNXI_DAUDIO_TX0CHMAP1		0x48
#define SUNXI_DAUDIO_TX1CHMAP0		0x4C
#define SUNXI_DAUDIO_TX1CHMAP1		0x50
#define SUNXI_DAUDIO_TX2CHMAP0		0x54
#define SUNXI_DAUDIO_TX2CHMAP1		0x58
#define SUNXI_DAUDIO_TX3CHMAP0		0x5C
#define SUNXI_DAUDIO_TX3CHMAP1		0x60
#define SUNXI_DAUDIO_RXCHSEL		0x64
#define SUNXI_DAUDIO_RXCHMAP0		0x68
#define SUNXI_DAUDIO_RXCHMAP1		0x6C
#define SUNXI_DAUDIO_RXCHMAP2		0x70
#define SUNXI_DAUDIO_RXCHMAP3		0x74

/* DAUDIO slot num max = 8 */
#define SUNXI_DAUDIO_8SLOT_CHCFG	0x30
#define SUNXI_DAUDIO_8SLOT_TX0CHSEL	0x34
#define SUNXI_DAUDIO_8SLOT_TX1CHSEL	0x38
#define SUNXI_DAUDIO_8SLOT_TX2CHSEL	0x3C
#define SUNXI_DAUDIO_8SLOT_TX3CHSEL	0x40
#define SUNXI_DAUDIO_8SLOT_TX0CHMAP	0x44
#define SUNXI_DAUDIO_8SLOT_TX1CHMAP	0x48
#define SUNXI_DAUDIO_8SLOT_TX2CHMAP	0x4C
#define SUNXI_DAUDIO_8SLOT_TX3CHMAP	0x50
#define SUNXI_DAUDIO_8SLOT_RXCHSEL	0x54
#define SUNXI_DAUDIO_8SLOT_RXCHMAP	0x58

#define SUNXI_DAUDIO_DEBUG		0x78
#define SUNXI_DAUDIO_REV		0x7C

#define SUNXI_DAUDIO_MAX_REG		SUNXI_DAUDIO_REV

#define SUN8IW11_DAUDIO_MAX_REG		SUNXI_DAUDIO_8SLOT_RXCHMAP

/* SUNXI_DAUDIO_CTL:0x00 */
#define RX_SYNC_EN_START		21
#define RX_SYNC_EN			20
#define BCLK_OUT			18
#define LRCK_OUT			17
#define LRCKR_CTL			16
#define SDO3_EN				11
#define SDO2_EN				10
#define SDO1_EN				9
#define SDO0_EN				8
#define MUTE_CTL			6
#define MODE_SEL			4
#define LOOP_EN				3
#define CTL_TXEN			2
#define CTL_RXEN			1
#define GLOBAL_EN			0

/* SUNXI_DAUDIO_FMT0:0x04 */
#define SDI_SYNC_SEL			31
#define LRCK_WIDTH			30
#define LRCKR_PERIOD			20
#define LRCK_POLARITY			19
#define LRCK_PERIOD			8
#define BCLK_POLARITY			7
#define DAUDIO_SAMPLE_RESOLUTION	4
#define EDGE_TRANSFER			3
#define SLOT_WIDTH			0

/* SUNXI_DAUDIO_FMT1:0x08 */
#define RX_MLS				7
#define TX_MLS				6
#define SEXT				4
#define RX_PDM				2
#define TX_PDM				0

/* SUNXI_DAUDIO_INTSTA:0x0C */
#define TXU_INT				6
#define TXO_INT				5
#define TXE_INT				4
#define RXU_INT				2
#define RXO_INT				1
#define RXA_INT				0

/* SUNXI_DAUDIO_FIFOCTL:0x14 */
#define HUB_EN				31
#define FIFO_CTL_FTX			25
#define FIFO_CTL_FRX			24
#define TXTL				12
#define RXTL				4
#define TXIM				2
#define RXOM				0

/* SUNXI_DAUDIO_FIFOSTA:0x18 */
#define FIFO_TXE			28
#define FIFO_TX_CNT			16
#define FIFO_RXA			8
#define FIFO_RX_CNT			0

/* SUNXI_DAUDIO_INTCTL:0x1C */
#define TXDRQEN				7
#define TXUI_EN				6
#define TXOI_EN				5
#define TXEI_EN				4
#define RXDRQEN				3
#define RXUIEN				2
#define RXOIEN				1
#define RXAIEN				0

/* SUNXI_DAUDIO_CLKDIV:0x24 */
#define MCLKOUT_EN			8
#define BCLK_DIV			4
#define MCLK_DIV			0

/* SUNXI_DAUDIO_CHCFG:0x30 */
#define TX_SLOT_HIZ			9
#define TX_STATE			8
#define RX_SLOT_NUM			4
#define TX_SLOT_NUM			0

/* SUNXI_DAUDIO_TXnCHSEL:0X34+n*0x04 */
#define TX_OFFSET			20
#define TX_CHSEL			16
#define TX_CHEN				0

/* SUNXI_DAUDIO_RXCHSEL */
#define RX_OFFSET			20
#define RX_CHSEL			16

#ifdef CONFIG_ARCH_SUN50IW10
#include "platforms/snd_sun50iw10_daudio.h"
#elif defined(CONFIG_ARCH_SUN8IW11)
#include "platforms/snd_sun8iw11_daudio.h"
/* e.g.
 * #elif defined(CONFIG_ARCH_SUNXXIWXX)
 * #include "platforms/snd_sunxxiwxx_daudio.h"
 */
#endif

struct sunxi_daudio;

/* To adapt diffent reg / bits */
struct sunxi_daudio_quirks {
	struct audio_reg_label *reg_labels;
	unsigned int reg_labels_size;
	unsigned int reg_max;

	bool rx_sync_en;

	int (*set_channel_enable)(struct sunxi_daudio *daudio, int stream, unsigned int channels);
	int (*set_daifmt_format)(struct sunxi_daudio *daudio, unsigned int format);
	int (*set_channels_map)(struct sunxi_daudio *daudio, unsigned int channels);
};

struct sunxi_daudio_mem {
	struct resource res;
	void __iomem *membase;
	struct resource *memregion;
	struct regmap *regmap;
};

struct sunxi_daudio_pinctl {
	struct pinctrl *pinctrl;
	struct pinctrl_state *pinstate;
	struct pinctrl_state *pinstate_sleep;

	bool pinctrl_used;
};

struct sunxi_daudio_dts {
	/* value must be (2^n)Kbyte */
	size_t playback_cma;
	size_t playback_fifo_size;
	size_t capture_cma;
	size_t capture_fifo_size;

	unsigned int dai_type;
	unsigned int tdm_num;
	bool tx_pin[4];
	bool rx_pin[4];

	/* tx_hub */
	bool tx_hub_en;

	/* components func -> rx_sync */
	bool rx_sync_en;	/* read from dts */
	bool rx_sync_ctl;
	int rx_sync_id;
	rx_sync_domain_t rx_sync_domain;

	/* quirks to adapt diffent chip */
	const struct sunxi_daudio_quirks *quirks;
};

struct sunxi_daudio_rglt {
	struct regulator *daudio_rglt;
	const char *rglt_name;
};

enum SUNXI_I2S_DAI_FMT_SEL {
	SUNXI_I2S_DAI_PLL = 0,
	SUNXI_I2S_DAI_MCLK,
	SUNXI_I2S_DAI_FMT,
	SUNXI_I2S_DAI_MASTER,
	SUNXI_I2S_DAI_INVERT,
	SUNXI_I2S_DAI_SLOT_NUM,
	SUNXI_I2S_DAI_SLOT_WIDTH,
};

struct sunxi_i2s_dai_fmt {
	unsigned int pllclk_freq;
	unsigned int moduleclk_freq;
	unsigned int fmt;
	unsigned int slots;
	unsigned int slot_width;
};

struct sunxi_daudio {
	struct platform_device *pdev;

	struct sunxi_daudio_mem mem;
	struct sunxi_daudio_clk clk;
	struct sunxi_daudio_pinctl pin;
	struct sunxi_daudio_dts dts;
	struct sunxi_daudio_rglt rglt;

	struct sunxi_dma_params playback_dma_param;
	struct sunxi_dma_params capture_dma_param;

	struct sunxi_i2s_dai_fmt i2s_dai_fmt;

	enum HDMI_FORMAT hdmi_fmt;

	const struct sunxi_daudio_quirks *quirks;
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
}
#endif

#endif /* __SND_SUNXI_DAUDIO_H */
