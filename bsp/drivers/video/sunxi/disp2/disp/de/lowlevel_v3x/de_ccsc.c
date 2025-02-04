/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

/******************************************************************************
 *    All Winner Tech, All Right Reserved. 2014-2016 Copyright (c)
 *
 *    File name   :    de_ccsc.c
 *
 *    Description :    display engine 3.0 channel csc basic function definition
 *
 *    History     :    2016/03/29  vito cheng  v0.1  Initial version
 ******************************************************************************/

#include "de_rtmx.h"
#include "de_csc_type.h"
#include "de_vep_table.h"
#include "de_csc.h"
#include "de_enhance.h"
#include "de_cdc_type.h"
#include "de_cdc_table.h"

static struct __ccsc_reg_t *ccsc_dev[DE_NUM][CHN_NUM];
static struct __icsc_reg_t *icsc_dev[DE_NUM][CHN_NUM];
static struct __fcc_csc_reg_t *fcc_csc_dev[DE_NUM][CHN_NUM];
static struct __cdc_reg_t *cdc_dev[DE_NUM][CHN_NUM];

static struct de_reg_blocks csc_block[DE_NUM][CHN_NUM];
static struct de_reg_blocks icsc_block[DE_NUM][CHN_NUM];
static struct de_reg_blocks fcc_csc_block[DE_NUM][CHN_NUM];
static struct de_reg_blocks cdc_block[DE_NUM][CHN_NUM];

static unsigned int vi_num[DE_NUM];
static unsigned int chn_num[DE_NUM];
static unsigned int vep_support[DE_NUM][CHN_NUM];
static unsigned int cdc_support[DE_NUM][CHN_NUM];

static struct __cdc_status *cdc_status[DE_NUM][CHN_NUM];

static int de_ccsc_set_reg_base(unsigned int sel, unsigned int chno, void *base)
{
	__inf("sel=%d, chno=%d, base=0x%p\n", sel, chno, base);
	ccsc_dev[sel][chno] = (struct __ccsc_reg_t *) base;

	return 0;
}

static int de_icsc_set_reg_base(unsigned int sel, unsigned int chno, void *base)
{
	__inf("sel=%d, chno=%d, base=0x%p\n", sel, chno, base);
	icsc_dev[sel][chno] = (struct __icsc_reg_t *) base;

	return 0;
}

static int de_fcsc_set_reg_base(unsigned int sel, unsigned int chno, void *base)
{
	__inf("sel=%d, chno=%d, base=0x%p\n", sel, chno, base);
	fcc_csc_dev[sel][chno] = (struct __fcc_csc_reg_t *) base;

	return 0;
}

static int de_cdc_set_reg_base(unsigned int sel, unsigned int chno, void *base)
{
	cdc_dev[sel][chno] = (struct __cdc_reg_t *) base;

	return 0;
}

static int de_set_icsc_coef(unsigned int sel, unsigned int ch_id,
			    int icsc_coeff[16])
{
	unsigned int temp0, temp1, temp2;

	temp0 = (icsc_coeff[12] >= 0) ? (icsc_coeff[12] & 0x3ff) :
		(0x400 - (unsigned int)(icsc_coeff[12] & 0x3ff));
	temp1 = (icsc_coeff[13] >= 0) ? (icsc_coeff[13] & 0x3ff) :
		(0x400 - (unsigned int)(icsc_coeff[13] & 0x3ff));
	temp2 = (icsc_coeff[14] >= 0) ? (icsc_coeff[14] & 0x3ff) :
		(0x400 - (unsigned int)(icsc_coeff[14] & 0x3ff));

	icsc_dev[sel][ch_id]->en.dwval = 0x1;
	icsc_dev[sel][ch_id]->d[0].dwval = temp0;
	icsc_dev[sel][ch_id]->d[1].dwval = temp1;
	icsc_dev[sel][ch_id]->d[2].dwval = temp2;

	icsc_dev[sel][ch_id]->c0[0].dwval = icsc_coeff[0];
	icsc_dev[sel][ch_id]->c0[1].dwval = icsc_coeff[1];
	icsc_dev[sel][ch_id]->c0[2].dwval = icsc_coeff[2];
	icsc_dev[sel][ch_id]->c03.dwval = icsc_coeff[3];

	icsc_dev[sel][ch_id]->c1[0].dwval = icsc_coeff[4];
	icsc_dev[sel][ch_id]->c1[1].dwval = icsc_coeff[5];
	icsc_dev[sel][ch_id]->c1[2].dwval = icsc_coeff[6];
	icsc_dev[sel][ch_id]->c13.dwval = icsc_coeff[7];

	icsc_dev[sel][ch_id]->c2[0].dwval = icsc_coeff[8];
	icsc_dev[sel][ch_id]->c2[1].dwval = icsc_coeff[9];
	icsc_dev[sel][ch_id]->c2[2].dwval = icsc_coeff[10];
	icsc_dev[sel][ch_id]->c23.dwval = icsc_coeff[11];

	icsc_block[sel][ch_id].dirty = 1;

	return 0;
}

