/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include "de_fcc_type.h"
#include "de_rtmx.h"
#include "de_vep_table.h"
#include "de_enhance.h"

#define FCC_OFST 0xAA000

static volatile __fcc_reg_t *fcc_dev[DEVICE_NUM][CHN_NUM];
static de_reg_blocks fcc_para_block[DEVICE_NUM][CHN_NUM];
/* static de_reg_blocks fcc_csc_block[DEVICE_NUM][CHN_NUM]; */

/* ********************************************************************************************************************* */
/* function       : de_fcc_set_reg_base(unsigned int sel, unsigned int chno, void *base) */
/* description    : set fcc reg base */
/* parameters     : */
/* sel         <rtmx select> */
/* chno        <overlay select> */
/* base        <reg base> */
/* return         : */
/* success */
/* ********************************************************************************************************************* */
int de_fcc_set_reg_base(unsigned int sel, unsigned int chno, void *base)
{
	fcc_dev[sel][chno] = (__fcc_reg_t *) base;

	return 0;
}

int de_fcc_init(unsigned int sel, unsigned int chno, unsigned int reg_base)
{
	unsigned int fcc_base;
	void *memory;

	fcc_base = reg_base + (sel + 1) * 0x00100000 + FCC_OFST;	/* FIXME  display path offset should be defined */
	__inf("sel %d, fcc_base[%d]=0x%x\n", sel, chno, fcc_base);

	memory = kmalloc(sizeof(__fcc_reg_t), GFP_KERNEL | __GFP_ZERO);
	if (NULL == memory) {
		__wrn("malloc vep fcc[%d][%d] memory fail! size=0x%x\n", sel,
		      chno, sizeof(__fcc_reg_t));
		return -1;
	}

	fcc_para_block[sel][chno].off = fcc_base;
	fcc_para_block[sel][chno].val = memory;
	fcc_para_block[sel][chno].size = 0x48;
	fcc_para_block[sel][chno].dirty = 0;

	/* fcc_csc_block[sel][chno].off          = fcc_base + 0x50; */
	/* fcc_csc_block[sel][chno].val          = memory + 0x50; */
	/* fcc_csc_block[sel][chno].size         = 0x40; */
	/* fcc_csc_block[sel][chno].dirty                = 0; */

	de_fcc_set_reg_base(sel, chno, memory);

	return 0;
}

int de_fcc_update_regs(unsigned int sel, unsigned int chno)
{
	if (fcc_para_block[sel][chno].dirty == 0x1) {
		memcpy((void *)fcc_para_block[sel][chno].off,
		       fcc_para_block[sel][chno].val,
		       fcc_para_block[sel][chno].size);
		fcc_para_block[sel][chno].dirty = 0x0;
	}
	/* if(fcc_csc_block[sel][chno].dirty == 0x1){ */
	/* memcpy((void *)fcc_csc_block[sel][chno].off,fcc_csc_block[sel][chno].val,fcc_csc_block[sel][chno].size); */
	/* fcc_csc_block[sel][chno].dirty = 0x0; */
	/* } */

	return 0;
}

/* ********************************************************************************************************************* */
/* function       : de_fcc_enable(unsigned int sel, unsigned int chno, unsigned int en) */
/* description    : enable/disable fcc */
/* parameters     : */
/* sel         <rtmx select> */
/* chno        <overlay select> */
/* en          <enable: 0-diable; 1-enable> */
/* return         : */
/* success */
/* ********************************************************************************************************************* */
int de_fcc_enable(unsigned int sel, unsigned int chno, unsigned int en)
{
	fcc_dev[sel][chno]->fcc_ctl.bits.en = en;
	fcc_para_block[sel][chno].dirty = 1;

	return 0;
}

/* ********************************************************************************************************************* */
/* function       : de_fcc_set_size(unsigned int sel, unsigned int chno, unsigned int width, unsigned int height) */
/* description    : set fcc size */
/* parameters     : */
/* sel         <rtmx select> */
/* chno        <overlay select> */
/* width       <input width> */
/* height  <input height> */
/* return         : */
/* success */
/* ********************************************************************************************************************* */
int de_fcc_set_size(unsigned int sel, unsigned int chno, unsigned int width,
		    unsigned int height)
{
	fcc_dev[sel][chno]->fcc_size.bits.width = width == 0 ? 0 : width - 1;
	fcc_dev[sel][chno]->fcc_size.bits.height = height == 0 ? 0 : height - 1;

	fcc_para_block[sel][chno].dirty = 1;

	return 0;
}

