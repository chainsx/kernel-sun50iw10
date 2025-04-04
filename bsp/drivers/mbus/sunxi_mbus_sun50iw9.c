// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Allwinner SUNXI MBUS driver for SUN50IW9
 *
 * Copyright (C) 2022-2027 AllWinnertech Ltd.
 * Author: xiafeng <xiafeng@allwinnertech.com>
 *         Martin <wuyan@allwinnertech.com>
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

#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of_address.h>
#include <linux/slab.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <asm/cacheflush.h>

#define DRIVER_NAME                 "MBUS"
#define DRIVER_NAME_PMU             DRIVER_NAME"_PMU"

/* Time Measurement Register */
#define MBUS_MAST_TMR_REG(n)        (0x000c)

/* Master N Configuration Register 0 */
#define MBUS_MAST_CFG0_REG(n)       (0x0210 + (0x10 * (n)))

/* Master N Configuration Register 1 */
#define MBUS_MAST_CFG1_REG(n)       (0x0214 + (0x10 * (n)))

/* Master N BW Limit Register */
#define MBUS_MAST_ABS_BWL_REG(n)    (0x0218 + (0x10 * (n)))

/* Bandwidth Window Configuration Register */
#define MBUS_BW_CFG_REG             (0x0200)

/* Master Access Enable Register 0 ~ 2,0:disable, 1:enable */
#define MBUS_MAST_ACEN_CFG_REG(n)   (0x0020 + (0x04 * (n)))

/* Some platform implement master access priority
 * in register MBUS_MAST_CFG0_REG(n)
 * register: Master Access Priority, 0:low, 1:high
 */
#define MBUS_MAST_ACPR_CFG_REG      (0x0098)

/* Master Counter General Configuration Register */
/* Counter Enable, 0x1:enable all */
#define MBUS_PMU_CNTEB_CFG_REG      (0x009c)

/* BW Counter Register, Counter n = 0 ~ 7 */
#define MBUS_PMU_CNT_REG(n)         (0x00a0 + (0x4 * (n)))

/* Software Clock ON  Register 0: 0=open by hws */
#define MBUS_SW_CLK_ON_REG          (0x0030)

/* Software Clock OFF Register 0: 1=dis-access */
#define MBUS_SW_CLK_OFF_REG         (0x0040)

/* For MBUS_MAST_CFG0_REG */
/* bit mask of MASTER_N_QOS_VALUE */
#define MBUS_QOS_MAX		0x03

/* bit mask of MASTER_N_WAIT_TIME */
#define MBUS_WT_MAX		0x0f

/* bit mask of COMMAND_NUMBER (access commands sequence) */
#define MBUS_ACS_MAX		0x0ff

/* bit mask of BANDWIDTH_LIMIT_0 */
#define MBUS_BWL_MAX		0x0ffff

/* bit shift of BANDWIDTH_LIMIT_ENABLE. 0:dis */
#define MBUS_BWLEN_SHIFT	0

/* bit shift of MASTER_N_ACCESS_PRIORITY. 0:low */
#define MBUS_PRI_SHIFT		1

/* bit shift of MASTER_N_QOS_VALUE. 0:lowest, 3:highest */
#define MBUS_QOS_SHIFT		2

/* bit shift of MASTER_N_WAIT_TIME */
#define MBUS_WT_SHIFT		4

/* bit shift of COMMAND_NUMBER */
#define MBUS_ACS_SHIFT		8

/* bit shift of BANDWIDTH_LIMIT_0. 0: no limit */
#define MBUS_BWL0_SHIFT		16

/* For MBUS_MAST_CFG1_REG */
/* bit shift of BANDWIDTH_LIMIT_1 */
#define MBUS_BWL1_SHIFT		0

/* bit shift of BANDWIDTH_LIMIT_2 */
#define MBUS_BWL2_SHIFT		16

/* For MBUS_MAST_ABS_BWL_REG */
/* bit mask of LIMIT_BANDWIDTH */
#define MBUS_ABS_BWL_MAX	0x0fff

/* bit mask of BANDWIDTH_SATURATION */
#define MBUS_BW_SATU_MAX	0x0fff

/* bit shift of LIMIT_BANDWIDTH */
#define MBUS_ABS_BWL_SHIFT	16

/* bit shift of BANDWIDTH_LIMIT_ENABLE */
#define MBUS_ABS_BWLEN_SHIFT	31

/* For MBUS_BW_CFG_REG */
#define MBUS_BWSIZE_MAX		0x0f
#define MBUS_BWEN_SHIFT		16

/* MBUS PMU ids */
/* See SDRAM Controller Spec: DRAMC BW Counter Register (offset:0xa0) */
enum mbus_pmu {
	MBUS_PMU_CPU        = 0,
	MBUS_PMU_MAHB       = 1,
	MBUS_PMU_DMA        = 2,
	MBUS_PMU_VE         = 3,
	MBUS_PMU_CE         = 4,
	MBUS_PMU_DISP       = 5,
	MBUS_PMU_CSI0       = 6,
	MBUS_PMU_DI         = 7,
	MBUS_PMU_CSI1       = 8,
	MBUS_PMU_G2D        = 9,
	MBUS_PMU_VE1        = 10,
	MBUS_PMU_IOMMU      = 11,
	MBUS_PMU_GPU        = 12,
	MBUS_PMU_TOTAL      = 13,
	MBUS_PMU_MAX        = 14,
};

