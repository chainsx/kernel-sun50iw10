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


#include <linux/clk.h>
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
#include <linux/stat.h>

#include <linux/mmc/core.h>
#include <linux/mmc/host.h>
#include "sunxi-mmc.h"
#include "sunxi-mmc-debug.h"
#include "sunxi-mmc-export.h"
#include "sunxi-mmc-sun50iw1p1-2.h"
#include "sunxi-mmc-panic.h"


#define GPIO_BASE_ADDR	0x1c20800
/* nc platform no use these value */
#define CCMU_BASE_ADDR_BEFORE_V2P1H	0x1c20000

#define SUNXI_MMC_MAX_HOST_PRT_ADDR  0x150
#define SUNXI_MMC_MAX_GPIO_PRT_ADDR	0x120
#define SUNXI_GPIOIC_PRT_EADDR	0x380
#define SUNXI_GPIOIC_PRT_SADDR	0x200

/* mmc bus clock gating register */
#define SUNXI_BCLKG_SADDR  0x60
#define SUNXI_BCLKG_EADDR	0x80

/* mmc moudule clock register */
#define SUNXI_CLK_PRT_SADDR  0x80
#define SUNXI_CLK_PRT_EADDR	0xa0

/* mmc bus soft reset register */
#define SUNXI_BSRES_SADDR  0x2C0
#define SUNXI_BSRES_EADDR	0x2DC


/* NC mmc bus gating,reset,moudule clouck register */
#define SUNXI_NCCM_EADDR	0x850
#define SUNXI_NCCM_SADDR	0x830

/* NC mmc PLL PERI register */
#define SUNXI_PP_NCM_EADDR	0x2C
#define SUNXI_PP_NCM_SADDR	0x20

#define SUNXI_DEG_MAX_MAP_REG	0x900

static struct device_attribute dump_register[3];

void sunxi_mmc_dumphex32(struct sunxi_mmc_host *host, char *name, char *base,
			 int len)
{
	u32 i;

	pr_cont("dump %s registers:", name);
	for (i = 0; i < len; i += 4) {
		if (!(i&0xf))
			pr_cont("\n0x%px : ", base + i);
		pr_cont("0x%08x ", __raw_readl(base+i));
	}
	pr_cont("\n");
}

void sunxi_mmc_dump_des(struct sunxi_mmc_host *host, char *base, int len)
{
	u32 i;

	pr_cont("dump des mem\n");
	for (i = 0; i < len; i += 4) {
		if (!(i&0xf))
			pr_cont("\n0x%px : ", base + i);
		pr_cont("0x%08x ", *(u32 *)(base+i));
	}
	pr_cont("\n");
}

static unsigned int sunxi_mmc_get_rate(uint64_t bytes, uint64_t time_us)
{
	uint64_t ns;

	ns = time_us * 1000;
	bytes *= 1000000000;

	while (ns > UINT_MAX) {
		bytes >>= 1;
		ns >>= 1;
	}

	if (!ns)
		return 0;

	do_div(bytes, (uint32_t)ns);

	return bytes;
}

static void sunxi_mmc_filter_rate(struct sunxi_mmc_host *host, struct mmc_data *data, int64_t bytes, uint64_t time_us)
{
	unsigned int rate = 0;

	if (!(host->filter_sector)
		|| !(host->filter_speed))
		return;

	if ((data->blocks) >= (host->filter_sector)) {
		rate = sunxi_mmc_get_rate(bytes, time_us);
		if (rate < (host->filter_speed))
			printk("c=%u,a=0x%8x,""bs=%5u,""t=%9lluus,""sp=%7uKB/s\n",
			data->mrq->cmd->opcode, data->mrq->cmd->arg,
			data->blocks, time_us, rate/1024);
	}
}


static ssize_t maual_insert_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	int ret;

	ret =
	    snprintf(buf, PAGE_SIZE,
		     "Usage: \"echo 1 > insert\" to scan card\n");
	return ret;
}

static ssize_t maual_insert_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	int ret;

	unsigned long insert = 0;
	struct platform_device *pdev = to_platform_device(dev);
	struct mmc_host *mmc = platform_get_drvdata(pdev);

	ret = kstrtoul(buf, 0, &insert);
	if (ret) {
		ret = -EINVAL;
		return ret;
	}

	SM_INFO(dev, "insert %ld\n", insert);
	if (insert)
		mmc_detect_change(mmc, 0);
	else
		SM_INFO(dev, "no detect change\n");

	ret = count;
	return ret;
}

