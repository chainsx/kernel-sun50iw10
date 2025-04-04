/*
 * linux-5.4/drivers/media/platform/sunxi-vin/top_reg_i.h
 *
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __CSIC__TOP__REG__I__H__
#define __CSIC__TOP__REG__I__H__

/*
 * Detail information of registers
 */
 /*
 *CSIC TOP registers
 */
#define CSIC_TOP_EN_REG_OFF			0X000
#define CSIC_TOP_EN				0
#define CSIC_TOP_EN_MASK			(0X1 << CSIC_TOP_EN)
#define CSIC_BIST_MODE_EN			2
#define CSIC_BIST_MODE_EN_MASK			(0X1 << CSIC_BIST_MODE_EN)
#define CSIC_ISP_BRIDGE_EN			3
#define CSIC_ISP_BRIDGE_EN_MASK			(0X1 << CSIC_ISP_BRIDGE_EN)
#define CSIC_SRAM_PWDN				8
#define CSIC_SRAM_PWDN_MASK			(0X1 << CSIC_SRAM_PWDN)
#define CSIC_VER_EN				31
#define CSIC_VER_EN_MASK			(0X1 << CSIC_VER_EN)

#define CSIC_PTN_GEN_EN_REG_OFF			0X004
#define CSIC_PTN_GEN_EN				0
#define CSIC_PTN_GEN_EN_MASK			(0X1 << CSIC_PTN_GEN_EN)
#define CSIC_PTN_GEN_START			4
#define CSIC_PTN_GEN_START_MASK			(0X1 << CSIC_PTN_GEN_START)
#define CSIC_PTN_GEN_CYCLE			16
#define CSIC_PTN_GEN_CYCLE_MASK			(0XFF << CSIC_PTN_GEN_CYCLE)

#define CSIC_PTN_CTRL_REG_OFF			0X008
#define CSIC_PTN_CLK_DIV			8
#define CSIC_PTN_CLK_DIV_MASK			(0X3 << CSIC_PTN_CLK_DIV)
#define CSIC_PTN_MODE				16
#define CSIC_PTN_MODE_MASK			(0XF << CSIC_PTN_MODE)
#define CSIC_PTN_DATA_WIDTH			20
#define CSIC_PTN_DATA_WIDTH_MASK		(0X3 << CSIC_PTN_DATA_WIDTH)
#define CSIC_PTN_PORT_SEL			24
#define CSIC_PTN_PORT_SEL_MASK			(0X7 << CSIC_PTN_PORT_SEL)

#define CSIC_PTN_LEN_REG_OFF			0X020
#define CSIC_PTN_ADDR_REG_OFF			0X024
#define CSIC_PTN_SIZE_REG_OFF			0X028
#define CSIC_PTN_WIDTH				0
#define CSIC_PTN_WIDTH_MASK			(0X1FFF << CSIC_PTN_WIDTH)
#define CSIC_PTN_HEIGHT				16
#define CSIC_PTN_HEIGHT_MASK			(0X1FFF << CSIC_PTN_HEIGHT)

#if defined(CONFIG_ARCH_SUN50IW3P1) || defined(CONFIG_ARCH_SUN50IW6P1)
#define CSIC_ISP0_IN0_REG_OFF			0X030
#define CSIC_ISP0_IN1_REG_OFF			0X034
#define CSIC_ISP0_IN2_REG_OFF			0X038
#define CSIC_ISP0_IN3_REG_OFF			0X03C

#define CSIC_ISP1_IN0_REG_OFF			0X040
#define CSIC_ISP1_IN1_REG_OFF			0X044
#define CSIC_ISP1_IN2_REG_OFF			0X048
#define CSIC_ISP1_IN3_REG_OFF			0X04C

#define CSIC_VIPP0_IN_REG_OFF			0X060
#define CSIC_VIPP1_IN_REG_OFF			0X064
#define CSIC_VIPP2_IN_REG_OFF			0X068
#define CSIC_VIPP3_IN_REG_OFF			0X06C

#define CSIC_FEATURE_REG_OFF			0X070
#define CSIC_VER_REG_OFF			0X074

#else

#define CSIC_ISP0_IN0_REG_OFF			0X030
#define CSIC_ISP0_IN1_REG_OFF			0X034
#define CSIC_ISP0_IN2_REG_OFF			0X038
#define CSIC_ISP0_IN3_REG_OFF			0X03C

