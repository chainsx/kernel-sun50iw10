/*
 * Allwinner SoCs tv driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#ifndef __DE_TVE_H__
#define __DE_TVE_H__

#if defined(CONFIG_ARCH_SUN8IW11)
#define TVE_TOP_SUPPORT
#define TVE_DEVICE_NUM 2
#define TVE_TOP_DEVIVE_NUM 1
#define TVE_DAC_NUM 4
#define USE_V40_DRIVER
#define TVE_SUPPLY_VOLTAGE 3300000
#elif defined(CONFIG_ARCH_SUN50IW2)
/* #define TVE_TOP_SUPPORT */
#define TVE_DEVICE_NUM 1
#define TVE_TOP_DEVIVE_NUM 1
#define TVE_DAC_NUM 1
#elif defined(CONFIG_ARCH_SUN8IW12) || defined(CONFIG_ARCH_SUN8IW16) \
	|| defined(CONFIG_ARCH_SUN50IW9) ||  defined(CONFIG_ARCH_SUN8IW20)
#define TVE_TOP_SUPPORT
#define TVE_DEVICE_NUM 1
#define TVE_TOP_DEVIVE_NUM 1
#define TVE_DAC_NUM 1
#define USE_V40_DRIVER
#elif defined(CONFIG_ARCH_SUN8IW17)
#define TVE_TOP_SUPPORT
#define TVE_DEVICE_NUM          1
#define TVE_TOP_DEVIVE_NUM      1
#define TVE_DAC_NUM             1
#define USE_V40_DRIVER
#else
#define TVE_DEVICE_NUM 1
#define TVE_TOP_DEVIVE_NUM 1
#define TVE_DAC_NUM 1
#endif

/* tv encoder registers offset */
#define TVE_000    (0x000)
#define TVE_004    (0x004)
#define TVE_008    (0x008)
#define TVE_00C    (0x00c)
#define TVE_010    (0x010)
#define TVE_014    (0x014)
#define TVE_018    (0x018)
#define TVE_01C    (0x01c)
#define TVE_020    (0x020)
#define TVE_024    (0x024)
#define TVE_030    (0X030)
#define TVE_034    (0x034)
#define TVE_038    (0x038)
#define TVE_03C    (0x03c)
#define TVE_040    (0x040)
#define TVE_044    (0x044)
#define TVE_048    (0x048)
#define TVE_04C    (0x04c)
#define TVE_0F8    (0x0f8)
#define TVE_0FC    (0x0fc)
#define TVE_100    (0x100)
#define TVE_104    (0x104)
#define TVE_108    (0x108)
#define TVE_10C    (0x10c)
#define TVE_110    (0x110)
#define TVE_114    (0x114)
#define TVE_118    (0x118)
#define TVE_11C    (0x11c)
#define TVE_120    (0x120)
#define TVE_124    (0x124)
#define TVE_128    (0x128)
#define TVE_12C    (0x12c)
#define TVE_130    (0x130)
#define TVE_134    (0x134)
#define TVE_138    (0x138)
#define TVE_13C    (0x13C)
#define TVE_300    (0x300)
#define TVE_304    (0x304)
#define TVE_308    (0x308)
#define TVE_30C    (0x30c)
#define TVE_380    (0x380)
#define TVE_3A0    (0x3a0)

#define TVE_TOP_020    (0x020)
#define TVE_TOP_024    (0x024)
#define TVE_TOP_028    (0x028)
#define TVE_TOP_02C    (0x02C)
#define TVE_TOP_030    (0x030)
#define TVE_TOP_034    (0x034)
#define TVE_TOP_0F0    (0x0F0)

#define TVE_GET_REG_BASE(sel)			(tve_reg_base[sel])
#define TVE_WUINT32(sel, offset, value) \
	(*((u32 *)(TVE_GET_REG_BASE(sel) + (offset))) = (value))
#define TVE_RUINT32(sel, offset) \
	(*((u32 *)(TVE_GET_REG_BASE(sel) + (offset))))
#define TVE_SET_BIT(sel, offset, bit) \
	(*((u32 *)(TVE_GET_REG_BASE(sel) + (offset))) |= (bit))
#define TVE_CLR_BIT(sel, offset, bit) \
	(*((u32 *)(TVE_GET_REG_BASE(sel) + (offset))) &= (~(bit)))
#define TVE_INIT_BIT(sel, offset, c, s) \
	(*((u32 *)(TVE_GET_REG_BASE(sel) + (offset))) = \
(((*(u32 *)(TVE_GET_REG_BASE(sel) + (offset))) & (~(c))) | (s)))

#define TVE_TOP_GET_REG_BASE()		(tve_top_reg_base[0])
#define TVE_TOP_WUINT32(offset, value) \
	(*((u32 *)(TVE_TOP_GET_REG_BASE() + (offset))) = (value))
#define TVE_TOP_RUINT32(offset) \
	(*((u32 *)(TVE_TOP_GET_REG_BASE() + (offset))))
#define TVE_TOP_SET_BIT(offset, bit) \
	(*((u32 *)(TVE_TOP_GET_REG_BASE() + (offset))) |= (bit))
#define TVE_TOP_CLR_BIT(offset, bit) \
	(*((u32 *)(TVE_TOP_GET_REG_BASE() + (offset))) &= (~(bit)))
#define TVE_TOP_INIT_BIT(offset, c, s) \
	(*((u32 *)(TVE_TOP_GET_REG_BASE() + (offset))) = \
(((*(u32 *)(TVE_TOP_GET_REG_BASE() + (offset))) & (~(c))) | (s)))

s32 tve_low_set_reg_base(u32 sel, void __iomem *address);
s32 tve_low_set_top_reg_base(void __iomem *address);
s32 tve_low_set_sid_base(void __iomem *address);
s32 tve_low_init(u32 sel, u32 *dac_no, u32 *cali, s32 *offset, u32 *dac_type,
			u32 num);
s32 tve_low_open(u32 sel);
s32 tve_low_close(u32 sel);
s32 tve_resync_enable(u32 sel);
s32 tve_resync_disable(u32 sel);
s32 tve_low_set_tv_mode(u32 sel, enum disp_tv_mode mode, u32 *cali);
s32 tve_low_get_dac_status(u32 sel);
s32 tve_low_dac_autocheck_enable(u32 sel);
s32 tve_low_dac_autocheck_disable(u32 sel);
u32 tve_low_get_sid(u32 index);
s32 tve_low_enhance(u32 sel, u32 mode);
s32 tve_low_dac_enable(u32 sel);

#ifdef USE_V40_DRIVER
s32 tve_low_sw_init(u32 sel, u32 *dac_no, u32 *dac_type,
			u32 num);
s32 tve_adjust_resync(u32 sel, s32 resync_pixel_num, s32 resync_line_num);
#else
static inline s32 tve_low_sw_init(u32 sel, u32 *dac_no, u32 *dac_type,
			u32 num) { return 0; }
static inline s32 tve_adjust_resync(u32 sel, s32 resync_pixel_num, s32 resync_line_num) { return 0; }
#endif

#endif