#define MBUS_PORT_PRI           (MBUS_PMU_MAX + 0)
#define MBUS_PORT_QOS           (MBUS_PMU_MAX + 1)
#define MBUS_PORT_WT            (MBUS_PMU_MAX + 2)
#define MBUS_PORT_ACS           (MBUS_PMU_MAX + 3)
#define MBUS_PORT_BWL0          (MBUS_PMU_MAX + 4)
#define MBUS_PORT_BWL1          (MBUS_PMU_MAX + 5)
#define MBUS_PORT_BWL2          (MBUS_PMU_MAX + 6)
#define MBUS_PORT_BWLEN         (MBUS_PMU_MAX + 7)
#define MBUS_PORT_ABS_BWLEN     (MBUS_PMU_MAX + 8)
#define MBUS_PORT_ABS_BWL       (MBUS_PMU_MAX + 9)
#define MBUS_PORT_BW_SATU       (MBUS_PMU_MAX + 10)

/* Port ids */
/* See SDRAM Controller Spec: DRAMC access master list */
enum mbus_port {
	MBUS_PORT_CPU           = 0,
	MBUS_PORT_GPU           = 1,
	MBUS_PORT_MAHB          = 2,
	MBUS_PORT_DMA           = 3,
	MBUS_PORT_VE0           = 4,
	MBUS_PORT_CE            = 5,
	MBUS_PORT_TSC0          = 6,
	MBUS_PORT_NDFC0         = 8,
	MBUS_PORT_CSI0          = 11,
	MBUS_PORT_DI0           = 14,
	MBUS_PORT_DE300         = 16,
	MBUS_PORT_G2D_ROT       = 21,
	MBUS_PORT_CSI1          = 22,
	MBUS_PORT_ISP1          = 23,
	MBUS_PORT_VE1           = 24,
	MBUS_PORT_IOMMU         = 25,
	MBUS_PORT_HDMI          = 39,
	MBUS_PORTS_MAX          = 40,
};

struct sunxi_mbus_port {
	void __iomem *base;
	unsigned long phys;
	struct device_node *dn;
};

static struct sunxi_mbus_port *ports;
static void __iomem *mbus_ctrl_base;
static unsigned long mbus_ctrl_phys;

static DEFINE_MUTEX(mbus_seting);
static DEFINE_MUTEX(mbus_pmureading);

#define mbus_pmu_getstate() \
	(readl_relaxed(mbus_ctrl_base + MBUS_PMU_CNTEB_CFG_REG) & 1)
#define mbus_pmu_enable() \
	writel_relaxed( \
	((readl_relaxed(mbus_ctrl_base + MBUS_PMU_CNTEB_CFG_REG)) | 1), \
		       mbus_ctrl_base + MBUS_PMU_CNTEB_CFG_REG)

/**
 * mbus_port_set_abs_bwlen() - enable a master absolutely bandwidth limit
 * function
 *
 * @port: index of the port to setup
 * @en: 0-disable, 1-enable
 */
int notrace mbus_port_set_abs_bwlen(enum mbus_port port, bool en)
{
	unsigned int value;

	if (port >= MBUS_PORTS_MAX)
		return -ENODEV;

	mutex_lock(&mbus_seting);
	value = readl_relaxed(mbus_ctrl_base + MBUS_MAST_ABS_BWL_REG(port));
	value &= ~(1U << MBUS_ABS_BWLEN_SHIFT);
	writel_relaxed(value | (en << MBUS_ABS_BWLEN_SHIFT),
			mbus_ctrl_base + MBUS_MAST_ABS_BWL_REG(port));
	mutex_unlock(&mbus_seting);

	return 0;
}
EXPORT_SYMBOL_GPL(mbus_port_set_abs_bwlen);

/**
 * mbus_port_set_abs_bwl() - set a master absolutely bandwidth limit
 *
 * @bwl: the number of bandwidth limit
 */
int notrace mbus_port_set_abs_bwl(enum mbus_port port, unsigned int bwl)
{
	unsigned int value;

	if (port >= MBUS_PORTS_MAX)
		return -ENODEV;

	if (bwl > MBUS_ABS_BWL_MAX)
		return -EPERM;

	mutex_lock(&mbus_seting);
	/* absolutely bwl, used when en BWL */
	value = readl_relaxed(mbus_ctrl_base + MBUS_MAST_ABS_BWL_REG(port));
	value &= ~(MBUS_ABS_BWL_MAX << MBUS_ABS_BWL_SHIFT);
	writel_relaxed(value | (bwl << MBUS_ABS_BWL_SHIFT),
			mbus_ctrl_base + MBUS_MAST_ABS_BWL_REG(port));
	mutex_unlock(&mbus_seting);

	return 0;
}
EXPORT_SYMBOL_GPL(mbus_port_set_abs_bwl);

/**
 * mbus_port_set_bw_saturation() - set a master bandwidth saturation
 *
 * @bw_satu: the number of bandwidth saturation
 */