static int de_set_ccsc_coef(unsigned int sel, unsigned int ch_id,
			    int csc_coeff[16])
{
	int c[3], d[3];

	ccsc_dev[sel][ch_id]->c0[0].dwval = *(csc_coeff);
	ccsc_dev[sel][ch_id]->c0[1].dwval = *(csc_coeff + 1);
	ccsc_dev[sel][ch_id]->c0[2].dwval = *(csc_coeff + 2);
#if defined(CONFIG_ARCH_SUN50IW6)
	if (sel == 1 && ch_id == 1) {
		c[0] = (*(csc_coeff + 3) >> 2) & 0x7ff;
		d[0] = (*(csc_coeff + 12) >> 2) & 0x7ff;
		c[1] = (*(csc_coeff + 7) >> 2) & 0x7ff;
		d[1] = (*(csc_coeff + 13) >> 2) & 0x7ff;
		c[2] = (*(csc_coeff + 11) >> 2) & 0x7ff;
		d[2] = (*(csc_coeff + 14) >> 2) & 0x7ff;
	} else {
		c[0] = *(csc_coeff + 3) & 0x7ff;
		d[0] = *(csc_coeff + 12) & 0x7ff;
		c[1] = *(csc_coeff + 7) & 0x7ff;
		d[1] = *(csc_coeff + 13) & 0x7ff;
		c[2] = *(csc_coeff + 11) & 0x7ff;
		d[2] = *(csc_coeff + 14) & 0x7ff;
	}
#else
	c[0] = *(csc_coeff + 3) & 0x7ff;
	d[0] = *(csc_coeff + 12) & 0x7ff;
	c[1] = *(csc_coeff + 7) & 0x7ff;
	d[1] = *(csc_coeff + 13) & 0x7ff;
	c[2] = *(csc_coeff + 11) & 0x7ff;
	d[2] = *(csc_coeff + 14) & 0x7ff;
#endif

	ccsc_dev[sel][ch_id]->c1[0].dwval = *(csc_coeff + 4);
	ccsc_dev[sel][ch_id]->c1[1].dwval = *(csc_coeff + 5);
	ccsc_dev[sel][ch_id]->c1[2].dwval = *(csc_coeff + 6);

	ccsc_dev[sel][ch_id]->c2[0].dwval = *(csc_coeff + 8);
	ccsc_dev[sel][ch_id]->c2[1].dwval = *(csc_coeff + 9);
	ccsc_dev[sel][ch_id]->c2[2].dwval = *(csc_coeff + 10);

	ccsc_dev[sel][ch_id]->d0.dwval = c[0] | (d[0] << 16);
	ccsc_dev[sel][ch_id]->d1.dwval = c[1] | (d[1] << 16);
	ccsc_dev[sel][ch_id]->d2.dwval = c[2] | (d[2] << 16);

	csc_block[sel][ch_id].dirty = 1;

	return 0;
}

static int de_set_fcc_csc_coef(unsigned int sel, unsigned int ch_id,
			       int in_csc_coeff[16], int out_csc_coeff[16])
{
	unsigned int temp0, temp1, temp2;

	temp0 = (in_csc_coeff[12] >= 0) ? (in_csc_coeff[12] & 0x3ff) :
		(0x400 - (unsigned int)(in_csc_coeff[12] & 0x3ff));
	temp1 = (in_csc_coeff[13] >= 0) ? (in_csc_coeff[13] & 0x3ff) :
		(0x400 - (unsigned int)(in_csc_coeff[13] & 0x3ff));
	temp2 = (in_csc_coeff[14] >= 0) ? (in_csc_coeff[14] & 0x3ff) :
		(0x400 - (unsigned int)(in_csc_coeff[14] & 0x3ff));

	fcc_csc_dev[sel][ch_id]->in_d[0].dwval = temp0;
	fcc_csc_dev[sel][ch_id]->in_d[1].dwval = temp1;
	fcc_csc_dev[sel][ch_id]->in_d[2].dwval = temp2;

	fcc_csc_dev[sel][ch_id]->in_c0[0].dwval = in_csc_coeff[0];
	fcc_csc_dev[sel][ch_id]->in_c0[1].dwval = in_csc_coeff[1];
	fcc_csc_dev[sel][ch_id]->in_c0[2].dwval = in_csc_coeff[2];
	fcc_csc_dev[sel][ch_id]->in_c03.dwval = in_csc_coeff[3];

	fcc_csc_dev[sel][ch_id]->in_c1[0].dwval = in_csc_coeff[4];
	fcc_csc_dev[sel][ch_id]->in_c1[1].dwval = in_csc_coeff[5];
	fcc_csc_dev[sel][ch_id]->in_c1[2].dwval = in_csc_coeff[6];
	fcc_csc_dev[sel][ch_id]->in_c13.dwval = in_csc_coeff[7];

	fcc_csc_dev[sel][ch_id]->in_c2[0].dwval = in_csc_coeff[8];
	fcc_csc_dev[sel][ch_id]->in_c2[1].dwval = in_csc_coeff[9];
	fcc_csc_dev[sel][ch_id]->in_c2[2].dwval = in_csc_coeff[10];
	fcc_csc_dev[sel][ch_id]->in_c23.dwval = in_csc_coeff[11];

	temp0 = (out_csc_coeff[12] >= 0) ? (out_csc_coeff[12] & 0x3ff) :
		(0x400 - (unsigned int)(out_csc_coeff[12] & 0x3ff));
	temp1 = (out_csc_coeff[13] >= 0) ? (out_csc_coeff[13] & 0x3ff) :
		(0x400 - (unsigned int)(out_csc_coeff[13] & 0x3ff));
	temp2 = (out_csc_coeff[14] >= 0) ? (out_csc_coeff[14] & 0x3ff) :
		(0x400 - (unsigned int)(out_csc_coeff[14] & 0x3ff));

	fcc_csc_dev[sel][ch_id]->out_d[0].dwval = temp0;
	fcc_csc_dev[sel][ch_id]->out_d[1].dwval = temp1;
	fcc_csc_dev[sel][ch_id]->out_d[2].dwval = temp2;

	fcc_csc_dev[sel][ch_id]->out_c0[0].dwval = out_csc_coeff[0];
	fcc_csc_dev[sel][ch_id]->out_c0[1].dwval = out_csc_coeff[1];
	fcc_csc_dev[sel][ch_id]->out_c0[2].dwval = out_csc_coeff[2];
	fcc_csc_dev[sel][ch_id]->out_c03.dwval = out_csc_coeff[3];

	fcc_csc_dev[sel][ch_id]->out_c1[0].dwval = out_csc_coeff[4];
	fcc_csc_dev[sel][ch_id]->out_c1[1].dwval = out_csc_coeff[5];
	fcc_csc_dev[sel][ch_id]->out_c1[2].dwval = out_csc_coeff[6];
	fcc_csc_dev[sel][ch_id]->out_c13.dwval = out_csc_coeff[7];

	fcc_csc_dev[sel][ch_id]->out_c2[0].dwval = out_csc_coeff[8];
	fcc_csc_dev[sel][ch_id]->out_c2[1].dwval = out_csc_coeff[9];
	fcc_csc_dev[sel][ch_id]->out_c2[2].dwval = out_csc_coeff[10];
	fcc_csc_dev[sel][ch_id]->out_c23.dwval = out_csc_coeff[11];

	fcc_csc_block[sel][ch_id].dirty = 1;

	return 0;
}