#define CSIC_ISP1_IN0_REG_OFF			0X040
#define CSIC_ISP1_IN1_REG_OFF			0X044
#define CSIC_ISP1_IN2_REG_OFF			0X048
#define CSIC_ISP1_IN3_REG_OFF			0X04C

#define CSIC_ISP2_IN0_REG_OFF			0X050
#define CSIC_ISP2_IN1_REG_OFF			0X054
#define CSIC_ISP2_IN2_REG_OFF			0X058
#define CSIC_ISP2_IN3_REG_OFF			0X05C

#define CSIC_ISP3_IN0_REG_OFF			0X060
#define CSIC_ISP3_IN1_REG_OFF			0X064
#define CSIC_ISP3_IN2_REG_OFF			0X068
#define CSIC_ISP3_IN3_REG_OFF			0X06C

#define CSIC_VIPP0_IN_REG_OFF			0X0A0
#define CSIC_VIPP1_IN_REG_OFF			0X0A4
#define CSIC_VIPP2_IN_REG_OFF			0X0A8
#define CSIC_VIPP3_IN_REG_OFF			0X0AC
#define CSIC_VIPP4_IN_REG_OFF			0X0B0
#define CSIC_VIPP5_IN_REG_OFF			0X0B4
#define CSIC_VIPP6_IN_REG_OFF			0X0B8
#define CSIC_VIPP7_IN_REG_OFF			0X0BC

#if defined CONFIG_ARCH_SUN8IW15P1 || defined CONFIG_ARCH_SUN8IW16P1 || defined CONFIG_ARCH_SUN8IW17P1 || defined CONFIG_ARCH_SUN50IW9
#define CSIC_FEATURE_REG_OFF			0X0F0
#define CSIC_VER_REG_OFF			0X0F4
#else
#define CSIC_MBUS_REQ_MAX			0x0F0
#define MCSI_MEM_REQ_MAX			0
#define MCSI_MEM_REQ_MAX_MASK			(0X1F << MCSI_MEM_REQ_MAX)
#define MCSI_MEM_1_REQ_MAX			8
#define MCSI_MEM_1_REQ_MAX_MASK			(0X1F << MCSI_MEM_1_REQ_MAX)
#define MISP_MEM_REQ_MAX			16
#define MISP_MEM_REQ_MAX_MASK			(0X1F << MISP_MEM_REQ_MAX)

#define CSIC_FEATURE_REG_OFF			0X1F0
#define CSIC_VER_REG_OFF			0X1F4
#endif
#endif

#define CSIC_FEATURE_RES0			0
#define CSIC_FEATURE_RES0_MASK			(0XFF << CSIC_FEATURE_RES0)
#define CSIC_DMA_NUM				8
#define CSIC_DMA_NUM_MASK			(0XF << CSIC_DMA_NUM)
#define CSIC_VIPP_NUM				12
#define CSIC_VIPP_NUM_MASK			(0XF << CSIC_VIPP_NUM)
#define CSIC_ISP_NUM				16
#define CSIC_ISP_NUM_MASK			(0XF << CSIC_ISP_NUM)
#define CSIC_NCSI_NUM				20
#define CSIC_NCSI_NUM_MASK			(0XF << CSIC_NCSI_NUM)
#define CSIC_MCSI_NUM				24
#define CSIC_MCSI_NUM_MASK			(0XF << CSIC_MCSI_NUM)
#define CSIC_PARSER_NUM				28
#define CSIC_PARSER_NUM_MASK			(0XF << CSIC_PARSER_NUM)

#define CSIC_VER_SMALL				0
#define CSIC_VER_SMALL_MASK			(0XFFF << CSIC_VER_SMALL)
#define CSIC_VER_BIG				12
#define CSIC_VER_BIG_MASK			(0XFFF << CSIC_VER_BIG)

#define CSIC_MULP_MODE_REG_OFF  		0X100
#define CSIC_MULP_EN				0
#define CSIC_MULP_EN_MASK			(0X1 << CSIC_MULP_EN)
#define CSIC_MULP_CS				8
#define CSIC_MULP_CS_MASK			(0XFF << CSIC_MULP_CS)
#define CSIC_MULP_STATUS			24
#define CSIC_MULP_STATUS_MASK			(0XFF << CSIC_MULP_STATUS)
#define CSIC_MULP_INT_REG_OFF   		0X104
#define CSIC_MULP_INT_EN			0
#define CSIC_MULP_INT_EN_MASK			(0X3 << CSIC_MULP_INT_EN)
#define CSIC_MULP_DONE_PD			16
#define CSIC_MULP_DONE_PD_MASK  		(0X1 << CSIC_MULP_DONE_PD)
#define CSIC_MULP_ERR_PD			17
#define CSIC_MULP_ERR_PD_MASK			(0X1 << CSIC_MULP_ERR_PD)
#define CSIC_MULP_INT_PD_MASK			(0X3 << CSIC_MULP_DONE_PD)

