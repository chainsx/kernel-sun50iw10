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
 *  File name   :       de_lti.c
 *
 *  Description :       display engine 3.0 LTI basic function definition
 *
 *  History     :       2016-3-3 zzz  v0.1  Initial version
 *
 ******************************************************************************/

#ifdef CONFIG_AW_DISP2_SUPPORT_ENAHNCE
#include "de_lti_type.h"
#include "de_enhance.h"

#define LTI_PARA_NUM (12)
int lti_para[LTI_PARA_NUM][2] = {
	/* lcd / hdmi */
	{0,     0},		/* 00 gain for yuv */
	{1,     1},		/* 01 */
	{2,     1},		/* 02 */
	{3,     2},		/* 03 */
	{4,     2},		/* 04 */
	{5,     3},		/* 05 */
	{6,     3},		/* 06 */
	{7,     4},		/* 07 */
	{8,     4},		/* 08 */
	{9,     5},		/* 09 */
	{10,    5},		/* 10 gain for yuv */
	{2,     1},		/* 11 gain for rgb */
};

static struct __lti_reg_t *lti_dev[DE_NUM][CHN_NUM];
static struct __lti_reg_t *cti_dev[DE_NUM][CHN_NUM];
static struct de_reg_blocks lti_block[DE_NUM][CHN_NUM];
static struct de_reg_blocks cti_block[DE_NUM][CHN_NUM];

/*******************************************************************************
 * function       : de_dns_set_reg_base(unsigned int sel, unsigned int chno,
 *                  void *base, void *base1)
 * description    : set lti reg base
 * parameters     :
 *                  sel         <rtmx select>
 *                  chno        <overlay select>
 *                  base        <reg base>
 * return         :
 *                  success
 ******************************************************************************/
int de_lti_set_reg_base(unsigned int sel, unsigned int chno,
		void *base, void *base1)
{
	lti_dev[sel][chno] = (struct __lti_reg_t *) base;
	cti_dev[sel][chno] = (struct __lti_reg_t *) base1;

	return 0;
}

int de_lti_update_regs(unsigned int sel, unsigned int chno)
{
	if (lti_block[sel][chno].dirty == 0x1) {
		memcpy((void *)lti_block[sel][chno].off,
			lti_block[sel][chno].val, lti_block[sel][chno].size);
		lti_block[sel][chno].dirty = 0x0;
	}

	if (cti_block[sel][chno].dirty == 0x1) {
		memcpy((void *)cti_block[sel][chno].off,
			cti_block[sel][chno].val, cti_block[sel][chno].size);
		cti_block[sel][chno].dirty = 0x0;
	}

	return 0;
}

int de_lti_init(unsigned int sel, unsigned int chno, uintptr_t reg_base)
{
	uintptr_t base;
	void *memory;
	void *memory1;

	/* FIXME  display path offset should be defined */
	base = reg_base + (sel + 1) * 0x00100000 + LTI_OFST;
	__inf("sel %d, lti_base[%d]=0x%p\n", sel, chno, (void *)base);

	memory = kmalloc(sizeof(struct __lti_reg_t), GFP_KERNEL | __GFP_ZERO);
	if (memory == NULL) {
		__wrn("malloc lti[%d][%d] memory fail! size=0x%x\n", sel, chno,
			    (unsigned int)sizeof(struct __lti_reg_t));
		return -1;
	}

	lti_block[sel][chno].off = base;
	lti_block[sel][chno].val = memory;
	lti_block[sel][chno].size = 0x44;
	lti_block[sel][chno].dirty = 0;

	memory1 = kmalloc(sizeof(struct __lti_reg_t), GFP_KERNEL | __GFP_ZERO);
	if (memory1 == NULL) {
		__wrn("malloc cti[%d][%d] memory fail! size=0x%x\n", sel, chno,
			    (unsigned int)sizeof(struct __lti_reg_t));
		return -1;
	}

	cti_block[sel][chno].off = base + 0x100;
	cti_block[sel][chno].val = memory1;
	cti_block[sel][chno].size = 0x44;
	cti_block[sel][chno].dirty = 0;
	de_lti_set_reg_base(sel, chno, memory, memory1);

	return 0;
}

int de_lti_double_init(unsigned int sel, unsigned int chno, uintptr_t reg_base)
{
	uintptr_t base;

	/* FIXME  display path offset should be defined */
	base = reg_base + (sel + 1) * 0x00100000 + LTI_OFST;
	__inf("sel %d, lti_base[%d]=0x%p\n", sel, chno, (void *)base);

	de_lti_set_reg_base(sel, chno, (void *)base, (void *)(base + 0x100));

	return 0;
}

