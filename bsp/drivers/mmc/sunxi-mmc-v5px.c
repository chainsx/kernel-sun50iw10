/*
* Sunxi SD/MMC host driver
*
* Copyright (C) 2015 AllWinnertech Ltd.
* Author: lixiang <lixiang@allwinnertech>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed "as is" WITHOUT ANY WARRANTY of any
* kind, whether express or implied; without even the implied warranty
* of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/


#ifdef CONFIG_ARCH_SUN8IW10P1

#include <linux/clk.h>
#include <linux/clk-private.h>
#include <linux/reset/sunxi.h>

#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/scatterlist.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/reset.h>

#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>

#include <linux/mmc/host.h>
#include <linux/mmc/sd.h>
#include <linux/mmc/sdio.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/core.h>
#include <linux/mmc/card.h>
#include <linux/mmc/slot-gpio.h>

#include "sunxi-smhc.h"
#include "sunxi-mmc-v5px.h"

#define SMHC_DS_DLY	(0x230)
#define SMHC_THLD		(0x20c)

#define SMHC_SAMP_DL_SW_MASK		(0x0000003F)
#define SMHC_DS_DL_SW_MASK			(0x0000003F)
#define SMHC_DS_DL_SW_EN			(1u<<7)

#define SMHC_CARD_RD_TH_SZ		0x000007FF
#define SMHC_CARD_RD_TH_MASK	0x000007FF
#define SMHC_CARD_RD_TH_SHIFT	0x0

#define SMHC_CARD_WR_TH_SZ		0x000007FF
#define SMHC_CARD_WR_TH_MASK	(0x000007FF<<16)
#define SMHC_CARD_WR_TH_SHIFT	16

#define SMHC_DES_NUM_SHIFT_V5PX	(15)
#define SMHC_DES_BUFFER_MAX_LEN_V5PX	(1U << SMHC_DES_NUM_SHIFT)

enum sunxi_mmc_speed_mode {
	SM0_DS26_SDR12 = 0,
	SM1_HSSDR52_SDR25,
	SM2_HSDDR52_DDR50,
	SM3_HS200_SDR104,
	SM4_HS400,
	SM_NUM,
};

struct sunxi_mmc_clk_dly {
	enum sunxi_mmc_speed_mode spm;
	char *mod_str;
	char *raw_tm_sm_str[2];
	u32 raw_tm_sm[2];
	u32 raw_tm_sm_def[2];
};

struct sunxi_mmc_ver_priv {
	/* struct sunxi_mmc_spec_regs bak_spec_regs; */
	struct sunxi_mmc_clk_dly mmc_clk_dly[SM_NUM];
};