int sunxi_mmc_res_start_addr(const char * const res_str,
		resource_size_t *res_addr)
{
	struct device_node *np = NULL;
	int ret = 0;
	struct resource res;

	if (res_str == NULL || res_addr == NULL) {
		pr_err("input arg is error\n");
		return -EINVAL;
	}

	np = of_find_node_by_type(NULL, res_str);
	if (IS_ERR(np)) {
		pr_err("Can not find device type\n");
		return -EINVAL;
	}

	ret = of_address_to_resource(np, 0, &res);
	if (ret || !res.start) {
		pr_err("Can not find resouce\n");
		return -EINVAL;
	}
	*res_addr = res.start;

	return 0;
}

void sunxi_dump_reg(struct mmc_host *mmc)
{
	int i = 0;
	struct sunxi_mmc_host *host = mmc_priv(mmc);
	int ret = 0;
	void __iomem *gpio_ptr =  NULL;
	void __iomem *ccmu_ptr =  NULL;
	resource_size_t res_saddr_ccmu;
	resource_size_t res_saddr_gpio;


	pr_cont("Dump %s (p%x) regs :\n", mmc_hostname(mmc), host->phy_index);
	for (i = 0; i < SUNXI_MMC_MAX_HOST_PRT_ADDR; i += 4) {
		if (!(i&0xf))
			pr_cont("\n0x%px : ", (host->reg_base + i));
		pr_cont("%08x ", readl(host->reg_base + i));
	}
	pr_cont("\n");

	ret = sunxi_mmc_res_start_addr("pio", &res_saddr_gpio);
	if (ret < 0)
		goto map_ccmu;
	gpio_ptr = ioremap(res_saddr_gpio, SUNXI_DEG_MAX_MAP_REG);
	if (gpio_ptr == NULL) {
		pr_err("Can not map gpio resource\n");
		goto map_ccmu;
	}

	pr_cont("Dump gpio regs:\n");
	for (i = 0; i < SUNXI_MMC_MAX_GPIO_PRT_ADDR; i += 4) {
		if (!(i&0xf))
			pr_cont("\n0x%px : ", (gpio_ptr + i));
		pr_cont("%08x ", readl(gpio_ptr + i));
	}
	pr_cont("\n");

	pr_cont("Dump gpio irqc regs:\n");
	for (i = SUNXI_GPIOIC_PRT_SADDR; i < SUNXI_GPIOIC_PRT_EADDR; i += 4) {
		if (!(i&0xf))
			pr_cont("\n0x%px : ", (gpio_ptr + i));
		pr_cont("%08x ", readl(gpio_ptr + i));
	}
	pr_cont("\n");

	iounmap(gpio_ptr);

map_ccmu:
	ret = sunxi_mmc_res_start_addr("clocks", &res_saddr_ccmu);
	if (ret < 0)
		return;
	ccmu_ptr = ioremap(res_saddr_ccmu, SUNXI_DEG_MAX_MAP_REG);
	if (ccmu_ptr == NULL) {
		pr_err("Can not map ccmu resource\n");
		return;
	}

	if (res_saddr_ccmu == CCMU_BASE_ADDR_BEFORE_V2P1H) {
		pr_cont("Dump ccmu regs:gating\n");
		for (i = SUNXI_BCLKG_SADDR; i < SUNXI_BCLKG_EADDR; i += 4) {
			if (!(i&0xf))
				pr_cont("\n0x%px : ", (ccmu_ptr + i));
			pr_cont("%08x ", readl(ccmu_ptr + i));
		}
		pr_cont("\n");

		pr_cont("Dump ccmu regs:module clk\n");
		for (i = SUNXI_CLK_PRT_SADDR; i < SUNXI_CLK_PRT_EADDR; i += 4) {
			if (!(i&0xf))
				pr_cont("\n0x%px : ", (ccmu_ptr + i));
			pr_cont("%08x ", readl(ccmu_ptr + i));
		}
		pr_cont("\n");

		pr_cont("Dump ccmu regs:reset\n");
		for (i = SUNXI_BSRES_SADDR; i < SUNXI_BSRES_EADDR; i += 4) {
			if (!(i&0xf))
				pr_cont("\n0x%px : ", (ccmu_ptr + i));
			pr_cont("%08x ", readl(ccmu_ptr + i));
		}
		pr_cont("\n");
	} else {
		pr_cont("Dump ccmu regs:pll,gating,reset,module clk\n");

		for (i = SUNXI_PP_NCM_SADDR; i < SUNXI_PP_NCM_EADDR; i += 4) {
			if (!(i&0xf))
				pr_cont("\n0x%px : ", (ccmu_ptr + i));
			pr_cont("%08x ", readl(ccmu_ptr + i));
		}
		pr_cont("\n");

		for (i = SUNXI_NCCM_SADDR; i < SUNXI_NCCM_EADDR; i += 4) {
			if (!(i&0xf))
				pr_cont("\n0x%px : ", (ccmu_ptr + i));
			pr_cont("%08x ", readl(ccmu_ptr + i));
		}
		pr_cont("\n");
	}

	iounmap(ccmu_ptr);

}



