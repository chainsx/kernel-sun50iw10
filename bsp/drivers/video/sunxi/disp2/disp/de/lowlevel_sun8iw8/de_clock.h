/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __DE_CLOCK_H__
#define __DE_CLOCK_H__

#include "../include.h"
#include "de_feat.h"

#define DE_TOP_CFG(_clk_no, _freq, _ahb_gate_adr, _ahb_gate_shift, _ahb_reset_adr, _ahb_reset_shift,\
					_dram_gate_adr, _dram_gate_shift, _mod_adr, _mod_enable_shift, _mod_div_adr, _mod_div_shift, _mod_div_width)\
{\
	.clk_no = _clk_no,\
	.freq = _freq,\
	.ahb_gate_adr = (void __iomem *)_ahb_gate_adr,\
	.ahb_gate_shift = _ahb_gate_shift,\
	.ahb_reset_adr = (void __iomem *)_ahb_reset_adr,\
	.ahb_reset_shift = _ahb_reset_shift,\
	.dram_gate_adr = (void __iomem *)_dram_gate_adr,\
	.dram_gate_shift = _dram_gate_shift,\
	.mod_adr = (void __iomem *)_mod_adr,\
	.mod_enable_shift = _mod_enable_shift,\
	.mod_div_adr = (void __iomem *)_mod_div_adr,\
	.mod_div_shift = _mod_div_shift,\
	.mod_div_width = _mod_div_width,\
},

typedef enum {
	DE_CLK_NONE = 0,

	DE_CLK_CORE0 = 1,
	DE_CLK_CORE1 = 2,
	DE_CLK_WB = 3,
} de_clk_id;

typedef struct {
	de_clk_id clk_no;
	u32 freq;
	void __iomem *ahb_gate_adr;
	u32 ahb_gate_shift;
	void __iomem *ahb_reset_adr;
	u32 ahb_reset_shift;
	void __iomem *dram_gate_adr;
	u32 dram_gate_shift;
	void __iomem *mod_adr;
	u32 mod_enable_shift;
	void __iomem *mod_div_adr;
	u32 mod_div_shift;
	u32 mod_div_width;	/* bit */
} de_clk_para;

extern s32 de_clk_enable(u32 clk_no);
extern s32 de_clk_disable(u32 clk_no);
extern s32 de_clk_set_reg_base(u32 reg_base);

#endif
