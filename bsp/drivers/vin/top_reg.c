/*
 * linux-5.4/drivers/media/platform/sunxi-vin/top_reg.c
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

#include <linux/kernel.h>
#include "top_reg_i.h"
#include "top_reg.h"

#include "utility/vin_io.h"

/*isp_id isp_input pasrer_id parser_ch*/
static char isp_input[4][4][4][4] = {
#if defined(CONFIG_ARCH_SUN50IW3P1) || defined(CONFIG_ARCH_SUN50IW6P1)
	/*isp0 input0~3*/
		/*parser0*/     /*parse1*/    /*parser2*/    /*parser3*/
	{
		{{0, 0, 0, 0}, {1, 2, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} },
		{{0, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} },
		{{0, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} },
		{{0, 0, 0, 0}, {0, 0, 0, 1}, {0, 0, 0, 0}, {0, 0, 0, 0} }
	},

	/*isp1 input0~3*/
	{
		{{1, 2, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} },
		{{0, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} },
		{{0, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} },
		{{0, 0, 0, 0}, {0, 0, 0, 1}, {0, 0, 0, 0}, {0, 0, 0, 0} }
	},
#elif defined (CONFIG_ARCH_SUN8IW15P1) || defined (CONFIG_ARCH_SUN8IW17P1) || defined (CONFIG_ARCH_SUN8IW16P1)
	/*isp0 input0~3*/
	{
		{{0, 4, 0, 0}, {1, 5, 0, 0}, {2, 6, 0, 0}, {3, 7, 0, 0} },
		{{0, 0, 0, 0}, {0, 1, 0, 0}, {0, 2, 0, 0}, {0, 3, 0, 0} },
		{{0, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 2, 0}, {0, 0, 3, 0} },
		{{0, 0, 0, 0}, {0, 0, 0, 1}, {0, 0, 0, 2}, {0, 0, 0, 3} }
	},

	/*isp1 input0~3*/
	{
		{{0, 4, 0, 0}, {1, 5, 0, 0}, {2, 6, 0, 0}, {3, 7, 0, 0} },
		{{0, 0, 0, 0}, {0, 1, 0, 0}, {0, 2, 0, 0}, {0, 3, 0, 0} },
		{{0, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 2, 0}, {0, 0, 3, 0} },
		{{0, 0, 0, 0}, {0, 0, 0, 1}, {0, 0, 0, 2}, {0, 0, 0, 3} }
	},

	/*isp2 input0~3*/
	{
		{{0, 4, 0, 0}, {1, 5, 0, 0}, {2, 6, 0, 0}, {3, 7, 0, 0} },
		{{0, 0, 0, 0}, {0, 1, 0, 0}, {0, 2, 0, 0}, {0, 3, 0, 0} },
		{{0, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 2, 0}, {0, 0, 3, 0} },
		{{0, 0, 0, 0}, {0, 0, 0, 1}, {0, 0, 0, 2}, {0, 0, 0, 3} }
	},

	/*isp3 input0~3*/
	{
		{{0, 4, 0, 0}, {1, 5, 0, 0}, {2, 6, 0, 0}, {3, 7, 0, 0} },
		{{0, 0, 0, 0}, {0, 1, 0, 0}, {0, 2, 0, 0}, {0, 3, 0, 0} },
		{{0, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 2, 0}, {0, 0, 3, 0} },
		{{0, 0, 0, 0}, {0, 0, 0, 1}, {0, 0, 0, 2}, {0, 0, 0, 3} }
	}
#else
	/*isp0 input0~3*/
	{
		{{0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}, {12, 13, 14, 15} },
		{{0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}, {12, 13, 14, 15} },
		{{0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}, {12, 13, 14, 15} },
		{{0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}, {12, 13, 14, 15} }
	},

	/*isp1 input0~3*/
	{
		{{0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}, {12, 13, 14, 15} },
		{{0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}, {12, 13, 14, 15} },
		{{0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}, {12, 13, 14, 15} },
		{{0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}, {12, 13, 14, 15} }
	},

	/*isp2 input0~3*/
	{
		{{0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}, {12, 13, 14, 15} },
		{{0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}, {12, 13, 14, 15} },
		{{0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}, {12, 13, 14, 15} },
		{{0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}, {12, 13, 14, 15} }
	},

	/*isp3 input0~3*/
	{
		{{0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}, {12, 13, 14, 15} },
		{{0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}, {12, 13, 14, 15} },
		{{0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}, {12, 13, 14, 15} },
		{{0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}, {12, 13, 14, 15} }
	},
#endif
};