int de_lti_exit(unsigned int sel, unsigned int chno)
{
	kfree(lti_block[sel][chno].val);
	kfree(cti_block[sel][chno].val);
	return 0;
}

int de_lti_double_exit(unsigned int sel, unsigned int chno)
{
	return 0;
}

/*******************************************************************************
 * function       : de_lti_enable(unsigned int sel, unsigned int chno,
 *                  unsigned int en)
 * description    : enable/disable lti
 * parameters     :
 *                  sel         <rtmx select>
 *                  chno        <overlay select>
 *                  en          <enable: 0-diable; 1-enable>
 * return         :
 *                  success
 ******************************************************************************/
int de_lti_enable(unsigned int sel, unsigned int chno, unsigned int en)
{
	de_set_bits(&lti_dev[sel][chno]->ctrl.dwval, en, 0, 1);
	lti_block[sel][chno].dirty = 1;
	de_set_bits(&cti_dev[sel][chno]->ctrl.dwval, en, 0, 1);
	cti_block[sel][chno].dirty = 1;
	return 0;
}

/*******************************************************************************
 * function       : de_lti_set_size(unsigned int sel, unsigned int chno,
 *                    unsigned int width, unsigned int height)
 * description    : set lti size
 * parameters     :
 *                  sel         <rtmx select>
 *                  chno        <overlay select>
 *                  width       <input width>
 *                                      height  <input height>
 * return         :
 *                  success
*******************************************************************************/
int de_lti_set_size(unsigned int sel, unsigned int chno, unsigned int width,
			  unsigned int height)
{
	lti_dev[sel][chno]->size.dwval = (height - 1) << 16 | (width - 1);
	lti_block[sel][chno].dirty = 1;
	cti_dev[sel][chno]->size.dwval = (height - 1) << 16 | (width - 1);
	cti_block[sel][chno].dirty = 1;
	return 0;
}

/*******************************************************************************
 * function       : de_lti_set_window(unsigned int sel, unsigned int chno,
 *                   unsigned int win_enable, de_rect window)
 * description    : set lti window
 * parameters     :
 *                  sel         <rtmx select>
 *                  chno        <overlay select>
 *                  win_enable  <enable: 0-window mode diable;
 *                                       1-window mode enable>
 *                  window  <window rectangle>
 * return         :
 *                  success
 ******************************************************************************/
int de_lti_set_window(unsigned int sel, unsigned int chno,
			    unsigned int win_enable, struct de_rect window)
{
	de_set_bits(&lti_dev[sel][chno]->ctrl.dwval, win_enable, 24, 1);

	if (win_enable) {
		lti_dev[sel][chno]->win0.dwval = window.y << 16 | window.x;
		lti_dev[sel][chno]->win1.dwval =
			  (window.y + window.h - 1) << 16 |
			  (window.x + window.w - 1);
	}
	lti_block[sel][chno].dirty = 1;

	de_set_bits(&cti_dev[sel][chno]->ctrl.dwval, win_enable, 24, 1);

	if (win_enable) {
		cti_dev[sel][chno]->win0.dwval = window.y << 16 | window.x;
		cti_dev[sel][chno]->win1.dwval =
			  (window.y + window.h - 1) << 16 |
			  (window.x + window.w - 1);
	}
	cti_block[sel][chno].dirty = 1;
	return 0;
}

/*******************************************************************************
 * function       : de_lti_init_para(unsigned int sel, unsigned int chno)
 * description    : set lti para
 * parameters     :
 *                  sel         <rtmx select>
 *                  chno        <overlay select>
 * return         :
 *                  success
 ******************************************************************************/
int de_lti_init_para(unsigned int sel, unsigned int chno)
{
	/* lti */
	lti_dev[sel][chno]->ctrl.dwval = 0 << 16 | 1 << 8 | 0;

	lti_dev[sel][chno]->corth.dwval = 4;
	lti_dev[sel][chno]->diff.dwval = 4 << 16 | 32;
	lti_dev[sel][chno]->edge_gain.dwval = 1;
	lti_dev[sel][chno]->os_con.dwval = 1 << 28 | 40 << 16 | 0;

	lti_dev[sel][chno]->win_range.dwval = 2;
	lti_dev[sel][chno]->elvel_th.dwval = 16;

	lti_block[sel][chno].dirty = 1;

	/* cti */
	cti_dev[sel][chno]->ctrl.dwval = 0 << 16 | 1 << 8 | 0;

	cti_dev[sel][chno]->corth.dwval = 2;
	cti_dev[sel][chno]->diff.dwval = 4 << 16 | 2;
	cti_dev[sel][chno]->edge_gain.dwval = 1;
	cti_dev[sel][chno]->os_con.dwval = 1 << 28 | 40 << 16 | 0;

	cti_dev[sel][chno]->win_range.dwval = 2;
	cti_dev[sel][chno]->elvel_th.dwval = 32;

	cti_block[sel][chno].dirty = 1;

	return 0;
}