int sunxi_remap_writel(resource_size_t  phy_addr, u32 val)
{
	void __iomem *v_addr =  NULL;

	v_addr = ioremap(phy_addr, 8);
	if (v_addr == NULL) {
		pr_err("Can not map addr %x resource\n", (unsigned int)phy_addr);
		return -EIO;
	}
	writel(val, v_addr);
	printk("addr %x, val %x\n", (unsigned int)phy_addr, readl(v_addr));

	iounmap(v_addr);
	return 0;
}


int sunxi_remap_readl(resource_size_t  phy_addr, u32 *pval)
{
	void __iomem *v_addr =  NULL;

	v_addr = ioremap(phy_addr, 8);
	if (v_addr == NULL) {
		pr_err("Can not map %x resource\n", (unsigned int)phy_addr);
		return -EIO;
	}

	*pval = readl(v_addr);
	printk("addr %x val %x\n", (unsigned int)phy_addr, readl(v_addr));

	iounmap(v_addr);
	return 0;
}




static ssize_t dump_host_reg_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	char *p = buf;
	int i = 0;
	struct platform_device *pdev = to_platform_device(dev);
	struct mmc_host *mmc = platform_get_drvdata(pdev);
	struct sunxi_mmc_host *host = mmc_priv(mmc);

	p += sprintf(p, "Dump sdmmc regs:\n");
	for (i = 0; i < SUNXI_MMC_MAX_HOST_PRT_ADDR; i += 4) {
		if (!(i&0xf))
			p += sprintf(p, "\n0x%px : ", (host->reg_base + i));
		p += sprintf(p, "%08x ", readl(host->reg_base + i));
	}
	p += sprintf(p, "\n");

	return p - buf;

}

static ssize_t dump_gpio_reg_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	char *p = buf;
	int i = 0;
	void __iomem *gpio_ptr =  NULL;
	resource_size_t res_saddr_gpio;
	int ret = 0;

	ret = sunxi_mmc_res_start_addr("pio", &res_saddr_gpio);
	if (ret < 0)
		goto out;

	gpio_ptr = ioremap(res_saddr_gpio, SUNXI_DEG_MAX_MAP_REG);
	if (!gpio_ptr) {
		pr_err("Can not map gpio resource\n");
		goto out;
	}

	p += sprintf(p, "Dump gpio regs:\n");
	for (i = 0; i < SUNXI_MMC_MAX_GPIO_PRT_ADDR; i += 4) {
		if (!(i&0xf))
			p += sprintf(p, "\n0x%px : ", (gpio_ptr + i));
		p += sprintf(p, "%08x ", readl(gpio_ptr + i));
	}
	p += sprintf(p, "\n");

	p += sprintf(p, "Dump gpio irqc regs:\n");
	for (i = SUNXI_GPIOIC_PRT_SADDR; i < SUNXI_GPIOIC_PRT_EADDR; i += 4) {
		if (!(i&0xf))
			p += sprintf(p, "\n0x%px : ", (gpio_ptr + i));
		p += sprintf(p, "%08x ", readl(gpio_ptr + i));
	}
	p += sprintf(p, "\n");

	iounmap(gpio_ptr);
out:
	return p-buf;

}