/*vipp_id isp_id isp_ch*/
static char vipp_input[8][4][4] = {
#if defined(CONFIG_ARCH_SUN50IW3P1) || defined(CONFIG_ARCH_SUN50IW6P1)
	/*vipp0*/
	/*isp0*/        /*isp1*/      /*isp2*/       /*isp3*/
	{{0, 0, 0, 0}, {1, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} },

	{{0, 2, 0, 0}, {1, 3, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} },

	{{0, 0, 2, 0}, {1, 0, 3, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} },

	{{0, 0, 0, 2}, {1, 4, 0, 3}, {0, 0, 0, 0}, {0, 0, 0, 0} },
#elif defined (CONFIG_ARCH_SUN8IW15P1) || defined (CONFIG_ARCH_SUN8IW17P1) || defined (CONFIG_ARCH_SUN8IW16P1)
	{{0, 0, 0, 0}, {1, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} },

	{{0, 2, 0, 0}, {1, 3, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} },

	{{0, 0, 2, 0}, {1, 0, 3, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} },

	{{0, 0, 0, 2}, {1, 4, 0, 3}, {0, 0, 0, 0}, {0, 0, 0, 0} },

	{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {1, 0, 0, 0} },

	{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 2, 0, 0}, {1, 3, 0, 0} },

	{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 2, 0}, {1, 0, 3, 0} },

	{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 2}, {1, 4, 0, 3} }
#else
	{{0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}, {12, 13, 14, 15} },

	{{0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}, {12, 13, 14, 15} },

	{{0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}, {12, 13, 14, 15} },

	{{0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}, {12, 13, 14, 15} },

	{{0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}, {12, 13, 14, 15} },

	{{0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}, {12, 13, 14, 15} },

	{{0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}, {12, 13, 14, 15} },

	{{0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}, {12, 13, 14, 15} },
#endif
};

#if defined (CONFIG_ARCH_SUN50IW9)
static char dma_input[8][4][4] = {
	/*parser0*/     /*parser1*/    /*parser2*/       /*parser3*/
	{{0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}, {12, 13, 14, 15} }, /*dma0*/

	{{0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}, {12, 13, 14, 15} }, /*dma1*/

	{{0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}, {12, 13, 14, 15} }, /*dma2*/

	{{0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}, {12, 13, 14, 15} }, /*dma3*/

	{{0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}, {12, 13, 14, 15} }, /*dma4*/

	{{0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}, {12, 13, 14, 15} }, /*dma5*/

	{{0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}, {12, 13, 14, 15} }, /*dma6*/

	{{0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}, {12, 13, 14, 15} }  /*dma7*/
};
#endif

volatile void __iomem *csic_top_base[MAX_CSIC_TOP_NUM];
volatile void __iomem *csic_ccu_base;

/*
 * functions about top register
 */
int csic_top_set_base_addr(unsigned int sel, unsigned long addr)
{
	if (sel > MAX_CSIC_TOP_NUM - 1)
		return -1;
	csic_top_base[sel] = (volatile void __iomem *)addr;

	return 0;
}

void csic_top_enable(unsigned int sel)
{
	vin_reg_clr_set(csic_top_base[sel] + CSIC_TOP_EN_REG_OFF,
			CSIC_TOP_EN_MASK, 1 << CSIC_TOP_EN);
}

void csic_top_disable(unsigned int sel)
{
	vin_reg_clr_set(csic_top_base[sel] + CSIC_TOP_EN_REG_OFF,
			CSIC_TOP_EN_MASK, 0 << CSIC_TOP_EN);
}

void csic_isp_bridge_enable(unsigned int sel)
{
	vin_reg_clr_set(csic_top_base[sel] + CSIC_TOP_EN_REG_OFF,
			CSIC_ISP_BRIDGE_EN_MASK, 1 << CSIC_ISP_BRIDGE_EN);
}

void csic_isp_bridge_disable(unsigned int sel)
{
	vin_reg_clr_set(csic_top_base[sel] + CSIC_TOP_EN_REG_OFF,
			CSIC_ISP_BRIDGE_EN_MASK, 0 << CSIC_ISP_BRIDGE_EN);
}

void csic_top_sram_pwdn(unsigned int sel, unsigned int en)
{
	vin_reg_clr_set(csic_top_base[sel] + CSIC_TOP_EN_REG_OFF,
			CSIC_SRAM_PWDN_MASK, en << CSIC_SRAM_PWDN);
}

