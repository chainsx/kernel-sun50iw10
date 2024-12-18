/* SPDX-License-Identifier: GPL-2.0-only
 * Based on drivers/input/sensor/sunxi_gpadc.c
 *
 * Copyright (C) 2016 Allwinner.
 * fuzhaoke <fuzhaoke@allwinnertech.com>
 *
 * SUNXI GPADC Controller Driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <linux/timer.h>
#include <linux/clk.h>
#include <linux/irq.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/pm.h>
#include <linux/pm_wakeirq.h>
#include <linux/pm_runtime.h>
#include <linux/workqueue.h>
#include "sunxi_gpadc.h"
#include <linux/sched.h>
#include <linux/clkdev.h>
#include <linux/clk-provider.h>
#include <linux/reset.h>

#if IS_ENABLED(CONFIG_IIO)
#include <linux/iio/iio.h>
#include <linux/iio/machine.h>
#include <linux/iio/driver.h>
#include <linux/regmap.h>
#endif

static u32 debug_mask = 1;
#define dprintk(level_mask, fmt, ...)				\
do {								\
	if (unlikely(debug_mask & level_mask))			\
		pr_info(fmt, ##__VA_ARGS__);	\
} while (0)

static struct sunxi_gpadc *sunxi_gpadc;
static struct status_reg  status_para;
static struct vol_reg     vol_para;
static struct sr_reg      sr_para;
static struct filter_reg  filter_para;
static struct channel_reg  channel_para;
u32 key_vol[VOL_NUM];
u8 filter_cnt = 5;
u8 channel;
void __iomem *vaddr;

static unsigned char keypad_mapindex[128] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0,		/* key 1, 0-8 */
	1, 1, 1, 1, 1,				/* key 2, 9-13 */
	2, 2, 2, 2, 2, 2,			/* key 3, 14-19 */
	3, 3, 3, 3, 3, 3,			/* key 4, 20-25 */
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,	/* key 5, 26-36 */
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,	/* key 6, 37-39 */
	6, 6, 6, 6, 6, 6, 6, 6, 6,		/* key 7, 40-49 */
	7, 7, 7, 7, 7, 7, 7			/* key 8, 50-63 */
};

static inline void sunxi_gpadc_save_regs(struct sunxi_gpadc *sunxi_gpadc)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sunxi_gpadc_regs_offset); i++)
		sunxi_gpadc->regs_backup[i] = readl(sunxi_gpadc->reg_base + sunxi_gpadc_regs_offset[i]);
}

static inline void sunxi_gpadc_restore_regs(struct sunxi_gpadc *sunxi_gpadc)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sunxi_gpadc_regs_offset); i++)
		writel(sunxi_gpadc->regs_backup[i], sunxi_gpadc->reg_base + sunxi_gpadc_regs_offset[i]);
}

#ifdef CONFIG_ARCH_SUN8IW18
static u32 sunxi_gpadc_check_vin(void)
{
	u32 reg_val = 0, ldoa;

	reg_val = readl(vaddr);

	if (reg_val > 0) {
		if ((reg_val & 0x80) == 0x80)
			ldoa = (reg_val & 0x7f) + 1800;
		else
			ldoa = (reg_val & 0x7f) + 1700;
	} else
		return 0;
	return ldoa;
}
#endif

u32 sunxi_gpadc_sample_rate_read(void __iomem *reg_base, u32 clk_in)
{
	u32 div, round_clk;

	div = readl(reg_base + GP_SR_REG);
	div = div >> 16;
	round_clk = clk_in / (div + 1);
//	round_clk = (div + 1) / clk_in;

	return round_clk;
}


/* clk_in: source clock, round_clk: sample rate */
void sunxi_gpadc_sample_rate_set(void __iomem *reg_base, u32 clk_in,
		u32 round_clk)
{
	u32 div, reg_val;

	if (round_clk > clk_in)
		pr_err("%s, invalid round clk!\n", __func__);
	div = clk_in / round_clk - 1;
	reg_val = readl(reg_base + GP_SR_REG);
	reg_val &= ~GP_SR_CON;
	reg_val |= (div << 16);
	writel(reg_val, reg_base + GP_SR_REG);
}

void sunxi_gpadc_ctrl_set(void __iomem *reg_base, u32 ctrl_para)
{
	u32 reg_val;

	reg_val = readl(reg_base + GP_CTRL_REG);
	reg_val |= ctrl_para;
	writel(reg_val, reg_base + GP_CTRL_REG);
}

static u32 sunxi_gpadc_ch_select(void __iomem *reg_base, enum gp_channel_id id)
{
	u32 reg_val;

	reg_val = readl(reg_base + GP_CS_EN_REG);
	switch (id) {
	case GP_CH_0:
		reg_val |= GP_CH0_SELECT;
		break;
	case GP_CH_1:
		reg_val |= GP_CH1_SELECT;
		break;
	case GP_CH_2:
		reg_val |= GP_CH2_SELECT;
		break;
	case GP_CH_3:
		reg_val |= GP_CH3_SELECT;
		break;
	case GP_CH_4:
		reg_val |= GP_CH4_SELECT;
		break;
	case GP_CH_5:
		reg_val |= GP_CH5_SELECT;
		break;
	case GP_CH_6:
		reg_val |= GP_CH6_SELECT;
		break;
	case GP_CH_7:
		reg_val |= GP_CH7_SELECT;
		break;
	default:
		pr_err("%s, invalid channel id!", __func__);
		return -EINVAL;
	}
	writel(reg_val, reg_base + GP_CS_EN_REG);

	return 0;
}

static u32 sunxi_gpadc_ch_deselect(void __iomem *reg_base,
		enum gp_channel_id id)
{
	u32 reg_val;

	reg_val = readl(reg_base + GP_CS_EN_REG);
	switch (id) {
	case GP_CH_0:
		reg_val &= ~GP_CH0_SELECT;
		break;
	case GP_CH_1:
		reg_val &= ~GP_CH1_SELECT;
		break;
	case GP_CH_2:
		reg_val &= ~GP_CH2_SELECT;
		break;
	case GP_CH_3:
		reg_val &= ~GP_CH3_SELECT;
		break;
	case GP_CH_4:
		reg_val &= ~GP_CH4_SELECT;
		break;
	case GP_CH_5:
		reg_val &= ~GP_CH5_SELECT;
		break;
	case GP_CH_6:
		reg_val &= ~GP_CH6_SELECT;
		break;
	case GP_CH_7:
		reg_val &= ~GP_CH7_SELECT;
		break;
	default:
		pr_err("%s, invalid channel id!", __func__);
		return -EINVAL;
	}
	writel(reg_val, reg_base + GP_CS_EN_REG);

	return 0;
}