static ssize_t dump_ccmu_reg_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	char *p = buf;
	int i = 0;
	void __iomem *ccmu_ptr =  NULL;
	int ret = 0;
	resource_size_t res_saddr_ccmu;

	ret = sunxi_mmc_res_start_addr("clocks", &res_saddr_ccmu);
	if (ret < 0)
		goto out;

	ccmu_ptr = ioremap(res_saddr_ccmu, SUNXI_DEG_MAX_MAP_REG);
	if (!ccmu_ptr) {
		pr_err("Can not map ccmu resource\n");
		goto out;
	}

	p += sprintf(p, "Dump ccmu\n");
	if (res_saddr_ccmu == CCMU_BASE_ADDR_BEFORE_V2P1H) {

		p += sprintf(p, "Dump ccmu regs:gating\n");
		for (i = SUNXI_BCLKG_SADDR; i < SUNXI_BCLKG_EADDR; i += 4) {
			if (!(i&0xf))
				p += sprintf(p, "\n0x%px : ", (ccmu_ptr + i));
			p += sprintf(p, "%08x ", readl(ccmu_ptr + i));
		}
		p += sprintf(p, "\n");

		p += sprintf(p, "Dump ccmu regs:module clk\n");
		for (i = SUNXI_CLK_PRT_SADDR; i < SUNXI_CLK_PRT_EADDR; i += 4) {
			if (!(i&0xf))
				p += sprintf(p, "\n0x%px : ", (ccmu_ptr + i));
			p += sprintf(p, "%08x ", readl(ccmu_ptr + i));
		}
		p += sprintf(p, "\n");

		p += sprintf(p, "Dump ccmu regs:reset\n");
		for (i = SUNXI_BSRES_SADDR; i < SUNXI_BSRES_EADDR; i += 4) {
			if (!(i&0xf))
				p += sprintf(p, "\n0x%px : ", (ccmu_ptr + i));
			p += sprintf(p, "%08x ", readl(ccmu_ptr + i));
		}
		p += sprintf(p, "\n");

	} else {
		p += sprintf(p, "Dump ccmu regs:pll,gating,reset,module clk\n");

		for (i = SUNXI_PP_NCM_SADDR; i < SUNXI_PP_NCM_EADDR; i += 4) {
			if (!(i&0xf))
				p += sprintf(p, "\n0x%px : ", (ccmu_ptr + i));
			p += sprintf(p, "%08x ", readl(ccmu_ptr + i));
		}
		p += sprintf(p, "\n");

		for (i = SUNXI_NCCM_SADDR; i < SUNXI_NCCM_EADDR; i += 4) {
			if (!(i&0xf))
				p += sprintf(p, "\n0x%px : ", (ccmu_ptr + i));
			p += sprintf(p, "%08x ", readl(ccmu_ptr + i));
		}
		p += sprintf(p, "\n");
	}
	p += sprintf(p, "\n");

	iounmap(ccmu_ptr);

out:
	return p-buf;

}

static ssize_t dump_clk_dly_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	char *p = buf;
	struct platform_device *pdev = to_platform_device(dev);
	struct mmc_host *mmc = platform_get_drvdata(pdev);
	struct sunxi_mmc_host *host = mmc_priv(mmc);

	if (host->sunxi_mmc_dump_dly_table)
		host->sunxi_mmc_dump_dly_table(host);
	else
		SM_WARN(mmc_dev(mmc), "not found the dump dly table\n");

	return p - buf;
}

int sunxi_mmc_uperf_stat(struct sunxi_mmc_host *host,
	struct mmc_data *data,
	struct mmc_request *mrq_busy,
	bool bhalf)
{
	ktime_t diff;

	if (!bhalf) {
		if (host->perf_enable && data) {
			diff = ktime_sub(ktime_get(), host->perf.start);
			if (data->flags & MMC_DATA_READ) {
				host->perf.rbytes += data->bytes_xfered;
				host->perf.rtime =
					ktime_add(host->perf.rtime, diff);
			} else if (data->flags & MMC_DATA_WRITE) {
				if (!mrq_busy) {
					host->perf.wbytes +=
						data->bytes_xfered;
					host->perf.wtime =
					ktime_add(host->perf.wtime, diff);
					sunxi_mmc_filter_rate(host, data, data->bytes_xfered, ktime_to_us(diff));
				}
				host->perf.wbytestran += data->bytes_xfered;
				host->perf.wtimetran =
					ktime_add(host->perf.wtimetran, diff);
			}
		}
	} else {
		if (host->perf_enable
			&& mrq_busy->data
			&& (mrq_busy->data->flags & MMC_DATA_WRITE)) {
			diff = ktime_sub(ktime_get(), host->perf.start);
			host->perf.wbytes += mrq_busy->data->bytes_xfered;
			host->perf.wtime = ktime_add(host->perf.wtime, diff);
			sunxi_mmc_filter_rate(host, data, data->bytes_xfered, ktime_to_us(diff));
		}
	}
	return 0;
}
EXPORT_SYMBOL_GPL(sunxi_mmc_uperf_stat);