static int de_set_cdc_csc_coef(unsigned int sel, unsigned int ch_id,
			       int in_csc_coeff[16], int out_csc_coeff[16])
{
	unsigned int temp0, temp1, temp2;

	temp0 = (in_csc_coeff[12] >= 0) ? (in_csc_coeff[12] & 0x3ff) :
		(0x400 - (unsigned int)(in_csc_coeff[12] & 0x3ff));
	temp1 = (in_csc_coeff[13] >= 0) ? (in_csc_coeff[13] & 0x3ff) :
		(0x400 - (unsigned int)(in_csc_coeff[13] & 0x3ff));
	temp2 = (in_csc_coeff[14] >= 0) ? (in_csc_coeff[14] & 0x3ff) :
		(0x400 - (unsigned int)(in_csc_coeff[14] & 0x3ff));

	cdc_dev[sel][ch_id]->in_d[0].dwval = temp0;
	cdc_dev[sel][ch_id]->in_d[1].dwval = temp1;
	cdc_dev[sel][ch_id]->in_d[2].dwval = temp2;

	cdc_dev[sel][ch_id]->in_c0[0].dwval = in_csc_coeff[0];
	cdc_dev[sel][ch_id]->in_c0[1].dwval = in_csc_coeff[1];
	cdc_dev[sel][ch_id]->in_c0[2].dwval = in_csc_coeff[2];
	cdc_dev[sel][ch_id]->in_c03.dwval = in_csc_coeff[3];

	cdc_dev[sel][ch_id]->in_c1[0].dwval = in_csc_coeff[4];
	cdc_dev[sel][ch_id]->in_c1[1].dwval = in_csc_coeff[5];
	cdc_dev[sel][ch_id]->in_c1[2].dwval = in_csc_coeff[6];
	cdc_dev[sel][ch_id]->in_c13.dwval = in_csc_coeff[7];

	cdc_dev[sel][ch_id]->in_c2[0].dwval = in_csc_coeff[8];
	cdc_dev[sel][ch_id]->in_c2[1].dwval = in_csc_coeff[9];
	cdc_dev[sel][ch_id]->in_c2[2].dwval = in_csc_coeff[10];
	cdc_dev[sel][ch_id]->in_c23.dwval = in_csc_coeff[11];

	temp0 = (out_csc_coeff[12] >= 0) ? (out_csc_coeff[12] & 0x3ff) :
		(0x400 - (unsigned int)(out_csc_coeff[12] & 0x3ff));
	temp1 = (out_csc_coeff[13] >= 0) ? (out_csc_coeff[13] & 0x3ff) :
		(0x400 - (unsigned int)(out_csc_coeff[13] & 0x3ff));
	temp2 = (out_csc_coeff[14] >= 0) ? (out_csc_coeff[14] & 0x3ff) :
		(0x400 - (unsigned int)(out_csc_coeff[14] & 0x3ff));

	cdc_dev[sel][ch_id]->out_d[0].dwval = temp0;
	cdc_dev[sel][ch_id]->out_d[1].dwval = temp1;
	cdc_dev[sel][ch_id]->out_d[2].dwval = temp2;

	cdc_dev[sel][ch_id]->out_c0[0].dwval = out_csc_coeff[0];
	cdc_dev[sel][ch_id]->out_c0[1].dwval = out_csc_coeff[1];
	cdc_dev[sel][ch_id]->out_c0[2].dwval = out_csc_coeff[2];
	cdc_dev[sel][ch_id]->out_c03.dwval = out_csc_coeff[3];

	cdc_dev[sel][ch_id]->out_c1[0].dwval = out_csc_coeff[4];
	cdc_dev[sel][ch_id]->out_c1[1].dwval = out_csc_coeff[5];
	cdc_dev[sel][ch_id]->out_c1[2].dwval = out_csc_coeff[6];
	cdc_dev[sel][ch_id]->out_c13.dwval = out_csc_coeff[7];

	cdc_dev[sel][ch_id]->out_c2[0].dwval = out_csc_coeff[8];
	cdc_dev[sel][ch_id]->out_c2[1].dwval = out_csc_coeff[9];
	cdc_dev[sel][ch_id]->out_c2[2].dwval = out_csc_coeff[10];
	cdc_dev[sel][ch_id]->out_c23.dwval = out_csc_coeff[11];

	cdc_block[sel][ch_id].dirty = 1;

	return 0;
}

int de_ccsc_apply(unsigned int sel, unsigned int ch_id,
		  struct disp_csc_config *config)
{
	int ccsc_coeff[16], icsc_coeff[16], fcc_incsc_coeff[16],
	    fcc_outcsc_coeff[16];
	unsigned int in_fmt, in_mode, in_color_range, out_fmt, out_mode,
		     out_color_range;
	unsigned int i_in_fmt, i_in_mode, i_in_color_range, i_out_fmt,
		     i_out_mode, i_out_color_range;
	unsigned int fcc_in_fmt, fcc_in_mode, fcc_in_color_range, fcc_out_fmt,
		     fcc_out_mode, fcc_out_color_range;
	unsigned int cdc_fmt, cdc_mode, cdc_color_range, cdc_eotf;
	/* CDC */

