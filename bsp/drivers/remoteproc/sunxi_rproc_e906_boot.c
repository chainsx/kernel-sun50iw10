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
 * E906 CFG Register
 */
#define E906_VER_REG			(0x0000) /* E906 Version Register */
#define E906_RF1P_CFG_REG		(0x0010) /* E906 Control Register0 */
#define E906_TS_TMODE_SEL_REG		(0x0040) /* E906 TEST MODE SELETE Register */
#define E906_STA_ADD_REG		(0x0204) /* E906 STAT Register */
#define E906_WAKEUP_EN_REG		(0x0220) /* E906 WakeUp Enable Register */
#define E906_WAKEUP_MASK0_REG		(0x0224) /* E906 WakeUp Mask0 Register */
#define E906_WAKEUP_MASK1_REG		(0x0228) /* E906 WakeUp Mask1 Register */
#define E906_WAKEUP_MASK2_REG		(0x022C) /* E906 WakeUp Mask2 Register */
#define E906_WAKEUP_MASK3_REG		(0x0230) /* E906 WakeUp Mask3 Register */
#define E906_WAKEUP_MASK4_REG		(0x0234) /* E906 WakeUp Mask4 Register */
#define E906_WORK_MODE_REG		(0x0248) /* E906 Worke Mode Register */

/*
 * E906 Version Register
 */
#define SMALL_VER_MASK			(0x1f << 0)
#define LARGE_VER_MASK			(0x1f << 16)

/*
 * E906 PRID Register
 */
#define BIT_TEST_MODE			(1 << 1)

/*
 * E906 WakeUp Enable Register
 */
#define BIT_WAKEUP_EN			(1 << 0)

/*
 * E906 Worke Mode Register
 */
#define BIT_LOCK_STA			(1 << 3)
#define BIT_DEBUG_MODE			(1 << 2)
#define BIT_LOW_POWER_MASK		(0x3)
#define BIT_DEEP_SLEEP_MODE		(0x0)
#define BIT_LIGHT_SLEEP_MODE		(0x1)