static u32 sunxi_gpadc_ch_cmp_low(void __iomem *reg_base, enum gp_channel_id id,
							u32 low_uv)
{
	u32 reg_val = 0, low = 0, unit = 0;

	/* analog voltage range 0~1.8v, 12bits sample rate, unit=1.8v/(2^12) */
	unit = VOL_RANGE / 4096; /* 12bits sample rate */
	low = low_uv / unit;
	if (low > VOL_VALUE_MASK)
		low = VOL_VALUE_MASK;

	switch (id) {
	case GP_CH_0:
		reg_val = readl(reg_base + GP_CH0_CMP_DATA_REG);
		reg_val &= ~VOL_VALUE_MASK;
		reg_val |= (low & VOL_VALUE_MASK);
		writel(reg_val, reg_base + GP_CH0_CMP_DATA_REG);
		break;
	case GP_CH_1:
		reg_val = readl(reg_base + GP_CH1_CMP_DATA_REG);
		reg_val &= ~VOL_VALUE_MASK;
		reg_val |= (low & VOL_VALUE_MASK);
		writel(reg_val, reg_base + GP_CH1_CMP_DATA_REG);
		break;
	case GP_CH_2:
		reg_val = readl(reg_base + GP_CH2_CMP_DATA_REG);
		reg_val &= ~VOL_VALUE_MASK;
		reg_val |= (low & VOL_VALUE_MASK);
		writel(reg_val, reg_base + GP_CH2_CMP_DATA_REG);
		break;
	case GP_CH_3:
		reg_val = readl(reg_base + GP_CH3_CMP_DATA_REG);
		reg_val &= ~VOL_VALUE_MASK;
		reg_val |= (low & VOL_VALUE_MASK);
		writel(reg_val, reg_base + GP_CH3_CMP_DATA_REG);
		break;
	case GP_CH_4:
		reg_val = readl(reg_base + GP_CH4_CMP_DATA_REG);
		reg_val &= ~VOL_VALUE_MASK;
		reg_val |= (low & VOL_VALUE_MASK);
		writel(reg_val, reg_base + GP_CH4_CMP_DATA_REG);
		break;
	case GP_CH_5:
		reg_val = readl(reg_base + GP_CH5_CMP_DATA_REG);
		reg_val &= ~VOL_VALUE_MASK;
		reg_val |= (low & VOL_VALUE_MASK);
		writel(reg_val, reg_base + GP_CH5_CMP_DATA_REG);
		break;
	case GP_CH_6:
		reg_val = readl(reg_base + GP_CH6_CMP_DATA_REG);
		reg_val &= ~VOL_VALUE_MASK;
		reg_val |= (low & VOL_VALUE_MASK);
		writel(reg_val, reg_base + GP_CH6_CMP_DATA_REG);
		break;
	case GP_CH_7:
		reg_val = readl(reg_base + GP_CH7_CMP_DATA_REG);
		reg_val &= ~VOL_VALUE_MASK;
		reg_val |= (low & VOL_VALUE_MASK);
		writel(reg_val, reg_base + GP_CH7_CMP_DATA_REG);
		break;

	default:
		pr_err("%s, invalid channel id!", __func__);
		return -EINVAL;
	}

	return 0;
}
static u32 sunxi_gpadc_ch_cmp_hig(void __iomem *reg_base, enum gp_channel_id id,
								u32 hig_uv)
{
	u32 reg_val = 0, hig = 0, unit = 0;

	/* anolog voltage range 0~1.8v, 12bits sample rate, unit=1.8v/(2^12) */
	unit = VOL_RANGE / 4096; /* 12bits sample rate */
	hig = hig_uv / unit;
	if (hig > VOL_VALUE_MASK)
		hig = VOL_VALUE_MASK;

	switch (id) {
	case GP_CH_0:
		reg_val = readl(reg_base + GP_CH0_CMP_DATA_REG);
		reg_val &= ~(VOL_VALUE_MASK << 16);
		reg_val |= (hig & VOL_VALUE_MASK) << 16;
		writel(reg_val, reg_base + GP_CH0_CMP_DATA_REG);
		break;
	case GP_CH_1:
		reg_val = readl(reg_base + GP_CH1_CMP_DATA_REG);
		reg_val &= ~(VOL_VALUE_MASK << 16);
		reg_val |= (hig & VOL_VALUE_MASK) << 16;
		writel(reg_val, reg_base + GP_CH1_CMP_DATA_REG);
		break;
	case GP_CH_2:
		reg_val = readl(reg_base + GP_CH2_CMP_DATA_REG);
		reg_val &= ~(VOL_VALUE_MASK << 16);
		reg_val |= (hig & VOL_VALUE_MASK) << 16;
		writel(reg_val, reg_base + GP_CH2_CMP_DATA_REG);
		break;
	case GP_CH_3:
		reg_val = readl(reg_base + GP_CH3_CMP_DATA_REG);
		reg_val &= ~(VOL_VALUE_MASK << 16);
		reg_val |= (hig & VOL_VALUE_MASK) << 16;
		writel(reg_val, reg_base + GP_CH3_CMP_DATA_REG);
		break;
	case GP_CH_4:
		reg_val = readl(reg_base + GP_CH4_CMP_DATA_REG);
		reg_val &= ~(VOL_VALUE_MASK << 16);
		reg_val |= (hig & VOL_VALUE_MASK) << 16;
		writel(reg_val, reg_base + GP_CH4_CMP_DATA_REG);
		break;
	case GP_CH_5:
		reg_val = readl(reg_base + GP_CH5_CMP_DATA_REG);
		reg_val &= ~(VOL_VALUE_MASK << 16);
		reg_val |= (hig & VOL_VALUE_MASK) << 16;
		writel(reg_val, reg_base + GP_CH5_CMP_DATA_REG);
		break;
	case GP_CH_6:
		reg_val = readl(reg_base + GP_CH6_CMP_DATA_REG);
		reg_val &= ~(VOL_VALUE_MASK << 16);
		reg_val |= (hig & VOL_VALUE_MASK) << 16;
		writel(reg_val, reg_base + GP_CH6_CMP_DATA_REG);
		break;
	case GP_CH_7:
		reg_val = readl(reg_base + GP_CH7_CMP_DATA_REG);
		reg_val &= ~(VOL_VALUE_MASK << 16);
		reg_val |= (hig & VOL_VALUE_MASK) << 16;
		writel(reg_val, reg_base + GP_CH7_CMP_DATA_REG);
		break;

	default:
		pr_err("%s, invalid channel id!", __func__);
		return -EINVAL;
	}

	return 0;
}

static u32 sunxi_gpadc_cmp_select(void __iomem *reg_base, enum gp_channel_id id)
{
	u32 reg_val;

	reg_val = readl(reg_base + GP_CS_EN_REG);
	switch (id) {
	case GP_CH_0:
		reg_val |= GP_CH0_CMP_EN;
		break;
	case GP_CH_1:
		reg_val |= GP_CH1_CMP_EN;
		break;
	case GP_CH_2:
		reg_val |= GP_CH2_CMP_EN;
		break;
	case GP_CH_3:
		reg_val |= GP_CH3_CMP_EN;
		break;
	case GP_CH_4:
		reg_val |= GP_CH4_CMP_EN;
		break;
	case GP_CH_5:
		reg_val |= GP_CH5_CMP_EN;
		break;
	case GP_CH_6:
		reg_val |= GP_CH6_CMP_EN;
		break;
	case GP_CH_7:
		reg_val |= GP_CH7_CMP_EN;
		break;
	default:
		pr_err("%s, invalid value!", __func__);
		return -EINVAL;
	}
	writel(reg_val, reg_base + GP_CS_EN_REG);

	return 0;
}

static u32 sunxi_gpadc_cmp_deselect(void __iomem *reg_base,
		enum gp_channel_id id)
{
	u32 reg_val;

	reg_val = readl(reg_base + GP_CS_EN_REG);
	switch (id) {
	case GP_CH_0:
		reg_val &= ~GP_CH0_CMP_EN;
		break;
	case GP_CH_1:
		reg_val &= ~GP_CH1_CMP_EN;
		break;
	case GP_CH_2:
		reg_val &= ~GP_CH2_CMP_EN;
		break;
	case GP_CH_3:
		reg_val &= ~GP_CH3_CMP_EN;
		break;
	case GP_CH_4:
		reg_val &= ~GP_CH4_CMP_EN;
		break;
	case GP_CH_5:
		reg_val &= ~GP_CH5_CMP_EN;
		break;
	case GP_CH_6:
		reg_val &= ~GP_CH6_CMP_EN;
		break;
	case GP_CH_7:
		reg_val &= ~GP_CH7_CMP_EN;
		break;

	default:
		pr_err("%s, invalid value!", __func__);
		return -EINVAL;
	}
	writel(reg_val, reg_base + GP_CS_EN_REG);

	return 0;
}

static u32 sunxi_gpadc_mode_select(void __iomem *reg_base,
		enum gp_select_mode mode)
{
	u32 reg_val;

	reg_val = readl(reg_base + GP_CTRL_REG);
	reg_val &= ~GP_MODE_SELECT;
	reg_val |= (mode << 18);
	writel(reg_val, reg_base + GP_CTRL_REG);

	return 0;
}

/* enable gpadc function, true:enable, false:disable */
static void sunxi_gpadc_enable(void __iomem *reg_base, bool onoff)
{
	u32 reg_val = 0;

	reg_val = readl(reg_base + GP_CTRL_REG);
	if (true == onoff)
		reg_val |= GP_ADC_EN;
	else
		reg_val &= ~GP_ADC_EN;
	writel(reg_val, reg_base + GP_CTRL_REG);
}

static void sunxi_gpadc_calibration_enable(void __iomem *reg_base)
{
	u32 reg_val;

	reg_val = readl(reg_base + GP_CTRL_REG);
	reg_val |= GP_CALIBRATION_ENABLE;
	writel(reg_val, reg_base + GP_CTRL_REG);

}

static u32 sunxi_enable_lowirq_ch_select(void __iomem *reg_base,
		enum gp_channel_id id)
{
	u32 reg_val;

	reg_val = readl(reg_base + GP_DATAL_INTC_REG);
	switch (id) {
	case GP_CH_0:
		reg_val |= GP_CH0_SELECT;
		break;
	case GP_CH_1:
		reg_val |= GP_CH1_SELECT;
		break;
	case GP_CH_2:
		reg_val |= GP_CH2_SELECT;
		break;
	case GP_CH_3:
		reg_val |= GP_CH3_SELECT;
		break;
	case GP_CH_4:
		reg_val |= GP_CH4_SELECT;
		break;
	case GP_CH_5:
		reg_val |= GP_CH5_SELECT;
		break;
	case GP_CH_6:
		reg_val |= GP_CH6_SELECT;
		break;
	case GP_CH_7:
		reg_val |= GP_CH7_SELECT;
		break;
	default:
		pr_err("%s, invalid channel id!", __func__);
		return -EINVAL;
	}
	writel(reg_val, reg_base + GP_DATAL_INTC_REG);

	return 0;
}

static u32 sunxi_disable_lowirq_ch_select(void __iomem *reg_base,
		enum gp_channel_id id)
{
	u32 reg_val;

	reg_val = readl(reg_base + GP_DATAL_INTC_REG);
	switch (id) {
	case GP_CH_0:
		reg_val &= ~GP_CH0_SELECT;
		break;
	case GP_CH_1:
		reg_val &= ~GP_CH1_SELECT;
		break;
	case GP_CH_2:
		reg_val &= ~GP_CH2_SELECT;
		break;
	case GP_CH_3:
		reg_val &= ~GP_CH3_SELECT;
		break;
	case GP_CH_4:
		reg_val &= ~GP_CH4_SELECT;
		break;
	case GP_CH_5:
		reg_val &= ~GP_CH5_SELECT;
		break;
	case GP_CH_6:
		reg_val &= ~GP_CH6_SELECT;
		break;
	case GP_CH_7:
		reg_val &= ~GP_CH7_SELECT;
		break;
	default:
		pr_err("%s, invalid channel id!", __func__);
		return -EINVAL;
	}
	writel(reg_val, reg_base + GP_DATAL_INTC_REG);