	if (cdc_support[sel][ch_id]) {
		/* select the conversion through input and output
		 * color space (rec.709 type or rec.2020 type) and
		 * EOTF (gamma type or hlg/ST.2084 type)
		 */
		cdc_fmt = config->in_fmt; /* donot change */
		cdc_mode = config->out_mode; /* follow output */
		cdc_color_range = config->in_color_range; /* donot change */
		cdc_eotf = config->out_eotf; /* follow output */
		de_cdc_coeff_set(sel, ch_id, config, cdc_fmt, cdc_mode,
				 cdc_color_range, cdc_eotf);
	} else {
		cdc_fmt = config->in_fmt;
		cdc_mode = config->in_mode;
		cdc_color_range = config->in_color_range;
		cdc_eotf = config->in_eotf;
	}

	if (vep_support[sel][ch_id]) {
		/* CSC in FCE */
		if (cdc_fmt == DE_RGB) {
			i_in_fmt = cdc_fmt;
			i_in_mode = cdc_mode;
			i_in_color_range = cdc_color_range;
			i_out_fmt = DE_YUV;
			i_out_mode = DE_BT709;
			i_out_color_range = DISP_COLOR_RANGE_0_255;
		} else {
			i_in_fmt = cdc_fmt;
			i_in_mode = cdc_mode;
			i_in_color_range = cdc_color_range;
			i_out_fmt = cdc_fmt;
			i_out_mode = cdc_mode;
			i_out_color_range = DISP_COLOR_RANGE_0_255;
		}

		de_csc_coeff_calc(i_in_fmt, i_in_mode, i_in_color_range,
				  i_out_fmt, i_out_mode, i_out_color_range,
				  icsc_coeff);

		de_set_icsc_coef(sel, ch_id, icsc_coeff);

		/* CSC in FCC */
		fcc_in_fmt = i_out_fmt;
		fcc_in_mode = i_out_mode;
		fcc_in_color_range = i_out_color_range;
		fcc_out_fmt = DE_RGB;
		fcc_out_mode = DE_BT709;
		fcc_out_color_range = DISP_COLOR_RANGE_0_255;

		de_csc_coeff_calc(fcc_in_fmt, fcc_in_mode, fcc_in_color_range,
				  fcc_out_fmt, fcc_out_mode,
				  fcc_out_color_range, fcc_incsc_coeff);

		de_csc_coeff_calc(fcc_out_fmt, fcc_out_mode,
				  fcc_out_color_range, fcc_in_fmt, fcc_in_mode,
				  fcc_in_color_range, fcc_outcsc_coeff);

		de_set_fcc_csc_coef(sel, ch_id, fcc_incsc_coeff,
				    fcc_outcsc_coeff);

		/* CSC in BLD */
		in_fmt = i_out_fmt;
		in_mode = i_out_mode;
		in_color_range = i_out_color_range;
		out_fmt = config->out_fmt;
		out_mode = config->out_mode;
		out_color_range = config->out_color_range;

	} else {
		/* CSC in BLD */
		in_fmt = cdc_fmt;
		in_mode = cdc_mode;
		in_color_range = cdc_color_range;
		out_fmt = config->out_fmt;
		out_mode = config->out_mode;
		out_color_range = config->out_color_range;
	}

	de_csc_coeff_calc(in_fmt, in_mode, in_color_range, out_fmt, out_mode,
			  out_color_range, ccsc_coeff);

	de_set_ccsc_coef(sel, ch_id, ccsc_coeff);

	return 0;
}

int de_ccsc_update_regs(unsigned int sel)
{
	int ch_id;

	for (ch_id = 0; ch_id < vi_num[sel]; ch_id++) {
		if (vep_support[sel][ch_id]) {
			if (icsc_block[sel][ch_id].dirty == 0x1) {
				memcpy((void *)icsc_block[sel][ch_id].off,
				       icsc_block[sel][ch_id].val,
				       icsc_block[sel][ch_id].size);
				icsc_block[sel][ch_id].dirty = 0x0;
			}

			if (fcc_csc_block[sel][ch_id].dirty == 0x1) {
				memcpy((void *)fcc_csc_block[sel][ch_id].off,
				       fcc_csc_block[sel][ch_id].val,
				       fcc_csc_block[sel][ch_id].size);
				fcc_csc_block[sel][ch_id].dirty = 0x0;
			}
		}
	}

	for (ch_id = 0; ch_id < chn_num[sel]; ch_id++) {
		if (csc_block[sel][ch_id].dirty == 0x1) {
			memcpy((void *)csc_block[sel][ch_id].off,
			       csc_block[sel][ch_id].val,
			       csc_block[sel][ch_id].size);
			csc_block[sel][ch_id].dirty = 0x0;
		}
	}

	for (ch_id = 0; ch_id < chn_num[sel]; ch_id++) {
		if (cdc_support[sel][ch_id])
			if (cdc_block[sel][ch_id].dirty == 0x1) {
				memcpy((void *)cdc_block[sel][ch_id].off,
				       cdc_block[sel][ch_id].val,
				       cdc_block[sel][ch_id].size);
				cdc_block[sel][ch_id].dirty = 0x0;
				cdc_dev[sel][ch_id]->update.dwval = 0;
			}
	}

	return 0;
}