static void sunxi_mmc_set_clk_dly(struct sunxi_mmc_host *host, int clk,
				  int bus_width, int timing)
{
	struct mmc_host *mmc = host->mmc;
	enum sunxi_mmc_speed_mode speed_mod = SM0_DS26_SDR12;
	char *raw_sm_str = NULL;
	char *m_str = NULL;
	struct device_node *np = NULL;
	u32 *raw_sm = 0;
	u32 *raw_sm_def = 0;
	u32 rval = 0;
	int frq_index = 0;
	u32 cmd_drv_ph = 1;
	u32 dat_drv_ph = 0;
	u32 sam_dly = 0;
	u32 ds_dly = 0;
	struct sunxi_mmc_clk_dly *mmc_clk_dly =
	    ((struct sunxi_mmc_ver_priv *)host->version_priv_dat)->mmc_clk_dly;

	if (!mmc->parent || !mmc->parent->of_node) {
		SM_ERR(mmc_dev(host->mmc),
			"no dts to parse clk dly,use default\n");
		return;
	}

	np = mmc->parent->of_node;

	switch (timing) {
	case MMC_TIMING_LEGACY:
	case MMC_TIMING_UHS_SDR12:
		speed_mod = SM0_DS26_SDR12;
		break;
	case MMC_TIMING_MMC_HS:
	case MMC_TIMING_SD_HS:
	case MMC_TIMING_UHS_SDR25:
		speed_mod = SM1_HSSDR52_SDR25;
		break;
	case MMC_TIMING_UHS_DDR50:
	case MMC_TIMING_MMC_DDR52:
		speed_mod = SM2_HSDDR52_DDR50;
		break;
	case MMC_TIMING_UHS_SDR50:
	case MMC_TIMING_UHS_SDR104:
	case MMC_TIMING_MMC_HS200:
		speed_mod = SM3_HS200_SDR104;
		break;
	case MMC_TIMING_MMC_HS400:
		speed_mod = SM4_HS400;
		break;
	default:
		SM_ERR(mmc_dev(mmc), "Wrong timing input\n");
		return;
	}

	if (clk <= 400 * 1000) {
		frq_index = 0;
	} else if (clk <= 25 * 1000 * 1000) {
		frq_index = 1;
	} else if (clk <= 50 * 1000 * 1000) {
		frq_index = 2;
	} else if (clk <= 100 * 1000 * 1000) {
		frq_index = 3;
	} else if (clk <= 150 * 1000 * 1000) {
		frq_index = 4;
	} else if (clk <= 200 * 1000 * 1000) {
		frq_index = 5;
	} else if (clk <= 250 * 1000 * 1000) {
		frq_index = 6;
	} else if (clk <= 300 * 1000 * 1000) {
		frq_index = 7;
	} else {
		SM_ERR(mmc_dev(mmc), "clk is over 300mhz\n");
		return;
	}

	if (frq_index / 4 > 2) {
		SM_ERR(mmc_dev(host->mmc), "err frq_index\n");
		return;
	}

	SM_DBG(mmc_dev(host->mmc), "freq %d frq index %d,frq/4 %x\n", clk,
		frq_index, frq_index / 4);
	raw_sm_str = mmc_clk_dly[speed_mod].raw_tm_sm_str[frq_index / 4];
	raw_sm = &mmc_clk_dly[speed_mod].raw_tm_sm[frq_index / 4];
	raw_sm_def = &mmc_clk_dly[speed_mod].raw_tm_sm_def[frq_index / 4];
	m_str = mmc_clk_dly[speed_mod].mod_str;

	rval = of_property_read_u32(np, raw_sm_str, raw_sm);
	if (rval) {
		SM_INFO(mmc_dev(host->mmc), "failed to get %s used default\n",
			 m_str);
	} else {
		u32 sm_shift = (frq_index % 4) * 8;

		rval = ((*raw_sm) >> sm_shift) & 0xff;
		if (rval != 0xff) {
			if (timing == MMC_TIMING_MMC_HS400) {
				u32 raw_sm_hs200 = 0;

				ds_dly = rval;
				raw_sm_hs200 =
				    mmc_clk_dly[SM3_HS200_SDR104].
				    raw_tm_sm[frq_index / 4];
				sam_dly = ((raw_sm_hs200) >> sm_shift) & 0xff;
			} else {
				sam_dly = rval;
			}
			SM_DBG(mmc_dev(host->mmc),
				"Get speed mode %s clk dly %s ok\n", m_str,
				raw_sm_str);
		} else {
			u32 sm_shift = (frq_index % 4) * 8;

			SM_DBG(mmc_dev(host->mmc), "%s use default value\n",
				m_str);
			rval = ((*raw_sm_def) >> sm_shift) & 0xff;
			if (timing == MMC_TIMING_MMC_HS400) {
				u32 raw_sm_hs200 = 0;

				ds_dly = rval;
				raw_sm_hs200 =
				    mmc_clk_dly[SM3_HS200_SDR104].
				    raw_tm_sm_def[frq_index / 4];
				sam_dly = ((raw_sm_hs200) >> sm_shift) & 0xff;
			} else {
				sam_dly = rval;
			}
		}

	}

	SM_DBG(mmc_dev(host->mmc), "Try set %s clk dly	ok\n", m_str);
	SM_DBG(mmc_dev(host->mmc), "cmd_drv_ph	%d\n", cmd_drv_ph);
	SM_DBG(mmc_dev(host->mmc), "dat_drv_ph	%d\n", dat_drv_ph);
	SM_DBG(mmc_dev(host->mmc), "sam_dly	%d\n", sam_dly);
	SM_DBG(mmc_dev(host->mmc), "ds_dly		%d\n", ds_dly);

	rval = smhc_readl(host, SMHC_DS_DLY);
	rval &= ~SMHC_DS_DL_SW_MASK;
	rval |= ds_dly & SMHC_DS_DL_SW_MASK;
	rval |= SMHC_DS_DL_SW_EN;
	smhc_writel(host, SMHC_DS_DLY, rval);
	SM_DBG(mmc_dev(host->mmc), " SMHC_DS_DLY      %08x\n",
		smhc_readl(host, SMHC_DS_DLY));
}