	return 0;
}

static u32 sunxi_enable_higirq_ch_select(void __iomem *reg_base,
		enum gp_channel_id id)
{
	u32 reg_val = 0;


	reg_val = readl(reg_base + GP_DATAH_INTC_REG);
	switch (id) {
	case GP_CH_0:
		reg_val |= GP_CH0_SELECT;
		break;
	case GP_CH_1:
		reg_val |= GP_CH1_SELECT;
		break;
	case GP_CH_2:
		reg_val |= GP_CH2_SELECT;
		break;
	case GP_CH_3:
		reg_val |= GP_CH3_SELECT;
		break;
	case GP_CH_4:
		reg_val |= GP_CH4_SELECT;
		break;
	case GP_CH_5:
		reg_val |= GP_CH5_SELECT;
		break;
	case GP_CH_6:
		reg_val |= GP_CH6_SELECT;
		break;
	case GP_CH_7:
		reg_val |= GP_CH7_SELECT;
		break;
	default:
		pr_err("%s, invalid channel id!", __func__);
		return -EINVAL;
	}
	writel(reg_val, reg_base + GP_DATAH_INTC_REG);

	return 0;
}

static u32 sunxi_disable_higirq_ch_select(void __iomem *reg_base,
		enum gp_channel_id id)
{
	u32 reg_val = 0;

	reg_val = readl(reg_base + GP_DATAH_INTC_REG);
	switch (id) {
	case GP_CH_0:
		reg_val &= ~GP_CH0_SELECT;
		break;
	case GP_CH_1:
		reg_val &= ~GP_CH1_SELECT;
		break;
	case GP_CH_2:
		reg_val &= ~GP_CH2_SELECT;
		break;
	case GP_CH_3:
		reg_val &= ~GP_CH3_SELECT;
		break;
	case GP_CH_4:
		reg_val &= ~GP_CH4_SELECT;
		break;
	case GP_CH_5:
		reg_val &= ~GP_CH5_SELECT;
		break;
	case GP_CH_6:
		reg_val &= ~GP_CH6_SELECT;
		break;
	case GP_CH_7:
		reg_val &= ~GP_CH7_SELECT;
		break;
	default:
		pr_err("%s, invalid channel id!", __func__);
		return -EINVAL;
	}
	writel(reg_val, reg_base + GP_DATAH_INTC_REG);

	return 0;
}

static u32 sunxi_gpadc_channel_id(enum gp_channel_id id)
{
	u32 channel_id;

	switch (id) {
	case GP_CH_0:
		channel_id = CHANNEL_0_SELECT;
		break;
	case GP_CH_1:
		channel_id = CHANNEL_1_SELECT;
		break;
	case GP_CH_2:
		channel_id = CHANNEL_2_SELECT;
		break;
	case GP_CH_3:
		channel_id = CHANNEL_3_SELECT;
		break;
	case GP_CH_4:
		channel_id = CHANNEL_4_SELECT;
		break;
	case GP_CH_5:
		channel_id = CHANNEL_5_SELECT;
		break;
	case GP_CH_6:
		channel_id = CHANNEL_6_SELECT;
		break;
	case GP_CH_7:
		channel_id = CHANNEL_7_SELECT;
		break;
	default:
		pr_err("%s,channel id select fail", __func__);
		return -EINVAL;
	}

	return channel_id;
}

static void sunxi_gpadc_enable_irq(void __iomem *reg_base)
{
	u32 reg_val;

	reg_val = readl(reg_base + GP_FIFO_INTC_REG);
	reg_val |= FIFO_DATA_IRQ_EN;
	writel(reg_val, reg_base + GP_FIFO_INTC_REG);
}

/* Need to clarify usage issues */
/*
static u32 sunxi_enable_irq_ch_select(void __iomem *reg_base,
		enum gp_channel_id id)
{
	u32 reg_val;

	reg_val = readl(reg_base + GP_DATA_INTC_REG);
	switch (id) {
	case GP_CH_0:
		reg_val |= GP_CH0_SELECT;
		break;
	case GP_CH_1:
		reg_val |= GP_CH1_SELECT;
		break;
	case GP_CH_2:
		reg_val |= GP_CH2_SELECT;
		break;
	case GP_CH_3:
		reg_val |= GP_CH3_SELECT;
		break;
	case GP_CH_4:
		reg_val |= GP_CH4_SELECT;
		break;
	case GP_CH_5:
		reg_val |= GP_CH5_SELECT;
		break;
	case GP_CH_6:
		reg_val |= GP_CH6_SELECT;
		break;
	case GP_CH_7:
		reg_val |= GP_CH7_SELECT;
		break;
	default:
		pr_err("%s, invalid channel id!", __func__);
		return -EINVAL;
	}
	writel(reg_val, reg_base + GP_DATA_INTC_REG);

	return 0;
}

static u32 sunxi_disable_irq_ch_select(void __iomem *reg_base,
		enum gp_channel_id id)
{
	u32 reg_val;

	reg_val = readl(reg_base + GP_DATA_INTC_REG);
	switch (id) {
	case GP_CH_0:
		reg_val &= ~GP_CH0_SELECT;
		break;
	case GP_CH_1:
		reg_val &= ~GP_CH1_SELECT;
		break;
	case GP_CH_2:
		reg_val &= ~GP_CH2_SELECT;
		break;
	case GP_CH_3:
		reg_val &= ~GP_CH3_SELECT;
		break;
	case GP_CH_4:
		reg_val &= ~GP_CH4_SELECT;
		break;
	case GP_CH_5:
		reg_val &= ~GP_CH5_SELECT;
		break;
	case GP_CH_6:
		reg_val &= ~GP_CH6_SELECT;
		break;
	case GP_CH_7:
		reg_val &= ~GP_CH7_SELECT;
		break;
	default:
		pr_err("%s, invalid channel id!", __func__);
		return -EINVAL;
	}
	writel(reg_val, reg_base + GP_DATA_INTC_REG);

	return 0;
}
*/

static u32 sunxi_ch_lowirq_status(void __iomem *reg_base)
{
	return readl(reg_base + GP_DATAL_INTS_REG);
}

static void sunxi_ch_lowirq_clear_flags(void __iomem *reg_base, u32 flags)
{
	writel(flags, reg_base + GP_DATAL_INTS_REG);
}

static u32 sunxi_ch_higirq_status(void __iomem *reg_base)
{
	return readl(reg_base + GP_DATAH_INTS_REG);
}

static void sunxi_ch_higirq_clear_flags(void __iomem *reg_base, u32 flags)
{
	writel(flags, reg_base + GP_DATAH_INTS_REG);
}

static u32 sunxi_ch_irq_status(void __iomem *reg_base)
{
	return readl(reg_base + GP_DATA_INTS_REG);
}

static void sunxi_ch_irq_clear_flags(void __iomem *reg_base, u32 flags)
{
	writel(flags, reg_base + GP_DATA_INTS_REG);
}

static u32 sunxi_gpadc_read_ch_lowirq_enable(void __iomem *reg_base)
{
	return readl(reg_base + GP_DATAL_INTC_REG);
}

static u32 sunxi_gpadc_read_ch_higirq_enable(void __iomem *reg_base)
{
	return readl(reg_base + GP_DATAH_INTC_REG);
}

static u32 sunxi_gpadc_read_ch_irq_enable(void __iomem *reg_base)
{
	return readl(reg_base + GP_DATA_INTC_REG);
}

static u32 sunxi_gpadc_read_data(void __iomem *reg_base, enum gp_channel_id id)
{
	switch (id) {
	case GP_CH_0:
		return readl(reg_base + GP_CH0_DATA_REG) & GP_CH0_DATA_MASK;
	case GP_CH_1:
		return readl(reg_base + GP_CH1_DATA_REG) & GP_CH1_DATA_MASK;
	case GP_CH_2:
		return readl(reg_base + GP_CH2_DATA_REG) & GP_CH2_DATA_MASK;
	case GP_CH_3:
		return readl(reg_base + GP_CH3_DATA_REG) & GP_CH3_DATA_MASK;
	case GP_CH_4:
		return readl(reg_base + GP_CH4_DATA_REG) & GP_CH4_DATA_MASK;
	case GP_CH_5:
		return readl(reg_base + GP_CH5_DATA_REG) & GP_CH5_DATA_MASK;
	case GP_CH_6:
		return readl(reg_base + GP_CH6_DATA_REG) & GP_CH6_DATA_MASK;
	case GP_CH_7:
		return readl(reg_base + GP_CH7_DATA_REG) & GP_CH7_DATA_MASK;
	default:
		pr_err("%s, invalid channel id!", __func__);
		return -EINVAL;
	}
}

static int sunxi_gpadc_input_event_set(struct input_dev *input_dev,
		enum gp_channel_id id)
{
	int i;

	if (!input_dev) {
		pr_err("%s:input_dev: not enough memory for input device\n",
				__func__);
		return -ENOMEM;
	}

	switch (id) {
	case GP_CH_0:
#ifdef CONFIG_ARCH_SUN8IW18
	case GP_CH_3:
		input_dev->evbit[0] = BIT_MASK(EV_KEY);
#else
		input_dev->evbit[0] = BIT_MASK(EV_KEY)|BIT_MASK(EV_REP);
#endif
		for (i = 0; i < KEY_MAX_CNT; i++)
			set_bit(sunxi_gpadc->scankeycodes[i],
					input_dev->keybit);
		break;

	case GP_CH_1:
	case GP_CH_2:
#ifndef CONFIG_ARCH_SUN8IW18
	case GP_CH_3:
#endif
	case GP_CH_4:
	case GP_CH_5:
	case GP_CH_6:
	case GP_CH_7:
		input_dev->evbit[0] = BIT_MASK(EV_MSC);
		set_bit(EV_MSC, input_dev->evbit);
		set_bit(MSC_SCAN, input_dev->mscbit);
		break;
	default:
		pr_err("%s, invalid channel id!", __func__);
		return -ENOMEM;
	}