int de_ccsc_init(struct disp_bsp_init_para *para)
{
	uintptr_t base, base_ofst;
	void *memory;
	int screen_id, ch_id, device_num;
	int fcc_csc_reg_struct_size = (int)sizeof(struct __fcc_csc_reg_t);
	int fcc_csc_reg_size = 0x7C;
	int block_size = 0;

	device_num = de_feat_get_num_screens();

	for (screen_id = 0; screen_id < device_num; screen_id++) {
		vi_num[screen_id] = de_feat_get_num_vi_chns(screen_id);
		chn_num[screen_id] = de_feat_get_num_chns(screen_id);

		for (ch_id = 0; ch_id < vi_num[screen_id]; ch_id++) {
			vep_support[screen_id][ch_id] =
			    de_feat_is_support_vep_by_chn(screen_id, ch_id);
			if (vep_support[screen_id][ch_id]) {
				/* fce csc */
				base = para->reg_base[DISP_MOD_DE] +
				       (screen_id + 1) * 0x00100000
				       + FCE_OFST + 0x40;

				__inf("sel%d, icsc_base[%d]=0x%p\n", screen_id,
				      ch_id, (void *)base);

				memory = kmalloc(sizeof(struct __icsc_reg_t),
						 GFP_KERNEL | __GFP_ZERO);
				if (memory == NULL) {
					__wrn("alloc icsc[%d][%d] mm fail!",
					      screen_id, ch_id);
					__wrn("size=0x%x\n", (unsigned int)
					sizeof(struct __icsc_reg_t));
					return -1;
				}
				icsc_block[screen_id][ch_id].off = base;
				icsc_block[screen_id][ch_id].val = memory;
				icsc_block[screen_id][ch_id].size = 0x40;
				icsc_block[screen_id][ch_id].dirty = 0;

				de_icsc_set_reg_base(screen_id, ch_id, memory);

				/* fcc csc */
				base = para->reg_base[DISP_MOD_DE] +
				       (screen_id + 1) * 0x00100000
				       + FCC_OFST + 0x60;

				__inf("sel%d, fcc_csc_base[%d]=0x%p\n",
				      screen_id, ch_id, (void *)base);

				memory = kmalloc(fcc_csc_reg_struct_size,
						 GFP_KERNEL | __GFP_ZERO);
				if (memory == NULL) {
					__wrn("alloc fcc_csc[%d][%d] mm fail!",
					      screen_id, ch_id);
					__wrn("size=0x%x\n", (unsigned int)
					      fcc_csc_reg_struct_size);
					return -1;
				}

				block_size = fcc_csc_reg_struct_size;

				if (fcc_csc_reg_struct_size != fcc_csc_reg_size) {
				    __wrn("fcc_csc size error: %d != %d \n",
						fcc_csc_reg_struct_size, fcc_csc_reg_size);

				    if (fcc_csc_reg_struct_size < fcc_csc_reg_size)
						block_size = fcc_csc_reg_struct_size;
				    else
						block_size = fcc_csc_reg_size;
				}

				fcc_csc_block[screen_id][ch_id].off = base;
				fcc_csc_block[screen_id][ch_id].val = memory;
				fcc_csc_block[screen_id][ch_id].size = block_size;
				fcc_csc_block[screen_id][ch_id].dirty = 0;

				de_fcsc_set_reg_base(screen_id, ch_id, memory);
			}
		}

		for (ch_id = 0; ch_id < chn_num[screen_id]; ch_id++) {
			base_ofst = 0x0910 + ch_id * 0x30;
			base = para->reg_base[DISP_MOD_DE] + (screen_id + 1)
				* 0x00100000 + base_ofst;

			memory = kmalloc(sizeof(struct __ccsc_reg_t),
					 GFP_KERNEL | __GFP_ZERO);
			if (memory == NULL) {
				__wrn("alloc Ccsc[%d][%d] mm fail!size=0x%x\n",
				     screen_id, ch_id,
				     (unsigned int)sizeof(struct __ccsc_reg_t));
				return -1;
			}

			csc_block[screen_id][ch_id].off = base;
			csc_block[screen_id][ch_id].val = memory;
			csc_block[screen_id][ch_id].size = 0x30;
			csc_block[screen_id][ch_id].dirty = 0;

			de_ccsc_set_reg_base(screen_id, ch_id, memory);
		}

		/* CDC */
		for (ch_id = 0; ch_id < chn_num[screen_id]; ch_id++) {
			cdc_support[screen_id][ch_id] =
			    de_feat_is_support_cdc_by_chn(screen_id, ch_id);
			if (cdc_support[screen_id][ch_id]) {
				base_ofst = 0xd0000 + ch_id * 0x8000;
				base = para->reg_base[DISP_MOD_DE] +
				       (screen_id + 1) * 0x00100000 + base_ofst;

				memory = kmalloc(sizeof(struct __cdc_reg_t),
					 GFP_KERNEL | __GFP_ZERO);
				if (memory == NULL) {
					__wrn("alloc CDC[%d][%d] mm fail!\n",
					      screen_id, ch_id);
					return -1;
				}

				cdc_block[screen_id][ch_id].off = base;
				cdc_block[screen_id][ch_id].val = memory;
				cdc_block[screen_id][ch_id].size = 0x90;
				cdc_block[screen_id][ch_id].dirty = 0;

				de_cdc_set_reg_base(screen_id, ch_id, memory);

				memory = kmalloc(sizeof(struct __cdc_status),
					 GFP_KERNEL | __GFP_ZERO);
				if (memory == NULL) {
					__wrn("alloc CDC[%d][%d] mm fail!\n",
					      screen_id, ch_id);
					return -1;
				}
				cdc_status[screen_id][ch_id] = memory;

			}
		}
	}
	return 0;
}

