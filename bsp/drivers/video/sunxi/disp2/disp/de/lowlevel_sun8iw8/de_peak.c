/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include "de_peak_type.h"
#include "de_rtmx.h"
#include "de_enhance.h"

#define PEAK_OFST	0xA6000	/* PEAKING offset based on RTMX */

static volatile __peak_reg_t *peak_dev[DEVICE_NUM][CHN_NUM];
static de_reg_blocks peak_block[DEVICE_NUM][CHN_NUM];

/* ********************************************************************************************************************* */
/* function       : de_peak_set_reg_base(unsigned int sel, unsigned int chno, unsigned int base) */
/* description    : set peak reg base */
/* parameters     : */
/* sel         <rtmx select> */
/* chno        <overlay select> */
/* base        <reg base> */
/* return         : */
/* success */
/* ********************************************************************************************************************* */
int de_peak_set_reg_base(unsigned int sel, unsigned int chno, unsigned int base)
{
	__inf("sel=%d, chno=%d, base=0x%x\n", sel, chno, base);
	peak_dev[sel][chno] = (__peak_reg_t *) base;

	return 0;
}

int de_peak_update_regs(unsigned int sel, unsigned int chno)
{
	if (peak_block[sel][chno].dirty == 0x1) {
		memcpy((void *)peak_block[sel][chno].off,
		       peak_block[sel][chno].val, peak_block[sel][chno].size);
		peak_block[sel][chno].dirty = 0x0;
	}

	return 0;
}

int de_peak_init(unsigned int sel, unsigned int chno, unsigned int reg_base)
{
	unsigned int base;
	void *memory;

	base = reg_base + (sel + 1) * 0x00100000 + PEAK_OFST;	/* FIXME: chno is not considered */
	__inf("sel %d, peak_base[%d]=0x%x\n", sel, chno, base);

	memory = disp_sys_malloc(sizeof(__peak_reg_t));
	if (NULL == memory) {
		__wrn("malloc peak[%d][%d] memory fail! size=0x%x\n", sel, chno,
		      sizeof(__peak_reg_t));
		return -1;
	}

	peak_block[sel][chno].off = base;
	peak_block[sel][chno].val = memory;
	peak_block[sel][chno].size = 0x30;
	peak_block[sel][chno].dirty = 0;

	de_peak_set_reg_base(sel, chno, (unsigned int)memory);

	return 0;
}

/* ********************************************************************************************************************* */
/* function       : de_peak_enable(unsigned int sel, unsigned int chno, unsigned int en) */
/* description    : enable/disable peak */
/* parameters     : */
/* sel         <rtmx select> */
/* chno        <overlay select> */
/* en          <enable: 0-diable; 1-enable> */
/* return         : */
/* success */
/* ********************************************************************************************************************* */
int de_peak_enable(unsigned int sel, unsigned int chno, unsigned int en)
{
	__inf("sel=%d, chno=%d, en=%d\n", sel, chno, en);
	peak_dev[sel][chno]->ctrl.bits.en = en;
	peak_block[sel][chno].dirty = 1;
	return 0;
}

/* ********************************************************************************************************************* */
/* function       : de_peak_set_size(unsigned int sel, unsigned int chno, unsigned int width, unsigned int height) */
/* description    : set peak size */
/* parameters     : */
/* sel         <rtmx select> */
/* chno        <overlay select> */
/* width       <input width> */
/* height  <input height> */
/* return         : */
/* success */
/* ********************************************************************************************************************* */
int de_peak_set_size(unsigned int sel, unsigned int chno, unsigned int width,
		     unsigned int height)
{
	peak_dev[sel][chno]->size.bits.width = width - 1;
	peak_dev[sel][chno]->size.bits.height = height - 1;
	peak_block[sel][chno].dirty = 1;
	return 0;
}

/* ********************************************************************************************************************* */
/* function       : de_peak_set_window(unsigned int sel, unsigned int chno, unsigned int win_enable, de_rect window) */
/* description    : set peak window */
/* parameters     : */
/* sel         <rtmx select> */
/* chno        <overlay select> */
/* win_enable  <enable: 0-window mode diable; 1-window mode enable> */
/* window  <window rectangle> */
/* return         : */
/* success */
/* ********************************************************************************************************************* */
int de_peak_set_window(unsigned int sel, unsigned int chno,
		       unsigned int win_enable, de_rect window)
{
	peak_dev[sel][chno]->ctrl.bits.win_en = win_enable;

	if (win_enable) {
		peak_dev[sel][chno]->win0.bits.win_left = window.x;
		peak_dev[sel][chno]->win0.bits.win_top = window.y;
		peak_dev[sel][chno]->win1.bits.win_right =
		    window.x + window.w - 1;
		peak_dev[sel][chno]->win1.bits.win_bot =
		    window.y + window.h - 1;
	}
	peak_block[sel][chno].dirty = 1;
	return 0;
}

/* ********************************************************************************************************************* */
/* function       : de_peak_set_para(unsigned int sel, unsigned int chno, unsigned int gain) */
/* description    : set peak para */
/* parameters     : */
/* sel         <rtmx select> */
/* chno        <overlay select> */
/* gain        <peak gain: normal setting 36-42> */
/* return         : */
/* success */
/* ********************************************************************************************************************* */
int de_peak_set_para(unsigned int sel, unsigned int chno, unsigned int gain)
{
	peak_dev[sel][chno]->gain.bits.gain = 36;	/* gain */
	peak_dev[sel][chno]->filter.bits.filter_sel = 0;
	peak_dev[sel][chno]->filter.bits.hp_ratio = 4;
	peak_dev[sel][chno]->filter.bits.bp0_ratio = 12;

	peak_dev[sel][chno]->filter.bits.bp1_ratio = 0;
	peak_dev[sel][chno]->gainctrl.bits.beta = 0;
	peak_dev[sel][chno]->gainctrl.bits.dif_up = 128;
	peak_dev[sel][chno]->shootctrl.bits.neg_gain = 31;
	peak_dev[sel][chno]->coring.bits.corthr = 4;

	peak_block[sel][chno].dirty = 1;
	return 0;
}

/* ********************************************************************************************************************* */
/* function       : de_peak_info2para(unsigned int sharp, de_rect window, __peak_config_data *para) */
/* description    : info->para conversion */
/* parameters     : */
/* sharp               <info from user> */
/* window              <window info> */
/* para                <bsp para> */
/* return         : */
/* success */
/* ********************************************************************************************************************* */
int de_peak_info2para(unsigned int sharp, de_rect window,
		      __peak_config_data *para)
{
	/* parameters */
	para->peak_en = (sharp == 1 || sharp == 3) ? 1 : 0;

	/* window */
	/* para->win_en = 1; */
	/* para->win.x = window.x; */
	/* para->win.y = window.y; */
	/* para->win.w = window.w; */
	/* para->win.h = window.h; */

	return 0;
}
