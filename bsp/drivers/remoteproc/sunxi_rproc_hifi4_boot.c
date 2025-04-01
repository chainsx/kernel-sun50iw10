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

/* #define DEBUG */
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/reset.h>
#include <asm/io.h>
#include "sunxi_rproc_boot.h"

/*
 * Register define
 */
#define HIFI4_ALT_RESET_VEC_REG		(0x0000) /* HIFI4 Reset Control Register */
#define HIFI4_CTRL_REG0			(0x0004) /* HIFI4 Control Register0 */
#define HIFI4_PRID_REG			(0x000c) /* HIFI4 PRID Register */
#define HIFI4_STAT_REG			(0x0010) /* HIFI4 STAT Register */
#define HIFI4_BIST_CTRL_REG		(0x0014) /* HIFI4 BIST CTRL Register */
#define HIFI4_JTRST_REG			(0x001c) /* HIFI4 JTAG CONFIG RESET Register */
#define HIFI4_VER_REG			(0x0020) /* HIFI4 Version Register */

/*
 * HIFI4 Control Register0
 */
#define BIT_RUN_STALL			(0)
#define BIT_START_VEC_SEL		(1)
#define BIT_HIFI4_CLKEN			(2)

#define HIFI4_BOOT_SRAM_REMAP_REG	(0x8)
#define BIT_SRAM_REMAP_ENABLE		(0)