int de_ccsc_double_init(struct disp_bsp_init_para *para)
{
	uintptr_t base, base_ofst;
	int screen_id, ch_id, device_num;

	device_num = de_feat_get_num_screens();

	for (screen_id = 0; screen_id < device_num; screen_id++) {
		vi_num[screen_id] = de_feat_get_num_vi_chns(screen_id);
		chn_num[screen_id] = de_feat_get_num_chns(screen_id);

		for (ch_id = 0; ch_id < vi_num[screen_id]; ch_id++) {
			vep_support[screen_id][ch_id] =
			    de_feat_is_support_vep_by_chn(screen_id, ch_id);
			if (vep_support[screen_id][ch_id]) {
				/* fce csc */
				base = para->reg_base[DISP_MOD_DE] +
				       (screen_id + 1) * 0x00100000
				       + FCE_OFST + 0x40;

				de_icsc_set_reg_base(screen_id, ch_id,
						    (void *)base);

				/* fcc csc */
				base = para->reg_base[DISP_MOD_DE] +
				       (screen_id + 1) * 0x00100000
				       + FCC_OFST + 0x60;

				de_fcsc_set_reg_base(screen_id, ch_id,
						     (void *)base);
			}
		}

		for (ch_id = 0; ch_id < chn_num[screen_id]; ch_id++) {
			base_ofst = 0x0910 + ch_id * 0x30;
			base = para->reg_base[DISP_MOD_DE] + (screen_id + 1)
				* 0x00100000 + base_ofst;

			de_ccsc_set_reg_base(screen_id, ch_id, (void *)base);
		}

		for (ch_id = 0; ch_id < chn_num[screen_id]; ch_id++) {
			cdc_support[screen_id][ch_id] =
			    de_feat_is_support_cdc_by_chn(screen_id, ch_id);
			if (cdc_support[screen_id][ch_id]) {
				base_ofst = 0xd0000 + ch_id * 0x8000;
				base = para->reg_base[DISP_MOD_DE] +
				       (screen_id + 1) * 0x00100000 + base_ofst;

				de_cdc_set_reg_base(screen_id, ch_id,
						    (void *)base);
			}
		}
	}
	return 0;
}

int de_ccsc_exit(void)
{
	int screen_id, ch_id, device_num;

	device_num = de_feat_get_num_screens();

	for (screen_id = 0; screen_id < device_num; screen_id++) {
		vi_num[screen_id] = de_feat_get_num_vi_chns(screen_id);
		chn_num[screen_id] = de_feat_get_num_chns(screen_id);

		for (ch_id = 0; ch_id < vi_num[screen_id]; ch_id++) {
			vep_support[screen_id][ch_id] =
			    de_feat_is_support_vep_by_chn(screen_id, ch_id);
			if (vep_support[screen_id][ch_id]) {
				kfree(icsc_block[screen_id][ch_id].val);
				kfree(fcc_csc_block[screen_id][ch_id].val);
			}
		}

		for (ch_id = 0; ch_id < chn_num[screen_id]; ch_id++)
			kfree(csc_block[screen_id][ch_id].val);

		for (ch_id = 0; ch_id < chn_num[screen_id]; ch_id++)
			if (cdc_support[screen_id][ch_id])
				kfree(cdc_block[screen_id][ch_id].val);
	}
	return 0;
}

int de_ccsc_double_exit(void)
{
	return 0;
}

static unsigned int get_cscmod_idx(unsigned int cscmod)
{
	unsigned int idx;

	switch (cscmod) {
	case DE_BT601:
	case DE_BT470BG:
		idx = 0; break;
	case DE_BT709:
		idx = 1; break;
	case DE_BT2020NC:
	case DE_BT2020C:
		idx = 2; break;
	case DE_FCC:
		idx = 3; break;
	case DE_SMPTE240M:
		idx = 4; break;
	case DE_YCGCO:
		idx = 5; break;
	case DE_GBR:
		idx = 6; break;
	default:
		idx = 0; break;
	}
	return idx;
}

int de_csc_coeff_calc(unsigned int infmt, unsigned int incscmod,
		      unsigned int inrange, unsigned int outfmt,
		      unsigned int outcscmod, unsigned int outrange,
		      int *csc_coeff)
{
	unsigned int inidx; /* index for incscmod */
	unsigned int outidx; /* index for outcscmod */
	int *ptr_coeff = NULL;

	inidx = get_cscmod_idx(incscmod);
	outidx = get_cscmod_idx(outcscmod);

	if (infmt == DE_RGB) {
		/* only support inrange is DISP_COLOR_RANGE_0_255 */
		if (outfmt == DE_YUV)
			if (outrange == DISP_COLOR_RANGE_0_255)
				ptr_coeff = &r2y[7+outidx][0];
			else /* outrange == DISP_COLOR_RANGE_16_235 */
				ptr_coeff = &r2y[outidx][0];
		else /* outfmt == DE_RGB */
			if (outrange == DISP_COLOR_RANGE_0_255)
				ptr_coeff = &r2r[0][0];
			else /* outrange == DISP_COLOR_RANGE_16_235 */
				ptr_coeff = &r2r[1][0];
	} else { /* infmt == DE_YUV */
		if (outfmt == DE_YUV) {
			if (inrange == DISP_COLOR_RANGE_0_255)
				if (outrange == DISP_COLOR_RANGE_0_255)
					ptr_coeff = &y2yf2f[inidx*3+outidx][0];
				else /* outrange == DISP_COLOR_RANGE_16_235 */
					ptr_coeff = &y2yf2l[inidx*3+outidx][0];
			else /* inrange == DISP_COLOR_RANGE_16_235 */
				if (outrange == DISP_COLOR_RANGE_0_255)
					ptr_coeff = &y2yl2f[inidx*3+outidx][0];
				else /* outrange == DISP_COLOR_RANGE_16_235 */
					ptr_coeff = &y2yl2l[inidx*3+outidx][0];
		} else { /* outfmt == DE_RGB */
			if (inrange == DISP_COLOR_RANGE_0_255)
				if (outrange == DISP_COLOR_RANGE_0_255)
					ptr_coeff = &y2rf2f[inidx][0];
				else
					ptr_coeff = &y2rf2l[inidx][0];
			else
				if (outrange == DISP_COLOR_RANGE_0_255)
					ptr_coeff = &y2rl2f[inidx][0];
				else
					ptr_coeff = &y2rl2l[inidx][0];
		}
	}

	if (ptr_coeff != NULL)
		memcpy((void *)csc_coeff, (void *)ptr_coeff, 64);
	else
		__wrn("in:fmt=%d,mod=%d,range=%d,out:%d,%d,%d not support!\n",
		    infmt, incscmod, inrange, outfmt, outcscmod, outrange);

	return 0;
}