void csic_top_version_read_en(unsigned int sel, unsigned int en)
{
	vin_reg_clr_set(csic_top_base[sel] + CSIC_TOP_EN_REG_OFF,
			CSIC_VER_EN_MASK, en << CSIC_VER_EN);
}

void csic_isp_input_select(unsigned int sel, unsigned int isp, unsigned int in,
				unsigned int psr, unsigned int ch)
{
	vin_reg_writel(csic_top_base[sel] + CSIC_ISP0_IN0_REG_OFF + isp * 16 + in * 4,
			isp_input[isp][in][psr][ch]);
}

void csic_vipp_input_select(unsigned int sel, unsigned int vipp,
				unsigned int isp, unsigned int ch)
{
	vin_reg_writel(csic_top_base[sel] + CSIC_VIPP0_IN_REG_OFF + vipp * 4,
			vipp_input[vipp][isp][ch]);
}

void csic_dma_input_select(unsigned int sel, unsigned int dma,
				unsigned int parser, unsigned int ch)
{
#if defined (CONFIG_ARCH_SUN50IW9)
	vin_reg_writel(csic_top_base[sel] + CSIC_VIPP0_IN_REG_OFF + dma * 4,
			dma_input[dma][parser][ch]);
#endif
}

void csic_feature_list_get(unsigned int sel, struct csic_feature_list *fl)
{
	unsigned int reg_val = 0;

	reg_val = vin_reg_readl(csic_top_base[sel] + CSIC_FEATURE_REG_OFF);
	fl->dma_num = (reg_val & CSIC_DMA_NUM_MASK) >> CSIC_DMA_NUM;
	fl->vipp_num = (reg_val & CSIC_VIPP_NUM_MASK) >> CSIC_VIPP_NUM;
	fl->isp_num = (reg_val & CSIC_ISP_NUM_MASK) >> CSIC_ISP_NUM;
	fl->ncsi_num = (reg_val & CSIC_NCSI_NUM_MASK) >> CSIC_NCSI_NUM;
	fl->mcsi_num = (reg_val & CSIC_MCSI_NUM_MASK) >> CSIC_MCSI_NUM;
	fl->parser_num = (reg_val & CSIC_PARSER_NUM_MASK) >> CSIC_PARSER_NUM;
}

void csic_version_get(unsigned int sel, struct csic_version *v)
{
	unsigned int reg_val = 0;

	reg_val = vin_reg_readl(csic_top_base[sel] + CSIC_VER_REG_OFF);
	v->ver_small = (reg_val & CSIC_VER_SMALL_MASK) >> CSIC_VER_SMALL;
	v->ver_big = (reg_val & CSIC_VER_BIG_MASK) >> CSIC_VER_BIG;
}


void csic_mbus_req_mex_set(unsigned int sel, unsigned int data)
{
#if !defined CONFIG_ARCH_SUN8IW15P1 && !defined CONFIG_ARCH_SUN8IW16P1 && !defined CONFIG_ARCH_SUN8IW17P1 && !defined CONFIG_ARCH_SUN50IW9 && !defined CONFIG_ARCH_SUN50IW3P1 && !defined CONFIG_ARCH_SUN50IW6P1
	vin_reg_clr_set(csic_top_base[sel] + CSIC_MBUS_REQ_MAX,
			MCSI_MEM_REQ_MAX_MASK, data << MCSI_MEM_REQ_MAX);
	vin_reg_clr_set(csic_top_base[sel] + CSIC_MBUS_REQ_MAX,
			MCSI_MEM_1_REQ_MAX_MASK, data << MCSI_MEM_1_REQ_MAX);
	vin_reg_clr_set(csic_top_base[sel] + CSIC_MBUS_REQ_MAX,
			MISP_MEM_REQ_MAX_MASK, data << MISP_MEM_REQ_MAX);
#endif
}

void csic_mulp_mode_en(unsigned int sel, unsigned int en)
{
	vin_reg_clr_set(csic_top_base[sel] + CSIC_MULP_MODE_REG_OFF,
			CSIC_MULP_EN_MASK, en << CSIC_MULP_EN);
}

void csic_mulp_dma_cs(unsigned int sel, enum csic_mulp_cs cs)
{
	vin_reg_clr_set(csic_top_base[sel] + CSIC_MULP_MODE_REG_OFF,
			CSIC_MULP_CS_MASK, cs << CSIC_MULP_CS);
}

void csic_mulp_int_enable(unsigned int sel, enum csis_mulp_int interrupt)
{
	vin_reg_clr_set(csic_top_base[sel] + CSIC_MULP_INT_REG_OFF,
			CSIC_MULP_INT_EN_MASK, interrupt << CSIC_MULP_INT_EN);
}