int notrace mbus_port_set_bw_saturation(enum mbus_port port,
							unsigned int bw_satu)
{
	unsigned int value;

	if (port >= MBUS_PORTS_MAX)
		return -ENODEV;

	if (bw_satu > MBUS_BW_SATU_MAX)
		return -EPERM;

	mutex_lock(&mbus_seting);
	/* absolutely bw_satu, used when en BWL */
	value = readl_relaxed(mbus_ctrl_base + MBUS_MAST_ABS_BWL_REG(port));
	value &= ~MBUS_BW_SATU_MAX;
	writel_relaxed(value | bw_satu,
		mbus_ctrl_base + MBUS_MAST_ABS_BWL_REG(port));
	mutex_unlock(&mbus_seting);

	return 0;
}
EXPORT_SYMBOL_GPL(mbus_port_set_bw_saturation);

/**
 * mbus_port_setreqn() - enable a master bandwidth limit function
 *
 * @port: index of the port to setup
 * @en: 0-disable, 1-enable
 */
int notrace mbus_port_setbwlen(enum mbus_port port, bool en)
{
	unsigned int value;

	if (port >= MBUS_PORTS_MAX)
		return -ENODEV;

	mutex_lock(&mbus_seting);
	value = readl_relaxed(mbus_ctrl_base + MBUS_MAST_CFG0_REG(port));
	value &= ~(1 << MBUS_BWLEN_SHIFT);
	writel_relaxed(value | (en << MBUS_BWLEN_SHIFT),
		       mbus_ctrl_base + MBUS_MAST_CFG0_REG(port));
	mutex_unlock(&mbus_seting);

	return 0;
}
EXPORT_SYMBOL_GPL(mbus_port_setbwlen);

/**
 * mbus_port_setthd() - set a master priority
 *
 * @pri, priority
 */
int notrace mbus_port_setpri(enum mbus_port port, bool pri)
{
	unsigned int value = 0;

	if (port >= MBUS_PORTS_MAX)
		return -ENODEV;

	mutex_lock(&mbus_seting);
#if (defined MBUS_MAST_ACPR_CFG_REG)
	value = readl_relaxed(mbus_ctrl_base + MBUS_MAST_ACPR_CFG_REG);
	value &= ~(1 << port);
	writel_relaxed(value | (pri << port),
			mbus_ctrl_base + MBUS_MAST_ACPR_CFG_REG);
#else
	value = readl_relaxed(mbus_ctrl_base + MBUS_MAST_CFG0_REG(port));
	value &= ~(1 << MBUS_PRI_SHIFT);
	writel_relaxed(value | (pri << MBUS_PRI_SHIFT),
			mbus_ctrl_base + MBUS_MAST_CFG0_REG(port));
#endif
	mutex_unlock(&mbus_seting);

	return 0;
}
EXPORT_SYMBOL_GPL(mbus_port_setpri);

/**
 * mbus_port_setqos() - set a master QOS
 *
 * @qos: the qos value want to set
 */
int notrace mbus_port_setqos(enum mbus_port port, unsigned int qos)
{
	unsigned int value;

	if (port >= MBUS_PORTS_MAX)
		return -ENODEV;

	if (qos > MBUS_QOS_MAX)
		return -EPERM;

	mutex_lock(&mbus_seting);
	value = readl_relaxed(mbus_ctrl_base + MBUS_MAST_CFG0_REG(port));
	value &= ~(MBUS_QOS_MAX << MBUS_QOS_SHIFT);
	writel_relaxed(value | (qos << MBUS_QOS_SHIFT),
		       mbus_ctrl_base + MBUS_MAST_CFG0_REG(port));
	mutex_unlock(&mbus_seting);

	return 0;
}
EXPORT_SYMBOL_GPL(mbus_port_setqos);

/**
 * mbus_bw_setbw() - set a master wait time
 *
 * @wt: the wait time want to set, based on MCLK
 */
int notrace mbus_port_setwt(enum mbus_port port, unsigned int wt)
{
	unsigned int value;

	if (port >= MBUS_PORTS_MAX)
		return -ENODEV;

	if (wt > MBUS_WT_MAX)
		return -EPERM;

	mutex_lock(&mbus_seting);
	value = readl_relaxed(mbus_ctrl_base + MBUS_MAST_CFG0_REG(port));
	value &= ~(MBUS_WT_MAX << MBUS_WT_SHIFT);
	writel_relaxed(value | (wt << MBUS_WT_SHIFT),
		       mbus_ctrl_base + MBUS_MAST_CFG0_REG(port));
	mutex_unlock(&mbus_seting);

	return 0;
}
EXPORT_SYMBOL_GPL(mbus_port_setwt);

/**
 * mbus_bw_setams() - set a master access commands sequence
 *
 * @acs: the number of access commands sequency
 */
int notrace mbus_port_setacs(enum mbus_port port, unsigned int acs)
{
	unsigned int value;

	if (port >= MBUS_PORTS_MAX)
		return -ENODEV;

	if (acs > MBUS_ACS_MAX)
		return -EPERM;

	mutex_lock(&mbus_seting);
	value = readl_relaxed(mbus_ctrl_base + MBUS_MAST_CFG0_REG(port));
	value &= ~(MBUS_ACS_MAX << MBUS_ACS_SHIFT);
	writel_relaxed(value | (acs << MBUS_ACS_SHIFT),
		       mbus_ctrl_base + MBUS_MAST_CFG0_REG(port));
	mutex_unlock(&mbus_seting);

	return 0;
}
EXPORT_SYMBOL_GPL(mbus_port_setacs);

/**
 * mbus_bw_setbwl0() - function to set bandwidth limit0
 *
 * @bwl: the number of bandwidth limit
 */