	return 0;
}

static int sunxi_gpadc_input_register(struct sunxi_gpadc *sunxi_gpadc, int id)
{
	static struct input_dev *input_dev;
	int ret = -1;

	input_dev = input_allocate_device();
	if (!input_dev) {
		pr_err("input_dev: not enough memory for input device\n");
		return -ENOMEM;
	}

	switch (id) {
	case GP_CH_0:
		input_dev->name = "sunxi-gpadc0";
		input_dev->phys = "sunxigpadc0/input0";
		break;
	case GP_CH_1:
		input_dev->name = "sunxi-gpadc1";
		input_dev->phys = "sunxigpadc1/input0";
		break;
	case GP_CH_2:
		input_dev->name = "sunxi-gpadc2";
		input_dev->phys = "sunxigpadc2/input0";
		break;
	case GP_CH_3:
		input_dev->name = "sunxi-gpadc3";
		input_dev->phys = "sunxigpadc3/input0";
		break;
	case GP_CH_4:
		input_dev->name = "sunxi-gpadc4";
		input_dev->phys = "sunxigpadc4/input0";
		break;
	case GP_CH_5:
		input_dev->name = "sunxi-gpadc5";
		input_dev->phys = "sunxigpadc5/input0";
		break;
	case GP_CH_6:
		input_dev->name = "sunxi-gpadc6";
		input_dev->phys = "sunxigpadc6/input0";
		break;
	case GP_CH_7:
		input_dev->name = "sunxi-gpadc7";
		input_dev->phys = "sunxigpadc7/input0";
		break;
	default:
		pr_err("%s, invalid channel id!", __func__);
		goto fail;
	}

	input_dev->id.bustype = BUS_HOST;
	input_dev->id.vendor = 0x0001;
	input_dev->id.product = 0x0001;
	input_dev->id.version = 0x0100;

	sunxi_gpadc_input_event_set(input_dev, id);
	sunxi_gpadc->input_gpadc[id] = input_dev;
	ret = input_register_device(sunxi_gpadc->input_gpadc[id]);
	if (ret) {
		pr_err("input register fail\n");
		goto fail1;
	}

	return 0;

fail1:
	input_unregister_device(sunxi_gpadc->input_gpadc[id]);
fail:
	input_free_device(sunxi_gpadc->input_gpadc[id]);
	return ret;
}

static int sunxi_key_init(struct sunxi_gpadc *sunxi_gpadc,
		struct platform_device *pdev)
{
	struct device_node *np = NULL;
	unsigned char i, j = 0;
	u32 vol, val;
	u32 set_vol[VOL_NUM];

	np = pdev->dev.of_node;
	if (of_property_read_u32(np, "key_cnt", &sunxi_gpadc->key_num)) {
		pr_err("%s: get key count failed", __func__);
		return -EBUSY;
	}
	pr_debug("%s key number = %d.\n", __func__, sunxi_gpadc->key_num);
	if (sunxi_gpadc->key_num < 1 || sunxi_gpadc->key_num > VOL_NUM) {
		pr_err("incorrect key number.\n");
		return -1;
	}
	for (i = 0; i < sunxi_gpadc->key_num; i++) {
		sprintf(sunxi_gpadc->key_name, "key%d_vol", i);
		if (of_property_read_u32(np, sunxi_gpadc->key_name, &vol)) {
			pr_err("%s:get%s err!\n", __func__,
					sunxi_gpadc->key_name);
			return -EBUSY;
		}
		key_vol[i] = vol;

		sprintf(sunxi_gpadc->key_name, "key%d_val", i);
		if (of_property_read_u32(np, sunxi_gpadc->key_name, &val)) {
			pr_err("%s:get%s err!\n", __func__,
					sunxi_gpadc->key_name);
			return -EBUSY;
		}
		sunxi_gpadc->scankeycodes[i] = val;
		pr_debug("%s: key%d vol= %d code= %d\n", __func__, i,
				key_vol[i], sunxi_gpadc->scankeycodes[i]);
	}
	key_vol[sunxi_gpadc->key_num] = MAXIMUM_INPUT_VOLTAGE;

	for (i = 0; i < sunxi_gpadc->key_num; i++)
		set_vol[i] = key_vol[i] + (key_vol[i+1] - key_vol[i])/2;

	for (i = 0; i < 128; i++) {
		if (i * SCALE_UNIT > set_vol[j])
			j++;
		keypad_mapindex[i] = j;
	}

	return 0;
}

static int sunxi_gpadc_input_register_setup(struct sunxi_gpadc *sunxi_gpadc)
{
	struct sunxi_config *config = NULL;
	int i;

	config = &sunxi_gpadc->gpadc_config;
	for (i = 0; i < sunxi_gpadc->channel_num; i++) {
		if (config->channel_select & sunxi_gpadc_channel_id(i))
			sunxi_gpadc_input_register(sunxi_gpadc, i);
	}

	return 0;
}

static int sunxi_gpadc_setup(struct platform_device *pdev,
					struct sunxi_gpadc *sunxi_gpadc)
{
	struct device_node *np = NULL;
	struct sunxi_config *gpadc_config = &sunxi_gpadc->gpadc_config;
	u32 val;
	int i;
	unsigned char name[30];

	np = pdev->dev.of_node;
	if (of_property_read_u32(np, "gpadc_sample_rate",
				&sunxi_gpadc->gpadc_sample_rate)) {
		pr_debug("%s: get sample rate failed\n", __func__);
		sunxi_gpadc->gpadc_sample_rate = DEFAULT_SR;
	} else {
		if (sunxi_gpadc->gpadc_sample_rate < MIN_SR ||
				sunxi_gpadc->gpadc_sample_rate > MAX_SR)
			sunxi_gpadc->gpadc_sample_rate = DEFAULT_SR;
	}

	if (of_property_read_u32(np, "channel_num", &sunxi_gpadc->channel_num)) {
		pr_err("%s: get channel num failed\n", __func__);
		return -EBUSY;
	}

	if (of_property_read_u32(np, "channel_select",
				&gpadc_config->channel_select)) {
		pr_err("%s: get channel set select failed\n", __func__);
		gpadc_config->channel_select = 0;
	}

	if (of_property_read_u32(np, "channel_data_select",
				&gpadc_config->channel_data_select)) {
		pr_err("%s: get channel data setlect failed\n", __func__);
		gpadc_config->channel_data_select = 0;
	}

	if (of_property_read_u32(np, "channel_compare_select",
				&gpadc_config->channel_compare_select)) {
		pr_err("%s: get channel compare select failed\n", __func__);
		gpadc_config->channel_compare_select = 0;
	}

	if (of_property_read_u32(np, "channel_cld_select",
				&gpadc_config->channel_cld_select)) {
		pr_err("%s: get channel compare low data select failed\n",
				__func__);
		gpadc_config->channel_cld_select = 0;
	}

	if (of_property_read_u32(np, "channel_chd_select",
				&gpadc_config->channel_chd_select)) {
		pr_err("%s: get channel compare hig data select failed\n",
				__func__);
		gpadc_config->channel_chd_select = 0;
	}

	if (of_property_read_u32(np, "channel_scan_data",
				&gpadc_config->channel_scan_data)) {
		pr_err("%s: get channel scan data failed\n",
				__func__);
		gpadc_config->channel_scan_data = 0;
	}

	for (i = 0; i < sunxi_gpadc->channel_num; i++) {
		sprintf(name, "channel%d_compare_lowdata", i);
		if (of_property_read_u32(np, name, &val)) {
			pr_err("%s:get %s err!\n", __func__, name);
			val = 0;
		}
		gpadc_config->channel_compare_lowdata[i] = val;

		sprintf(name, "channel%d_compare_higdata", i);
		if (of_property_read_u32(np, name, &val)) {
			pr_err("%s:get %s err!\n", __func__, name);
			val = 0;
		}
		gpadc_config->channel_compare_higdata[i] = val;
	}

	sunxi_gpadc->wakeup_en = 0;
	if (of_property_read_bool(np, "wakeup-source")) {
		device_init_wakeup(&pdev->dev, true);
		dev_pm_set_wake_irq(&pdev->dev, sunxi_gpadc->irq_num);
		sunxi_gpadc->wakeup_en = 1;
	}

	return 0;
}

static int sunxi_gpadc_request_clk(struct sunxi_gpadc *sunxi_gpadc)
{
	sunxi_gpadc->bus_clk = devm_clk_get(sunxi_gpadc->dev, "bus");
	if (IS_ERR_OR_NULL(sunxi_gpadc->bus_clk)) {
		pr_err("[gpadc%d] request GPADC clock failed\n", sunxi_gpadc->bus_num);
		return -1;
	}

	sunxi_gpadc->reset = devm_reset_control_get(sunxi_gpadc->dev, NULL);
	if (IS_ERR_OR_NULL(sunxi_gpadc->reset)) {
		pr_err("[gpadc%d] request GPADC reset failed\n", sunxi_gpadc->bus_num);
		return -1;
	}

	return 0;
}