void csic_mulp_int_disable(unsigned int sel, enum csis_mulp_int interrupt)
{
	vin_reg_clr_set(csic_top_base[sel] + CSIC_MULP_INT_REG_OFF,
			CSIC_MULP_INT_EN_MASK, ~interrupt << CSIC_MULP_INT_EN);
}

void csic_mulp_int_get_status(unsigned int sel, struct cisc_mulp_int_status *status)
{
	unsigned int reg_val = vin_reg_readl(csic_top_base[sel] + CSIC_MULP_INT_REG_OFF);

	status->mulf_done = (reg_val & CSIC_MULP_DONE_PD_MASK) >> CSIC_MULP_DONE_PD;
	status->mulf_err = (reg_val & CSIC_MULP_ERR_PD_MASK) >> CSIC_MULP_ERR_PD;
}

void csic_mulp_int_clear_status(unsigned int sel, enum csis_mulp_int interrupt)
{
	vin_reg_clr_set(csic_top_base[sel] + CSIC_MULP_INT_REG_OFF,
			CSIC_MULP_INT_PD_MASK, interrupt << CSIC_MULP_DONE_PD);
}

void csic_ptn_generation_en(unsigned int sel, unsigned int en)
{
	vin_reg_clr_set(csic_top_base[sel] + CSIC_PTN_GEN_EN_REG_OFF,
			CSIC_PTN_GEN_CYCLE_MASK, 0 << CSIC_PTN_GEN_CYCLE);
	vin_reg_clr_set(csic_top_base[sel] + CSIC_PTN_GEN_EN_REG_OFF,
			CSIC_PTN_GEN_EN_MASK, en << CSIC_PTN_GEN_EN);
	vin_reg_clr_set(csic_top_base[sel] + CSIC_PTN_GEN_EN_REG_OFF,
			CSIC_PTN_GEN_START_MASK, en << CSIC_PTN_GEN_START);
}

void csic_ptn_control(unsigned int sel, int mode, int dw, int port)
{
	vin_reg_clr_set(csic_top_base[sel] + CSIC_PTN_CTRL_REG_OFF,
			CSIC_PTN_CLK_DIV_MASK, 0 << CSIC_PTN_CLK_DIV);
	vin_reg_clr_set(csic_top_base[sel] + CSIC_PTN_CTRL_REG_OFF,
			CSIC_PTN_MODE_MASK, mode << CSIC_PTN_MODE);
	vin_reg_clr_set(csic_top_base[sel] + CSIC_PTN_CTRL_REG_OFF,
			CSIC_PTN_DATA_WIDTH_MASK, dw << CSIC_PTN_DATA_WIDTH);
	vin_reg_clr_set(csic_top_base[sel] + CSIC_PTN_CTRL_REG_OFF,
			CSIC_PTN_PORT_SEL_MASK, port << CSIC_PTN_PORT_SEL);
}

void csic_ptn_length(unsigned int sel, unsigned int len)
{
	vin_reg_writel(csic_top_base[sel] + CSIC_PTN_LEN_REG_OFF, len);
}

void csic_ptn_addr(unsigned int sel, unsigned long dma_addr)
{
	vin_reg_writel(csic_top_base[sel] + CSIC_PTN_ADDR_REG_OFF, dma_addr >> 2);
}

void csic_ptn_size(unsigned int sel, unsigned int w, unsigned int h)
{
	vin_reg_clr_set(csic_top_base[sel] + CSIC_PTN_SIZE_REG_OFF,
			CSIC_PTN_WIDTH_MASK, w << CSIC_PTN_WIDTH);
	vin_reg_clr_set(csic_top_base[sel] + CSIC_PTN_SIZE_REG_OFF,
			CSIC_PTN_HEIGHT_MASK, h << CSIC_PTN_HEIGHT);
}

/*
 * functions about ccu register
 */
int csic_ccu_set_base_addr(unsigned long addr)
{
	csic_ccu_base = (volatile void __iomem *)addr;

	return 0;
}

void csic_ccu_clk_gating_enable(void)
{
	vin_reg_clr_set(csic_ccu_base + CSIC_CCU_MODE_REG_OFF,
			CSIC_CCU_CLK_GATING_DISABLE_MASK, 0 << CSIC_CCU_CLK_GATING_DISABLE);
}

void csic_ccu_clk_gating_disable(void)
{
	vin_reg_clr_set(csic_ccu_base + CSIC_CCU_MODE_REG_OFF,
			CSIC_CCU_CLK_GATING_DISABLE_MASK, 1 << CSIC_CCU_CLK_GATING_DISABLE);
}

