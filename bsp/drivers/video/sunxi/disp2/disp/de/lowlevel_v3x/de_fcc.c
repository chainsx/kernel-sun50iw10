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
 *  All Winner Tech, All Right Reserved. 2015-2016 Copyright (c)
 *
 *  File name   :       de_fcc.c
 *
 *  Description :       display engine fcc base functions implement
 *
 *  History     :       2016/03/22  iptang  v0.1  Initial version
 *
 ******************************************************************************/
#ifdef CONFIG_AW_DISP2_SUPPORT_ENAHNCE

#include "de_fcc_type.h"
#include "de_rtmx.h"
#include "de_vep_table.h"
#include "de_enhance.h"

#define FCC_PARA_NUM (12)
static volatile struct __fcc_reg_t *fcc_dev[DE_NUM][CHN_NUM];
static volatile struct __fcc_lut_reg_t *lut_dev[DE_NUM][CHN_NUM];
static struct de_reg_blocks fcc_para_block[DE_NUM][CHN_NUM];
static struct de_reg_blocks fcc_lut_block[DE_NUM][CHN_NUM];
static uintptr_t fcc_base[DE_NUM][CHN_NUM] = { {0} };

/*******************************************************************************
 *   function       : de_fcc_set_reg_base(unsigned int sel, unsigned int chno,
 *                                        void *base)
 *   description    : set fcc reg base
 *   parameters     :
 *                    sel         <rtmx select>
 *                    chno        <overlay select>
 *                    base        <reg base>
 *   return         :
 *                    success
 ******************************************************************************/
int de_fcc_set_reg_base(unsigned int sel, unsigned int chno, void *base)
{
	fcc_dev[sel][chno] = (struct __fcc_reg_t *) base;

	return 0;
}

int de_fcc_set_lut_reg_base(unsigned int sel, unsigned int chno, void *base)
{
	lut_dev[sel][chno] = (struct __fcc_lut_reg_t *) base;

	return 0;
}

int de_fcc_init(unsigned int sel, unsigned int chno, uintptr_t reg_base)
{
	uintptr_t lut_base;
	void *memory;

	fcc_base[sel][chno] = reg_base + (sel + 1) * 0x00100000 + FCC_OFST;
	lut_base = fcc_base[sel][chno] + 0x100;

	/* FIXME  display path offset should be defined */
	memory = kmalloc(sizeof(struct __fcc_reg_t), GFP_KERNEL | __GFP_ZERO);
	if (memory == NULL) {
		__wrn("malloc vep fcc[%d][%d] memory fail! size=0x%x\n", sel,
		      chno, (unsigned int)sizeof(struct __fcc_reg_t));
		return -1;
	}

	fcc_para_block[sel][chno].off = fcc_base[sel][chno];
	fcc_para_block[sel][chno].val = memory;
	fcc_para_block[sel][chno].size = 0x58;
	fcc_para_block[sel][chno].dirty = 0;

	de_fcc_set_reg_base(sel, chno, memory);

	memory = kmalloc(sizeof(struct __fcc_lut_reg_t),
			 GFP_KERNEL | __GFP_ZERO);
	if (memory == NULL) {
		__wrn("malloc vep fcc lut[%d][%d] memory fail! size=0x%x\n",
		      sel, chno, (unsigned int)sizeof(struct __fcc_lut_reg_t));
		return -1;
	}

	fcc_lut_block[sel][chno].off = lut_base;
	fcc_lut_block[sel][chno].val = memory;
	fcc_lut_block[sel][chno].size = 0x400;
	fcc_lut_block[sel][chno].dirty = 0;

	de_fcc_set_lut_reg_base(sel, chno, memory);

	return 0;
}

int de_fcc_exit(unsigned int sel, unsigned int chno)
{
	kfree(fcc_para_block[sel][chno].val);
	kfree(fcc_lut_block[sel][chno].val);

	return 0;
}

int de_fcc_double_init(unsigned int sel, unsigned int chno, uintptr_t reg_base)
{
	uintptr_t fcc_base, lut_base;

	fcc_base = reg_base + (sel + 1) * 0x00100000 + FCC_OFST;
	lut_base = fcc_base + 0x100;

	de_fcc_set_reg_base(sel, chno, (void *)fcc_base);
	de_fcc_set_lut_reg_base(sel, chno, (void *)lut_base);

	return 0;
}

int de_fcc_double_exit(unsigned int sel, unsigned int chno)
{
	return 0;
}

int de_fcc_update_regs(unsigned int sel, unsigned int chno)
{
	uintptr_t base;

	if (fcc_para_block[sel][chno].dirty == 0x1) {
		memcpy((void *)fcc_para_block[sel][chno].off,
		       fcc_para_block[sel][chno].val,
		       fcc_para_block[sel][chno].size);
		fcc_para_block[sel][chno].dirty = 0x0;
	}

	if (fcc_lut_block[sel][chno].dirty == 0x1) {
		base = fcc_base[sel][chno];

		/* AHB access LUT */
		writel(0x0, (void __iomem *)(base + 0x50));
		memcpy((void *)fcc_lut_block[sel][chno].off,
		       fcc_lut_block[sel][chno].val,
		       fcc_lut_block[sel][chno].size);

		/* Module access LUT */
		writel(0x1, (void __iomem *)(base + 0x50));
		fcc_lut_block[sel][chno].dirty = 0x0;
	}

	return 0;
}

/*******************************************************************************
 * function       : de_fcc_enable(unsigned int sel, unsigned int chno,
 *                   unsigned int en)
 * description    : enable/disable fcc
 * parameters     :
 *                  sel         <rtmx select>
 *                  chno        <overlay select>
 *                  en          <enable: 0-diable; 1-enable>
 * return         :
 *                  success
 ******************************************************************************/