static unsigned int get_convert_type(unsigned int in_type,
				     unsigned int out_type)
{
	unsigned int convert_type = DE_TFC_SDR2SDR;

	if (in_type == DE_SDR) {
		if (out_type == DE_SDR)
			convert_type = DE_TFC_SDR2SDR;
		else if (out_type == DE_WCG)
			convert_type = DE_TFC_SDR2WCG;
		else if (out_type == DE_HDR10)
			convert_type = DE_TFC_SDR2HDR10;
		else if (out_type == DE_HLG)
			convert_type = DE_TFC_SDR2HLG;
	} else if (in_type == DE_WCG) {
		if (out_type == DE_SDR)
			convert_type = DE_TFC_WCG2SDR;
		else if (out_type == DE_WCG)
			convert_type = DE_TFC_WCG2WCG;
		else if (out_type == DE_HDR10)
			convert_type = DE_TFC_WCG2HDR10;
		else if (out_type == DE_HLG)
			convert_type = DE_TFC_WCG2HLG;
	} else if (in_type == DE_HDR10) {
		if (out_type == DE_SDR)
			convert_type = DE_TFC_HDR102SDR;
		else if (out_type == DE_WCG)
			convert_type = DE_TFC_HDR102WCG;
		else if (out_type == DE_HDR10)
			convert_type = DE_TFC_HDR102HDR10;
		else if (out_type == DE_HLG)
			convert_type = DE_TFC_HDR102HLG;
	} else if (in_type == DE_HLG) {
		if (out_type == DE_SDR)
			convert_type = DE_TFC_HLG2SDR;
		else if (out_type == DE_WCG)
			convert_type = DE_TFC_HLG2WCG;
		else if (out_type == DE_HDR10)
			convert_type = DE_TFC_HLG2HDR10;
		else if (out_type == DE_HLG)
			convert_type = DE_TFC_HLG2HLG;
	}

	return convert_type;
}

static int de_set_cdc_lut(unsigned int screen_id, unsigned int ch_id,
			  unsigned int **lut_ptr)
{

	memcpy((void *)(cdc_block[screen_id][ch_id].off + 0x1000),
	       (void *)lut_ptr[0], sizeof(unsigned int) * 729);
	memcpy((void *)(cdc_block[screen_id][ch_id].off + 0x1c00),
	       (void *)lut_ptr[1], sizeof(unsigned int) * 648);
	memcpy((void *)(cdc_block[screen_id][ch_id].off + 0x2800),
	       (void *)lut_ptr[2], sizeof(unsigned int) * 648);
	memcpy((void *)(cdc_block[screen_id][ch_id].off + 0x3400),
	       (void *)lut_ptr[3], sizeof(unsigned int) * 576);
	memcpy((void *)(cdc_block[screen_id][ch_id].off + 0x4000),
	       (void *)lut_ptr[4], sizeof(unsigned int) * 648);
	memcpy((void *)(cdc_block[screen_id][ch_id].off + 0x4c00),
	       (void *)lut_ptr[5], sizeof(unsigned int) * 576);
	memcpy((void *)(cdc_block[screen_id][ch_id].off + 0x5800),
	       (void *)lut_ptr[6], sizeof(unsigned int) * 576);
	memcpy((void *)(cdc_block[screen_id][ch_id].off + 0x6400),
	       (void *)lut_ptr[7], sizeof(unsigned int) * 512);

	cdc_dev[screen_id][ch_id]->update.dwval = 1;
	cdc_block[screen_id][ch_id].dirty = 1;

	return 0;
}