/*
 *CSIC CCU registers
 */
#define CSIC_CCU_MODE_REG_OFF			0x000
#define CSIC_MCSI_CLK_MODE			0
#define CSIC_MCSI_CLK_MODE_MASK 		(0X1 << CSIC_MCSI_CLK_MODE)
#define CSIC_MCSI_POST_CLK_MODE 		1
#define CSIC_MCSI_POST_CLK_MODE_MASK		(0X1 << CSIC_MCSI_POST_CLK_MODE)
#define CSIC_CCU_CLK_GATING_DISABLE		31
#define CSIC_CCU_CLK_GATING_DISABLE_MASK	(0X1 << CSIC_CCU_CLK_GATING_DISABLE)

#define CSIC_CCU_PARSER_CLK_EN_REG_OFF  	0x004
#define CSIC_MCSI_PARSER0_CLK_EN		0
#define CSIC_MCSI_PARSER0_CLK_EN_MASK   	(0X1 << CSIC_MCSI_PARSER0_CLK_EN)
#define CSIC_MCSI_PARSER1_CLK_EN		1
#define CSIC_MCSI_PARSER1_CLK_EN_MASK   	(0X1 << CSIC_MCSI_PARSER1_CLK_EN)
#define CSIC_MCSI_COMBO0_CLK_EN 		8
#define CSIC_MCSI_COMBO0_CLK_EN_MASK		(0X1 << CSIC_MCSI_COMBO0_CLK_EN)
#define CSIC_MCSI_MIPI0_CLK_EN  		16
#define CSIC_MCSI_MIPI0_CLK_EN_MASK		(0X1 << CSIC_MCSI_MIPI0_CLK_EN)

#define CSIC_CCU_ISP_CLK_EN_REG_OFF		0x008
#define CSIC_MISP0_CLK_EN			0
#define CSIC_MISP0_CLK_EN_MASK  		(0X1 << CSIC_MISP0_CLK_EN)
#define CSIC_MISP0_BRIDGE_CLK_EN		4
#define CSIC_MISP0_BRIDGE_CLK_EN_MASK   	(0X1 << CSIC_MISP0_BRIDGE_CLK_EN)

#define CSIC_CCU_POST0_CLK_EN_REG_OFF   	0x00c
#define CSIC_MCSI_BK0_CLK_EN			0
#define CSIC_MCSI_BK0_CLK_EN_MASK		(0X1 << CSIC_MCSI_BK0_CLK_EN)
#define CSIC_MCSI_BK1_CLK_EN			1
#define CSIC_MCSI_BK1_CLK_EN_MASK		(0X1 << CSIC_MCSI_BK1_CLK_EN)
#define CSIC_MCSI_BK2_CLK_EN			2
#define CSIC_MCSI_BK2_CLK_EN_MASK		(0X1 << CSIC_MCSI_BK2_CLK_EN)
#define CSIC_MCSI_BK3_CLK_EN			3
#define CSIC_MCSI_BK3_CLK_EN_MASK		(0X1 << CSIC_MCSI_BK3_CLK_EN)
#define CSIC_MCSI_VIPP0_CLK_EN			8
#define CSIC_MCSI_VIPP0_CLK_EN_MASK		(0X1 << CSIC_MCSI_VIPP0_CLK_EN)
#define CSIC_MCSI_VIPP1_CLK_EN			9
#define CSIC_MCSI_VIPP1_CLK_EN_MASK		(0X1 << CSIC_MCSI_VIPP1_CLK_EN)
#define CSIC_MCSI_VIPP2_CLK_EN			10
#define CSIC_MCSI_VIPP2_CLK_EN_MASK		(0X1 << CSIC_MCSI_VIPP2_CLK_EN)
#define CSIC_MCSI_VIPP3_CLK_EN			11
#define CSIC_MCSI_VIPP3_CLK_EN_MASK		(0X1 << CSIC_MCSI_VIPP3_CLK_EN)
#define CSIC_MCSI_POST0_CLK_EN			16
#define CSIC_MCSI_POST0_CLK_EN_MASK		(0X1 << CSIC_MCSI_POST0_CLK_EN)

#endif /*__CSIC__TOP__REG__I__H__*/

