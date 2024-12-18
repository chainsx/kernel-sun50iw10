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
 *  File name   :   de_peak.c
 *
 *  Description :   display engine 3.0 peaking basic function definition
 *
 *  History     :   2016-3-3 zzz  v0.1  Initial version
 *
 ******************************************************************************/

#ifdef CONFIG_AW_DISP2_SUPPORT_ENAHNCE
#include "de_peak_type.h"
#include "de_rtmx.h"
#include "de_enhance.h"

static struct __peak_reg_t *peak_dev[DE_NUM][CHN_NUM];
static struct de_reg_blocks peak_block[DE_NUM][CHN_NUM];
static struct de_reg_blocks peak_gain_block[DE_NUM][CHN_NUM];

#define PEAK_PARA_NUM (12)
int peak_para[PEAK_PARA_NUM][2] = {
	/* lcd / hdmi */
	{0,    0 },		/* 00 gain for yuv */
	{4,    2 },		/* 01 */
	{10,    5 },		/* 02 */
	{16,    8 },		/* 03 */
	{20,    10},		/* 04 */
	{24,    12},		/* 05 */
	{28,    14},		/* 06 */
	{32,    16},		/* 07 */
	{36,    18},		/* 08 */
	{44,    22},		/* 09 */
	{52,    26},		/* 10 gain for yuv */
	{8,     4 },		/* 11 gain for rgb */
};

/*******************************************************************************
 * function       : de_peak_set_reg_base(unsigned int sel, unsigned int chno,
 *                  unsigned int base)
 * description    : set peak reg base
 * parameters     :
 *                  sel         <rtmx select>
 *                  chno        <overlay select>
 *                  base        <reg base>
 * return         :
 *                  success
 ******************************************************************************/
int de_peak_set_reg_base(unsigned int sel, unsigned int chno, void *base)
{
	__inf("sel=%d, chno=%d, base=0x%p\n", sel, chno, base);
	peak_dev[sel][chno] = (struct __peak_reg_t *) base;

	return 0;
}

int de_peak_update_regs(unsigned int sel, unsigned int chno)
{
	if (peak_block[sel][chno].dirty == 0x1) {
		memcpy((void *)peak_block[sel][chno].off,
			peak_block[sel][chno].val, peak_block[sel][chno].size);
		peak_block[sel][chno].dirty = 0x0;
	}

	if (peak_gain_block[sel][chno].dirty == 0x1) {
		memcpy((void *)peak_gain_block[sel][chno].off,
			peak_gain_block[sel][chno].val,
			peak_gain_block[sel][chno].size);
		peak_gain_block[sel][chno].dirty = 0x0;
	}

	return 0;
}

int de_peak_init(unsigned int sel, unsigned int chno, uintptr_t reg_base)
{
	uintptr_t base;
	void *memory;

	/* FIXME: chno is not considered */
	base = reg_base + (sel + 1) * 0x00100000 + PEAK_OFST;
	__inf("sel %d, peak_base[%d]=0x%p\n", sel, chno, (void *)base);

	memory = kmalloc(sizeof(struct __peak_reg_t), GFP_KERNEL | __GFP_ZERO);
	if (memory == NULL) {
		__wrn("malloc peak[%d][%d] memory fail! size=0x%x\n", sel, chno,
			    (unsigned int)sizeof(struct __peak_reg_t));
		return -1;
	}

	peak_block[sel][chno].off = base;
	peak_block[sel][chno].val = memory;
	peak_block[sel][chno].size = 0x10;
	peak_block[sel][chno].dirty = 0;

	peak_gain_block[sel][chno].off = base + 0x10;
	peak_gain_block[sel][chno].val = memory + 0x10;
	peak_gain_block[sel][chno].size = 0x20;
	peak_gain_block[sel][chno].dirty = 0;

	de_peak_set_reg_base(sel, chno, memory);

	return 0;
}

int de_peak_double_init(unsigned int sel, unsigned int chno, uintptr_t reg_base)
{
	uintptr_t base;

	/* FIXME: chno is not considered */
	base = reg_base + (sel + 1) * 0x00100000 + PEAK_OFST;
	__inf("sel %d, peak_base[%d]=0x%p\n", sel, chno, (void *)base);

	de_peak_set_reg_base(sel, chno, (void *)base);

	return 0;
}

int de_peak_exit(unsigned int sel, unsigned int chno)
{
	kfree(peak_block[sel][chno].val);
	return 0;
}

int de_peak_double_exit(unsigned int sel, unsigned int chno)
{
	return 0;
}

/*******************************************************************************
 * function       : de_peak_enable(unsigned int sel, unsigned int chno,
 *                  unsigned int en)
 * description    : enable/disable peak
 * parameters     :
 *                  sel         <rtmx select>
 *                  chno        <overlay select>
 *                  en          <enable: 0-diable; 1-enable>
 * return         :
 *                  success
 ******************************************************************************/