int de_cdc_coeff_set(unsigned int sel, unsigned int ch_id,
			    struct disp_csc_config *config, unsigned int fmt,
			    unsigned int mode, unsigned int color_range,
			    unsigned int eotf)
{
	unsigned int in_type, out_type;
	unsigned int cdc_bypass = 0;
	unsigned int convert_type;
	unsigned int *lut_ptr[DE_CDC_LUT_NUM];
	unsigned int csc0_in_fmt, csc0_in_mode, csc0_in_color_range,
		     csc0_out_fmt, csc0_out_mode, csc0_out_color_range;
	unsigned int csc1_in_fmt, csc1_in_mode, csc1_in_color_range,
		     csc1_out_fmt, csc1_out_mode, csc1_out_color_range;
	int csc0_coeff[16], csc1_coeff[16];
	int i;

	/* decide what type of the input content is */
	if (config->in_mode == DE_BT2020NC || config->in_mode == DE_BT2020C) {
		if (config->in_eotf == DISP_EOTF_SMPTE2084)
			in_type = DE_HDR10;
		else if (config->in_eotf == DISP_EOTF_ARIB_STD_B67)
			in_type = DE_HLG;
		else
			in_type = DE_WCG;
	} else
		in_type = DE_SDR;

	/* decide what type of output device is */
	if (config->out_mode == DE_BT2020NC)
		if (config->out_eotf == DISP_EOTF_SMPTE2084)
			out_type = DE_HDR10;
		else if (config->out_eotf == DISP_EOTF_ARIB_STD_B67)
			out_type = DE_HLG;
		else
			out_type = DE_WCG;
	else
		out_type = DE_SDR;

	convert_type = get_convert_type(in_type, out_type);

	/* decide the CSC in/out para and CDC LUT ptr */
	if (config->in_fmt == DE_RGB) {
		/* For RGB format, CSC0/1 in CDC must bypass */
		csc0_in_fmt = DE_RGB;
		csc0_in_mode = config->in_mode;
		csc0_in_color_range = config->in_color_range;
		csc0_out_fmt = DE_RGB;
		csc0_out_mode = config->in_mode;
		csc0_out_color_range = config->in_color_range;

		csc1_in_fmt = fmt;
		csc1_in_mode = mode;
		csc1_in_color_range = color_range;
		csc1_out_fmt = fmt;
		csc1_out_mode = mode;
		csc1_out_color_range = color_range;

		/* Copy lut pointer */
		for (i = 0; i < DE_CDC_LUT_NUM; i++)
			lut_ptr[i] = cdc_lut_ptr_r[convert_type][i];

		/* Bypass CDC if type unchange or no support YET */
		switch (convert_type) {
		/* Bypass type */
		case DE_TFC_SDR2SDR:
		case DE_TFC_WCG2WCG:
		case DE_TFC_HDR102HDR10:
		case DE_TFC_HLG2HLG:
			cdc_bypass = 1; break;
		/* Support type (Add support type here in the future) */
		case DE_TFC_SDR2WCG:
		case DE_TFC_SDR2HDR10:
			cdc_bypass = 0; break;
		/* Non-Support type */
		default:
			__wrn("de_cdc_coeff_set:conversion type no support %d",
			      convert_type);
			cdc_bypass = 1; break;
		}
	} else { /* config->in_fmt == DE_YUV */
		/* For YUV format:
		 * CSC0 in CDC convert input to limit data range and
		 * bt.709 if framebuffer is non-BT2020 color space
		 * (assume color primaries and EOTF is BT709);
		 * CSC1 in CDC convert CDC result back to original data range
		 */
		csc0_in_fmt = DE_YUV;
		csc0_in_mode = config->in_mode;
		csc0_in_color_range = config->in_color_range;
		csc0_out_fmt = DE_YUV;
		if (config->in_mode != DE_BT2020NC &&
		    config->in_mode != DE_BT2020C)
			csc0_out_mode = DE_BT709;
		else
			csc0_out_mode = config->in_mode;
		csc0_out_color_range = DISP_COLOR_RANGE_16_235;

		csc1_in_fmt = fmt;
		csc1_in_mode = mode;
		csc1_in_color_range = DISP_COLOR_RANGE_16_235;
		csc1_out_fmt = fmt;
		csc1_out_mode = mode;
		csc1_out_color_range = color_range;

		/* Copy lut pointer */
		for (i = 0; i < DE_CDC_LUT_NUM; i++)
			lut_ptr[i] = cdc_lut_ptr_y[convert_type][i];

		/* Bypass CDC if type unchange or no support YET */
		switch (convert_type) {
		/* Bypass type */
		case DE_TFC_SDR2SDR:
		case DE_TFC_WCG2WCG:
		case DE_TFC_HDR102HDR10:
		case DE_TFC_HLG2HLG:
			cdc_bypass = 1;
		break;
		/* Support type (Add support type here in the future) */
		case DE_TFC_SDR2WCG:
		case DE_TFC_SDR2HDR10:
		case DE_TFC_WCG2SDR:
		case DE_TFC_WCG2HDR10:
		case DE_TFC_HDR102SDR:
		case DE_TFC_HDR102WCG:
		case DE_TFC_HLG2SDR:
		case DE_TFC_HLG2WCG:
		case DE_TFC_HLG2HDR10:
			cdc_bypass = 0;
		break;
		/* Non-Support type */
		default:
			__wrn("de_cdc_coeff_set:conversion type no support %d",
			      convert_type);
			cdc_bypass = 1;
		break;
		}
	}

	/* set register */
	if (cdc_bypass == 0) {
		/* calc and set csc, Only CDC in Video channel has CSC */
		if (ch_id < vi_num[sel]) {
			de_csc_coeff_calc(csc0_in_fmt, csc0_in_mode,
					  csc0_in_color_range, csc0_out_fmt,
					  csc0_out_mode, csc0_out_color_range,
					  csc0_coeff);

			de_csc_coeff_calc(csc1_in_fmt, csc1_in_mode,
					  csc1_in_color_range, csc1_out_fmt,
					  csc1_out_mode, csc1_out_color_range,
					  csc1_coeff);

			de_set_cdc_csc_coef(sel, ch_id, csc0_coeff, csc1_coeff);
		}
		/* set 3D-LUT if convert_type or in_fmt changed */
		if ((convert_type != cdc_status[sel][ch_id]->convert_type) ||
		    (config->in_fmt != cdc_status[sel][ch_id]->in_fmt)) {
			cdc_status[sel][ch_id]->convert_type = convert_type;
			cdc_status[sel][ch_id]->in_fmt = config->in_fmt;
			de_set_cdc_lut(sel, ch_id, lut_ptr);
		}
		/* enable cdc */
		cdc_dev[sel][ch_id]->ctrl.dwval = 1;
		cdc_block[sel][ch_id].dirty = 1;
	} else {
		/* disable cdc reg */
		cdc_dev[sel][ch_id]->ctrl.dwval = 0;
		cdc_block[sel][ch_id].dirty = 1;
	}

	return 0;
}

static int __de_cdc_enable(unsigned int screen_id, unsigned int ch_id,
		  unsigned int enable)
{
	/* initial cdc status */
	cdc_status[screen_id][ch_id]->convert_type = DE_TFC_INIT;
	cdc_status[screen_id][ch_id]->in_fmt = DE_RGB;
	return 0;
}

int de_cdc_enable(unsigned int screen_id, unsigned int enable)
{
	int ch_id, channel_num;

	channel_num = de_feat_get_num_chns(screen_id);

	for (ch_id = 0; ch_id < channel_num; ch_id++) {
		cdc_support[screen_id][ch_id] =
		    de_feat_is_support_cdc_by_chn(screen_id, ch_id);
		if (cdc_support[screen_id][ch_id])
			__de_cdc_enable(screen_id, ch_id, enable);
	}

	return 0;
}
