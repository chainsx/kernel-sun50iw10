/* sound\soc\sunxi\snd_sun8iw_spdif.h
 * (C) Copyright 2021-2025
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * huhaoxin <huhaoxin@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __SND_SUN8IW11_SPDIF_H
#define __SND_SUN8IW11_SPDIF_H

#define HLOG		"SPDIF"

struct sunxi_spdif_clk {
	struct clk *clk_pll;
	struct clk *clk_spdif;

	unsigned int pll_fs_tx;

	struct clk *clk_bus;
	struct reset_control *clk_rst;
};

static int snd_sunxi_clk_init(struct platform_device *pdev, struct sunxi_spdif_clk *clk);
static void snd_sunxi_clk_exit(struct sunxi_spdif_clk *clk);
static int snd_sunxi_clk_enable(struct sunxi_spdif_clk *clk);
static void snd_sunxi_clk_disable(struct sunxi_spdif_clk *clk);

static inline int snd_sunxi_clk_init(struct platform_device *pdev, struct sunxi_spdif_clk *clk)
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
	clk->clk_bus = of_clk_get_by_name(np, "clk_bus_spdif");
	if (IS_ERR_OR_NULL(clk->clk_bus)) {
		SND_LOG_ERR(HLOG, "clk bus get failed\n");
		ret = PTR_ERR(clk->clk_bus);
		goto err_get_clk_bus;
	}

	clk->clk_pll = of_clk_get_by_name(np, "clk_pll_audio");
	if (IS_ERR_OR_NULL(clk->clk_pll)) {
		SND_LOG_ERR(HLOG, "clk pll get failed\n");
		ret = PTR_ERR(clk->clk_pll);
		goto err_get_clk_pll;
	}

	/* get spdif clk */
	clk->clk_spdif = of_clk_get_by_name(np, "clk_spdif");
	if (IS_ERR_OR_NULL(clk->clk_spdif)) {
		SND_LOG_ERR(HLOG, "clk spdif get failed\n");
		ret = PTR_ERR(clk->clk_spdif);
		goto err_get_clk_spdif;
	}

	/* set clk spdif parent of pllaudio */
	if (clk_set_parent(clk->clk_spdif, clk->clk_pll)) {
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

	clk->pll_fs_tx = 1;

	return 0;

err_clk_enable:
err_set_parent:
	clk_put(clk->clk_spdif);
err_get_clk_spdif:
	clk_put(clk->clk_pll);
err_get_clk_pll:
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
	clk_put(clk->clk_pll);
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

	if (clk_prepare_enable(clk->clk_pll)) {
		SND_LOG_ERR(HLOG, "clk pll enable failed\n");
		goto err_enable_clk_pll;
	}

	if (clk_prepare_enable(clk->clk_spdif)) {
		SND_LOG_ERR(HLOG, "clk_spdif enable failed\n");
		goto err_enable_clk_spdif;
	}

	return 0;

err_enable_clk_spdif:
	clk_disable_unprepare(clk->clk_pll);
err_enable_clk_pll:
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
	clk_disable_unprepare(clk->clk_pll);
	clk_disable_unprepare(clk->clk_bus);
	reset_control_assert(clk->clk_rst);
}

static inline int snd_sunxi_clk_rate(struct sunxi_spdif_clk *clk, unsigned int freq_in,
				     unsigned int freq_out)
{
	SND_LOG_DEBUG(HLOG, "\n");

	if (clk_set_rate(clk->clk_pll, freq_out)) {
		SND_LOG_ERR(HLOG, "set clk_i2s rate failed, rate: %u\n", freq_out);
		return -EINVAL;
	}

	return 0;
}

#endif /* __SND_SUN8IW11_SPDIF_H */