int de_peak_enable(unsigned int sel, unsigned int chno, unsigned int en)
{
	__inf("sel=%d, chno=%d, en=%d\n", sel, chno, en);
	de_set_bits(&peak_dev[sel][chno]->ctrl.dwval, en, 0, 1);
	peak_block[sel][chno].dirty = 1;
	return 0;
}

/*******************************************************************************
 * function       : de_peak_set_size(unsigned int sel, unsigned int chno,
 *                   unsigned int width, unsigned int height)
 * description    : set peak size
 * parameters     :
 *                  sel         <rtmx select>
 *                  chno        <overlay select>
 *                  width       <input width>
 *                                      height  <input height>
 * return         :
 *                  success
 ******************************************************************************/
int de_peak_set_size(unsigned int sel, unsigned int chno, unsigned int width,
			   unsigned int height)
{
	peak_dev[sel][chno]->size.dwval = (height - 1) << 16 | (width - 1);
	peak_block[sel][chno].dirty = 1;
	return 0;
}

/*******************************************************************************
 * function       : de_peak_set_window(unsigned int sel, unsigned int chno,
 *                  unsigned int win_enable, de_rect window)
 * description    : set peak window
 * parameters     :
 *                  sel         <rtmx select>
 *                  chno        <overlay select>
 *                  win_enable  <enable: 0-window mode diable;
 *                                       1-window mode enable>
 *                                      window  <window rectangle>
 * return         :
 *                  success
 ******************************************************************************/
int de_peak_set_window(unsigned int sel, unsigned int chno,
			     unsigned int win_enable, struct de_rect window)
{
	de_set_bits(&peak_dev[sel][chno]->ctrl.dwval, win_enable, 8, 1);

	if (win_enable) {
		peak_dev[sel][chno]->win0.dwval = window.y << 16 | window.x;
		peak_dev[sel][chno]->win1.dwval =
			(window.y + window.h - 1) << 16 |
			(window.x + window.w - 1);
	}

	peak_block[sel][chno].dirty = 1;
	return 0;
}

/*******************************************************************************
 * function       : de_peak_init_para(unsigned int sel, unsigned int chno)
 * description    : set peak para
 * parameters     :
 *                  sel         <rtmx select>
 *                  chno        <overlay select>
 * return         :
 *                  success
 ******************************************************************************/
int de_peak_init_para(unsigned int sel, unsigned int chno)
{
	peak_dev[sel][chno]->gainctrl.dwval = 128 << 16 | 0;
	peak_dev[sel][chno]->shootctrl.dwval = 31;
	peak_dev[sel][chno]->coring.dwval = 12;

	peak_gain_block[sel][chno].dirty = 1;
	return 0;
}

/*******************************************************************************
 * function       : de_peak_info2para(unsigned int sel, unsigned int chno,
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
int de_peak_info2para(unsigned int sel, unsigned int chno,
			   unsigned int fmt, unsigned int dev_type,
			   struct __peak_config_data *para,
			   unsigned int bypass)
{
	int gain, en;
	int hp_ratio, bp0_ratio;
	unsigned int linebuf = 2048;
	unsigned int peak2d_bypass;

	linebuf = de_feat_get_scale_linebuf_for_ed(sel, chno);
	if ((para->inw >= para->outw) || (para->inh >= para->outh) ||
	    (para->inw > linebuf))
		peak2d_bypass = 1;
	else
		peak2d_bypass = 0;

	/* parameters */
	en = (((fmt == 0) && (para->level == 0)) || (bypass == 1)) ? 0 : 1;
	para->mod_en = en; /* return enable info */

	if (en == 0 || (peak2d_bypass == 0 && para->peak2d_exist == 1)) {
		/* if level=0, module will be disabled,
		 * no need to set para register
		 */
		de_set_bits(&peak_dev[sel][chno]->ctrl.dwval, en, 0, 1);
		goto exit;
	}

	if (fmt == 1) /* rgb */
		gain = peak_para[PEAK_PARA_NUM-1][dev_type];
	else
		gain = peak_para[para->level][dev_type];

	if (dev_type == 0) {
		/* lcd */
		hp_ratio = 0x4;
		bp0_ratio = 0xc;
	} else {
		/* hdmi */
		hp_ratio = 0xe;
		bp0_ratio = 0x2;
	}
	de_set_bits(&peak_dev[sel][chno]->ctrl.dwval, en, 0, 1);
	peak_dev[sel][chno]->filter.dwval = 0 << 31 | hp_ratio << 16 |
			bp0_ratio << 8 | 0;
	peak_dev[sel][chno]->gain.dwval = gain;
	peak_gain_block[sel][chno].dirty = 1;

exit:
	peak_block[sel][chno].dirty = 1;
	return 0;
}
#endif