static int sunxi_gpadc_clk_init(struct sunxi_gpadc *sunxi_gpadc)
{
	if (reset_control_deassert(sunxi_gpadc->reset)) {
		pr_err("[gpadc%d] reset control deassert failed!\n", sunxi_gpadc->bus_num);
		return -1;
	}

	if (clk_prepare_enable(sunxi_gpadc->bus_clk)) {
		pr_err("[gpadc%d] enable clock failed!\n", sunxi_gpadc->bus_num);
		return -1;
	}

	return 0;
}

static void sunxi_gpadc_clk_exit(struct sunxi_gpadc *sunxi_gpadc)
{
	if (!IS_ERR_OR_NULL(sunxi_gpadc->bus_clk) && __clk_is_enabled(sunxi_gpadc->bus_clk))
		clk_disable_unprepare(sunxi_gpadc->bus_clk);
}

static int sunxi_gpadc_hw_init(struct sunxi_gpadc *sunxi_gpadc)
{
	struct sunxi_config *gpadc_config = &sunxi_gpadc->gpadc_config;
	int i;

	if (sunxi_gpadc_request_clk(sunxi_gpadc)) {
		pr_err("[gpadc%d] request gpadc clk failed\n", sunxi_gpadc->bus_num);
		return -EPROBE_DEFER;
	}

	if (sunxi_gpadc_clk_init(sunxi_gpadc)) {
		pr_err("[gpadc%d] init gpadc clk failed\n", sunxi_gpadc->bus_num);
		return -EPROBE_DEFER;
	}

	sunxi_gpadc_sample_rate_set(sunxi_gpadc->reg_base, OSC_24MHZ,
			sunxi_gpadc->gpadc_sample_rate);
	for (i = 0; i < sunxi_gpadc->channel_num; i++) {
		if (gpadc_config->channel_select & sunxi_gpadc_channel_id(i)) {
			sunxi_gpadc_ch_select(sunxi_gpadc->reg_base, i);
//			if (gpadc_config->channel_data_select & sunxi_gpadc_channel_id(i))
//				sunxi_enable_irq_ch_select(sunxi_gpadc->reg_base, i);
//			if (gpadc_config->channel_compare_select & sunxi_gpadc_channel_id(i)) {
//				sunxi_gpadc_cmp_select(sunxi_gpadc->reg_base, i);
				if (gpadc_config->channel_cld_select & sunxi_gpadc_channel_id(i)) {
					sunxi_gpadc_cmp_select(sunxi_gpadc->reg_base, i);
					sunxi_enable_lowirq_ch_select(sunxi_gpadc->reg_base, i);
					sunxi_gpadc_ch_cmp_low(sunxi_gpadc->reg_base, i,
							gpadc_config->channel_compare_lowdata[i]);
				}
				if (gpadc_config->channel_chd_select & sunxi_gpadc_channel_id(i)) {
					sunxi_gpadc_cmp_select(sunxi_gpadc->reg_base, i);
					sunxi_enable_higirq_ch_select(sunxi_gpadc->reg_base, i);
					sunxi_gpadc_ch_cmp_hig(sunxi_gpadc->reg_base, i,
							gpadc_config->channel_compare_higdata[i]);
				}
//			}
		}
	}
	sunxi_gpadc_calibration_enable(sunxi_gpadc->reg_base);
	sunxi_gpadc_mode_select(sunxi_gpadc->reg_base, GP_CONTINUOUS_MODE);
	sunxi_gpadc_enable(sunxi_gpadc->reg_base, true);
	sunxi_gpadc_enable_irq(sunxi_gpadc->reg_base);

	return 0;
}

static int sunxi_gpadc_hw_exit(struct sunxi_gpadc *sunxi_gpadc)
{
	struct sunxi_config *gpadc_config = &sunxi_gpadc->gpadc_config;
	int i;

	for (i = 0; i < sunxi_gpadc->channel_num; i++) {
		if (gpadc_config->channel_select & sunxi_gpadc_channel_id(i)) {
			sunxi_gpadc_ch_deselect(sunxi_gpadc->reg_base, i);
//			if (gpadc_config->channel_data_select & sunxi_gpadc_channel_id(i))
//				sunxi_disable_irq_ch_select(sunxi_gpadc->reg_base, i);
//			if (gpadc_config->channel_compare_select & sunxi_gpadc_channel_id(i)) {
//				sunxi_gpadc_cmp_deselect(sunxi_gpadc->reg_base, i);
				if (gpadc_config->channel_cld_select & sunxi_gpadc_channel_id(i))
					sunxi_disable_lowirq_ch_select(sunxi_gpadc->reg_base, i);
				if (gpadc_config->channel_chd_select & sunxi_gpadc_channel_id(i))
					sunxi_disable_higirq_ch_select(sunxi_gpadc->reg_base, i);
				if ((gpadc_config->channel_chd_select |
				gpadc_config->channel_cld_select) & sunxi_gpadc_channel_id(i))
					sunxi_gpadc_cmp_deselect(sunxi_gpadc->reg_base, i);
//					sunxi_disable_irq_ch_select(sunxi_gpadc->reg_base, i);
//			}
		}
	}
	sunxi_gpadc_enable(sunxi_gpadc->reg_base, false);
	sunxi_gpadc_clk_exit(sunxi_gpadc);

	return 0;
}

static u32 sunxi_gpadc_key_xfer(struct sunxi_gpadc *sunxi_gpadc, u32 data, u32 irq_low_set)
{
	u32 vol_data;
	u8 ch_num;

#ifdef CONFIG_ARCH_SUN8IW18
	u32 vin;

	vin = sunxi_gpadc_check_vin();
	if (vin == 0)
		return 0;

	data = ((vin - 6)*data) / 4096;
	vol_data = data;
	data = data * 1000;
#else
	data = ((VOL_RANGE / 4096)*data);	/* 12bits sample rate */
	vol_data = data / 1000;
#endif
	if (vol_data < SUNXIKEY_DOWN) {
		/* MAX compare_before = 128 */
		sunxi_gpadc->compare_before = ((data / SCALE_UNIT) / 1000)&0xff;
		dprintk(DEBUG_RUN, "bf=%3d lt=%3d\n",
				sunxi_gpadc->compare_before,
				sunxi_gpadc->compare_later);

		if (sunxi_gpadc->compare_before >= sunxi_gpadc->compare_later - 1
			&& sunxi_gpadc->compare_before <= sunxi_gpadc->compare_later + 1)
			sunxi_gpadc->key_cnt++;
		else
			sunxi_gpadc->key_cnt = 0;
		sunxi_gpadc->compare_later = sunxi_gpadc->compare_before;
		if (sunxi_gpadc->key_cnt >= filter_cnt) {
			sunxi_gpadc->compare_later = sunxi_gpadc->compare_before;
			sunxi_gpadc->key_code = keypad_mapindex[sunxi_gpadc->compare_before];

			switch (irq_low_set) {
			case GP_CH0_LOW:
				ch_num = GP_CH_0;
				break;
			case GP_CH1_LOW:
				ch_num = GP_CH_1;
				break;
			case GP_CH2_LOW:
				ch_num = GP_CH_2;
				break;
			case GP_CH3_LOW:
				ch_num = GP_CH_3;
				break;
			default:
				pr_err("%s, invalid channel id!", __func__);
				return -EINVAL;
			}
			if (sunxi_gpadc->key_code != sunxi_gpadc->key_num) {
				input_report_key(sunxi_gpadc->input_gpadc[ch_num],
					sunxi_gpadc->scankeycodes[sunxi_gpadc->key_code], 1);
				input_sync(sunxi_gpadc->input_gpadc[ch_num]);
			}
			sunxi_gpadc->compare_later = 0;
			sunxi_gpadc->key_cnt = 0;
		}

	}

	return 0;
}