static ssize_t
sunxi_mmc_show_perf(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct mmc_host	*mmc = platform_get_drvdata(pdev);
	struct sunxi_mmc_host *host = mmc_priv(mmc);
	int64_t rtime_drv, wtime_drv, wtime_drv_tran;
	int64_t rbytes_drv, wbytes_drv, wbytes_drv_tran;


	mmc_claim_host(mmc);

	rbytes_drv = host->perf.rbytes;
	wbytes_drv = host->perf.wbytes;
	wbytes_drv_tran = host->perf.wbytestran;

	rtime_drv = ktime_to_us(host->perf.rtime);
	wtime_drv = ktime_to_us(host->perf.wtime);
	wtime_drv_tran = ktime_to_us(host->perf.wtimetran);

	mmc_release_host(mmc);

	return snprintf(buf, PAGE_SIZE, "Write performance at host driver Level:"
					"%lld bytes in %lld microseconds\n"
					"Read performance at host driver Level:"
					"%lld bytes in %lld microseconds\n"
					"write performance at host driver Level(no wait busy):"
					"%lld bytes in %lld microseconds\n",
					wbytes_drv, wtime_drv,
					rbytes_drv, rtime_drv,
					wbytes_drv_tran, wtime_drv_tran);
}

static ssize_t
sunxi_mmc_set_perf(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct mmc_host	*mmc = platform_get_drvdata(pdev);
	struct sunxi_mmc_host *host = mmc_priv(mmc);
	int64_t value;

	sscanf(buf, "%lld", &value);
	printk("set perf value %lld\n", value);

	mmc_claim_host(mmc);
	if (!value) {
		memset(&host->perf, 0, sizeof(host->perf));
		host->perf_enable = false;
	} else {
		host->perf_enable = true;
	}
	mmc_release_host(mmc);

	return count;
}

extern void sunxi_mmc_set_ds_dl_raw(struct sunxi_mmc_host *host, int sunxi_ds_dl);
extern void sunxi_mmc_set_samp_dl_raw(struct sunxi_mmc_host *host, int sunxi_samp_dl);

int sunxi_mmc_bus_clk_en(struct sunxi_mmc_host *host, int enable);
void sunxi_mmc_regs_save(struct sunxi_mmc_host *host);
void sunxi_mmc_regs_restore(struct sunxi_mmc_host *host);

static ssize_t
sunxi_mmc_set_samp_dly_sys(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct mmc_host	*mmc = platform_get_drvdata(pdev);
	struct sunxi_mmc_host *host = mmc_priv(mmc);
	int64_t value;

	sscanf(buf, "%lld", &value);
	printk("set sample delay %lld\n", value);
	mmc_claim_host(mmc);
	sunxi_mmc_set_samp_dl_raw(host, value);
	mmc_release_host(mmc);

	return count;
}

static ssize_t
sunxi_mmc_set_ds_dly_sys(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct mmc_host	*mmc = platform_get_drvdata(pdev);
	struct sunxi_mmc_host *host = mmc_priv(mmc);
	int64_t value;

	sscanf(buf, "%lld", &value);
	printk("set ds delay %lld\n", value);
	mmc_claim_host(mmc);
	sunxi_mmc_set_ds_dl_raw(host, value);
	mmc_release_host(mmc);

	return count;
}

static ssize_t
sunxi_mmc_send_status(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct mmc_host	*mmc = platform_get_drvdata(pdev);
	u32 status = 0;

	mmc_claim_host(mmc);
	mmc_send_status(mmc->card, &status);
	printk("mmc status %x\n", status);
	mmc_release_host(mmc);

	return (ssize_t)buf;
}

static ssize_t
sunxi_mmc_show_filter_sector(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct mmc_host	*mmc = platform_get_drvdata(pdev);
	struct sunxi_mmc_host *host = mmc_priv(mmc);

	return snprintf(buf, PAGE_SIZE, "filter speed %d\n", host->filter_sector);
}