void csic_ccu_mcsi_clk_mode(unsigned int mode)
{
	vin_reg_clr_set(csic_ccu_base + CSIC_CCU_MODE_REG_OFF,
			CSIC_MCSI_CLK_MODE_MASK, mode << CSIC_MCSI_CLK_MODE);
	vin_reg_clr_set(csic_ccu_base + CSIC_CCU_MODE_REG_OFF,
			CSIC_MCSI_POST_CLK_MODE_MASK, mode << CSIC_MCSI_POST_CLK_MODE);
}

void csic_ccu_mcsi_combo_clk_en(unsigned int sel, unsigned int en)
{
	vin_reg_clr_set(csic_ccu_base + CSIC_CCU_PARSER_CLK_EN_REG_OFF,
			CSIC_MCSI_COMBO0_CLK_EN_MASK << sel, en << (CSIC_MCSI_COMBO0_CLK_EN + sel));
}

void csic_ccu_mcsi_mipi_clk_en(unsigned int sel, unsigned int en)
{
	vin_reg_clr_set(csic_ccu_base + CSIC_CCU_PARSER_CLK_EN_REG_OFF,
			CSIC_MCSI_MIPI0_CLK_EN_MASK << sel, en << (CSIC_MCSI_MIPI0_CLK_EN + sel));
}

void csic_ccu_mcsi_parser_clk_en(unsigned int sel, unsigned int en)
{
	vin_reg_clr_set(csic_ccu_base + CSIC_CCU_PARSER_CLK_EN_REG_OFF,
			CSIC_MCSI_PARSER0_CLK_EN_MASK << sel, en << (CSIC_MCSI_PARSER0_CLK_EN + sel));
}

void csic_ccu_misp_isp_clk_en(unsigned int sel, unsigned int en)
{
	vin_reg_clr_set(csic_ccu_base + CSIC_CCU_ISP_CLK_EN_REG_OFF,
			CSIC_MISP0_CLK_EN_MASK << sel, en << (CSIC_MISP0_CLK_EN + sel));
	vin_reg_clr_set(csic_ccu_base + CSIC_CCU_ISP_CLK_EN_REG_OFF,
			CSIC_MISP0_BRIDGE_CLK_EN_MASK << sel, en << (CSIC_MISP0_BRIDGE_CLK_EN + sel));
}

void csic_ccu_mcsi_post_clk_enable(unsigned int sel)
{
	vin_reg_clr_set(csic_ccu_base + CSIC_CCU_POST0_CLK_EN_REG_OFF + sel*4,
			CSIC_MCSI_POST0_CLK_EN_MASK, 1 << CSIC_MCSI_POST0_CLK_EN);
}

void csic_ccu_mcsi_post_clk_disable(unsigned int sel)
{
	vin_reg_clr_set(csic_ccu_base + CSIC_CCU_POST0_CLK_EN_REG_OFF + sel*4,
			CSIC_MCSI_POST0_CLK_EN_MASK, 0 << CSIC_MCSI_POST0_CLK_EN);
}

void csic_ccu_bk_clk_en(unsigned int sel, unsigned int en)
{
	if (sel < 4)
		vin_reg_clr_set(csic_ccu_base + CSIC_CCU_POST0_CLK_EN_REG_OFF,
			CSIC_MCSI_BK0_CLK_EN_MASK << sel, en << (CSIC_MCSI_BK0_CLK_EN + sel));
	else
		vin_reg_clr_set(csic_ccu_base + CSIC_CCU_POST0_CLK_EN_REG_OFF + 0x4,
			CSIC_MCSI_BK0_CLK_EN_MASK << (sel-4), en << (CSIC_MCSI_BK0_CLK_EN + (sel-4)));
}

void csic_ccu_vipp_clk_en(unsigned int sel, unsigned int en)
{
	if (sel < 4)
		vin_reg_clr_set(csic_ccu_base + CSIC_CCU_POST0_CLK_EN_REG_OFF,
			CSIC_MCSI_VIPP0_CLK_EN_MASK << sel, en << (CSIC_MCSI_VIPP0_CLK_EN + sel));
	else
		vin_reg_clr_set(csic_ccu_base + CSIC_CCU_POST0_CLK_EN_REG_OFF + 0x4,
			CSIC_MCSI_VIPP0_CLK_EN_MASK << (sel-4), en << (CSIC_MCSI_VIPP0_CLK_EN + (sel-4)));
}