static irqreturn_t sunxi_gpadc_interrupt(int irqno, void *dev_id)
{
	struct sunxi_gpadc *sunxi_gpadc = (struct sunxi_gpadc *)dev_id;
	u32  irq_data_set, irq_low_set, irq_hig_set;
	u32  irq_data_val, irq_low_en, irq_hig_en;
	u32 data;
#ifndef CONFIG_ARCH_SUN8IW18
	u32 i;
#endif
	irq_data_val = sunxi_gpadc_read_ch_irq_enable(sunxi_gpadc->reg_base);

	irq_low_en = sunxi_gpadc_read_ch_lowirq_enable(
			sunxi_gpadc->reg_base);
	irq_hig_en = sunxi_gpadc_read_ch_higirq_enable(
			sunxi_gpadc->reg_base);

	irq_data_set = sunxi_ch_irq_status(sunxi_gpadc->reg_base);
	sunxi_ch_irq_clear_flags(sunxi_gpadc->reg_base, irq_data_set);
	irq_low_set = sunxi_ch_lowirq_status(sunxi_gpadc->reg_base);
	sunxi_ch_lowirq_clear_flags(sunxi_gpadc->reg_base, irq_low_set);
	irq_hig_set = sunxi_ch_higirq_status(sunxi_gpadc->reg_base);
	sunxi_ch_higirq_clear_flags(sunxi_gpadc->reg_base, irq_hig_set);

	if (irq_data_set & GP_CH0_DATA)
		data = sunxi_gpadc_read_data(sunxi_gpadc->reg_base, GP_CH_0);

	if (irq_low_set & GP_CH0_LOW & irq_low_en) {
		data = sunxi_gpadc_read_data(sunxi_gpadc->reg_base, GP_CH_0);
		sunxi_gpadc_key_xfer(sunxi_gpadc, data, irq_low_set);
		sunxi_enable_higirq_ch_select(sunxi_gpadc->reg_base, GP_CH_0);
	}

	if (irq_hig_set & GP_CH0_HIG & irq_hig_en) {
		input_report_key(sunxi_gpadc->input_gpadc[GP_CH_0],
			sunxi_gpadc->scankeycodes[sunxi_gpadc->key_code], 0);
		input_sync(sunxi_gpadc->input_gpadc[GP_CH_0]);
		sunxi_gpadc->compare_later = 0;
		sunxi_gpadc->key_cnt = 0;
		sunxi_disable_higirq_ch_select(sunxi_gpadc->reg_base, GP_CH_0);
	}

#ifdef CONFIG_ARCH_SUN8IW18
	if (irq_low_set & GP_CH3_LOW & irq_low_en) {
		data = sunxi_gpadc_read_data(sunxi_gpadc->reg_base, GP_CH_3);
		sunxi_gpadc_key_xfer(sunxi_gpadc, data, irq_low_set);
		sunxi_enable_higirq_ch_select(sunxi_gpadc->reg_base, GP_CH_3);
	}

	if (irq_hig_set & GP_CH3_HIG & irq_hig_en) {
		input_report_key(sunxi_gpadc->input_gpadc[GP_CH_3],
			sunxi_gpadc->scankeycodes[sunxi_gpadc->key_code], 0);
		input_sync(sunxi_gpadc->input_gpadc[GP_CH_3]);
		sunxi_gpadc->compare_later = 0;
		sunxi_gpadc->key_cnt = 0;
		sunxi_disable_higirq_ch_select(sunxi_gpadc->reg_base, GP_CH_3);
	}
#else
	for (i = 1; i < sunxi_gpadc->channel_num; i++) {
		if (irq_data_set & irq_data_val & (1 << i)) {
			data = sunxi_gpadc_read_data(sunxi_gpadc->reg_base, i);
			input_event(sunxi_gpadc->input_gpadc[i],
					EV_MSC, MSC_SCAN, data);
			input_sync(sunxi_gpadc->input_gpadc[i]);
			pr_debug("channel %d data pend\n", i);
		}

		if (irq_low_en & irq_hig_en & (1 << i)) {
			if (irq_low_set & irq_hig_set & (1 << i)) {
				data = sunxi_gpadc_read_data(
						sunxi_gpadc->reg_base, i);
				input_event(sunxi_gpadc->input_gpadc[i],
						EV_MSC, MSC_SCAN, data);
				input_sync(sunxi_gpadc->input_gpadc[i]);
				pr_debug("channel %d low and hig pend\n", i);
			} else
				pr_debug("invalid range\n");
		} else {
			if (irq_low_set & irq_low_en & ~irq_hig_en
					& (1 << i)) {
				data = sunxi_gpadc_read_data(
						sunxi_gpadc->reg_base, i);
				input_event(sunxi_gpadc->input_gpadc[i],
						EV_MSC, MSC_SCAN, data);
				input_sync(sunxi_gpadc->input_gpadc[i]);
				pr_debug("channel %d low pend\n", i);
			} else {
				if (irq_hig_set & irq_hig_en & ~irq_low_en
					& (1 << i)) {
					data = sunxi_gpadc_read_data(
							sunxi_gpadc->reg_base,
							i);
					input_event(sunxi_gpadc->input_gpadc[i],
							EV_MSC, MSC_SCAN, data);
					input_sync(sunxi_gpadc->input_gpadc[i]);
					pr_debug("channel %d hig pend\n", i);
				}
			}

		}
	}
#endif
	return IRQ_HANDLED;
}

#ifdef USE_DATA_SCAN
static void push_event_status(struct work_struct *work)
{
	unsigned int data, i;
	unsigned int scan_channel = sunxi_gpadc->gpadc_config.channel_scan_data;

	for (i = 1; i < sunxi_gpadc->channel_num; i++) {
		if (scan_channel & (1 << i)) {
			data = sunxi_gpadc_read_data(sunxi_gpadc->reg_base, i);
			input_event(sunxi_gpadc->input_gpadc[i],
					EV_MSC, MSC_SCAN, data);
			input_sync(sunxi_gpadc->input_gpadc[i]);
		}
	}
	schedule_delayed_work(&sunxi_gpadc->gpadc_work, sunxi_gpadc->interval);
}
#endif

void __status_regs_ex(void __iomem *reg_base)
{
	unsigned char i;
	u32 reg_val;

	reg_val = readl(reg_base + GP_CS_EN_REG);

	for (i = 0; i < sunxi_gpadc->channel_num; i++) {
		if (reg_val & (1 << i))
			pr_warn("gpadc%d: okay\n", i);
		else
			pr_warn("gpadc%d: disable\n", i);
	}
}

void __vol_regs_ex(struct sunxi_gpadc *sunxi_gpadc)
{
	unsigned char i;
	char tmp = -1;

	pr_info("key_cnt = %d\n", sunxi_gpadc->key_num);
	for (i = 0; i < sunxi_gpadc->key_num; i++)
		pr_info("key%d_vol = %d\n", i, key_vol[i]);

	dprintk(DEBUG_INFO, "keypad_mapindex[%d] = {", MAXIMUM_SCALE);
	for (i = 0; i < MAXIMUM_SCALE; i++) {
		if (keypad_mapindex[i] != tmp)
			dprintk(DEBUG_INFO, "\n\t");
		dprintk(DEBUG_INFO, "%d, ", keypad_mapindex[i]);
		tmp = keypad_mapindex[i];
	}
	dprintk(DEBUG_INFO, "\n}\n");
}

static struct status_reg __parse_status_str(const char *buf, size_t size)
{
	char *ptr = NULL;
	char *str = "gpadc";
	struct status_reg reg_tmp;

	if (!buf)
		goto err;

	reg_tmp.pst = (char *)buf;
	reg_tmp.ped = reg_tmp.pst;
	reg_tmp.ped = strnchr(reg_tmp.pst, size, ',');
	if (!reg_tmp.ped)
		goto err;

	*reg_tmp.ped = '\0';
	reg_tmp.ped++;
	reg_tmp.pst = strim(reg_tmp.pst);
	reg_tmp.ped = strim(reg_tmp.ped);

	ptr = strstr(reg_tmp.pst, str);
	if (!ptr) {
		ptr = reg_tmp.pst;
		while (*ptr != 0) {
			if (*ptr >= 'A' && *ptr <= 'Z')
				*ptr += 32;
			ptr++;
		}
		ptr = reg_tmp.pst;
		ptr = strstr(ptr, str);
		if (!ptr)
			goto err;
	}

	reg_tmp.channel = *(reg_tmp.pst + strlen(str)) - 48;
	if (reg_tmp.channel < 0
			|| reg_tmp.channel > (sunxi_gpadc->channel_num - 1))
		goto err;

	if (!(strlen(reg_tmp.ped) == 1))
		goto err;

	reg_tmp.val = *reg_tmp.ped - 48;

	return reg_tmp;

err:
	reg_tmp.pst = NULL;
	reg_tmp.ped = NULL;
	return reg_tmp;
}

static struct vol_reg __parse_vol_str(const char *buf, size_t size)
{
	int ret = 0;
	char *ptr = NULL;
	char *str = "vol";
	struct vol_reg reg_tmp;

	if (!buf)
		goto err;

	reg_tmp.pst = (char *)buf;
	reg_tmp.ped = reg_tmp.pst;
	reg_tmp.ped = strnchr(reg_tmp.pst, size, ',');
	if (!reg_tmp.ped)
		goto err;

	*reg_tmp.ped = '\0';
	reg_tmp.ped++;
	reg_tmp.pst = strim(reg_tmp.pst);
	reg_tmp.ped = strim(reg_tmp.ped);

	ptr = strstr(reg_tmp.pst, str);
	if (!ptr) {
		ptr = reg_tmp.pst;
		while (*ptr != 0) {
			if (*ptr >= 'A' && *ptr <= 'Z')
				*ptr += 32;
			ptr++;
		}
		ptr = reg_tmp.pst;
		ptr = strstr(ptr, str);
		if (!ptr)
			goto err;
	}

	reg_tmp.index = *(reg_tmp.pst + strlen(str)) - 48;
	if (reg_tmp.index < 0
			|| reg_tmp.index > (sunxi_gpadc->key_num - 1))
		goto err;

	ret = kstrtoul(reg_tmp.ped, 10, &reg_tmp.vol);
	if (ret)
		goto err;

	return reg_tmp;

err:
	reg_tmp.pst = NULL;
	reg_tmp.ped = NULL;
	return reg_tmp;
}

static struct sr_reg __parse_sr_str(const char *buf, size_t size)
{
	int ret = 0;
	struct sr_reg reg_tmp;

	if (!buf)
		reg_tmp.pst = NULL;

	reg_tmp.pst = (char *)buf;
	reg_tmp.pst = strim(reg_tmp.pst);

	ret = kstrtoul(reg_tmp.pst, 10, &reg_tmp.val);
	if (ret)
		goto err;
	if (reg_tmp.val < MIN_SR || reg_tmp.val > MAX_SR) {
		pr_err("%s,%d err. sampling frequency: [%lu,%lu]\n",
				__func__, __LINE__, MIN_SR, MAX_SR);
		reg_tmp.pst = NULL;
	}