int notrace mbus_port_setbwl0(enum mbus_port port, unsigned int bwl0)
{
	unsigned int value;

	if (port >= MBUS_PORTS_MAX)
		return -ENODEV;

	if (bwl0 > MBUS_BWL_MAX)
		return -EPERM;

	mutex_lock(&mbus_seting);
	/* bwl0, used when BWL function enable */
	value = readl_relaxed(mbus_ctrl_base + MBUS_MAST_CFG0_REG(port));
	value &= ~(MBUS_BWL_MAX << MBUS_BWL0_SHIFT);
	writel_relaxed(value | (bwl0 << MBUS_BWL0_SHIFT),
		       mbus_ctrl_base + MBUS_MAST_CFG0_REG(port));
	mutex_unlock(&mbus_seting);

	return 0;
}
EXPORT_SYMBOL_GPL(mbus_port_setbwl0);

/**
 * mbus_bw_setbwl1() - set a master bandwidth limit1
 *
 * @bwl: the number of bandwidth limit
 */
int notrace mbus_port_setbwl1(enum mbus_port port, unsigned int bwl1)
{
	unsigned int value;

	if (port >= MBUS_PORTS_MAX)
		return -ENODEV;

	if (bwl1 > MBUS_BWL_MAX)
		return -EPERM;

	mutex_lock(&mbus_seting);
	/* bwl1, used when en BWL */
	value = readl_relaxed(mbus_ctrl_base + MBUS_MAST_CFG1_REG(port));
	value &= ~(MBUS_BWL_MAX << MBUS_BWL1_SHIFT);
	writel_relaxed(value | (bwl1 << MBUS_BWL1_SHIFT),
		       mbus_ctrl_base + MBUS_MAST_CFG1_REG(port));
	mutex_unlock(&mbus_seting);

	return 0;
}
EXPORT_SYMBOL_GPL(mbus_port_setbwl1);

/**
 * mbus_bw_setbwl2() - set a master bandwidth limit2
 *
 * @bwl: the number of bandwidth limit
 */
int notrace mbus_port_setbwl2(enum mbus_port port, unsigned int bwl2)
{
	unsigned int value;

	if (port >= MBUS_PORTS_MAX)
		return -ENODEV;

	if (bwl2 > MBUS_BWL_MAX)
		return -EPERM;

	mutex_lock(&mbus_seting);
	/* bwl2, used when en BWL */
	value = readl_relaxed(mbus_ctrl_base + MBUS_MAST_CFG1_REG(port));
	value &= ~(MBUS_BWL_MAX << MBUS_BWL2_SHIFT);
	writel_relaxed(value | (bwl2 << MBUS_BWL2_SHIFT),
		       mbus_ctrl_base + MBUS_MAST_CFG1_REG(port));
	mutex_unlock(&mbus_seting);

	return 0;
}
EXPORT_SYMBOL_GPL(mbus_port_setbwl2);

/**
 * mbus_bw_setbwl() - set a master bandwidth limit
 *
 * @bwl0/1/2: the number of bandwidth limit0/1/2
 */
int notrace mbus_port_setbwl(enum mbus_port port, unsigned int bwl0,
			     unsigned int bwl1, unsigned int bwl2)
{
	if (port >= MBUS_PORTS_MAX)
		return -ENODEV;

	if ((bwl0 > MBUS_BWL_MAX) || (bwl1 > MBUS_BWL_MAX)
	    || (bwl2 > MBUS_BWL_MAX))
		return -EPERM;

	mbus_port_setbwl0(port, bwl0);
	mbus_port_setbwl1(port, bwl1);
	mbus_port_setbwl2(port, bwl2);

	return 0;
}
EXPORT_SYMBOL_GPL(mbus_port_setbwl);

/**
 * mbus_bw_control() - set BandWidth limit window enable or disable
 *
 * @enable: if true enables the bwlw, if false disables it
 */
int notrace mbus_set_bwlwen(bool enable)
{
	unsigned int value;

	mutex_lock(&mbus_seting);
	value = readl_relaxed(mbus_ctrl_base + MBUS_BW_CFG_REG);

	writel_relaxed(enable ? (value | (1 << MBUS_BWEN_SHIFT))
		       : (value & ~(1 << MBUS_BWEN_SHIFT)),
		       mbus_ctrl_base + MBUS_BW_CFG_REG);
	mutex_unlock(&mbus_seting);

	return 0;
}
EXPORT_SYMBOL_GPL(mbus_set_bwlwen);

/**
 * mbus_bw_control() - set BandWidth limit window size
 *
 * @size: the size of bwl window, base on MCLK
 */
int notrace mbus_set_bwlwsiz(unsigned int size)
{
	unsigned int value;

	mutex_lock(&mbus_seting);
	value = readl_relaxed(mbus_ctrl_base + MBUS_BW_CFG_REG);
	value &= ~MBUS_BWSIZE_MAX;
	writel_relaxed(value | size, mbus_ctrl_base + MBUS_BW_CFG_REG);
	mutex_unlock(&mbus_seting);

	return 0;
}
EXPORT_SYMBOL_GPL(mbus_set_bwlwsiz);

/**
 * They are called by low-level power management code to disable slave
 * interfaces snoops and DVM broadcast.
 */

/**
 * mbus_port_control() - control a master port access DRAM
 *
 * @enable: if true enables the port, if false disables it
 */
