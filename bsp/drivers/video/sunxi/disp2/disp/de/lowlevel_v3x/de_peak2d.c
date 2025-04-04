/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

/*******************************************************************************
 *  All Winner Tech, All Right Reserved. 2014-2016 Copyright (c)
 *
 *  File name   :  display engine 3.0 2D-peak basic function definition
 *
 *  History     :  2016/03/30      vito cheng  v0.1  Initial version
 *
 ******************************************************************************/

#ifdef CONFIG_AW_DISP2_SUPPORT_ENAHNCE
#include "de_peak2d_type.h"
#include "de_rtmx.h"
#include "de_enhance.h"
#include "de_scaler.h"

#define PEAK2D_PARA_NUM (11)
int peak2d_para[PEAK2D_PARA_NUM][2] = {
	/* lcd tv */
	{0,      0},		/* gain for yuv */
	{5,      5},		/*              */
	{10,     10},		/*              */
	{15,     15},		/*              */
	{20,     20},		/*              */
	{25,     25},		/*              */
	{30,     30},		/*              */
	{35,     35},		/*              */
	{40,     40},		/*              */
	{45,     45},		/*              */
	{50,     50},		/* gain for yuv */
};

static struct __peak2d_reg_t *peak2d_dev[DE_NUM][CHN_NUM];
static struct de_reg_blocks peak2d_para_block[DE_NUM][CHN_NUM];

static int de_peak2d_set_reg_base(unsigned int sel, unsigned int chno,
				  void *base)
{
	peak2d_dev[sel][chno] = (struct __peak2d_reg_t *) base;

	return 0;
}

int de_peak2d_update_regs(unsigned int sel, unsigned int chno)
{
	if (peak2d_para_block[sel][chno].dirty == 0x1) {
		memcpy((void *)peak2d_para_block[sel][chno].off,
		       peak2d_para_block[sel][chno].val,
		       peak2d_para_block[sel][chno].size);
		peak2d_para_block[sel][chno].dirty = 0x0;
	}

	return 0;
}

int de_peak2d_init(unsigned int sel, unsigned int chno, uintptr_t reg_base)
{
	uintptr_t base;
	void *memory;

	base = reg_base + (sel + 1) * 0x00100000 + VSU_OFST
				+ chno * VSU_MEM_SIZE + PEAK2D_OFST;

	memory = kmalloc(sizeof(struct __peak2d_reg_t), GFP_KERNEL |
		 __GFP_ZERO);
	if (memory == NULL) {
		__wrn("malloc peak2d[%d][%d] memory fail! size=0x%x\n", sel,
		      chno, (unsigned int)sizeof(struct __peak2d_reg_t));
		return -1;
	}

	peak2d_para_block[sel][chno].off = base;
	peak2d_para_block[sel][chno].val = memory;
	peak2d_para_block[sel][chno].size = 0x10;
	peak2d_para_block[sel][chno].dirty = 0;

	de_peak2d_set_reg_base(sel, chno, memory);

	return 0;
}

int de_peak2d_double_init(unsigned int sel, unsigned int chno,
			  uintptr_t reg_base)
{
	uintptr_t base;

	base = reg_base + (sel + 1) * 0x00100000 + VSU_OFST
				+ chno * VSU_MEM_SIZE + PEAK2D_OFST;

	de_peak2d_set_reg_base(sel, chno, (void *)base);

	return 0;
}

int de_peak2d_exit(unsigned int sel, unsigned int chno)
{
	kfree(peak2d_para_block[sel][chno].val);

	return 0;
}

int de_peak2d_double_exit(unsigned int sel, unsigned int chno)
{
	return 0;
}

int de_peak2d_init_para(unsigned int sel, unsigned int chno)
{
	peak2d_dev[sel][chno]->en.dwval = 0x0;
	peak2d_dev[sel][chno]->coring.dwval = 0x20;
	peak2d_dev[sel][chno]->gain0.dwval = 0x03200078;
	peak2d_dev[sel][chno]->gain1.dwval = 0x01080000;

	peak2d_para_block[sel][chno].dirty = 0x1;

	return 0;
}

int de_peak2d_info2para(unsigned int sel, unsigned int chno,
			unsigned int fmt, unsigned int dev_type,
			struct __peak2d_config_data *para,
			unsigned int bypass)
{
	unsigned int level;
	unsigned int en;
	unsigned int gain;
	unsigned int linebuf = 2048;

	linebuf = de_feat_get_scale_linebuf_for_ed(sel, chno);

	/* enable when 1.scale up, 2.user level > 0, 3.YUV format, 4.inw<=lb */
	if ((para->inw < para->outw) && (para->inh < para->outh) &&
	    (para->level > 0) &&
	    (fmt == 0) &&
	    (para->inw <= linebuf) &&
	    bypass == 0)
		en = 1;
	else
		en = 0;

	if (en == 0)
		peak2d_dev[sel][chno]->en.dwval = 0x0;
	else {
		peak2d_dev[sel][chno]->en.dwval = 0x1;

		level = para->level > (PEAK2D_PARA_NUM - 1) ?
			(PEAK2D_PARA_NUM - 1) : para->level;
		gain = peak2d_para[level][dev_type];
		de_set_bits(&peak2d_dev[sel][chno]->gain1.dwval, gain, 0, 8);
	}

	peak2d_para_block[sel][chno].dirty = 0x1;
	return 0;
}

int de_peak2d_enable(unsigned int sel, unsigned int chno, unsigned int en)
{
	de_set_bits(&peak2d_dev[sel][chno]->en.dwval, en, 0, 1);
	peak2d_para_block[sel][chno].dirty = 1;

	return 0;
}

#endif