	return reg_tmp;

err:
	reg_tmp.pst = NULL;
	return reg_tmp;
}

static struct filter_reg __parse_filter_str(const char *buf, size_t size)
{
	int ret = 0;
	struct filter_reg reg_tmp;

	if (!buf)
		reg_tmp.pst = NULL;

	reg_tmp.pst = (char *)buf;
	reg_tmp.pst = strim(reg_tmp.pst);

	ret = kstrtoul(reg_tmp.pst, 10, &reg_tmp.val);
	if (ret)
		goto err;

	return reg_tmp;

err:
	reg_tmp.pst = NULL;
	return reg_tmp;
}

static struct channel_reg __parse_channel_str(const char *buf, size_t size)
{
	int ret = 0;
	struct channel_reg reg_tmp;

	if (!buf)
		reg_tmp.pst = NULL;

	reg_tmp.pst = (char *)buf;
	reg_tmp.pst = strim(reg_tmp.pst);

	ret = kstrtoul(reg_tmp.pst, 10, &reg_tmp.val);
	if (ret)
		goto err;

	return reg_tmp;

err:
	reg_tmp.pst = NULL;
	return reg_tmp;
}

static ssize_t
status_show(struct class *class, struct class_attribute *attr, char *buf)
{
	__status_regs_ex(sunxi_gpadc->reg_base);

	return 0;
}

static ssize_t
status_store(struct class *class, struct class_attribute *attr,
		const char *buf, size_t count)
{
	status_para = __parse_status_str(buf, count);
	if (!status_para.pst || !status_para.ped)
		goto err;

	if (status_para.val) {
		sunxi_gpadc_ch_select(sunxi_gpadc->reg_base,
				status_para.channel);
		pr_warn("Enable gpadc%u\n", status_para.channel);
	} else {
		sunxi_gpadc_ch_deselect(sunxi_gpadc->reg_base,
				status_para.channel);
		pr_warn("Disable gpadc%u\n", status_para.channel);
	}

	return count;

err:
	pr_err("%s,%d err, invalid para!\n", __func__, __LINE__);
	status_para.pst = NULL;
	status_para.ped = NULL;
	status_para.channel = -1;
	status_para.val = -1;
	return -EINVAL;
}

static ssize_t
vol_show(struct class *class, struct class_attribute *attr, char *buf)
{
	__vol_regs_ex(sunxi_gpadc);
	return 0;
}

static ssize_t
vol_store(struct class *class, struct class_attribute *attr,
		const char *buf, size_t count)
{
	unsigned char i, j = 0;
	u32 gap = 0, half_gap = 0, cnt = 0;
	u32 set_vol[VOL_NUM];

	vol_para = __parse_vol_str(buf, count);
	if (!vol_para.pst || !vol_para.ped)
		goto err;

	key_vol[vol_para.index] = vol_para.vol;

	for (i = sunxi_gpadc->key_num - 1; i > 0; i--) {
		gap = gap + (key_vol[i] - key_vol[i-1]);
		cnt++;
	}
	half_gap = gap/cnt/2;
	for (i = 0; i < sunxi_gpadc->key_num; i++)
		set_vol[i] = key_vol[i] + half_gap;

	for (i = 0; i < 128; i++) {
		if (i * SCALE_UNIT > set_vol[j])
			j++;
		keypad_mapindex[i] = j;
	}

	return count;

err:
	pr_err("%s,%d err, invalid para!\n", __func__, __LINE__);
	vol_para.pst = NULL;
	vol_para.ped = NULL;
	vol_para.index = -1;
	vol_para.vol = -1;
	return -EINVAL;
}

u32 sunxi_gpadc_read_channel_data(u8 channel)
{
	u32 data, vol_data;

	data = readl(sunxi_gpadc->reg_base + GP_CS_EN_REG);

	if ((data & (0x01 << channel)) == 0)
		return VOL_RANGE + 1;

	data = sunxi_gpadc_read_data(sunxi_gpadc->reg_base, channel);

	data = ((VOL_RANGE / 4096)*data);  /* 12bits sample rate */
	vol_data = data / 1000;  /* data to val_data */

	printk("vol_data: %d\n", vol_data);

	return vol_data;
}
EXPORT_SYMBOL_GPL(sunxi_gpadc_read_channel_data);

static ssize_t
sr_show(struct class *class, struct class_attribute *attr, char *buf)
{
	u32 sr;

	sr = sunxi_gpadc_sample_rate_read(sunxi_gpadc->reg_base, OSC_24MHZ);
	pr_warn("gpadc sampling frequency: %u\n", sr);

	return 0;
}

static ssize_t
sr_store(struct class *class, struct class_attribute *attr,
		const char *buf, size_t count)
{
	sr_para = __parse_sr_str(buf, count);
	if (!sr_para.pst)
		goto err;

	sunxi_gpadc_sample_rate_set(sunxi_gpadc->reg_base, OSC_24MHZ,
			sr_para.val);

	return count;

err:
	pr_err("%s,%d err, invalid para!\n", __func__, __LINE__);
	return -EINVAL;
}

static ssize_t
filter_show(struct class *class, struct class_attribute *attr, char *buf)
{
	dprintk(DEBUG_INFO, "filter_cnt = %u\n", filter_cnt);

	return 0;
}

static ssize_t
filter_store(struct class *class, struct class_attribute *attr,
		const char *buf, size_t count)
{
	filter_para = __parse_filter_str(buf, count);
	if (!filter_para.pst)
		goto err;

	filter_cnt = filter_para.val;

	return count;

err:
	pr_err("%s,%d err, invalid para!\n", __func__, __LINE__);
	return -EINVAL;
}

static ssize_t
data_store(struct class *class, struct class_attribute *attr,
		const char *buf, size_t count)
{
	channel_para = __parse_channel_str(buf, count);
	if (!channel_para.pst)
		goto err;

	channel = channel_para.val;

	return count;

err:
	pr_err("%s,%d err, invalid para!\n", __func__, __LINE__);
	return -EINVAL;
}

static ssize_t
data_show(struct class *class, struct class_attribute *attr, char *buf)
{
	unsigned int data;

	data = sunxi_gpadc_read_channel_data(channel);

	return sprintf(buf, "%d\n", data);
}

static struct class_attribute gpadc_class_attrs[] = {
	__ATTR(status, 0644, status_show, status_store),
	__ATTR(vol,    0644, vol_show,    vol_store),
	__ATTR(sr,     0644, sr_show,     sr_store),
	__ATTR(filter, 0644, filter_show, filter_store),
	__ATTR(data,   0644, data_show,   data_store),
};

static struct class gpadc_class = {
	.name		= "gpadc",
	.owner		= THIS_MODULE,
};

#if IS_ENABLED(CONFIG_IIO)
struct sunxi_gpadc_iio {
	struct sunxi_gpadc *sunxi_gpadc;
};

static int sunxi_gpadc_read_raw(struct iio_dev *indio_dev,
			struct iio_chan_spec const *chan,
			int *val, int *val2, long mask)
{
	int ret = 0;
	struct sunxi_gpadc_iio *info = iio_priv(indio_dev);
	struct sunxi_gpadc *sunxi_gpadc = info->sunxi_gpadc;
	u32 data, vol_data;

	mutex_lock(&indio_dev->mlock);
	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		/*
		* This is an example,so if there are new requirements,
		* you can replace it with your interface to upload data.
		* *val and *val2 are used as needed.
		* In the shell console you can use the "cat in_voltage0_raw" command to view the data.
		*/
		data = sunxi_gpadc_read_data(sunxi_gpadc->reg_base, chan->channel);

		data = ((VOL_RANGE / 4096)*data);		/* 12bits sample rate */
		vol_data = data / 1000;					/* data to val_data */
		*val  = vol_data;

		ret = IIO_VAL_INT;
		break;

	case IIO_CHAN_INFO_SCALE:
		break;

	default:
		ret = -EINVAL;
	}
	mutex_unlock(&indio_dev->mlock);

	return ret;
}

/*
* If necessary, you can fill other callback functions
* in this data structure, for example:
* write_raw/read_event_value/write_event_value and so on...
*/
static const struct iio_info sunxi_gpadc_iio_info = {
	.read_raw = &sunxi_gpadc_read_raw,
};

static void sunxi_gpadc_remove_iio(void *data)
{
	struct iio_dev *indio_dev = data;

	if (!indio_dev)
		pr_err("indio_dev is null\n");
	else
		iio_device_unregister(indio_dev);

}

#define ADC_CHANNEL(_index, _id) {	\
	.type = IIO_VOLTAGE,			\
	.indexed = 1,					\
	.channel = _index,				\
	.address = _index,				\
	.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),	\
	.datasheet_name = _id,							\
}

/*
* According to the GPADC spec,
* the current GPADC only supports 4 channels.
*/
static const struct iio_chan_spec gpadc_adc_iio_channels[] = {
	ADC_CHANNEL(0, "adc_chan0"),
	ADC_CHANNEL(1, "adc_chan1"),
	ADC_CHANNEL(2, "adc_chan2"),
	ADC_CHANNEL(3, "adc_chan3"),
	/* ADC_CHANNEL(4, "adc_chan4"), */
	/* ADC_CHANNEL(5, "adc_chan5"), */
	/* ADC_CHANNEL(6, "adc_chan6"), */
	/* ADC_CHANNEL(7, "adc_chan7"), */
};