void sunxi_mmc_dump_dly2(struct sunxi_mmc_host *host)
{
	SM_DBG(mmc_dev(host->mmc), "no imple %s %d\n", __func__, __LINE__);
}

static int __sunxi_mmc_do_oclk_onoff(struct sunxi_mmc_host *host, u32 oclk_en,
				     u32 pwr_save, u32 ignore_dat0)
{
	u32 tmp = 0;

	tmp = smhc_readl(host, SMHC_RST_CLK_CTRL);
	if (oclk_en)
		tmp |= SdclkEn;
	else
		tmp &= ~SdclkEn;

	smhc_writel(host, SMHC_RST_CLK_CTRL, tmp);

	tmp = smhc_readl(host, SMHC_CTRL3);
	if (pwr_save)
		tmp |= SdclkIdleCtrl;
	else
		tmp &= ~SdclkIdleCtrl;

	smhc_writel(host, SMHC_CTRL3, tmp);

	return 0;

}

int sunxi_mmc_oclk_onoff(struct sunxi_mmc_host *host, u32 oclk_en)
{
	struct device_node *np = NULL;
	struct mmc_host *mmc = host->mmc;
	int pwr_save = 0;
	int len = 0;

	if (!mmc->parent || !mmc->parent->of_node) {
		SM_ERR(mmc_dev(host->mmc),
			"no dts to parse power save mode\n");
		return -EIO;
	}

	np = mmc->parent->of_node;
	if (of_find_property(np, "sunxi-power-save-mode", &len))
		pwr_save = 1;
	return __sunxi_mmc_do_oclk_onoff(host, oclk_en, pwr_save, 1);
}
int sunxi_mmc_clk_set_rate_v5px(struct sunxi_mmc_host *host,
				struct mmc_ios *ios)
{
	u32 mod_clk = 0;
	u32 src_clk = 0;
	u32 rval = 0;
	s32 err = 0;
	u32 rate = 0;
	char *sclk_name = NULL;
	struct clk *mclk = host->clk_mmc;
	struct clk *sclk = NULL;
	struct device *dev = mmc_dev(host->mmc);

	if (ios->clock == 0) {
		__sunxi_mmc_do_oclk_onoff(host, 0, 0, 1);
		return 0;
	}

	if (sunxi_mmc_ddr_timing(ios->timing))
		mod_clk = ios->clock << 3;
	else
		mod_clk = ios->clock << 2;

	if (ios->clock <= 400000) {
		sclk = clk_get(dev, "osc24m");
		sclk_name = "osc24m";
	} else {
		sclk = clk_get(dev, "pll_periph");
		sclk_name = "pll_periph";
	}
	if (IS_ERR(sclk)) {
		SM_ERR(mmc_dev(host->mmc), "Error to get source clock %s\n",
			sclk_name);
		return -1;
	}

	sunxi_mmc_oclk_onoff(host, 0);

	err = clk_set_parent(mclk, sclk);
	if (err) {
		SM_ERR(mmc_dev(host->mmc), "set parent failed\n");
		clk_put(sclk);
		return -1;
	}

	rate = clk_round_rate(mclk, mod_clk);

	SM_DBG(mmc_dev(host->mmc), "get round rate %d\n", rate);

	clk_disable_unprepare(host->clk_mmc);

	err = clk_set_rate(mclk, rate);
	if (err) {
		SM_ERR(mmc_dev(host->mmc), "set mclk rate error, rate %dHz\n",
			rate);
		clk_put(sclk);
		return -1;
	}

	rval = clk_prepare_enable(host->clk_mmc);
	if (rval) {
		SM_ERR(mmc_dev(host->mmc), "Enable mmc clk err %d\n", rval);
		return -1;
	}

	src_clk = clk_get_rate(sclk);
	clk_put(sclk);

	SM_DBG(mmc_dev(host->mmc), "set round clock %d, soure clk is %d\n",
		rate, src_clk);