static void notrace mbus_port_control(enum mbus_port port, bool enable)
{
	unsigned int value, reg, pos;

	reg = MBUS_MAST_ACEN_CFG_REG(port/32);
	pos = port%32;

	/*
	 * This function is called from power down procedures
	 * and must not execute any instruction that might
	 * cause the processor to be put in a quiescent state
	 * (eg wfi). Hence, cpu_relax() can not be added to this
	 * read loop to optimize power, since it might hide possibly
	 * disruptive operations.
	 */
	mutex_lock(&mbus_seting);
	value = readl_relaxed(mbus_ctrl_base + reg);
	if (enable)
		value |= (1 << pos);
	else
		value &= ~(1 << pos);
	writel_relaxed(value, mbus_ctrl_base + reg);
	mutex_unlock(&mbus_seting);
}

/**
 * mbus_control_port_by_index() - control a master port by port index
 *
 * @port: port index previously retrieved with mbus_ace_get_port()
 * @enable: if true enables the port, if false disables it
 *
 * Return:
 *	0 on success
 *	-ENODEV on port index out of range
 *	-EPERM if operation carried out on an ACE PORT
 */
int notrace mbus_port_control_by_index(enum mbus_port port, bool enable)
{
	if (port >= MBUS_PORTS_MAX)
		return -ENODEV;
	/*
	 * MBUS control for ports connected to CPUS is extremely fragile
	 * and must be made to go through a specific.
	 */

	mbus_port_control(port, enable);
	return 0;
}
EXPORT_SYMBOL_GPL(mbus_port_control_by_index);

static const struct of_device_id sunxi_mbus_matches[] = {
	{.compatible = "allwinner,sun50i-mbus", },
	{},
};

static int mbus_probe(void)
{
	int ret;
	struct device_node *np;
	struct resource res;

	np = of_find_matching_node(NULL, sunxi_mbus_matches);
	if (!np)
		return -ENODEV;

	ports = kcalloc(1, sizeof(*ports), GFP_KERNEL);
	if (!ports)
		return -ENOMEM;

	ret = of_address_to_resource(np, 0, &res);
	if (!ret) {
		mbus_ctrl_base = ioremap(res.start, resource_size(&res));
		mbus_ctrl_phys = res.start;

	}
	if (ret || !mbus_ctrl_base) {
		WARN(1, "unable to ioremap mbus ctrl\n");
		ret = -ENXIO;
		goto memalloc_err;
	}

	/* the purpose freq of MBUS is 400M, has been configied by boot */

	/* all the port is default opened */

	/* set default bandwidth */

	/* set default QOS */

	/* set masters' request number sequency */

	/* set masters' bandwidth limit0/1/2 */

	/* sync_cache_w(&mbus_ctrl_base); */
	/* sync_cache_w(&mbus_ctrl_phys); */
	/* sync_cache_w(&ports); */
	/* __sync_cache_range_w(ports, sizeof(*ports)); */

memalloc_err:
	kfree(ports);

	return 0;
}

static int mbus_init_status = -EAGAIN;
static DEFINE_MUTEX(mbus_proing);

static int mbus_init(void)
{

	if (mbus_init_status != -EAGAIN)
		return mbus_init_status;

	mutex_lock(&mbus_proing);
	if (mbus_init_status == -EAGAIN)
		mbus_init_status = mbus_probe();
	mutex_unlock(&mbus_proing);

	return mbus_init_status;
}

/**
 * To sort out early init calls ordering a helper function is provided to
 * check if the mbus driver has beed initialized. Function check if the driver
 * has been initialized, if not it calls the init function that probes
 * the driver and updates the return value.
 */
bool mbus_probed(void)
{
	return mbus_init() == 0;
}
EXPORT_SYMBOL_GPL(mbus_probed);

struct mbus_data {
	struct device *hwmon_dev;
	struct mutex update_lock;
	bool valid;
	unsigned long last_updated;
	int kind;
};

static struct mbus_data hw_mbus_pmu;

static unsigned int mbus_update_device(struct mbus_data *data,
				       enum mbus_pmu port)
{
	unsigned int value = 0;

	mutex_lock(&data->update_lock);

	/* confirm the pmu is enabled */
	if (!mbus_pmu_getstate())
		mbus_pmu_enable();

	/* read pmu conter */
	value = readl_relaxed(mbus_ctrl_base + MBUS_PMU_CNT_REG(port));

	mutex_unlock(&data->update_lock);

	return value;
}

#define for_each_ports(port) for (port = 0; port < MBUS_PORTS_MAX; port++)