/*******************************************************************************
 * function       : sel_coef(int insz, int outsz, int* coef)
 * description    : select win coef for internal
 * parameters     :
 *                  insz         <image in width>
 *                  outsz        <image out width>
 *                  coef        <return coef>
 * return         :
 *                  win_sel    for register setting
 ******************************************************************************/
int sel_coef(int insz, int outsz, int *coef)
{
	int i;
	int ratio;
	int win_sel = 0;

	if (insz != 0) {
		ratio = (outsz + insz / 2) / insz;
		ratio = (ratio > 7) ? 7 : ratio;
	} else
		ratio = 1;

	for (i = 0; i < 8; i++)
		coef[i] = 0;

	if (ratio < 3) {
		coef[0] = 64;
		coef[1] = 32;
		coef[2] = -16;
		coef[3] = -32;
		coef[4] = -16;
		win_sel = 0;
	} else if (ratio == 3 || ratio == 4) {
		coef[0] = 64;
		coef[ratio] = -32;
		win_sel = 1;
	} else if (ratio >= 5) {
		coef[0] = 64;
		coef[ratio] = -32;
		win_sel = 2;
	}

	return win_sel;
}

/*******************************************************************************
 * function       : de_lti_info2para(unsigned int sel, unsigned int chno,
 *                  unsigned int fmt, unsigned int dev_type,
 *                  __dns_config_data *para)
 * description    : info->para conversion
 * parameters     :
 *                  sel               <rtmx select>
 *                  chno              <overlay select>
 *                  fmt               <rgb/yuv>
 *                  dev_type          <output dev>
 *                  para              <bsp para>
 * return         :
 *                  success
 ******************************************************************************/
int de_lti_info2para(unsigned int sel, unsigned int chno,
			   unsigned int fmt, unsigned int dev_type,
			   struct __lti_config_data *para,
			   unsigned int bypass)
{
	int gain, en, win_sel;
	int coef[8];

	/* parameters */
	en = (((fmt == 0) && (para->level == 0)) || (bypass == 1)) ? 0 : 1;
	para->mod_en = en; /* return enable info */
	if (en == 0) {
		/* if level=0, module will be disabled,
		 * no need to set para register
		 */
		de_set_bits(&lti_dev[sel][chno]->ctrl.dwval, en, 0, 1);
		de_set_bits(&cti_dev[sel][chno]->ctrl.dwval, en, 0, 1);
		lti_block[sel][chno].dirty = 1;
		cti_block[sel][chno].dirty = 1;
		return 0;
	}

	if (fmt == 1) /* rgb */
		gain = lti_para[LTI_PARA_NUM - 1][dev_type];
	else
		gain = lti_para[para->level][dev_type];

	win_sel = sel_coef(para->inw, para->outw, coef);

	/* lti */
	de_set_bits(&lti_dev[sel][chno]->ctrl.dwval, en, 0, 1);
	lti_dev[sel][chno]->gain.dwval = gain;

	lti_dev[sel][chno]->coef0.dwval = coef[1] << 16 | (coef[0] & 0xffff);
	lti_dev[sel][chno]->coef1.dwval = coef[3] << 16 | (coef[2] & 0xffff);
	lti_dev[sel][chno]->coef2.dwval = coef[7] << 24 |
	    (coef[6] & 0xff) << 16 | (coef[5] & 0xff) << 8 | (coef[4] & 0xff);

	lti_dev[sel][chno]->win_range.dwval = win_sel << 8 | 2;

	lti_block[sel][chno].dirty = 1;

	/* cti */
	de_set_bits(&cti_dev[sel][chno]->ctrl.dwval, en, 0, 1);
	cti_dev[sel][chno]->gain.dwval = gain;
	cti_dev[sel][chno]->coef0.dwval = coef[1] << 16 | (coef[0] & 0xffff);
	cti_dev[sel][chno]->coef1.dwval = coef[3] << 16 | (coef[2] & 0xffff);
	cti_dev[sel][chno]->coef2.dwval = coef[7] << 24 |
	    (coef[6] & 0xff) << 16 | (coef[5] & 0xff) << 8 | (coef[4] & 0xff);

	cti_dev[sel][chno]->win_range.dwval = win_sel << 8 | 2;

	cti_block[sel][chno].dirty = 1;
	return 0;
}
#endif