	/* sunxi_of_parse_clk_dly(host); */
	if (sunxi_mmc_ddr_timing(ios->timing))
		ios->clock = rate >> 3;
	else
		ios->clock = rate >> 2;


	sunxi_mmc_set_clk_dly(host, ios->clock, ios->bus_width, ios->timing);

	return sunxi_mmc_oclk_onoff(host, 1);
}

void sunxi_mmc_thld_ctl_v5px(struct sunxi_mmc_host *host,
			     struct mmc_ios *ios, struct mmc_data *data)
{
	u32 bsz = data->blksz;
	u32 rval = 0;

	if ((data->flags & MMC_DATA_WRITE)
	    && (bsz <= SMHC_CARD_WR_TH_SZ)) {
		rval = smhc_readl(host, SMHC_THLD);
		rval &= ~SMHC_CARD_WR_TH_MASK;
		rval |= data->blksz << SMHC_CARD_WR_TH_SHIFT;
		/* rval |= SDXC_CARD_WR_THLD_ENB; */
		smhc_writel(host, SMHC_THLD, rval);
	} else {
		/*
		*   rval = mmc_readl(host, REG_THLD);
		*   rval &= ~SDXC_CARD_WR_THLD_ENB;
		 *  mmc_writel(host, REG_THLD, rval);
		 */
	}

	if ((data->flags & MMC_DATA_READ)
	    && (bsz <= SMHC_CARD_RD_TH_SZ)
	    && ((ios->timing == MMC_TIMING_MMC_HS200)
		|| (ios->timing == MMC_TIMING_MMC_HS400)
		|| (ios->timing == MMC_TIMING_UHS_SDR50)
		|| (ios->timing == MMC_TIMING_UHS_SDR104))) {
		rval = smhc_readl(host, SMHC_THLD);
		rval &= ~SMHC_CARD_RD_TH_MASK;
		rval |= data->blksz << SMHC_CARD_RD_TH_SHIFT;
		/* rval |= SDXC_CARD_RD_THLD_ENB; */
		smhc_writel(host, SMHC_THLD, rval);
	} else {
		/*
		*   rval = mmc_readl(host, REG_THLD);
		*  rval &= ~SDXC_CARD_RD_THLD_ENB;
		*  mmc_writel(host, REG_THLD, rval);
		 */
	}

	SM_DBG(mmc_dev(host->mmc), "SDXC_REG_THLD: 0x%08x\n",
		smhc_readl(host, SMHC_THLD));
}

void sunxi_mmc_save_spec_reg_v5px(struct sunxi_mmc_host *host)
{
	SM_DBG(mmc_dev(host->mmc), "no imple %s %d\n", __func__, __LINE__);

}

void sunxi_mmc_restore_spec_reg_v5px(struct sunxi_mmc_host *host)
{
	SM_DBG(mmc_dev(host->mmc), "no imple %s %d\n", __func__, __LINE__);

}



static int sunxi_mmc_can_poweroff_notify(const struct mmc_card *card)
{
	return card &&
	    mmc_card_mmc(card) &&
	    (card->ext_csd.power_off_notification == EXT_CSD_POWER_ON);
}

static int sunxi_mmc_poweroff_notify(struct mmc_card *card,
				     unsigned int notify_type)
{
	unsigned int timeout = card->ext_csd.generic_cmd6_time;
	int err;

	/* Use EXT_CSD_POWER_OFF_SHORT as default notification type. */
	if (notify_type == EXT_CSD_POWER_OFF_LONG)
		timeout = card->ext_csd.power_off_longtime;

	err = __mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
			   EXT_CSD_POWER_OFF_NOTIFICATION,
			   notify_type, timeout, true, false, false);
	if (err)
		pr_err("%s: Power Off Notification timed out, %u\n",
		       mmc_hostname(card->host), timeout);

	/* Disable the power off notification after the switch operation. */
	card->ext_csd.power_off_notification = EXT_CSD_NO_POWER_NOTIFICATION;

	return err;
}

static int sunxi_mmc_sleep(struct mmc_host *host)
{
	struct mmc_card *card = host->card;
	int err = -1;

	if (card && card->ext_csd.rev >= 3) {
		err = mmc_card_sleepawake(host, 1);
		if (err < 0)
			pr_debug("%s: Error %d while putting card into sleep",
				 mmc_hostname(host), err);
	}

	return err;
}