static unsigned int mbus_get_value(struct mbus_data *data,
				   unsigned int index, char *buf)
{
	unsigned int i, size = 0;
	unsigned int value;

	mutex_lock(&data->update_lock);
	switch (index) {
	case MBUS_PORT_PRI:
		for_each_ports(i) {
#if (defined MBUS_MAST_ACPR_CFG_REG)
			value = readl_relaxed(mbus_ctrl_base +
					MBUS_MAST_ACPR_CFG_REG);
			value >>= i;
#else
			value = readl_relaxed(mbus_ctrl_base +
					MBUS_MAST_CFG0_REG(i));
			value >>= MBUS_PRI_SHIFT;
#endif
			size += sprintf(buf + size, "master%2u priority:%1u\n",
					i, (value & 1));
		}
		break;
	case MBUS_PORT_QOS:
		for_each_ports(i) {
			value = readl_relaxed(mbus_ctrl_base +
					      MBUS_MAST_CFG0_REG(i));
			value >>= MBUS_QOS_SHIFT;
			value &= MBUS_QOS_MAX;
			size += sprintf(buf + size, "master%2u qos:%1u\n",
					i, value);
		}
		break;
	case MBUS_PORT_WT:
		for_each_ports(i) {
			value = readl_relaxed(mbus_ctrl_base +
					      MBUS_MAST_CFG0_REG(i));
			value >>= MBUS_WT_SHIFT;
			value &= MBUS_WT_MAX;
			size += sprintf(buf + size,
				"master%2u threshold0:%2u\n", i, value);
		}
		break;
	case MBUS_PORT_ACS:
		for_each_ports(i) {
			value = readl_relaxed(mbus_ctrl_base +
					      MBUS_MAST_CFG0_REG(i));
			value >>= MBUS_ACS_SHIFT;
			value &= MBUS_ACS_MAX;
			size += sprintf(buf + size,
					"master%2u accsess commands:%4u\n",
					i, value);
		}
		break;
	case MBUS_PORT_BWL0:
		 for_each_ports(i) {
			value = readl_relaxed(mbus_ctrl_base +
					      MBUS_MAST_CFG0_REG(i));
			value >>= MBUS_BWL0_SHIFT;
			value &= MBUS_BWL_MAX;
			size += sprintf(buf + size,
					"master%2u bandwidth limit0:%5u\n",
					i, value);
		}
		break;
	case MBUS_PORT_BWL1:
		for_each_ports(i) {
			value = readl_relaxed(mbus_ctrl_base +
					      MBUS_MAST_CFG1_REG(i));
			value >>= MBUS_BWL1_SHIFT;
			value &= MBUS_BWL_MAX;
			size += sprintf(buf + size,
					"master%2u bandwidth limit1:%5u\n",
					i, value);
		}
		break;
	case MBUS_PORT_BWL2:
		for_each_ports(i) {
			value = readl_relaxed(mbus_ctrl_base +
					      MBUS_MAST_CFG1_REG(i));
			value >>= MBUS_BWL2_SHIFT;
			value &= MBUS_BWL_MAX;
			size += sprintf(buf + size,
					"master%2u bandwidth limit2:%5u\n",
					i, value);
		}
		break;
	case MBUS_PORT_BWLEN:
		for_each_ports(i) {
			value = readl_relaxed(mbus_ctrl_base +
					      MBUS_MAST_CFG0_REG(i));
			value &= 1;
			size += sprintf(buf + size,
					"master%2u BWLimit_en:%1u\n",
					i, value);
		}
		break;
	case MBUS_PORT_ABS_BWLEN:
		for_each_ports(i) {
			value = readl_relaxed(mbus_ctrl_base +
					MBUS_MAST_ABS_BWL_REG(i));
			value >>= MBUS_ABS_BWLEN_SHIFT;
			value &= 1;
			size += sprintf(buf + size,
			"master%2u absolutely BWLimit_en:%1u\n", i, value);
		}
		break;
	case MBUS_PORT_ABS_BWL:
		for_each_ports(i) {
			value = readl_relaxed(mbus_ctrl_base +
					MBUS_MAST_ABS_BWL_REG(i));
			value >>= MBUS_ABS_BWL_SHIFT;
			value &= MBUS_ABS_BWL_MAX;
			size += sprintf(buf + size,
			"master%2u absolutely bandwidth limit:%5u\n", i, value);
		}
		break;
	case MBUS_PORT_BW_SATU:
		for_each_ports(i) {
			value = readl_relaxed(mbus_ctrl_base +
					MBUS_MAST_ABS_BWL_REG(i));
			value &= MBUS_BW_SATU_MAX;
			size += sprintf(buf + size,
			"master%2u bandwidth saturation:%5u\n", i, value);
		}
		break;
	default:
		/* programmer goofed */
		WARN_ON_ONCE(1);
		value = 0;
		break;
	}
	mutex_unlock(&data->update_lock);

	return size;
}

static ssize_t mbus_show_value(struct device *dev,
			       struct device_attribute *da, char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	unsigned int len;

	if (attr->index >= MBUS_PMU_MAX) {
		len = mbus_get_value(&hw_mbus_pmu, attr->index, buf);
		len = (len < PAGE_SIZE) ? len : PAGE_SIZE;
		return len;
	}

	return snprintf(buf, PAGE_SIZE, "%u\n",
			mbus_update_device(&hw_mbus_pmu, attr->index));
}

static unsigned int mbus_set_value(struct mbus_data *data, unsigned int index,
				   enum mbus_port port, unsigned int val)
{
	unsigned int value;

