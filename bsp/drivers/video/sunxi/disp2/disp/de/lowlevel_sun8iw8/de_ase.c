/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include "de_ase_type.h"
#include "de_rtmx.h"
#include "de_enhance.h"

#define ASE_OFST	0xA8000	/* ASE offset based on RTMX */

static volatile __ase_reg_t *ase_dev[DEVICE_NUM][CHN_NUM];
static de_reg_blocks ase_block[DEVICE_NUM][CHN_NUM];

/* ********************************************************************************************************************* */
/* function       : de_ase_set_reg_base(unsigned int sel, unsigned int chno, unsigned int base) */
/* description    : set ase reg base */
/* parameters     : */
/* sel         <rtmx select> */
/* chno        <overlay select> */
/* base        <reg base> */
/* return         : */
/* success */
/* ********************************************************************************************************************* */
int de_ase_set_reg_base(unsigned int sel, unsigned int chno, unsigned int base)
{
	ase_dev[sel][chno] = (__ase_reg_t *) base;

	return 0;
}

int de_ase_update_regs(unsigned int sel, unsigned int chno)
{
	if (ase_block[sel][chno].dirty == 0x1) {
		memcpy((void *)ase_block[sel][chno].off,
		       ase_block[sel][chno].val, ase_block[sel][chno].size);
		ase_block[sel][chno].dirty = 0x0;
	}

	return 0;
}

int de_ase_init(unsigned int sel, unsigned int chno, unsigned int reg_base)
{
	unsigned int base;
	void *memory;

	base = reg_base + (sel + 1) * 0x00100000 + ASE_OFST;	/* FIXME  display path offset should be defined */
	__inf("sel %d, ase_base[%d]=0x%x\n", sel, chno, base);

	memory = disp_sys_malloc(sizeof(__ase_reg_t));
	if (NULL == memory) {
		__wrn("malloc ase[%d][%d] memory fail! size=0x%x\n", sel, chno,
		      sizeof(__ase_reg_t));
		return -1;
	}

	ase_block[sel][chno].off = base;
	ase_block[sel][chno].val = memory;
	ase_block[sel][chno].size = 0x14;
	ase_block[sel][chno].dirty = 0;

	de_ase_set_reg_base(sel, chno, (unsigned int)memory);

	return 0;
}

/* ********************************************************************************************************************* */
/* function       : de_ase_enable(unsigned int sel, unsigned int chno, unsigned int en) */
/* description    : enable/disable ase */
/* parameters     : */
/* sel         <rtmx select> */
/* chno        <overlay select> */
/* en          <enable: 0-diable; 1-enable> */
/* return         : */
/* success */
/* ********************************************************************************************************************* */
int de_ase_enable(unsigned int sel, unsigned int chno, unsigned int en)
{
	ase_dev[sel][chno]->ctrl.bits.en = en;
	ase_block[sel][chno].dirty = 1;
	return 0;
}

/* ********************************************************************************************************************* */
/* function       : de_ase_set_size(unsigned int sel, unsigned int chno, unsigned int width, unsigned int height) */
/* description    : set ase size */
/* parameters     : */
/* sel         <rtmx select> */
/* chno        <overlay select> */
/* width       <input width> */
/* height  <input height> */
/* return         : */
/* success */
/* ********************************************************************************************************************* */
int de_ase_set_size(unsigned int sel, unsigned int chno, unsigned int width,
		    unsigned int height)
{
	ase_dev[sel][chno]->size.bits.width = width - 1;
	ase_dev[sel][chno]->size.bits.height = height - 1;
	ase_block[sel][chno].dirty = 1;
	return 0;
}

/* ********************************************************************************************************************* */
/* function       : de_ase_set_window(unsigned int sel, unsigned int chno, unsigned int win_enable, de_rect window) */
/* description    : set ase window */
/* parameters     : */
/* sel         <rtmx select> */
/* chno        <overlay select> */
/* win_enable  <enable: 0-window mode diable; 1-window mode enable> */
/* window  <window rectangle> */
/* return         : */
/* success */
/* ********************************************************************************************************************* */
int de_ase_set_window(unsigned int sel, unsigned int chno,
		      unsigned int win_enable, de_rect window)
{
	ase_dev[sel][chno]->ctrl.bits.win_en = win_enable;

	if (win_enable) {
		ase_dev[sel][chno]->win0.bits.left = window.x;
		ase_dev[sel][chno]->win0.bits.top = window.y;
		ase_dev[sel][chno]->win1.bits.right = window.x + window.w - 1;
		ase_dev[sel][chno]->win1.bits.bot = window.y + window.h - 1;
	}
	ase_block[sel][chno].dirty = 1;
	return 0;
}

/* ********************************************************************************************************************* */
/* function       : de_ase_set_para(unsigned int sel, unsigned int chno, unsigned int gain) */
/* description    : set ase para */
/* parameters     : */
/* sel         <rtmx select> */
/* chno        <overlay select> */
/* gain        <ase gain: normal setting 16-24> */
/* return         : */
/* success */
/* ********************************************************************************************************************* */
int de_ase_set_para(unsigned int sel, unsigned int chno, unsigned int gain)
{
	/* ase_dev[sel][chno]->gain.bits.gain = (gain==0)?1:gain; */
	ase_dev[sel][chno]->gain.bits.gain = 16;
	ase_block[sel][chno].dirty = 1;
	return 0;
}

/* ********************************************************************************************************************* */
/* function       : de_ase_info2para(unsigned int gain, de_rect window, __ase_config_data *para) */
/* description    : info->para conversion */
/* parameters     : */
/* gain                <gain info from user> */
/* window              <window info> */
/* para                <bsp para> */
/* return         : */
/* success */
/* ********************************************************************************************************************* */
int de_ase_info2para(unsigned int gain, de_rect window,
		     __ase_config_data *para)
{
	/* parameters */
	para->ase_en = (gain == 0) ? 0 : 1;

	/* window */
	/* para->win_en = 1; */
	/* para->win.x = window.x; */
	/* para->win.y = window.y; */
	/* para->win.w = window.w; */
	/* para->win.h = window.h; */

	return 0;
}