static int sunxi_mmc_suspend(struct mmc_host *host, bool is_suspend)
{
	int err = 0;
	unsigned int notify_type = is_suspend ? EXT_CSD_POWER_OFF_SHORT :
	    EXT_CSD_POWER_OFF_LONG;

	if (!host) {
		pr_err("Host should be null\n");
		return -1;
	}
	if (!host->card) {
		pr_err("Card should be null\n");
		return -1;
	}

	mmc_claim_host(host);

	/*
	 *  if (mmc_card_suspended(host->card))
	 *  goto out;
	 */

	if (mmc_card_doing_bkops(host->card)) {
		err = mmc_stop_bkops(host->card);
		if (err)
			goto out;
	}

	err = mmc_flush_cache(host->card);

	if (err)
		goto out;

	if (sunxi_mmc_can_poweroff_notify(host->card) &&
	    ((host->caps2 & MMC_CAP2_POWEROFF_NOTIFY) || !is_suspend)) {
		err = sunxi_mmc_poweroff_notify(host->card, notify_type);
	} else if (mmc_card_can_sleep(host)) {
		err = sunxi_mmc_sleep(host);
	} else if (!mmc_host_is_spi(host)) {
		err = mmc_deselect_cards(host);
	}

	if (!err) {
		pr_info("%s: %s %d\n",
			mmc_hostname(host), __func__, __LINE__);
		mmc_power_off(host);
		/* mmc_card_set_suspended(host->card); */
	}

out:
	mmc_release_host(host);
	return err;
}

void sunxi_mmc_do_shutdown_v5px(struct platform_device *pdev)
{
	struct mmc_host *mmc = platform_get_drvdata(pdev);
	u32 shutdown_notify_type = 0;
	u32 rval =
	    of_property_read_u32(mmc->parent->of_node, "shutdown_notify_type",
				 &shutdown_notify_type);
	if (!rval)
		sunxi_mmc_suspend(mmc, shutdown_notify_type);
	else
		sunxi_mmc_suspend(mmc, false);
}

void sunxi_mmc_init_priv_v5px(struct sunxi_mmc_host *host,
			      struct platform_device *pdev, int phy_index)
{
	struct sunxi_mmc_ver_priv *ver_priv =
	    devm_kzalloc(&pdev->dev, sizeof(struct sunxi_mmc_ver_priv),
			 GFP_KERNEL);
	host->version_priv_dat = ver_priv;

	ver_priv->mmc_clk_dly[SM0_DS26_SDR12].spm = SM0_DS26_SDR12;
	ver_priv->mmc_clk_dly[SM0_DS26_SDR12].mod_str = "DS26_SDR12";
	ver_priv->mmc_clk_dly[SM0_DS26_SDR12].raw_tm_sm_str[0] =
	    "sdc_tm4_sm0_freq0";
	ver_priv->mmc_clk_dly[SM0_DS26_SDR12].raw_tm_sm_str[1] =
	    "sdc_tm4_sm0_freq1";
	ver_priv->mmc_clk_dly[SM0_DS26_SDR12].raw_tm_sm[0] = 0;
	ver_priv->mmc_clk_dly[SM0_DS26_SDR12].raw_tm_sm[1] = 0;
	ver_priv->mmc_clk_dly[SM0_DS26_SDR12].raw_tm_sm_def[0] = 0;
	ver_priv->mmc_clk_dly[SM0_DS26_SDR12].raw_tm_sm_def[1] = 0;

	ver_priv->mmc_clk_dly[SM1_HSSDR52_SDR25].spm = SM1_HSSDR52_SDR25;
	ver_priv->mmc_clk_dly[SM1_HSSDR52_SDR25].mod_str = "HSSDR52_SDR25";
	ver_priv->mmc_clk_dly[SM1_HSSDR52_SDR25].raw_tm_sm_str[0] =
	    "sdc_tm4_sm1_freq0";
	ver_priv->mmc_clk_dly[SM1_HSSDR52_SDR25].raw_tm_sm_str[1] =
	    "sdc_tm4_sm1_freq1";
	ver_priv->mmc_clk_dly[SM1_HSSDR52_SDR25].raw_tm_sm[0] = 0;
	ver_priv->mmc_clk_dly[SM1_HSSDR52_SDR25].raw_tm_sm[1] = 0;
	ver_priv->mmc_clk_dly[SM1_HSSDR52_SDR25].raw_tm_sm_def[0] = 0;
	ver_priv->mmc_clk_dly[SM1_HSSDR52_SDR25].raw_tm_sm_def[1] = 0;