	mutex_lock(&data->update_lock);
	switch (index) {
	case MBUS_PORT_PRI:
		mbus_port_setpri(port, val);
		break;
	case MBUS_PORT_QOS:
		mbus_port_setqos(port, val);
		break;
	case MBUS_PORT_WT:
		mbus_port_setwt(port, val);
		break;
	case MBUS_PORT_ACS:
		mbus_port_setacs(port, val);
		break;
	case MBUS_PORT_BWL0:
		mbus_port_setbwl0(port, val);
		break;
	case MBUS_PORT_BWL1:
		mbus_port_setbwl1(port, val);
		break;
	case MBUS_PORT_BWL2:
		mbus_port_setbwl2(port, val);
		break;
	case MBUS_PORT_BWLEN:
		mbus_port_setbwlen(port, val);
		break;
	case MBUS_PORT_ABS_BWLEN:
		mbus_port_set_abs_bwlen(port, val);
		break;
	case MBUS_PORT_ABS_BWL:
		mbus_port_set_abs_bwl(port, val);
		break;
	case MBUS_PORT_BW_SATU:
		mbus_port_set_bw_saturation(port, val);
		break;
	default:
		/* programmer goofed */
		WARN_ON_ONCE(1);
		value = 0;
		break;
	}
	mutex_unlock(&data->update_lock);

	return 0;
}

static ssize_t mbus_store_value(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int nr = to_sensor_dev_attr(attr)->index;
	unsigned long port, val;
	unsigned char buffer[64];
	unsigned char *pbuf, *pbufi;
	int err;

	if (strlen(buf) >= 64) {
		dev_err(dev, "arguments out of range!\n");
		return -EINVAL;
	}

	while (*buf == ' ') /* find the first unblank character */
		buf++;
	strncpy(buffer, buf, strlen(buf));

	pbufi = buffer;
	while (*pbufi != ' ') /* find the first argument */
		pbufi++;
	*pbufi = 0x0;
	pbuf = (unsigned char *)buffer;
	err = kstrtoul(pbuf, 10, &port);
	if (err < 0)
		return err;
	if (port >= MBUS_PORTS_MAX) {
		dev_err(dev, "master is illegal\n");
		return -EINVAL;
	}

	pbuf = ++pbufi;
	while (*pbuf == ' ') /* remove extra space character */
		pbuf++;
	pbufi = pbuf;
	while ((*pbufi != ' ') && (*pbufi != '\n'))
		pbufi++;
	*pbufi = 0x0;

	err = kstrtoul(pbuf, 10, &val);
	if (err < 0)
		return err;

	mbus_set_value(&hw_mbus_pmu, nr,
		(enum mbus_port)port, (unsigned int)val);

	return count;
}

#define MBUS_SENSOR_DEVICE_ATTR_RO(name, index) \
	static SENSOR_DEVICE_ATTR(name, 0400, mbus_show_value, NULL, index)
#define MBUS_SENSOR_DEVICE_ATTR_RW(name, index) \
	static SENSOR_DEVICE_ATTR(name, 0644, mbus_show_value, mbus_store_value, index)
#define SENSOR_DEVICE_ATTR_PTR(name) &sensor_dev_attr_##name.dev_attr.attr

MBUS_SENSOR_DEVICE_ATTR_RO(pmu_cpuddr,     MBUS_PMU_CPU);
MBUS_SENSOR_DEVICE_ATTR_RO(pmu_mahb_ddr,   MBUS_PMU_MAHB);
MBUS_SENSOR_DEVICE_ATTR_RO(pmu_dma_ddr,    MBUS_PMU_DMA);
MBUS_SENSOR_DEVICE_ATTR_RO(pmu_ve_ddr,     MBUS_PMU_VE);
MBUS_SENSOR_DEVICE_ATTR_RO(pmu_ce_ddr,     MBUS_PMU_CE);
MBUS_SENSOR_DEVICE_ATTR_RO(pmu_de_ddr,     MBUS_PMU_DISP);
MBUS_SENSOR_DEVICE_ATTR_RO(pmu_csi0_ddr,   MBUS_PMU_CSI0);
MBUS_SENSOR_DEVICE_ATTR_RO(pmu_di_ddr,     MBUS_PMU_DI);
MBUS_SENSOR_DEVICE_ATTR_RO(pmu_csi1_ddr,   MBUS_PMU_CSI1);
MBUS_SENSOR_DEVICE_ATTR_RO(pmu_g2d_ddr,    MBUS_PMU_G2D);
MBUS_SENSOR_DEVICE_ATTR_RO(pmu_ve1_ddr,    MBUS_PMU_VE1);
MBUS_SENSOR_DEVICE_ATTR_RO(pmu_iommu_ddr,  MBUS_PMU_IOMMU);
MBUS_SENSOR_DEVICE_ATTR_RO(pmu_gpuddr,    MBUS_PMU_GPU);
MBUS_SENSOR_DEVICE_ATTR_RO(pmu_totddr,     MBUS_PMU_TOTAL);
MBUS_SENSOR_DEVICE_ATTR_RW(port_prio,      MBUS_PORT_PRI);
MBUS_SENSOR_DEVICE_ATTR_RW(port_qos,       MBUS_PORT_QOS);
MBUS_SENSOR_DEVICE_ATTR_RW(port_watt,      MBUS_PORT_WT);
MBUS_SENSOR_DEVICE_ATTR_RW(port_acs,       MBUS_PORT_ACS);
MBUS_SENSOR_DEVICE_ATTR_RW(port_bwl0,      MBUS_PORT_BWL0);
MBUS_SENSOR_DEVICE_ATTR_RW(port_bwl1,      MBUS_PORT_BWL1);
MBUS_SENSOR_DEVICE_ATTR_RW(port_bwl2,      MBUS_PORT_BWL2);
MBUS_SENSOR_DEVICE_ATTR_RW(port_bwlen,     MBUS_PORT_BWLEN);
MBUS_SENSOR_DEVICE_ATTR_RW(port_abs_bwlen, MBUS_PORT_ABS_BWLEN);
MBUS_SENSOR_DEVICE_ATTR_RW(port_abs_bwl,   MBUS_PORT_ABS_BWL);
MBUS_SENSOR_DEVICE_ATTR_RW(port_bw_satu,   MBUS_PORT_BW_SATU);

