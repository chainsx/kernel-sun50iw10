/* sound\soc\sunxi\snd_sunxi_spdif.h
 * (C) Copyright 2021-2025
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Dby <dby@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __SND_SUN50IW9_SPDIF_H
#define __SND_SUN50IW9_SPDIF_H

#define HLOG		"SPDIF"

struct sunxi_spdif_clk {
	struct clk *clk_pll_audio;
	struct clk *clk_pll_audio_4x;
	struct clk *clk_parent;
	struct clk *clk_spdif;	/* tx & rx for 50iw9 */

	unsigned int pll_fs_tx;

	struct clk *clk_bus;
	struct reset_control *clk_rst;
};

static int snd_sunxi_clk_init(struct platform_device *pdev, struct sunxi_spdif_clk *clk);
static void snd_sunxi_clk_exit(struct sunxi_spdif_clk *clk);
static int snd_sunxi_clk_enable(struct sunxi_spdif_clk *clk);
static void snd_sunxi_clk_disable(struct sunxi_spdif_clk *clk);
static int snd_sunxi_clk_rate(struct sunxi_spdif_clk *clk, unsigned int freq_in,
			      unsigned int freq_out);

static inline int snd_sunxi_clk_init(struct platform_device *pdev, struct sunxi_spdif_clk *clk)
{
	int ret = 0;
	unsigned int temp_val;
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
	clk->clk_bus = of_clk_get_by_name(np, "clk_bus_spdif");
	if (IS_ERR_OR_NULL(clk->clk_bus)) {
		SND_LOG_ERR(HLOG, "clk bus get failed\n");
		ret = PTR_ERR(clk->clk_bus);
		goto err_get_clk_bus;
	}

	/* get parent clk */
	ret = of_property_read_u32(np, "pll-fs", &temp_val);
	if (ret < 0) {
		clk->pll_fs_tx = 1;
	} else {
		clk->pll_fs_tx = temp_val;
	}
	switch (clk->pll_fs_tx) {
	case 1:
		clk->clk_pll_audio = of_clk_get_by_name(np, "clk_pll_audio");
		if (IS_ERR_OR_NULL(clk->clk_pll_audio)) {
			SND_LOG_ERR(HLOG, "clk pll get failed\n");
			ret = PTR_ERR(clk->clk_pll_audio);
			goto err_get_clk_parent;
		}
		clk->clk_parent = clk->clk_pll_audio;
		break;
	case 4:
		clk->clk_pll_audio_4x = of_clk_get_by_name(np, "clk_pll_audio_4x");
		if (IS_ERR_OR_NULL(clk->clk_pll_audio_4x)) {
			SND_LOG_ERR(HLOG, "clk pll get failed\n");
			ret = PTR_ERR(clk->clk_pll_audio_4x);
			goto err_get_clk_parent;
		}
		clk->clk_parent = clk->clk_pll_audio_4x;
		break;
	default:
		SND_LOG_ERR(HLOG, "unsupport pll fs\n");
		goto err_get_clk_parent;
	}

	/* get spdif clk */
	clk->clk_spdif = of_clk_get_by_name(np, "clk_spdif");
	if (IS_ERR_OR_NULL(clk->clk_spdif)) {
		SND_LOG_ERR(HLOG, "clk spdif get failed\n");
		ret = PTR_ERR(clk->clk_spdif);
		goto err_get_clk_spdif;
	}

	/* set clk spdif parent of pllaudio */
	if (clk_set_parent(clk->clk_spdif, clk->clk_parent)) {
		SND_LOG_ERR(HLOG, "set parent clk spdif failed\n");
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
	clk_put(clk->clk_spdif);
err_get_clk_spdif:
	if (IS_ERR_OR_NULL(clk->clk_parent))
		clk_put(clk->clk_parent);
err_get_clk_parent:
	clk_put(clk->clk_bus);
err_get_clk_bus:
err_get_clk_rst:
	return ret;
}

static inline void snd_sunxi_clk_exit(struct sunxi_spdif_clk *clk)
{
	SND_LOG_DEBUG(HLOG, "\n");

	snd_sunxi_clk_disable(clk);
	clk_put(clk->clk_spdif);
	clk_put(clk->clk_parent);
	clk_put(clk->clk_bus);
}

static inline int snd_sunxi_clk_enable(struct sunxi_spdif_clk *clk)
{
	int ret = 0;

	SND_LOG_DEBUG(HLOG, "\n");

	if (reset_control_deassert(clk->clk_rst)) {
		SND_LOG_ERR(HLOG, "clk_rst deassert failed\n");
		goto err_deassert_rst;
	}

	if (clk_prepare_enable(clk->clk_bus)) {
		SND_LOG_ERR(HLOG, "clk_bus enable failed\n");
		goto err_enable_clk_bus;
	}

	if (clk_prepare_enable(clk->clk_parent)) {
		SND_LOG_ERR(HLOG, "clk_parent enable failed\n");
		goto err_enable_clk_parent;
	}

	if (clk_prepare_enable(clk->clk_spdif)) {
		SND_LOG_ERR(HLOG, "clk_spdif enable failed\n");
		goto err_enable_clk_spdif;
	}

	return 0;

err_enable_clk_spdif:
	clk_disable_unprepare(clk->clk_parent);
err_enable_clk_parent:
	clk_disable_unprepare(clk->clk_bus);
err_enable_clk_bus:
	reset_control_assert(clk->clk_rst);
err_deassert_rst:
	return ret;
}

static inline void snd_sunxi_clk_disable(struct sunxi_spdif_clk *clk)
{
	SND_LOG_DEBUG(HLOG, "\n");

	clk_disable_unprepare(clk->clk_spdif);
	clk_disable_unprepare(clk->clk_parent);
	clk_disable_unprepare(clk->clk_bus);
	reset_control_assert(clk->clk_rst);
}

static inline int snd_sunxi_clk_rate(struct sunxi_spdif_clk *clk, unsigned int freq_in,
				     unsigned int freq_out)
{
	SND_LOG_DEBUG(HLOG, "\n");

	if (clk_set_rate(clk->clk_parent, freq_in)) {
		SND_LOG_ERR(HLOG, "clk parent set rate failed\n");
		return -EINVAL;
	}

	if (clk_set_rate(clk->clk_spdif, freq_out / clk->pll_fs_tx)) {
		SND_LOG_ERR(HLOG, "clk spdif set rate failed\n");
		return -EINVAL;
	}

	return 0;
}

#endif /* __SND_SUN50IW9_SPDIF_H */