static ssize_t
sunxi_mmc_set_filter_sector(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct mmc_host	*mmc = platform_get_drvdata(pdev);
	struct sunxi_mmc_host *host = mmc_priv(mmc);
	int64_t value;

	sscanf(buf, "%lld", &value);
	printk("get filter sector %lld\n", value);
	host->filter_sector = value;

	return count;
}

#ifdef CONFIG_AW_MMC_DEBUG
static ssize_t
sunxi_mmc_show_dbglevel(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct mmc_host	*mmc = platform_get_drvdata(pdev);
	struct sunxi_mmc_host *host = mmc_priv(mmc);

	return snprintf(buf, PAGE_SIZE, "Current debuglevel %d\n", host->debuglevel);
}

static ssize_t
sunxi_mmc_set_dbglevel(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct mmc_host	*mmc = platform_get_drvdata(pdev);
	struct sunxi_mmc_host *host = mmc_priv(mmc);
	int64_t value;

	sscanf(buf, "%lld", &value);
	printk("set debug level %lld\n", value);
	host->debuglevel = value;

	return count;
}
#endif


static ssize_t
sunxi_mmc_show_filter_speed(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct mmc_host	*mmc = platform_get_drvdata(pdev);
	struct sunxi_mmc_host *host = mmc_priv(mmc);

	return snprintf(buf, PAGE_SIZE, "filter speed %d\n", host->filter_speed);
}

static ssize_t
sunxi_mmc_set_filter_speed(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct mmc_host	*mmc = platform_get_drvdata(pdev);
	struct sunxi_mmc_host *host = mmc_priv(mmc);
	int64_t value;

	sscanf(buf, "%lld", &value);
	printk("get filter speed %lld\n", value);
	host->filter_speed = value;

	return count;
}