int simulator_debug;
module_param(simulator_debug, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(simulator_debug, "Debug for simulator");

static int sunxi_rproc_hifi4_reset(struct sunxi_rproc_priv *rproc_priv);
static int sunxi_rproc_hifi4_set_runstall(struct sunxi_rproc_priv *rproc_priv, u32 value);
static int sunxi_rproc_hifi4_set_localram(struct sunxi_rproc_priv *rproc_priv, u32 value);

static int devm_sunxi_rproc_hifi4_resource_get(struct sunxi_rproc_priv *rproc_priv, struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct resource *res;
	u32 *map_array;
	int ret, i;

	rproc_priv->dev = dev;

	rproc_priv->hifi4_cfg = devm_kzalloc(dev, sizeof(struct sunxi_rproc_hifi4_cfg), GFP_KERNEL);
	if (!rproc_priv->hifi4_cfg) {
		dev_err(dev, "alloc hifi4 cfg error\n");
		return -ENOMEM;
	}

	rproc_priv->hifi4_cfg->pll_clk = devm_clk_get(dev, "pll");
	if (IS_ERR_OR_NULL(rproc_priv->hifi4_cfg->pll_clk)) {
		dev_err(dev, "no find pll in dts\n");
		return -ENXIO;
	}

	rproc_priv->hifi4_cfg->mod_clk = devm_clk_get(dev, "mod");
	if (IS_ERR_OR_NULL(rproc_priv->hifi4_cfg->mod_clk)) {
		dev_err(dev, "no find mod in dts\n");
		return -ENXIO;
	}

	rproc_priv->hifi4_cfg->cfg_clk = devm_clk_get(dev, "cfg");
	if (IS_ERR_OR_NULL(rproc_priv->hifi4_cfg->cfg_clk)) {
		dev_err(dev, "no find bus in dts\n");
		return -ENXIO;
	}

	rproc_priv->hifi4_cfg->ahbs_clk = devm_clk_get(dev, "ahbs");
	if (IS_ERR_OR_NULL(rproc_priv->hifi4_cfg->ahbs_clk)) {
		dev_err(dev, "no find ahbs in dts\n");
		return -ENXIO;
	}

	/* hifi4 module rst clk */
	rproc_priv->hifi4_cfg->mod_rst = devm_reset_control_get(dev, "mod-rst");
	if (IS_ERR_OR_NULL(rproc_priv->hifi4_cfg->mod_rst)) {
		dev_err(dev, "no find mod_rst in dts\n");
		return -ENXIO;
	}

	/* hifi4 cfg rst clk */
	rproc_priv->hifi4_cfg->cfg_rst = devm_reset_control_get(dev, "cfg-rst");
	if (IS_ERR_OR_NULL(rproc_priv->hifi4_cfg->cfg_rst)) {
		dev_err(dev, "no find cfg in dts\n");
		return -ENXIO;
	}

	/* hifi4 dbg rst clk */
	rproc_priv->hifi4_cfg->dbg_rst = devm_reset_control_get(dev, "dbg-rst");
	if (IS_ERR_OR_NULL(rproc_priv->hifi4_cfg->dbg_rst)) {
		dev_err(dev, "no find dbg in dts\n");
		return -ENXIO;
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "sram-for-cpux");
	if (IS_ERR_OR_NULL(res)) {
		dev_err(dev, "no find sram-for-cpux in dts\n");
		return -ENXIO;
	}

	rproc_priv->hifi4_cfg->sram_remap = devm_ioremap_resource(dev, res);
	if (IS_ERR_OR_NULL(rproc_priv->hifi4_cfg->sram_remap)) {
		dev_err(dev, "fail to ioremap sram-remap\n");
		return -ENXIO;
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "hifi4-cfg");
	if (IS_ERR_OR_NULL(res)) {
		dev_err(dev, "no find hifi4-cfg in dts\n");
		return -ENXIO;
	}

	rproc_priv->hifi4_cfg->hifi4_cfg = devm_ioremap_resource(dev, res);
	if (IS_ERR_OR_NULL(rproc_priv->hifi4_cfg->hifi4_cfg)) {
		dev_err(dev, "fail to ioremap hifi4-cfg\n");
		return -ENXIO;
	}

	ret = of_property_read_u32(np, "clock-frequency", &rproc_priv->hifi4_cfg->mod_clk_freq);
	if (ret) {
		dev_err(dev, "fail to get clock-frequency\n");
		return -ENXIO;
	}

	dev_dbg(dev, "%s,%d rproc_priv->hifi4_cfg->mod_clk_freq = %d\n", __func__, __LINE__, rproc_priv->hifi4_cfg->mod_clk_freq);

	ret = of_property_count_elems_of_size(np, "memory-mappings", sizeof(u32) * 3);
	if (ret <= 0) {
		dev_err(dev, "fail to get memory-mappings\n");
		return -ENXIO;
	}
	rproc_priv->mem_maps_cnt = ret;
	rproc_priv->mem_maps = devm_kcalloc(dev, rproc_priv->mem_maps_cnt,
				       sizeof(struct sunxi_rproc_memory_mapping),
				       GFP_KERNEL);
	if (!rproc_priv->mem_maps)
		return -ENOMEM;

	map_array = devm_kcalloc(dev, rproc_priv->mem_maps_cnt * 3, sizeof(u32), GFP_KERNEL);
	if (!map_array)
		return -ENOMEM;

	ret = of_property_read_u32_array(np, "memory-mappings", map_array,
					 rproc_priv->mem_maps_cnt * 3);
	if (ret) {
		dev_err(dev, "fail to read memory-mappings\n");
		return -ENXIO;
	}

	for (i = 0; i < rproc_priv->mem_maps_cnt; i++) {
		rproc_priv->mem_maps[i].da = map_array[i * 3];
		rproc_priv->mem_maps[i].len = map_array[i * 3 + 1];
		rproc_priv->mem_maps[i].pa = map_array[i * 3 + 2];
		dev_dbg(dev, "memory-mappings[%d]: da: 0x%llx, len: 0x%llx, pa: 0x%llx\n",
			i, rproc_priv->mem_maps[i].da, rproc_priv->mem_maps[i].len,
			rproc_priv->mem_maps[i].pa);
	}

	devm_kfree(dev, map_array);
	return 0;
}

static int sunxi_rproc_hifi4_start(struct sunxi_rproc_priv *rproc_priv)
{
	struct device *dev = rproc_priv->dev;
	int ret, rate;
	u32 reg_val;

	dev_dbg(dev, "%s,%d\n", __func__, __LINE__);

	if (simulator_debug) {
		dev_dbg(dev, "%s,%d hifi4 does not need to reset clk\n",
				__func__, __LINE__);
		return 0;
	}

	/* reset hifi4 */
	ret = sunxi_rproc_hifi4_reset(rproc_priv);
	if (ret) {
		dev_err(dev, "rproc reset err\n");
		return ret;
	}

	/* set pll_clk */
	ret = clk_prepare_enable(rproc_priv->hifi4_cfg->pll_clk);
	if (ret) {
		dev_err(dev, "pll clk enable err\n");
		return ret;
	}

	ret = clk_set_parent(rproc_priv->hifi4_cfg->mod_clk, rproc_priv->hifi4_cfg->pll_clk);
	if (ret) {
		dev_err(dev, "set mod clk parent to pll_clk err\n");
		return ret;
	}

	/* set mod_clk_freq */
	rate = clk_round_rate(rproc_priv->hifi4_cfg->mod_clk, rproc_priv->hifi4_cfg->mod_clk_freq);
	ret = clk_set_rate(rproc_priv->hifi4_cfg->mod_clk, rate);
	if (ret) {
		dev_err(dev, "set mod clk freq err\n");
		return ret;
	}

	/* set mod_clk */
	ret = clk_prepare_enable(rproc_priv->hifi4_cfg->mod_clk);
	if (ret) {
		dev_err(dev, "mod clk enable err\n");
		return ret;
	}

	/* set cfg_clk */
	ret = clk_prepare_enable(rproc_priv->hifi4_cfg->cfg_clk);
	if (ret) {
		dev_err(dev, "bus clk enable err\n");
		return ret;
	}

	/* set ahbs_clk */
	ret = clk_prepare_enable(rproc_priv->hifi4_cfg->ahbs_clk);
	if (ret) {
		dev_err(dev, "ahbs clk enable err\n");
		return ret;
	}

	/* set cfg to deassert  */
	ret = reset_control_deassert(rproc_priv->hifi4_cfg->cfg_rst);
	if (ret) {
		dev_err(dev, "set cfg to deassert err\n");
		return ret;
	}

	/* set dbg to deassert  */
	ret = reset_control_deassert(rproc_priv->hifi4_cfg->dbg_rst);
	if (ret) {
		dev_err(dev, "set dbg to deassert err\n");
		return ret;
	}

	/* set vector */
	writel(rproc_priv->pc_entry, (rproc_priv->hifi4_cfg->hifi4_cfg + HIFI4_ALT_RESET_VEC_REG));

	/* set statVactorSel */
	reg_val = readl(rproc_priv->hifi4_cfg->hifi4_cfg + HIFI4_CTRL_REG0);
	reg_val |= 1 << BIT_START_VEC_SEL;
	writel(reg_val, (rproc_priv->hifi4_cfg->hifi4_cfg + HIFI4_CTRL_REG0));

	/* set runstall */
	sunxi_rproc_hifi4_set_runstall(rproc_priv, 1);

	/* set hifi4 clken */
	reg_val = readl(rproc_priv->hifi4_cfg->hifi4_cfg + HIFI4_CTRL_REG0);
	reg_val |= 1 << BIT_HIFI4_CLKEN;
	writel(reg_val, (rproc_priv->hifi4_cfg->hifi4_cfg + HIFI4_CTRL_REG0));


	/* set rst to deassert  */
	ret = reset_control_deassert(rproc_priv->hifi4_cfg->mod_rst);
	if (ret) {
		dev_err(dev, "set mod_rst to deassert err\n");
		return ret;
	}

	/* hifi4 can use local ram */
	sunxi_rproc_hifi4_set_localram(rproc_priv, 0);

	/* hifi4 run */
	sunxi_rproc_hifi4_set_runstall(rproc_priv, 0);

	return 0;
}

static int sunxi_rproc_hifi4_stop(struct sunxi_rproc_priv *rproc_priv)
{
	int ret;

	dev_dbg(rproc_priv->dev, "%s,%d\n", __func__, __LINE__);

	if (simulator_debug) {
		dev_dbg(rproc_priv->dev, "%s,%d hifi4 does not need to close clk\n",
				__func__, __LINE__);
		return 0;
	}

	clk_disable(rproc_priv->hifi4_cfg->cfg_clk);

	clk_disable(rproc_priv->hifi4_cfg->mod_clk);

	ret = sunxi_rproc_hifi4_reset(rproc_priv);
	if (ret) {
		dev_err(rproc_priv->dev, "rproc reset err\n");
		return ret;
	}

	return 0;
}

static int sunxi_rproc_hifi4_reset(struct sunxi_rproc_priv *rproc_priv)
{
	int ret;

	ret = reset_control_assert(rproc_priv->hifi4_cfg->mod_rst);
	if (ret)
		return -ENXIO;

	ret = reset_control_assert(rproc_priv->hifi4_cfg->cfg_rst);
	if (ret)
		return -ENXIO;

	ret = reset_control_assert(rproc_priv->hifi4_cfg->dbg_rst);
	if (ret)
		return -ENXIO;

	return 0;
}

static int sunxi_rproc_hifi4_set_localram(struct sunxi_rproc_priv *rproc_priv, u32 value)
{
	u32 reg_val;

	reg_val = readl(rproc_priv->hifi4_cfg->sram_remap);
	reg_val &= ~(1 << BIT_SRAM_REMAP_ENABLE);
	reg_val |= (value << BIT_SRAM_REMAP_ENABLE);
	writel(reg_val, (rproc_priv->hifi4_cfg->sram_remap));

	return 0;
}

static int sunxi_rproc_hifi4_set_runstall(struct sunxi_rproc_priv *rproc_priv, u32 value)
{
	u32 reg_val;

	reg_val = readl(rproc_priv->hifi4_cfg->hifi4_cfg + HIFI4_CTRL_REG0);
	reg_val &= ~(1 << BIT_RUN_STALL);
	reg_val |= (value << BIT_RUN_STALL);
	writel(reg_val, (rproc_priv->hifi4_cfg->hifi4_cfg + HIFI4_CTRL_REG0));

	return 0;
}

static struct sunxi_rproc_ops sunxi_rproc_hifi4_ops = {
	.resource_get = devm_sunxi_rproc_hifi4_resource_get,
	.start = sunxi_rproc_hifi4_start,
	.stop = sunxi_rproc_hifi4_stop,
	.reset = sunxi_rproc_hifi4_reset,
	.set_localram = sunxi_rproc_hifi4_set_localram,
	.set_runstall = sunxi_rproc_hifi4_set_runstall,
};

/* hifi4_boot_init must run before sunxi_rproc probe */
static int __init sunxi_rproc_hifi4_boot_init(void)
{
	int ret;

	ret = sunxi_rproc_priv_ops_register("hifi4", &sunxi_rproc_hifi4_ops, NULL);
	if (ret) {
		pr_err("hifi4 register ops failed\n");
		return ret;
	}

	return 0;
}
subsys_initcall(sunxi_rproc_hifi4_boot_init);

static void __exit sunxi_rproc_hifi4_boot_exit(void)
{
	int ret;

	ret = sunxi_rproc_priv_ops_unregister("hifi4");
	if (ret)
		pr_err("hifi4 unregister ops failed\n");
}
module_exit(sunxi_rproc_hifi4_boot_exit)

MODULE_DESCRIPTION("Allwinner sunxi rproc hifi4 boot driver");
MODULE_AUTHOR("xuminghui <xuminghui@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.0");