	ver_priv->mmc_clk_dly[SM2_HSDDR52_DDR50].spm = SM2_HSDDR52_DDR50;
	ver_priv->mmc_clk_dly[SM2_HSDDR52_DDR50].mod_str = "HSDDR52_DDR50";
	ver_priv->mmc_clk_dly[SM2_HSDDR52_DDR50].raw_tm_sm_str[0] =
	    "sdc_tm4_sm2_freq0";
	ver_priv->mmc_clk_dly[SM2_HSDDR52_DDR50].raw_tm_sm_str[1] =
	    "sdc_tm4_sm2_freq1";
	ver_priv->mmc_clk_dly[SM2_HSDDR52_DDR50].raw_tm_sm[0] = 0;
	ver_priv->mmc_clk_dly[SM2_HSDDR52_DDR50].raw_tm_sm[1] = 0;
	ver_priv->mmc_clk_dly[SM2_HSDDR52_DDR50].raw_tm_sm_def[0] = 0;
	ver_priv->mmc_clk_dly[SM2_HSDDR52_DDR50].raw_tm_sm_def[1] = 0;

	ver_priv->mmc_clk_dly[SM3_HS200_SDR104].spm = SM3_HS200_SDR104;
	ver_priv->mmc_clk_dly[SM3_HS200_SDR104].mod_str = "HS200_SDR104";
	ver_priv->mmc_clk_dly[SM3_HS200_SDR104].raw_tm_sm_str[0] =
	    "sdc_tm4_sm3_freq0";
	ver_priv->mmc_clk_dly[SM3_HS200_SDR104].raw_tm_sm_str[1] =
	    "sdc_tm4_sm3_freq1";
	ver_priv->mmc_clk_dly[SM3_HS200_SDR104].raw_tm_sm[0] = 0;
	ver_priv->mmc_clk_dly[SM3_HS200_SDR104].raw_tm_sm[1] = 0;
	ver_priv->mmc_clk_dly[SM3_HS200_SDR104].raw_tm_sm_def[0] = 0;
	ver_priv->mmc_clk_dly[SM3_HS200_SDR104].raw_tm_sm_def[1] = 0x00000405;

	ver_priv->mmc_clk_dly[SM4_HS400].spm = SM4_HS400;
	ver_priv->mmc_clk_dly[SM4_HS400].mod_str = "HS400";
	ver_priv->mmc_clk_dly[SM4_HS400].raw_tm_sm_str[0] = "sdc_tm4_sm4_freq0";
	ver_priv->mmc_clk_dly[SM4_HS400].raw_tm_sm_str[1] = "sdc_tm4_sm4_freq1";
	ver_priv->mmc_clk_dly[SM4_HS400].raw_tm_sm[0] = 0;
	ver_priv->mmc_clk_dly[SM4_HS400].raw_tm_sm[1] = 0x00000608;
	ver_priv->mmc_clk_dly[SM4_HS400].raw_tm_sm_def[0] = 0;
	ver_priv->mmc_clk_dly[SM4_HS400].raw_tm_sm_def[1] = 0x00000408;

	host->sunxi_mmc_clk_set_rate = sunxi_mmc_clk_set_rate_v5px;
	/* host->idma_des_size_bits = 15; */
	host->idma_des_size_bits = SMHC_DES_NUM_SHIFT_V5PX;
	host->sunxi_mmc_thld_ctl = sunxi_mmc_thld_ctl_v5px;
	host->sunxi_mmc_save_spec_reg = sunxi_mmc_save_spec_reg_v5px;
	host->sunxi_mmc_restore_spec_reg = sunxi_mmc_restore_spec_reg_v5px;
	/* sunxi_mmc_reg_ex_res_inter(host, phy_index);
	*  host->sunxi_mmc_set_acmda = sunxi_mmc_set_a12a;
	 */
	host->sunxi_mmc_dump_dly_table = sunxi_mmc_dump_dly2;
	host->phy_index = phy_index;
}

#endif