/* pointers to created device attributes */
static struct attribute *mbus_attributes[] = {
	SENSOR_DEVICE_ATTR_PTR(pmu_cpuddr),
	SENSOR_DEVICE_ATTR_PTR(pmu_mahb_ddr),
	SENSOR_DEVICE_ATTR_PTR(pmu_dma_ddr),
	SENSOR_DEVICE_ATTR_PTR(pmu_ve_ddr),
	SENSOR_DEVICE_ATTR_PTR(pmu_ce_ddr),
	SENSOR_DEVICE_ATTR_PTR(pmu_de_ddr),
	SENSOR_DEVICE_ATTR_PTR(pmu_csi0_ddr),
	SENSOR_DEVICE_ATTR_PTR(pmu_di_ddr),
	SENSOR_DEVICE_ATTR_PTR(pmu_csi1_ddr),
	SENSOR_DEVICE_ATTR_PTR(pmu_g2d_ddr),
	SENSOR_DEVICE_ATTR_PTR(pmu_ve1_ddr),
	SENSOR_DEVICE_ATTR_PTR(pmu_iommu_ddr),
	SENSOR_DEVICE_ATTR_PTR(pmu_gpuddr),
	SENSOR_DEVICE_ATTR_PTR(pmu_totddr),
	SENSOR_DEVICE_ATTR_PTR(port_prio),
	SENSOR_DEVICE_ATTR_PTR(port_qos),
	SENSOR_DEVICE_ATTR_PTR(port_watt),
	SENSOR_DEVICE_ATTR_PTR(port_acs),
	SENSOR_DEVICE_ATTR_PTR(port_bwl0),
	SENSOR_DEVICE_ATTR_PTR(port_bwl1),
	SENSOR_DEVICE_ATTR_PTR(port_bwl2),
	SENSOR_DEVICE_ATTR_PTR(port_bwlen),
	SENSOR_DEVICE_ATTR_PTR(port_abs_bwlen),
	SENSOR_DEVICE_ATTR_PTR(port_abs_bwl),
	SENSOR_DEVICE_ATTR_PTR(port_bw_satu),
	NULL,
};

static struct attribute_group mbus_group = {
	.attrs = mbus_attributes,
};

static const struct attribute_group *mbus_groups[] = {
	&mbus_group,
	NULL,
};

static int mbus_pmu_probe(struct platform_device *pdev)
{
	int ret;

	hw_mbus_pmu.hwmon_dev = devm_hwmon_device_register_with_groups(
				&pdev->dev, "mbus_pmu", NULL, mbus_groups);
	if (IS_ERR(hw_mbus_pmu.hwmon_dev)) {
		ret = PTR_ERR(hw_mbus_pmu.hwmon_dev);
		goto out_err;
	}

	hw_mbus_pmu.last_updated = 0;
	hw_mbus_pmu.valid = 0;
	mutex_init(&hw_mbus_pmu.update_lock);

	return 0;

out_err:
	dev_err(&(pdev->dev), "probed failed\n");
	sysfs_remove_group(&pdev->dev.kobj, &mbus_group);

	return ret;
}

static int mbus_pmu_remove(struct platform_device *pdev)
{
	hwmon_device_unregister(hw_mbus_pmu.hwmon_dev);
	sysfs_remove_group(&pdev->dev.kobj, &mbus_group);

	return 0;
}

#ifdef CONFIG_PM
static int sunxi_mbus_suspend(struct device *dev)
{
	dev_info(dev, "suspend okay\n");

	return 0;
}

static int sunxi_mbus_resume(struct device *dev)
{
	dev_info(dev, "resume okay\n");

	return 0;
}

static const struct dev_pm_ops sunxi_mbus_pm_ops = {
	.suspend = sunxi_mbus_suspend,
	.resume = sunxi_mbus_resume,
};

#define SUNXI_MBUS_PM_OPS (&sunxi_mbus_pm_ops)
#else
#define SUNXI_MBUS_PM_OPS NULL
#endif

static struct platform_driver mbus_pmu_driver = {
	.driver = {
		.name   = DRIVER_NAME_PMU,
		.owner  = THIS_MODULE,
		.pm     = SUNXI_MBUS_PM_OPS,
		.of_match_table = sunxi_mbus_matches,
	},
	.probe = mbus_pmu_probe,
	.remove = mbus_pmu_remove,
};

static int __init mbus_pmu_init(void)
{
	int ret;

	ret = platform_driver_register(&mbus_pmu_driver);
	if (ret) {
		pr_err("register sunxi mbus platform driver failed\n");
		goto drv_err;
	}

	return ret;

drv_err:
	platform_driver_unregister(&mbus_pmu_driver);

	return -EINVAL;
}

early_initcall(mbus_init);
device_initcall(mbus_pmu_init);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("SUNXI SUN50IW9 MBUS support");
MODULE_AUTHOR("xiafeng");
Module_Version("1.0.1");