int mmc_create_sys_fs(struct sunxi_mmc_host *host, struct platform_device *pdev)
{
	int ret;

	host->maual_insert.show = maual_insert_show;
	host->maual_insert.store = maual_insert_store;
	sysfs_attr_init(&(host->maual_insert.attr));
	host->maual_insert.attr.name = "sunxi_insert";
	host->maual_insert.attr.mode = S_IRUGO | S_IWUSR;
	ret = device_create_file(&pdev->dev, &host->maual_insert);
	if (ret)
		return ret;

	host->host_perf.show = sunxi_mmc_show_perf;
	host->host_perf.store = sunxi_mmc_set_perf;
	sysfs_attr_init(&(host->host_perf.attr));
	host->host_perf.attr.mode = S_IRUGO | S_IWUSR;
	host->host_perf.attr.name = "sunxi_host_perf";
	ret = device_create_file(&pdev->dev, &host->host_perf);
	if (ret)
		return ret;

	host->filter_sector_perf.show = sunxi_mmc_show_filter_sector;
	host->filter_sector_perf.store = sunxi_mmc_set_filter_sector;
	sysfs_attr_init(&(host->filter_sector_perf.attr));
	host->filter_sector_perf.attr.name = "sunxi_host_filter_w_sector";
	host->filter_sector_perf.attr.mode = S_IRUGO | S_IWUSR;
	ret = device_create_file(&pdev->dev, &host->filter_sector_perf);
	if (ret)
		return ret;

	host->filter_speed_perf.show = sunxi_mmc_show_filter_speed;;
	host->filter_speed_perf.store = sunxi_mmc_set_filter_speed;;
	sysfs_attr_init(&(host->filter_speed_perf.attr));
	host->filter_speed_perf.attr.name = "sunxi_host_filter_w_speed";
	host->filter_speed_perf.attr.mode = S_IRUGO | S_IWUSR;
	ret = device_create_file(&pdev->dev, &host->filter_speed_perf);
	if (ret)
		return ret;


	host->dump_register = dump_register;
	host->dump_register[0].show = dump_host_reg_show;
	sysfs_attr_init(&(host->dump_register[0].attr));
	host->dump_register[0].attr.name = "sunxi_dump_host_register";
	host->dump_register[0].attr.mode = S_IRUGO;
	ret = device_create_file(&pdev->dev, &host->dump_register[0]);
	if (ret)
		return ret;

	host->dump_register[1].show = dump_gpio_reg_show;
	sysfs_attr_init(&(host->dump_register[1].attr));
	host->dump_register[1].attr.name = "sunxi_dump_gpio_register";
	host->dump_register[1].attr.mode = S_IRUGO;
	ret = device_create_file(&pdev->dev, &host->dump_register[1]);
	if (ret)
		return ret;

	host->dump_register[2].show = dump_ccmu_reg_show;
	sysfs_attr_init(&(host->dump_register[2].attr));
	host->dump_register[2].attr.name = "sunxi_dump_ccmu_register";
	host->dump_register[2].attr.mode = S_IRUGO;
	ret = device_create_file(&pdev->dev, &host->dump_register[2]);
	if (ret)
		return ret;

	host->dump_clk_dly.show = dump_clk_dly_show;
	sysfs_attr_init(&(host->dump_clk_dly.attr));
	host->dump_clk_dly.attr.name = "sunxi_dump_clk_dly";
	host->dump_clk_dly.attr.mode = S_IRUGO;
	ret = device_create_file(&pdev->dev, &host->dump_clk_dly);
	if (ret)
		return ret;

	host->host_sample_dly.show = NULL;
	host->host_sample_dly.store = sunxi_mmc_set_samp_dly_sys;
	sysfs_attr_init(&(host->host_sample_dly.attr));
	host->host_sample_dly.attr.name = "sunxi_host_set_sample_dly";
	host->host_sample_dly.attr.mode =  S_IWUSR;
	ret = device_create_file(&pdev->dev, &host->host_sample_dly);
	if (ret)
		return ret;

	host->host_ds_dly.show = NULL;
	host->host_ds_dly.store = sunxi_mmc_set_ds_dly_sys;
	sysfs_attr_init(&(host->host_ds_dly.attr));
	host->host_ds_dly.attr.name = "sunxi_host_set_ds_dly";
	host->host_ds_dly.attr.mode =  S_IWUSR;
	ret = device_create_file(&pdev->dev, &host->host_ds_dly);
	if (ret)
		return ret;

	host->host_send_status.show = sunxi_mmc_send_status;
	host->host_send_status.store = NULL;
	sysfs_attr_init(&(host->host_send_status.attr));
	host->host_send_status.attr.name = "sunxi_mmc_send_status";
	host->host_send_status.attr.mode =  S_IRUGO;
	ret = device_create_file(&pdev->dev, &host->host_send_status);
	if (ret)
		return ret;
#ifdef CONFIG_AW_MMC_DEBUG
	host->host_debuglevel.show = sunxi_mmc_show_dbglevel;
	host->host_debuglevel.store = sunxi_mmc_set_dbglevel;
	sysfs_attr_init(&(host->debuglevel.attr));
	host->host_debuglevel.attr.name = "sunxi_debug_level";
	host->host_debuglevel.attr.mode = S_IRUGO | S_IWUSR;
	ret = device_create_file(&pdev->dev, &host->host_debuglevel);
	if (ret)
		return ret;
#endif

	host->host_mwr.show = sunxi_mmc_panic_rtest;
	host->host_mwr.store = sunxi_mmc_pancic_wrtest;
	sysfs_attr_init(&(host->host_mwr.attr));
	host->host_mwr.attr.name = "sunxi_host_panic_wr";
	host->host_mwr.attr.mode = S_IRUGO | S_IWUSR;
	ret = device_create_file(&pdev->dev, &host->host_mwr);

	return ret;
}
EXPORT_SYMBOL_GPL(mmc_create_sys_fs);

void mmc_remove_sys_fs(struct sunxi_mmc_host *host,
		       struct platform_device *pdev)
{
	device_remove_file(&pdev->dev, &host->host_mwr);
	device_remove_file(&pdev->dev, &host->host_perf);
	device_remove_file(&pdev->dev, &host->maual_insert);
	device_remove_file(&pdev->dev, &host->dump_register[0]);
	device_remove_file(&pdev->dev, &host->dump_register[1]);
	device_remove_file(&pdev->dev, &host->dump_register[2]);
	device_remove_file(&pdev->dev, &host->dump_clk_dly);
	device_remove_file(&pdev->dev, &host->filter_sector_perf);
	device_remove_file(&pdev->dev, &host->filter_speed_perf);
	device_remove_file(&pdev->dev, &host->host_sample_dly);
	device_remove_file(&pdev->dev, &host->host_ds_dly);
	device_remove_file(&pdev->dev, &host->host_send_status);
}
EXPORT_SYMBOL_GPL(mmc_remove_sys_fs);