int simulator_debug;
module_param(simulator_debug, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(simulator_debug, "Debug for simulator");

static int sunxi_rproc_e906_assert(struct sunxi_rproc_priv *rproc_priv);
static int sunxi_rproc_e906_deassert(struct sunxi_rproc_priv *rproc_priv);

static int devm_sunxi_rproc_e906_resource_get(struct sunxi_rproc_priv *rproc_priv, struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct resource *res;
	u32 *map_array;
	int ret, i;

	rproc_priv->dev = dev;

	rproc_priv->e906_cfg = devm_kzalloc(dev, sizeof(struct sunxi_rproc_e906_cfg), GFP_KERNEL);
	if (!rproc_priv->e906_cfg) {
		dev_err(dev, "alloc e906 cfg error\n");
		return -ENOMEM;
	}

	rproc_priv->e906_cfg->pubsram_clk = devm_clk_get(dev, "pubsram");
	if (IS_ERR_OR_NULL(rproc_priv->e906_cfg->pubsram_clk)) {
		dev_err(dev, "no find pubsram in dts\n");
		return -ENXIO;
	}

	rproc_priv->e906_cfg->mod_clk = devm_clk_get(dev, "mod");
	if (IS_ERR_OR_NULL(rproc_priv->e906_cfg->mod_clk)) {
		dev_err(dev, "no find mod in dts\n");
		return -ENXIO;
	}

	rproc_priv->e906_cfg->cfg_clk = devm_clk_get(dev, "cfg");
	if (IS_ERR_OR_NULL(rproc_priv->e906_cfg->cfg_clk)) {
		dev_err(dev, "no find cfg in dts\n");
		return -ENXIO;
	}

	rproc_priv->e906_cfg->pubsram_rst = devm_reset_control_get(dev, "pubsram-rst");
	if (IS_ERR_OR_NULL(rproc_priv->e906_cfg->pubsram_rst)) {
		dev_err(dev, "no find pubsram-rst in dts\n");
		return -ENXIO;
	}

	rproc_priv->e906_cfg->mod_rst = devm_reset_control_get(dev, "mod-rst");
	if (IS_ERR_OR_NULL(rproc_priv->e906_cfg->mod_rst)) {
		dev_err(dev, "no find mod-rst in dts\n");
		return -ENXIO;
	}

	rproc_priv->e906_cfg->cfg_rst = devm_reset_control_get(dev, "cfg-rst");
	if (IS_ERR_OR_NULL(rproc_priv->e906_cfg->cfg_rst)) {
		dev_err(dev, "no find cfg-rst in dts\n");
		return -ENXIO;
	}

	rproc_priv->e906_cfg->dbg_rst = devm_reset_control_get(dev, "dbg-rst");
	if (IS_ERR_OR_NULL(rproc_priv->e906_cfg->dbg_rst)) {
		dev_err(dev, "no find dbg-rst in dts\n");
		return -ENXIO;
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "e906-cfg");
	if (IS_ERR_OR_NULL(res)) {
		dev_err(dev, "no find e906-cfg in dts\n");
		return -ENXIO;
	}

	rproc_priv->e906_cfg->e906_cfg = devm_ioremap_resource(dev, res);
	if (IS_ERR_OR_NULL(rproc_priv->e906_cfg->e906_cfg)) {
		dev_err(dev, "fail to ioremap e906-cfg\n");
		return -ENXIO;
	}

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

static int sunxi_rproc_e906_start(struct sunxi_rproc_priv *rproc_priv)
{
	struct device *dev = rproc_priv->dev;
	int ret;

	dev_dbg(dev, "%s,%d\n", __func__, __LINE__);

	if (simulator_debug) {
		dev_dbg(dev, "%s,%d e906 does not need to reset clk\n",
				__func__, __LINE__);
		return 0;
	}

	ret = sunxi_rproc_e906_assert(rproc_priv);
	if (ret) {
		dev_err(dev, "rproc assert err\n");
		return ret;
	}

	/* set vector */
	writel(rproc_priv->pc_entry, (rproc_priv->e906_cfg->e906_cfg + E906_STA_ADD_REG));

	ret = sunxi_rproc_e906_deassert(rproc_priv);
	if (ret) {
		dev_err(dev, "rproc deassert err\n");
		return ret;
	}

	ret = clk_prepare_enable(rproc_priv->e906_cfg->cfg_clk);
	if (ret) {
		dev_err(dev, "cfg clk enable err\n");
		return ret;
	}

	ret = clk_prepare_enable(rproc_priv->e906_cfg->mod_clk);
	if (ret) {
		dev_err(dev, "mod clk enable err\n");
		return ret;
	}

	return 0;
}

static int sunxi_rproc_e906_stop(struct sunxi_rproc_priv *rproc_priv)
{
	int ret;

	dev_dbg(rproc_priv->dev, "%s,%d\n", __func__, __LINE__);

	if (simulator_debug) {
		dev_dbg(rproc_priv->dev, "%s,%d e906 does not need to close clk\n",
				__func__, __LINE__);
		return 0;
	}

	clk_disable(rproc_priv->e906_cfg->cfg_clk);

	clk_disable(rproc_priv->e906_cfg->mod_clk);

	ret = sunxi_rproc_e906_assert(rproc_priv);
	if (ret) {
		dev_err(rproc_priv->dev, "rproc assert err\n");
		return ret;
	}

	return 0;
}

static int sunxi_rproc_e906_assert(struct sunxi_rproc_priv *rproc_priv)
{
	int ret;

	ret = reset_control_assert(rproc_priv->e906_cfg->mod_rst);
	if (ret) {
		dev_err(rproc_priv->dev, "mod rst assert err\n");
		return -ENXIO;
	}

	ret = reset_control_assert(rproc_priv->e906_cfg->cfg_rst);
	if (ret) {
		dev_err(rproc_priv->dev, "cfg rst assert err\n");
		return -ENXIO;
	}

	ret = reset_control_assert(rproc_priv->e906_cfg->dbg_rst);
	if (ret) {
		dev_err(rproc_priv->dev, "dbg rst assert err\n");
		return -ENXIO;
	}

	return ret;
}

static int sunxi_rproc_e906_deassert(struct sunxi_rproc_priv *rproc_priv)
{
	int ret;

	ret = reset_control_deassert(rproc_priv->e906_cfg->mod_rst);
	if (ret) {
		dev_err(rproc_priv->dev, "mod rst de-assert err\n");
		return -ENXIO;
	}

	ret = reset_control_deassert(rproc_priv->e906_cfg->cfg_rst);
	if (ret) {
		dev_err(rproc_priv->dev, "cfg rst de-assert err\n");
		return -ENXIO;
	}

	ret = reset_control_deassert(rproc_priv->e906_cfg->dbg_rst);
	if (ret) {
		dev_err(rproc_priv->dev, "dbg rst de-assert err\n");
		return -ENXIO;
	}

	return ret;
}

static int sunxi_rproc_e906_reset(struct sunxi_rproc_priv *rproc_priv)
{
	int ret;

	ret = sunxi_rproc_e906_assert(rproc_priv);
	if (ret)
		return -ENXIO;

	ret = sunxi_rproc_e906_deassert(rproc_priv);
	if (ret)
		return -ENXIO;

	return ret;
}

static int sunxi_rproc_e906_enable_sram(struct sunxi_rproc_priv *rproc_priv, u32 value)
{
	int ret;

	if (value) {
		ret = reset_control_deassert(rproc_priv->e906_cfg->pubsram_rst);
		if (ret) {
			dev_err(rproc_priv->dev, "pubsram reset err\n");
			return ret;
		}

		/* must enable sram clk, so that arm can memcpy elf to sram */
		ret = clk_prepare_enable(rproc_priv->e906_cfg->pubsram_clk);
		if (ret) {
			dev_err(rproc_priv->dev, "pubsram clk enable err\n");
			return ret;
		}
	}

	return 0;
}

static int sunxi_rproc_e906_set_runstall(struct sunxi_rproc_priv *rproc_priv, u32 value)
{
	/* e906 do not have runstall reg bit */
	return 0;
}

static struct sunxi_rproc_ops sunxi_rproc_e906_ops = {
	.resource_get = devm_sunxi_rproc_e906_resource_get,
	.start = sunxi_rproc_e906_start,
	.stop = sunxi_rproc_e906_stop,
	.reset = sunxi_rproc_e906_reset,
	.set_localram = sunxi_rproc_e906_enable_sram,
	.set_runstall = sunxi_rproc_e906_set_runstall,
};

/* e906_boot_init must run before sunxi_rproc probe */
static int __init sunxi_rproc_e906_boot_init(void)
{
	int ret;

	ret = sunxi_rproc_priv_ops_register("e906", &sunxi_rproc_e906_ops, NULL);
	if (ret) {
		pr_err("e906 register ops failed\n");
		return ret;
	}

	return 0;
}
subsys_initcall(sunxi_rproc_e906_boot_init);

static void __exit sunxi_rproc_e906_boot_exit(void)
{
	int ret;

	ret = sunxi_rproc_priv_ops_unregister("e906");
	if (ret)
		pr_err("e906 unregister ops failed\n");
}
module_exit(sunxi_rproc_e906_boot_exit)

MODULE_DESCRIPTION("Allwinner sunxi rproc e906 boot driver");
MODULE_AUTHOR("xuminghui <xuminghui@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.0");
