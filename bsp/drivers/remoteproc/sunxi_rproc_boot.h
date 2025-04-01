/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright © 2020-2025, Allwinnertech
 *
 * This file is provided under a dual BSD/GPL license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef SUNXI_RPROC_BOOT_H
#define SUNXI_RPROC_BOOT_H

#include <linux/clk.h>
#include <linux/reset.h>

struct sunxi_rproc_priv;

struct sunxi_rproc_ops {
	int (*resource_get)(struct sunxi_rproc_priv *rproc_priv, struct platform_device *pdev);
	int (*start)(struct sunxi_rproc_priv *rproc_priv);
	int (*stop)(struct sunxi_rproc_priv *rproc_priv);
	int (*reset)(struct sunxi_rproc_priv *rproc_priv);
	/* set remote processors local ram access to arm */
	int (*set_localram)(struct sunxi_rproc_priv *rproc_priv, u32 value);
	/* set remote processors start reg bit */
	int (*set_runstall)(struct sunxi_rproc_priv *rproc_priv, u32 value);
};

struct sunxi_rproc_memory_mapping {
	u64 pa;				/* Address seen on the cpu side */
	u64 da;				/* Address seen on the remote processor side */
	u64 len;
};

struct sunxi_rproc_hifi4_cfg {
	struct clk *pll_clk;		/* pll clock */
	struct clk *mod_clk;		/* mod clock */
	struct clk *cfg_clk;		/* cfg clock */
	struct clk *ahbs_clk;		/* ahbs clock */
	struct reset_control *mod_rst;	/* mod rst control */
	struct reset_control *cfg_rst;	/* cfg rst control */
	struct reset_control *dbg_rst;	/* dbg rst control */
	u32 mod_clk_freq;		/* DSP freq */
	void __iomem *sram_remap;
	void __iomem *hifi4_cfg;
};

struct sunxi_rproc_hifi5_cfg {
	/* TODO */
};

struct sunxi_rproc_e906_cfg {
	struct clk *pubsram_clk;	/* pubsram clock */
	struct clk *mod_clk;		/* mod clock */
	struct clk *cfg_clk;		/* cfg clock */
	struct reset_control *pubsram_rst;  /* pubsram rst control  */
	struct reset_control *mod_rst;	/* mod rst control */
	struct reset_control *cfg_rst;  /* cfg rst control */
	struct reset_control *dbg_rst;	/* dbg rst control */
	void __iomem  *e906_cfg;
};

struct sunxi_rproc_e907_cfg {
	/* TODO */
};

struct sunxi_rproc_priv {
	const char *name;
	struct sunxi_rproc_memory_mapping *mem_maps;
	struct sunxi_rproc_ops *ops;
	struct sunxi_rproc_hifi4_cfg *hifi4_cfg;
	struct sunxi_rproc_hifi5_cfg *hifi5_cfg;
	struct sunxi_rproc_e906_cfg *e906_cfg;
	struct sunxi_rproc_e907_cfg *e907_cfg;
	struct device *dev;
	int mem_maps_cnt;
	int irq;
	u32 pc_entry;
	void *priv;
};

struct sunxi_rproc_priv *sunxi_rproc_priv_find(const char *name);
int sunxi_rproc_priv_ops_register(const char *name, struct sunxi_rproc_ops *rproc_ops, void *priv);
int sunxi_rproc_priv_ops_unregister(const char *name);
int devm_sunxi_rproc_priv_resource_get(struct sunxi_rproc_priv *rproc_priv, struct platform_device *pdev);
int sunxi_rproc_priv_start(struct sunxi_rproc_priv *rproc_priv);
int sunxi_rproc_priv_stop(struct sunxi_rproc_priv *rproc_priv);
int sunxi_rproc_priv_assert(struct sunxi_rproc_priv *rproc_priv);
int sunxi_rproc_priv_set_localram(struct sunxi_rproc_priv *rproc_priv, u32 value);
int sunxi_rproc_priv_set_runstall(struct sunxi_rproc_priv *rproc_priv, u32 value);

#endif