/*
* Register the IIO data structure,
* the basic data has been initialized in the function: sunxi_gpadc_probe,
* you can use it directly here: platform_get_drvdata.
*/
static int sunxi_gpadc_iio_init(struct platform_device *pdev)
{
	int ret;
	struct iio_dev *indio_dev;
	struct sunxi_gpadc_iio *info;
	struct sunxi_gpadc *sunxi_gpadc = platform_get_drvdata(pdev);

	indio_dev = devm_iio_device_alloc(&pdev->dev, sizeof(*sunxi_gpadc));
	if (!indio_dev)
		return -ENOMEM;

	info = iio_priv(indio_dev);
	info->sunxi_gpadc = sunxi_gpadc;

	indio_dev->dev.parent = &pdev->dev;
	indio_dev->name = pdev->name;
	indio_dev->channels = gpadc_adc_iio_channels;
	indio_dev->num_channels = ARRAY_SIZE(gpadc_adc_iio_channels);
	indio_dev->info = &sunxi_gpadc_iio_info;
	indio_dev->modes = INDIO_DIRECT_MODE;

	ret = iio_device_register(indio_dev);
	if (ret < 0) {
		dev_err(&pdev->dev, "unable to register iio device\n");
		ret = -EINVAL;
	}
/*
 * Destroy IIO:the kernel will call the callback to execute sunxi_gpadc_remove_iio.
 */
	ret = devm_add_action(&pdev->dev, sunxi_gpadc_remove_iio, indio_dev);

	if (ret) {
		dev_err(&pdev->dev, "unable to add iio cleanup action\n");
		goto err_iio_unregister;
	}

	return 0;

err_iio_unregister:
	iio_device_unregister(indio_dev);

	return ret;
}
#endif


int sunxi_gpadc_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	int ret = 0;

	if (!of_device_is_available(np)) {
		pr_err("%s: sunxi gpadc is disable\n", __func__);
		return -EPERM;
	}

	sunxi_gpadc = kzalloc(sizeof(*sunxi_gpadc), GFP_KERNEL);
	if (IS_ERR_OR_NULL(sunxi_gpadc)) {
		pr_err("not enough memory for sunxi_gpadc\n");
		return -ENOMEM;
	}

	sunxi_gpadc->reg_base = of_iomap(np, 0);
	if (NULL == sunxi_gpadc->reg_base) {
		pr_err("sunxi_gpadc of_iomap fail\n");
		ret = -EBUSY;
		goto fail1;
	}

	vaddr = ioremap(LDOA_EFUSE_REG, 1);
	if (!vaddr) {
		pr_err("sunxi_gpadc iomap fail\n");
		ret = -EBUSY;
		goto fail1;
	}

	sunxi_gpadc->irq_num = irq_of_parse_and_map(np, 0);
	if (0 == sunxi_gpadc->irq_num) {
		pr_err("sunxi_gpadc fail to map irq\n");
		ret = -EBUSY;
		goto fail2;
	}

	/* Need to clarify usage issues */
	/*
	sunxi_gpadc->mclk = of_clk_get(np, 0);
	if (IS_ERR_OR_NULL(sunxi_gpadc->mclk)) {
		pr_err("sunxi_gpadc has no clk\n");
		ret = -EINVAL;
		goto fail3;
	} else {
		if (clk_prepare_enable(sunxi_gpadc->mclk)) {
			pr_err("enable sunxi_gpadc clock failed!\n");
			ret = -EINVAL;
			goto fail3;
		}
	}
	*/

	sunxi_gpadc->dev = &pdev->dev;
	sunxi_key_init(sunxi_gpadc, pdev);
	sunxi_gpadc_setup(pdev, sunxi_gpadc);
	sunxi_gpadc_hw_init(sunxi_gpadc);
	sunxi_gpadc_input_register_setup(sunxi_gpadc);

#ifdef USE_DATA_SCAN
	sunxi_gpadc->interval = msecs_to_jiffies(1 * 1000);
	INIT_DELAYED_WORK(&sunxi_gpadc->gpadc_work, push_event_status);
	schedule_delayed_work(&sunxi_gpadc->gpadc_work, sunxi_gpadc->interval);
#endif

	platform_set_drvdata(pdev, sunxi_gpadc);

	if (request_irq(sunxi_gpadc->irq_num, sunxi_gpadc_interrupt,
			IRQF_TRIGGER_NONE, "sunxi-gpadc", sunxi_gpadc)) {
		pr_err("sunxi_gpadc request irq failure\n");
		return -1;
	}

/*
* This function will only be used when IIO is enabled !!!
*/
#if IS_ENABLED(CONFIG_IIO)
	sunxi_gpadc_iio_init(pdev);
#endif
	return 0;

/* Need to clarify usage issues */
/*
fail3:
	free_irq(sunxi_gpadc->irq_num, sunxi_gpadc);
*/

fail2:
	iounmap(vaddr);
	iounmap(sunxi_gpadc->reg_base);
fail1:
	kfree(sunxi_gpadc);

	return ret;
}

int sunxi_gpadc_remove(struct platform_device *pdev)

{
	struct sunxi_gpadc *sunxi_gpadc = platform_get_drvdata(pdev);

	cancel_delayed_work_sync(&sunxi_gpadc->gpadc_work);

	sunxi_gpadc_hw_exit(sunxi_gpadc);
	free_irq(sunxi_gpadc->irq_num, sunxi_gpadc);
	iounmap(sunxi_gpadc->reg_base);
	kfree(sunxi_gpadc);

	return 0;
}

#ifdef CONFIG_PM
static int sunxi_gpadc_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sunxi_gpadc *sunxi_gpadc = platform_get_drvdata(pdev);

#ifdef USE_DATA_SCAN
	schedule_delayed_work(&sunxi_gpadc->gpadc_work, 0);
	flush_delayed_work(&sunxi_gpadc->gpadc_work);
	cancel_delayed_work_sync(&sunxi_gpadc->gpadc_work);
#endif

	if (sunxi_gpadc->wakeup_en)
		sunxi_gpadc_save_regs(sunxi_gpadc);		/* standby need irq enable */
	else {
		disable_irq_nosync(sunxi_gpadc->irq_num);
		sunxi_gpadc_save_regs(sunxi_gpadc);
		clk_disable_unprepare(sunxi_gpadc->bus_clk);
		reset_control_assert(sunxi_gpadc->reset);
	}

	return 0;
}

static int sunxi_gpadc_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sunxi_gpadc *sunxi_gpadc = platform_get_drvdata(pdev);

	if (sunxi_gpadc->wakeup_en)
		sunxi_gpadc_restore_regs(sunxi_gpadc);
	else {
		reset_control_deassert(sunxi_gpadc->reset);

		if (clk_prepare_enable(sunxi_gpadc->bus_clk)) {
			pr_err("[gpadc%d] enable clock failed!\n", sunxi_gpadc->bus_num);
			return 0;
		}

		sunxi_gpadc_restore_regs(sunxi_gpadc);
		enable_irq(sunxi_gpadc->irq_num);
	}
#ifdef USE_DATA_SCAN
	schedule_delayed_work(&sunxi_gpadc->gpadc_work, sunxi_gpadc->interval);
#endif

	return 0;
}

static const struct dev_pm_ops sunxi_gpadc_dev_pm_ops = {
	.suspend = sunxi_gpadc_suspend,
	.resume = sunxi_gpadc_resume,
};

#define SUNXI_GPADC_DEV_PM_OPS (&sunxi_gpadc_dev_pm_ops)
#else
#define SUNXI_GPADC_DEV_PM_OPS NULL
#endif

static const struct of_device_id sunxi_gpadc_of_match[] = {
	{ .compatible = "allwinner,sunxi-gpadc"},
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_gpadc_of_match);

static struct platform_driver sunxi_gpadc_driver = {
	.probe  = sunxi_gpadc_probe,
	.remove = sunxi_gpadc_remove,
	.driver = {
		.name   = "sunxi-gpadc",
		.owner  = THIS_MODULE,
		.pm = SUNXI_GPADC_DEV_PM_OPS,
		.of_match_table = of_match_ptr(sunxi_gpadc_of_match),
	},
};
module_param_named(debug_mask, debug_mask, int, 0644);

static int __init sunxi_gpadc_init(void)
{
	int ret;
	int i;
	int err;

	ret = class_register(&gpadc_class);
	if (ret < 0)
		pr_err("%s,%d err, ret:%d\n", __func__, __LINE__, ret);
	else
		pr_info("%s,%d, success\n", __func__, __LINE__);

	for (i = 0; i < ARRAY_SIZE(gpadc_class_attrs); i++) {
		err = class_create_file(&gpadc_class, &gpadc_class_attrs[i]);
		if (err) {
			pr_err("%s(): class_create_file() failed. err=%d\n", __func__, err);
			while (i--)
				class_remove_file(&gpadc_class, &gpadc_class_attrs[i]);
			class_unregister(&gpadc_class);
			return err;
		}
	}

	ret = platform_driver_register(&sunxi_gpadc_driver);
	if (ret != 0)
		class_unregister(&gpadc_class);

	return ret;
}
module_init(sunxi_gpadc_init);

static void __exit sunxi_gpadc_exit(void)
{
	platform_driver_unregister(&sunxi_gpadc_driver);
	class_unregister(&gpadc_class);
}
module_exit(sunxi_gpadc_exit);

MODULE_AUTHOR("Fuzhaoke");
MODULE_DESCRIPTION("sunxi-gpadc driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.1.0");