/* ********************************************************************************************************************* */
/* function       : de_fcc_set_window(unsigned int sel, unsigned int chno, unsigned int win_en, de_rect window) */
/* description    : set fcc window */
/* parameters     : */
/* sel         <rtmx select> */
/* chno        <overlay select> */
/* win_en      <enable: 0-window mode diable; 1-window mode enable> */
/* window  <window rectangle> */
/* return         : */
/* success */
/* ********************************************************************************************************************* */
int de_fcc_set_window(unsigned int sel, unsigned int chno, unsigned int win_en,
		      de_rect window)
{
	fcc_dev[sel][chno]->fcc_ctl.bits.win_en = win_en & 0x1;

	if (win_en) {
		fcc_dev[sel][chno]->fcc_win0.bits.left = window.x;
		fcc_dev[sel][chno]->fcc_win0.bits.top = window.y;
		fcc_dev[sel][chno]->fcc_win1.bits.right =
		    window.x + window.w - 1;
		fcc_dev[sel][chno]->fcc_win1.bits.bot = window.y + window.h - 1;
	}

	fcc_para_block[sel][chno].dirty = 1;

	return 0;
}

/* ********************************************************************************************************************* */
/* function       : de_fcc_set_para(unsigned int sel, unsigned int chno, unsigned int mode) */
/* description    : set fcc para */
/* parameters     : */
/* sel         <rtmx select> */
/* chno        <overlay select> */
/* sgain */
/* return         : */
/* success */
/* ********************************************************************************************************************* */
int de_fcc_set_para(unsigned int sel, unsigned int chno, unsigned int sgain[6])
{
	memcpy((void *)fcc_dev[sel][chno]->fcc_range,
	       (void *)&fcc_range_gain[0], sizeof(int) * 6);
	memcpy((void *)fcc_dev[sel][chno]->fcc_gain, (void *)&sgain[0],
	       sizeof(int) * 6);

	fcc_para_block[sel][chno].dirty = 1;

	return 0;
}

/* ********************************************************************************************************************* */
/* function       : de_csc_enable(unsigned int sel, unsigned int chno, unsigned int en) */
/* description    : enable/disable csc */
/* parameters     : */
/* sel         <rtmx select> */
/* chno        <overlay select> */
/* en          <enable: 0-diable; 1-enable> */
/* mode        <mode: 0-bt601; 1-bt709; 2-ycc;> */
/* return         : */
/* success */
/* ********************************************************************************************************************* */
#if 0
int de_fcc_csc_set(unsigned int sel, unsigned int chno, unsigned int en,
		   unsigned int mode)
{
	__inf("sel %d, chno%d, en %d, mode %d\n", sel, chno, en, mode);
	fcc_dev[sel][chno]->fcc_csc_ctl.bits.bypass = en;

	switch (mode) {
	case DE_BT601:
		{
			memcpy((void *)fcc_dev[sel][chno]->fcc_csc_coff0,
			       (void *)&y2r[0], sizeof(int) * 12);
			break;
		}
	case DE_BT709:
		{
			memcpy((void *)fcc_dev[sel][chno]->fcc_csc_coff0,
			       (void *)&y2r[12], sizeof(int) * 12);
			break;
		}
	case DE_YCC:
		{
			memcpy((void *)fcc_dev[sel][chno]->fcc_csc_coff0,
			       (void *)&y2r[24], sizeof(int) * 12);
			break;
		}
	default:
		break;
	}

	fcc_csc_block[sel][chno].dirty = 1;

	return 0;
}
#endif

/* ********************************************************************************************************************* */
/* function       : de_fcc_info2para(unsigned int gain, de_rect window, __fcc_config_data *para) */
/* description    : info->para conversion */
/* parameters     : */
/* gain                <gain info from user> */
/* window              <window info> */
/* para                <bsp para> */
/* return         : */
/* success */
/* ********************************************************************************************************************* */
int de_fcc_info2para(unsigned int sgain0, unsigned int sgain1,
		     unsigned int sgain2, unsigned int sgain3,
		     unsigned int sgain4, unsigned int sgain5, de_rect window,
		     __fcc_config_data *para)
{
	/* parameters */
	para->fcc_en =
	    ((sgain0 | sgain1 | sgain2 | sgain3 | sgain4 | sgain5) ==
	     0) ? 0 : 1;

	para->sgain[0] = (sgain0 == 0) ? 0 : ((sgain0 == 1) ? 5 : 10);
	para->sgain[1] = (sgain1 == 0) ? 0 : ((sgain1 == 1) ? 12 : 24);
	para->sgain[2] = (sgain2 == 0) ? 0 : ((sgain2 == 1) ? 4 : 8);
	para->sgain[3] = 0;
	para->sgain[4] = 0;
	para->sgain[5] = 0;

	/* window */
	/* para->win_en = 1; */
	/* para->win.x = window.x; */
	/* para->win.y = window.y; */
	/* para->win.w = window.w; */
	/* para->win.h = window.h; */

	return 0;
}