int de_fcc_enable(unsigned int sel, unsigned int chno, unsigned int en)
{
	unsigned int tmp;

	tmp = fcc_dev[sel][chno]->ctl.dwval & 0x170;
	tmp |= (7 << 4);
	tmp |= (en & 0x1);
	fcc_dev[sel][chno]->ctl.dwval = tmp;
	fcc_para_block[sel][chno].dirty = 1;

	return 0;
}

/*******************************************************************************
 * function       : de_fcc_set_size(unsigned int sel, unsigned int chno,
 *                  unsigned int width, unsigned int height)
 * description    : set fcc size
 * parameters     :
 *                  sel         <rtmx select>
 *                  chno        <overlay select>
 *                  width       <input width>
 *                                      height  <input height>
 * return         :
 *                  success
 ******************************************************************************/
int de_fcc_set_size(unsigned int sel, unsigned int chno, unsigned int width,
		    unsigned int height)
{
	unsigned int tmp;

	tmp = width == 0 ? 0 : width - 1;
	tmp |= ((height == 0 ? 0 : height - 1) << 16);
	fcc_dev[sel][chno]->size.dwval = tmp;

	fcc_para_block[sel][chno].dirty = 1;

	return 0;
}

/*******************************************************************************
 * function       : de_fcc_set_window(unsigned int sel, unsigned int chno,
 *                    unsigned int win_en, struct de_rect window)
 * description    : set fcc window
 * parameters     :
 *                  sel         <rtmx select>
 *                  chno        <overlay select>
 *                  win_en      <enable: 0-window mode diable;
 *                                       1-window mode enable>
 *                  window  <window rectangle>
 * return         :
 *                  success
 ******************************************************************************/
int de_fcc_set_window(unsigned int sel, unsigned int chno, unsigned int win_en,
		      struct de_rect window)
{
	unsigned int tmp;

	tmp = fcc_dev[sel][chno]->ctl.dwval & 0x71;
	tmp |= ((win_en & 0x1) << 8);
	fcc_dev[sel][chno]->ctl.dwval = tmp;
	if (win_en) {
		tmp = window.x | (window.y << 16);
		fcc_dev[sel][chno]->win0.dwval = tmp;
		tmp = (window.x + window.w - 1);
		tmp |= ((window.y + window.h - 1) << 16);
		fcc_dev[sel][chno]->win1.dwval = tmp;
	}

	fcc_para_block[sel][chno].dirty = 1;

	return 0;
}

int de_fcc_set_lut(unsigned int sel, unsigned int chno)
{
	unsigned int i, tmp;

	for (i = 0; i < 256; i++) {
		tmp = fcc_hue_tab[i << 1] | (fcc_hue_tab[(i << 1)+1] << 16);
		lut_dev[sel][chno]->lut[i].dwval = tmp;
	}

	fcc_lut_block[sel][chno].dirty = 1;

	return 0;
}

/*******************************************************************************
 * function       : de_fcc_set_para(unsigned int sel, unsigned int chno)
 * description    : set fcc para
 * parameters     :
 *                  sel         <rtmx select>
 *                  chno        <overlay select>
 *                  sgain
 * return         :
 *                  success
 ******************************************************************************/
int de_fcc_init_para(unsigned int sel, unsigned int chno)
{
	memcpy((void *)fcc_dev[sel][chno]->hue_range,
	       (void *)&fcc_range_gain[0], sizeof(int) * 6);
	memcpy((void *)fcc_dev[sel][chno]->hue_gain, (void *)&fcc_hue_gain[0],
	       sizeof(int) * 6);

	fcc_dev[sel][chno]->color_gain.dwval = 0x268;
	fcc_dev[sel][chno]->light_ctrl.dwval = 0x1000080;
	fcc_dev[sel][chno]->lut_ctrl.dwval = 0x1;
	fcc_para_block[sel][chno].dirty = 1;
	de_fcc_set_lut(sel, chno);

	return 0;
}

/*******************************************************************************
 * function       : de_fcc_info2para(unsigned int gain, struct de_rect window,
 *                    struct __fcc_config_data *para)
 * description    : info->para conversion
 * parameters     :
 *                  gain                <gain info from user>
 *                  window              <window info>
 *                  para                <bsp para>
 * return         :
 *                  success
 ******************************************************************************/
int de_fcc_info2para(unsigned int sel, unsigned int chno, unsigned int fmt,
		     unsigned int dev_type, struct __fcc_config_data *para,
		     unsigned int bypass)
{
	int sgain;
	unsigned int en;

	int fcc_para[FCC_PARA_NUM][2] = {
		/* lcd / hdmi */
		{-255,  -255},		/* 00 gain for yuv */
		{-128,  -128},		/* 01 */
		{-64,   -64},		/* 02 */
		{-32,   -32},		/* 03 */
		{0,      0},		/* 04 */
		{32,     32},		/* 05 */
		{64,     64},		/* 06 */
		{80,     80},		/* 07 */
		{96,     96},		/* 08 */
		{128,    128},		/* 09 */
		{192,    192},		/* 10 gain for yuv */
		{0,      0},		/* 11 gain for rgb */
	};

	/* parameters */
	if (fmt == 1)
		sgain = fcc_para[FCC_PARA_NUM - 1][dev_type];
	else
		sgain = fcc_para[para->level][dev_type];

	en = (((fmt == 0) && (sgain == 0)) || (bypass == 1)) ? 0 : 1;
	de_fcc_enable(sel, chno, en);

	fcc_dev[sel][chno]->sat_gain.dwval = sgain;
	fcc_para_block[sel][chno].dirty = 1;

	return 0;
}
#endif
