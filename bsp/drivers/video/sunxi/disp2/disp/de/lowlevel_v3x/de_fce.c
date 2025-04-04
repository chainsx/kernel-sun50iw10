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
 *  File name   :  display engine 3.0 fce basic function definition
 *
 *  History     :  2016/03/30      vito cheng  v0.1  Initial version
 *
 ******************************************************************************/

#ifdef CONFIG_AW_DISP2_SUPPORT_ENAHNCE
#include "de_fce_type.h"
#include "de_rtmx.h"
#include "de_enhance.h"
#include "de_vep_table.h"

static struct __fce_reg_t *fce_dev[DE_NUM][CHN_NUM];

static struct de_reg_blocks fce_para_block[DE_NUM][CHN_NUM];
static struct de_reg_blocks fce_celut_block[DE_NUM][CHN_NUM];
static struct de_reg_blocks fce_hist_block[DE_NUM][CHN_NUM];

static uintptr_t fce_hw_base[DE_NUM][CHN_NUM] = { {0} };

static unsigned int g_sum[DE_NUM][VI_CHN_NUM];
static unsigned short *g_celut[DE_NUM][VI_CHN_NUM];
static unsigned int *g_hist[DE_NUM][VI_CHN_NUM];
static unsigned int *g_hist_p[DE_NUM][VI_CHN_NUM];
static struct hist_data *hist_res[DE_NUM][VI_CHN_NUM];
static struct __ce_status_t *g_ce_status[DE_NUM][VI_CHN_NUM];
struct __hist_status_t *g_hist_status[DE_NUM][VI_CHN_NUM];

#define FCE_PARA_NUM (12)
int fce_b_para[FCE_PARA_NUM][2] = {
	/* parameters TBD */
	/* lcd tv */
	{-64, -64},		/* gain for yuv */
	{-46, -46},		/*              */
	{-30, -30},		/*              */
	{-18, -18},		/*              */
	{-8,   -8},		/*              */
	{0,     0},		/* default */
	{8,     8},		/*              */
	{18,   18},		/*              */
	{30,   30},		/*              */
	{46,   46},		/*              */
	{64,   64},		/* gain for yuv */
	{0,     0}		/* gain for rgb */
};

int fce_c_para[FCE_PARA_NUM][6][2] = {
	/* parameters TBD */
	/* lcd tv */
	/* auto fixpara  bws     thr     slope      update */
	{{0, 0}, {0, 0}, {0, 0}, {0, 0}, {256, 256}, {16, 8} },
	{{1, 1}, {0, 0}, {32, 2}, {5, 3}, {384, 300}, {16, 8} },
	{{1, 1}, {0, 0}, {32, 4}, {6, 4}, {384, 300}, {16, 8} },
	{{1, 1}, {0, 0}, {32, 6}, {13, 10}, {384, 300}, {16, 8} },
	{{1, 1}, {0, 0}, {32, 8}, {21, 16}, {384, 300}, {16, 8} },
	{{1, 1}, {0, 0}, {32, 10}, {30, 22}, {384, 300}, {16, 8} },
	{{1, 1}, {0, 0}, {32, 12}, {40, 30}, {384, 300}, {16, 8} },
	{{1, 1}, {0, 0}, {32, 14}, {50, 38}, {384, 300}, {16, 8} },
	{{1, 1}, {0, 0}, {32, 16}, {60, 45}, {384, 300}, {16, 8} },
	{{1, 1}, {0, 0}, {32, 18}, {70, 53}, {384, 300}, {16, 8} },
	{{1, 1}, {0, 0}, {32, 20}, {80, 60}, {384, 300}, {16, 8} },
	{{0, 0}, {1, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0} }
};

/*******************************************************************************
 * function       : de_fce_set_reg_base(unsigned int sel, unsigned int chno,
 *                  unsigned int base)
 * description    : set fce reg base
 * parameters     :
 *                  sel         <rtmx select>
 *                  chno        <overlay select>
 *                  base        <reg base>
 * return         :
 *                  success
 ******************************************************************************/
static int de_fce_set_reg_base(unsigned int sel, unsigned int chno, void *base)
{
	fce_dev[sel][chno] = (struct __fce_reg_t *) base;

	return 0;
}

int de_fce_update_regs(unsigned int sel, unsigned int chno)
{
	uintptr_t base;

	if (fce_para_block[sel][chno].dirty == 0x1) {
		memcpy((void *)fce_para_block[sel][chno].off,
		       fce_para_block[sel][chno].val,
		       fce_para_block[sel][chno].size);
		fce_para_block[sel][chno].dirty = 0x0;
	}

	if (fce_celut_block[sel][chno].dirty == 0x1) {
		base = fce_hw_base[sel][chno];
		writel(0x1, (void __iomem *)(base + 0x28));
		/* AHB access CE LUT */
		memcpy((void *)fce_celut_block[sel][chno].off,
		       fce_celut_block[sel][chno].val,
		       fce_celut_block[sel][chno].size);
		writel(0x0, (void __iomem *)(base + 0x28));
		/* Module access CE LUT */
		fce_celut_block[sel][chno].dirty = 0x0;
	}

	return 0;
}

int de_fce_init(unsigned int sel, unsigned int chno, uintptr_t reg_base)
{
	uintptr_t base;
	void *memory;

	base = reg_base + (sel + 1) * 0x00100000 + FCE_OFST;

	fce_hw_base[sel][chno] = base;

	memory = kmalloc(sizeof(struct __fce_reg_t), GFP_KERNEL | __GFP_ZERO);
	if (memory == NULL) {
		__wrn("malloc fce[%d][%d] memory fail! size=0x%x\n", sel, chno,
		      (unsigned int)sizeof(struct __fce_reg_t));
		return -1;
	}

	fce_para_block[sel][chno].off = base;
	fce_para_block[sel][chno].val = memory;
	fce_para_block[sel][chno].size = 0x40;
	fce_para_block[sel][chno].dirty = 0;

	fce_celut_block[sel][chno].off = base + 0x200;
	fce_celut_block[sel][chno].val = memory + 0x200;
	fce_celut_block[sel][chno].size = 0x200;
	fce_celut_block[sel][chno].dirty = 0;

	fce_hist_block[sel][chno].off = base + 0x400;
	fce_hist_block[sel][chno].val = memory + 0x400;
	fce_hist_block[sel][chno].size = 0x400;
	fce_hist_block[sel][chno].dirty = 0;

	de_fce_set_reg_base(sel, chno, memory);

	/* hist */
	g_hist_status[sel][chno] =
	    kmalloc(sizeof(struct __hist_status_t),
	    GFP_KERNEL | __GFP_ZERO);
	if (g_hist_status[sel][chno] == NULL) {
		__wrn("malloc g_hist_status[%d][%d] memory fail! size=0x%x\n",
		      sel, chno, (unsigned int)sizeof(struct __hist_status_t));
		return -1;
	}

	g_hist_status[sel][chno]->runtime = 0;
	g_hist_status[sel][chno]->isenable = 0;
	g_hist_status[sel][chno]->twohistready = 0;

	g_hist[sel][chno] = kmalloc(1024, GFP_KERNEL | __GFP_ZERO);
	if (g_hist[sel][chno] == NULL) {
		__wrn("malloc hist[%d][%d] memory fail! size=0x%x\n",
		sel, chno, 1024);
		return -1;
	}
	g_hist_p[sel][chno] = kmalloc(1024, GFP_KERNEL | __GFP_ZERO);
	if (g_hist_p[sel][chno] == NULL) {
		__wrn("malloc g_hist_p[%d][%d] memory fail! size=0x%x\n", sel,
		      chno, 1024);
		return -1;
	}
	/* ce */
	g_ce_status[sel][chno] =
	    kmalloc(sizeof(struct __ce_status_t), GFP_KERNEL | __GFP_ZERO);
	if (g_ce_status[sel][chno] == NULL) {
		__wrn("malloc g_ce_status[%d][%d] memory fail! size=0x%x\n",
		      sel, chno, (unsigned int)sizeof(struct __ce_status_t));
		return -1;
	}

	g_ce_status[sel][chno]->isenable = 0;

	g_celut[sel][chno] = kmalloc(512, GFP_KERNEL | __GFP_ZERO);
	if (g_celut[sel][chno] == NULL) {
		__wrn("malloc celut[%d][%d] memory fail! size=0x%x\n",
		      sel, chno, 512);
		return -1;
	}

	hist_res[sel][chno] =
	    kmalloc(sizeof(struct hist_data), GFP_KERNEL | __GFP_ZERO);
	if (hist_res[sel][chno] == NULL) {
		__wrn("malloc hist_res[%d][%d] memory fail! size=0x%x\n", sel,
		      chno, (unsigned int)sizeof(struct hist_data));
		return -1;
	}

	memset(hist_res[sel][chno], 0, sizeof(struct hist_data));

	return 0;
}

int de_fce_double_init(unsigned int sel, unsigned int chno, uintptr_t reg_base)
{
	uintptr_t base;

	base = reg_base + (sel + 1) * 0x00100000 + FCE_OFST;

	fce_hw_base[sel][chno] = base;

	de_fce_set_reg_base(sel, chno, (void *)base);

	/* hist */
	g_hist_status[sel][chno] =
	    kmalloc(sizeof(struct __hist_status_t),
	    GFP_KERNEL | __GFP_ZERO);
	if (g_hist_status[sel][chno] == NULL) {
		__wrn("malloc g_hist_status[%d][%d] memory fail! size=0x%x\n",
		      sel, chno, (unsigned int)sizeof(struct __hist_status_t));
		return -1;
	}

	g_hist_status[sel][chno]->runtime = 0;
	g_hist_status[sel][chno]->isenable = 0;
	g_hist_status[sel][chno]->twohistready = 0;

	g_hist[sel][chno] = kmalloc(1024, GFP_KERNEL | __GFP_ZERO);
	if (g_hist[sel][chno] == NULL) {
		__wrn("malloc hist[%d][%d] memory fail! size=0x%x\n",
		sel, chno, 1024);
		return -1;
	}
	g_hist_p[sel][chno] = kmalloc(1024, GFP_KERNEL | __GFP_ZERO);
	if (g_hist_p[sel][chno] == NULL) {
		__wrn("malloc g_hist_p[%d][%d] memory fail! size=0x%x\n", sel,
		      chno, 1024);
		return -1;
	}
	/* ce */
	g_ce_status[sel][chno] =
	    kmalloc(sizeof(struct __ce_status_t), GFP_KERNEL | __GFP_ZERO);
	if (g_ce_status[sel][chno] == NULL) {
		__wrn("malloc g_ce_status[%d][%d] memory fail! size=0x%x\n",
		      sel, chno, (unsigned int)sizeof(struct __ce_status_t));
		return -1;
	}

	g_ce_status[sel][chno]->isenable = 0;

	g_celut[sel][chno] = kmalloc(512, GFP_KERNEL | __GFP_ZERO);
	if (g_celut[sel][chno] == NULL) {
		__wrn("malloc celut[%d][%d] memory fail! size=0x%x\n",
		      sel, chno, 512);
		return -1;
	}

	hist_res[sel][chno] =
	    kmalloc(sizeof(struct hist_data), GFP_KERNEL | __GFP_ZERO);
	if (hist_res[sel][chno] == NULL) {
		__wrn("malloc hist_res[%d][%d] memory fail! size=0x%x\n", sel,
		      chno, (unsigned int)sizeof(struct hist_data));
		return -1;
	}

	memset(hist_res[sel][chno], 0, sizeof(struct hist_data));

	return 0;
}

int de_fce_exit(unsigned int sel, unsigned int chno)
{
	kfree(fce_para_block[sel][chno].val);
	kfree(g_hist_status[sel][chno]);
	kfree(g_hist[sel][chno]);
	kfree(g_hist_p[sel][chno]);
	kfree(g_ce_status[sel][chno]);
	kfree(g_celut[sel][chno]);
	kfree(hist_res[sel][chno]);

	return 0;
}

int de_fce_double_exit(unsigned int sel, unsigned int chno)
{
	kfree(g_hist_status[sel][chno]);
	kfree(g_hist[sel][chno]);
	kfree(g_hist_p[sel][chno]);
	kfree(g_ce_status[sel][chno]);
	kfree(g_celut[sel][chno]);
	kfree(hist_res[sel][chno]);

	return 0;
}

/*******************************************************************************
 * function       : de_fce_set_size(unsigned int sel, unsigned int chno,
 *                   unsigned int width, unsigned int height)
 * description    : set fce size
 * parameters     :
 *                  sel         <rtmx select>
 *                  chno        <overlay select>
 *                  width       <input width>
 *                                      height  <input height>
 * return         :
 *                  success
 ******************************************************************************/
int de_fce_set_size(unsigned int sel, unsigned int chno, unsigned int width,
		    unsigned int height)
{
	fce_dev[sel][chno]->size.dwval = ((height - 1)<<16) | (width - 1);
	fce_para_block[sel][chno].dirty = 1;
	g_ce_status[sel][chno]->width = width;
	g_ce_status[sel][chno]->height = height;

	return 0;
}

/*******************************************************************************
 * function       : de_fce_set_window(unsigned int sel, unsigned int chno,
 *                     unsigned int win_enable, struct de_rect window)
 * description    : set fce window
 * parameters     :
 *                  sel         <rtmx select>
 *                  chno        <overlay select>
 *                  win_enable  <enable: 0-window mode diable;
 *                                       1-window mode enable>
 *                  window  <window rectangle>
 * return         :
 *                  success
 ******************************************************************************/
int de_fce_set_window(unsigned int sel, unsigned int chno,
		      unsigned int win_enable, struct de_rect window)
{
	de_set_bits(&fce_dev[sel][chno]->ctrl.dwval, win_enable, 31, 1);
	if (win_enable) {
		fce_dev[sel][chno]->win0.dwval = window.y << 16 | window.x;
		fce_dev[sel][chno]->win1.dwval =
		    (window.y + window.h - 1) << 16 | (window.x + window.w - 1);
	}
	fce_para_block[sel][chno].dirty = 1;

	return 0;
}

/*******************************************************************************
 * function       : de_fce_get_hist(unsigned int sel, unsigned int chno,
 *                  unsigned int hist[256], unsigned int *sum)
 * description    : get histogram result
 * parameters     :
 *                  sel         <rtmx select>
 *                  chno        <overlay select>
 *                  hist[256]   <frame histogram>
 *                  sum             <frame pixel value sum>
 * return         :
 *                  success
 ******************************************************************************/
static int de_fce_get_hist(unsigned int sel, unsigned int chno,
			   unsigned int hist[256], unsigned int *sum)
{
	uintptr_t base;

	base = fce_hw_base[sel][chno];

	/* Read histogram to hist[256] */
	memcpy((unsigned char *)hist,
	       (unsigned char *)fce_hist_block[sel][chno].off,
	       sizeof(unsigned int) * 256);

	/* Read */
	*sum = readl((void __iomem *)(base + 0x20));

	return 0;
}

/*******************************************************************************
 * function       : de_fce_set_ce(unsigned int sel, unsigned int chno,
 *                           unsigned short ce_lut[256])
 * description    : set ce lut, directly write to registers
 * parameters     :
 *                  sel         <rtmx select>
 *                  chno        <overlay select>
 *                  ce_lut[256] <ce lut>
 * return         :
 *                  success
 ******************************************************************************/
static int de_fce_set_ce(unsigned int sel, unsigned int chno,
		  unsigned short ce_lut[256])
{
	memcpy(fce_celut_block[sel][chno].val, (unsigned char *)ce_lut,
	       sizeof(unsigned short) * 256);
	fce_celut_block[sel][chno].dirty = 1;

	return 0;
}

static int de_hist_apply(unsigned int sel, unsigned int chno,
		  unsigned int hist_en)
{
	if (hist_en == 1) {
		/* enable this time */
		/* reset hist buffer */
		memset((unsigned char *)g_hist[sel][chno], 0, 1024);

		g_hist_status[sel][chno]->runtime = 0;
		g_hist_status[sel][chno]->twohistready = 0;
		g_hist_status[sel][chno]->isenable = 1;
	} else {
		/* disable this time */
		g_hist_status[sel][chno]->runtime = 0;
		g_hist_status[sel][chno]->twohistready = 0;
		g_hist_status[sel][chno]->isenable = 0;
	}

	return 0;
}

static int de_ce_apply(unsigned int sel, unsigned int chno, unsigned int ce_en,
		int brightness, unsigned int auto_contrast_en,
		unsigned int fix_contrast_en, unsigned int precent_thr,
		unsigned int update_diff_thr, unsigned int slope_lmt,
		unsigned int bwslvl)
{
	if (ce_en == 1) {
		/* enable this time */
		g_ce_status[sel][chno]->isenable = 1;
		g_ce_status[sel][chno]->b_automode = auto_contrast_en;

		if (auto_contrast_en) {
			g_ce_status[sel][chno]->up_precent_thr =
			    precent_thr;
			g_ce_status[sel][chno]->down_precent_thr =
			    precent_thr;
			g_ce_status[sel][chno]->update_diff_thr =
			    update_diff_thr;
			g_ce_status[sel][chno]->slope_black_lmt =
			    slope_lmt;
			g_ce_status[sel][chno]->slope_white_lmt =
			    slope_lmt;
			g_ce_status[sel][chno]->bls_lvl = bwslvl;
			g_ce_status[sel][chno]->wls_lvl = bwslvl;
			g_ce_status[sel][chno]->brightness = brightness;

			/* clear hist data when disable */
			memset(hist_res[sel][chno], 0,
				sizeof(struct hist_data));

		} else if (fix_contrast_en)
			memcpy((void *)g_celut[sel][chno],
			       (void *)ce_constant_lut, 512);

		de_fce_set_ce(sel, chno, g_celut[sel][chno]);

	} else {
		/* disable this time */
		g_ce_status[sel][chno]->isenable = 0;

	}

	return 0;

}

/*******************************************************************************
 * function       : de_fce_info2para(unsigned int sharp,
 *                  unsigned int auto_contrast, unsigned int auto_color,
 *                  struct de_rect window, struct __fce_config_data *para)
 * description    : info->para conversion
 * parameters     :
 *
 * return         :
 *                  success
 ******************************************************************************/
int de_fce_info2para(unsigned int sel, unsigned int chno,
		     unsigned int fmt, unsigned int dev_type,
		     struct __fce_config_data *para, unsigned int bypass)
{
	int brightness;
	int auto_contrast_en;
	int fix_contrast_en;
	int bwslvl;
	int precent_thr;
	int slope_limit;
	int update_thr;
	unsigned int fce_en;
	unsigned int hist_en;
	unsigned int ce_en;
	unsigned int ce_cc_en;
	unsigned int reg_val;
	unsigned int b_level, c_level;

	if (fmt == 1) {
		b_level = FCE_PARA_NUM - 1;
		c_level = FCE_PARA_NUM - 1;
	} else {
		b_level = para->bright_level;
		c_level = para->contrast_level;
	}

	brightness = fce_b_para[b_level][dev_type];
	auto_contrast_en = fce_c_para[c_level][0][dev_type];
	fix_contrast_en = fce_c_para[c_level][1][dev_type];
	bwslvl = fce_c_para[c_level][2][dev_type];
	precent_thr = fce_c_para[c_level][3][dev_type];
	slope_limit = fce_c_para[c_level][4][dev_type];
	update_thr = fce_c_para[c_level][5][dev_type];

	/* limitation of auto contrast and brightness */
	/* auto_contrast_en must be 1 when brightness isnot equal to 0 */
	if (brightness != 0)
		auto_contrast_en = 1;

	/* decide enable registers */
	if ((brightness == 0 && auto_contrast_en == 0) || (bypass == 1))
		hist_en = 0;
	else
		hist_en = 1;

	if ((brightness == 0 &&
	     auto_contrast_en == 0 &&
	     fix_contrast_en == 0)
	    || (bypass == 1)) {
		ce_en = 0;
		ce_cc_en = 0;
	} else {
		ce_en = 1;
		ce_cc_en = 1;
	}

	/* set enable registers */
	fce_en = 1;
	reg_val = (ce_en << 17) | (hist_en << 16) | fce_en;
	de_set_bits(&fce_dev[sel][chno]->ctrl.dwval, reg_val, 0, 18);

	fce_dev[sel][chno]->cecc.dwval = ce_cc_en;

	/* set software variable */
	de_hist_apply(sel, chno, hist_en);
	de_ce_apply(sel, chno, ce_en, brightness, auto_contrast_en,
		    fix_contrast_en, precent_thr, update_thr, slope_limit,
		    bwslvl);
	fce_para_block[sel][chno].dirty = 0x1;

	return 0;
}

int de_fce_enable(unsigned int sel, unsigned int chno, unsigned int en)
{
	de_set_bits(&fce_dev[sel][chno]->ctrl.dwval, en, 0, 1);
	fce_para_block[sel][chno].dirty = 1;

	return 0;
}

int de_fce_init_para(unsigned int sel, unsigned int chno)
{
	fce_dev[sel][chno]->ctrl.dwval = 0x00000001; /* always enable fce */
	fce_dev[sel][chno]->ftcgain.dwval = 0x00000000; /* disable FTC */
	fce_dev[sel][chno]->ftdhue.dwval = 0x0096005a;
	fce_dev[sel][chno]->ftdchr.dwval = 0x0028000a;
	fce_dev[sel][chno]->ftdslp.dwval = 0x04040604;

	fce_para_block[sel][chno].dirty = 0x1;

	memcpy((void *)g_celut[sel][chno],
		   (void *)ce_bypass_lut, 512);
	de_fce_set_ce(sel, chno, g_celut[sel][chno]);

	return 0;
}

static int toupdate(int hist_mean, int hist_diff, int diff_coeff,
		    int change_thr)
{
	int toupdate = 0;
	int diff_div = 1;

	if (hist_diff < 0)
		diff_div = 2;
	else
		diff_div = 1;

	if (hist_mean > 96)
		change_thr = 3 * change_thr / diff_div;
	if (hist_mean > 64)
		change_thr = 2 * change_thr / diff_div;
	else
		change_thr = change_thr / diff_div;

	hist_diff = abs(hist_diff);

	if (hist_diff > change_thr ||
	    (diff_coeff > 100 && hist_diff > change_thr / 8) ||
	    (diff_coeff > 64 && hist_diff > change_thr / 4) ||
	    (diff_coeff > 32 && hist_diff > change_thr / 2)) {
		toupdate = 1;
	} else
		toupdate = 0;

	return toupdate;
}

static int bright_thr(int i, int c, int thr)
{
	int ret = (((128 - i) * c + 4096) * thr) >> 12;

	return ret;
}

/*******************************************************************************
 * function       :auto_ce_model(unsigned int width, unsigned height,
 *                 unsigned int sumcnt, unsigned int hist[256],
 *                 unsigned int up_precent_thr, unsigned int down_precent_thr,
 *                 unsigned char celut[256])
 * description    : Auto-ce Alg
 * parameters     :
 *                  width               <layer width>
 *                  height              <layer height>
 *                  hist[256]   <the latest frame histogram>
 *                  sumcnt       <the latest frame pixel value sum>
 *                  up_precent_thr/down_precent_thr <ce para>
 *                  celut <auto-ce result>
 * return         :
 ******************************************************************************/
static void auto_ce_model(unsigned int width, unsigned height,
			  unsigned int sumcnt, unsigned int hist[256],
			  unsigned int up_precent_thr,
			  unsigned int down_precent_thr,
			  int brightness,
			  unsigned int blelvl, unsigned int wlelvl,
			  unsigned int change_thr, unsigned int lowest_black,
			  unsigned int highest_white, unsigned int ce_thr,
			  unsigned short celut[256],
			  struct hist_data *p_hist_data)
{
	static unsigned int hist_r[256], p[256];
	unsigned int i;
	unsigned int mean;
	unsigned int total_pixel, total_pixel_r, total_size;
	int uthr, lthr;
	unsigned int half;
	int be_update;
	unsigned int black_str_lv;
	unsigned int white_str_lv;
	unsigned int gray_level;
	unsigned int bitextented;
	unsigned long long tmp;
	unsigned int temp0, temp1, temp2;
	int lthr_tmp;

	bitextented = 2;
	gray_level = 1<<8;

	if (height % 2 == 0) {
		total_size = total_pixel = (width * height) >> 1;
	} else {
		total_size = total_pixel = (width * (height - 1)
		    + (width + 1)) >> 1;
	}

	for (i = 0; i < lowest_black; i++)
		total_pixel -= hist[i];

	for (i = highest_white; i < 256; i++)
		total_pixel -= hist[i];

	/* less than 66% pixel in valid range */
	if (total_size > 3 * total_pixel)
		total_size = 0;

	total_pixel_r = 0;

	/* MEAN */
	mean = total_pixel / (highest_white - lowest_black);

	/* picture too small, can't ce */
	if ((mean == 0) || (p_hist_data->hist_mean < ce_thr)
	  || total_size == 0) {
		for (i = 0; i < gray_level; i++)
			celut[i] = i<<bitextented;
	} else {
		int black_thr;

		uthr = mean + mean * up_precent_thr / 100;
		lthr = mean - mean * down_precent_thr / 100;

		black_thr = lthr;

		if (p_hist_data->hist_mean > 96)
			black_str_lv = black_thr;
		else if (p_hist_data->hist_mean > 64)
			black_str_lv = black_thr - black_thr * blelvl / 32 / 3;
		else if (p_hist_data->hist_mean < 21)
			black_str_lv = black_thr - black_thr * blelvl / 16 / 3;
		else
			black_str_lv = (black_thr - black_thr * blelvl / 16 / 3)
					+ (black_thr * blelvl / 3 / 32)
					* (p_hist_data->hist_mean - 21)/43;

		white_str_lv = lthr - lthr * wlelvl / 16 / 3;

		/* generate p */
		lthr_tmp = bright_thr(lowest_black, brightness, black_str_lv);
		if (hist[lowest_black] > mean)
			hist_r[lowest_black] = mean;
		else if (hist[lowest_black] > lthr_tmp)
			hist_r[lowest_black] = hist[lowest_black];
		else
			hist_r[lowest_black] = lthr_tmp < 0 ? 0 : lthr_tmp;

		total_pixel_r = hist_r[lowest_black];
		p[lowest_black] = hist_r[lowest_black];

		/* black zone */
		for (i = lowest_black + 1; i < p_hist_data->black_thr0; i++) {
			lthr_tmp = bright_thr(i, brightness, black_str_lv);
			if (hist[i] > mean)
				hist_r[i] = mean;
			else if (hist[i] > lthr_tmp)
				hist_r[i] = hist[i];
			else
				hist_r[i] = lthr_tmp < 0 ? 0 : lthr_tmp;

			total_pixel_r = total_pixel_r + hist_r[i];
			p[i] = p[i - 1] + hist_r[i];
		}

		for (i = p_hist_data->black_thr0; i < p_hist_data->black_thr1;
		     i++) {
			lthr_tmp = bright_thr(i, brightness, lthr);
			if (hist[i] > mean)
				hist_r[i] = mean;
			else if (hist[i] > lthr_tmp)
				hist_r[i] = hist[i];
			else
				hist_r[i] = lthr_tmp < 0 ? 0 : lthr_tmp;

			total_pixel_r = total_pixel_r + hist_r[i];
			p[i] = p[i - 1] + hist_r[i];
		}

		if (p_hist_data->white_thr0 >= 256) {
			__wrn("p_hist_data->white_thr0(%d) >= 256",
			      p_hist_data->white_thr0);
			p_hist_data->white_thr0 = 255;
		}
		for (i = p_hist_data->black_thr1; i < p_hist_data->white_thr0;
		     i++) {
			lthr_tmp = bright_thr(i, brightness, lthr);
			if (hist[i] > uthr)
				hist_r[i] = uthr;
			else if (hist[i] > lthr_tmp)
				hist_r[i] = hist[i];
			else
				hist_r[i] = lthr_tmp < 0 ? 0 : lthr_tmp;

			total_pixel_r = total_pixel_r + hist_r[i];
			p[i] = p[i - 1] + hist_r[i];
		}

		/* white zone */
		for (i = p_hist_data->white_thr0; i < p_hist_data->white_thr1;
		     i++) {
			lthr_tmp = bright_thr(i, brightness, lthr);
			if (hist[i] > uthr)
				hist_r[i] = uthr;
			else if (hist[i] > lthr_tmp)
				hist_r[i] = hist[i];
			else
				hist_r[i] = lthr_tmp < 0 ? 0 : lthr_tmp;

			total_pixel_r = total_pixel_r + hist_r[i];
			p[i] = p[i - 1] + hist_r[i];
		}
		for (i = p_hist_data->white_thr1; i < highest_white + 1; i++) {
			lthr_tmp = bright_thr(i, brightness, white_str_lv);
			if (hist[i] > uthr)
				hist_r[i] = uthr;
			else if (hist[i] > lthr_tmp)
				hist_r[i] = hist[i];
			else
				hist_r[i] = lthr_tmp < 0 ? 0 : lthr_tmp;

			total_pixel_r = total_pixel_r + hist_r[i];
			p[i] = p[i - 1] + hist_r[i];
		}

		for (i = highest_white + 1; i < gray_level; i++)
			p[i] = p[i - 1] + mean;

		be_update = toupdate(p_hist_data->hist_mean,
			    p_hist_data->hist_mean_diff,
			    p_hist_data->diff_coeff, change_thr);

		if (be_update) {
			temp0 = ((lowest_black - 1) << bitextented);
			temp1 = (highest_white - lowest_black + 1) *
				(1 << bitextented);
			/* temp2 = highest_white << bitextented; */
			temp2 = (gray_level << bitextented) - 1;
			if (total_pixel_r != 0) {
				half = total_pixel_r >> 1;
				/* for (i = lowest_black; i < highest_white + 1;
				 *      i++) {
				 */
				for (i = lowest_black; i < gray_level;
				     i++) {
					tmp = (unsigned long long)p[i] * temp1
					      + half;
					do_div(tmp, total_pixel_r);
					tmp += temp0;
					if (tmp > temp2)
						tmp = temp2;

					celut[i] = (unsigned short)tmp;
				}
			} else {
				half = total_pixel >> 1;
				/* for (i = lowest_black; i < highest_white + 1;
				 *      i++) {
				 */
				for (i = lowest_black; i < gray_level;
				     i++) {
					tmp = (unsigned long long)p[i] * temp1
					      + half;
					do_div(tmp, total_pixel);
					tmp += temp0;
					if (tmp > temp2)
						tmp = temp2;

					celut[i] = (unsigned short)tmp;
				}
			}

			for (i = 0; i < lowest_black; i++)
				celut[i] = i << bitextented;
			/* for (i = highest_white + 1; i < gray_level; i++)
			 *      celut[i] = i  << bitextented;
			 */
		}
	}

}

/*******************************************************************************
 * function       : auto_bws_model(unsigned int width, unsigned int height,
 *                  unsigned int hist[256], unsigned int sum,
 *                  unsigned int pre_slope_black, unsigned int pre_slope_white,
 *                  unsigned int frame_bld_en, unsigned int bld_high_thr,
 *                  unsigned int bld_low_thr, unsigned int bld_weight_lmt,
 *                  unsigned int present_black, unsigned int present_white,
 *                  unsigned int slope_black_lmt, unsigned int slope_white_lmt,
 *                  unsigned int black_prec, unsigned int white_prec,
 *                  unsigned int lowest_black, unsigned int highest_white,
 *                  unsigned int *ymin, unsigned int *black,
 *                  unsigned int *white, unsigned int *ymax,
 *                  unsigned int *slope0, unsigned int *slope1,
 *                  unsigned int *slope2, unsigned int *slope3)
 * description    : Auto-BWS Alg
 * parameters     :
 *                  width               <layer width>
 *                  height              <layer height>
 *                  hist[256]   <the latest frame histogram>
 *                  hist_pre[256] <the frame before the latest frame histogram>
 *                  sum                     <the latest frame pixel value sum>
 *                  pre_slope_black/pre_slope_white
 *                         <the frame before the latest frame auto-bws result>
 *                  ymin/black/white/ymax/shope0/1/2/3 <auto-bws result>
 * return         :
 *
 ******************************************************************************/
/*
 * R_ROPC_EN--frame_bld_en--1
 * R_ROPC_TH_UPPER--bld_high_thr--90
 * R_ROPC_TH_LOWER--bld_low_thr--74
 * R_ROPC_WEIGHT_MIN--bld_weight_lmt--8
 * R_PRESET_TILT_BLACK--present_black--53
 * R_PRESET_TILT_WHITE--present_white--235
 * R_SLOPE_LIMIT_BLACK--slope_black_lmt--512
 * R_SLOPE_LIMIT_WHITE--slope_white_lmt--384
 * R_BLACK_PERCENT--black_prec--5
 * R_WHITE_PERCENT--white_prec--2
 * R_LOWEST_BLACK--lowest_black--3
 * R_HIGHEST_WHITE--highest_white--252
 */
static void auto_bws_model(unsigned int width, unsigned int height,
			   unsigned int hist[256], unsigned int hist_pre[256],
			   unsigned int sum,
			   unsigned int pre_slope_black,
			   unsigned int pre_slope_white,
			   unsigned int frame_bld_en, unsigned int bld_high_thr,
			   unsigned int bld_low_thr,
			   unsigned int bld_weight_lmt,
			   unsigned int present_black,
			   unsigned int present_white,
			   unsigned int slope_black_lmt,
			   unsigned int slope_white_lmt,
			   unsigned int black_prec,
			   unsigned int white_prec,
			   unsigned int lowest_black,
			   unsigned int highest_white,
			   struct hist_data *p_hist_data)
{
	int total, k;
	int validcnt, validsum;
	int ratio_b, ratio_w, cdf_b, cdf_w;
	int mean;
	int pd_ymin = lowest_black, pd_ymax = highest_white;
	int pd_black, pd_white;
	int pd_ymin_fix, pd_ymax_fix;
	int pd_s0, pd_s1, pd_s2, pd_s3;
	int tmp;
	int i = 0;

	int coeff, diff_hist = 0;

	total = 0;
	for (k = 0; k < 256; k++) {
		diff_hist += abs(hist[k] - hist_pre[k]);
		total += hist[k];
	}
	if (total == 0) {
		if (diff_hist == 0)
			coeff = 0;
		else
			coeff = 200;
	} else
		coeff = (100 * diff_hist) / total;
	p_hist_data->diff_coeff = coeff;

	/* 2.kick out the lowest black and the highest white in hist and sum */
	validsum = sum;
	for (k = 0; k < lowest_black; k++) {
		if (validsum < hist[k] * k)
			break;

		validsum -= hist[k] * k;
	}
	for (k = 255; k > highest_white - 1; k--) {
		if (validsum < hist[k] * k)
			break;

		validsum -= hist[k] * k;
	}

	validcnt = total;
	for (k = 0; k < lowest_black; k++)
		validcnt -= hist[k];

	if (validcnt == 0)
		mean = lowest_black;
	else {
		for (k = 255; k > highest_white - 1; k--)
			validcnt -= hist[k];

		if (validcnt == 0)
			mean = highest_white;
		else
			mean = validsum / validcnt;
	}

	if (validcnt != 0) {
		/* 3.find Ymin and Ymax */
		ratio_b = validcnt * black_prec / 100;
		cdf_b = 0;
		for (k = lowest_black; k < 255; k++) {
			cdf_b += hist[k];
			if (cdf_b > ratio_b) {
				pd_ymin = k;
				break;
			}
		}

		ratio_w = validcnt * white_prec / 100;
		cdf_w = 0;
		for (k = highest_white; k >= 0; k--) {
			cdf_w += hist[k];
			if (cdf_w > ratio_w) {
				pd_ymax = k;
				break;
			}
		}

		/* bright */
		if (p_hist_data->hist_mean <= 16) {
			slope_black_lmt = slope_black_lmt;
		} else {
			int step = slope_black_lmt - 256;

			if (step < 0)
				step = 0;

			slope_black_lmt = slope_black_lmt
			    - (p_hist_data->hist_mean) * step / (5 * 16);

			if (slope_black_lmt < 256)
				slope_black_lmt = 256;
		}

		/* 4.limit black and white don't cross mean */
		pd_black = (present_black < mean * 3 / 2) ?
		    present_black : mean * 3 / 2;
		pd_white = (present_white > mean) ? present_white : mean;

		/* 5.calculate slope1/2
		 * and limit to slope_black_lmt or slope_white_lmt
		 */
		pd_s1 = (pd_ymin < pd_black) ?
		    ((pd_black << 8) / (pd_black - pd_ymin)) : 256;

		pd_s1 = (pd_s1 > slope_black_lmt) ? slope_black_lmt : pd_s1;

		pd_s2 = (pd_ymax > pd_white) ?
		    (((255 - pd_white) << 8) / (pd_ymax - pd_white)) : 256;
		pd_s2 = (pd_s2 > slope_white_lmt) ? slope_white_lmt : pd_s2;

		tmp = pd_black + ((pd_s1 * (pd_ymin - pd_black) + 128) >> 8);
		/* 7.calculate slope0/3 and re-calculate ymin and ymax */
		if ((tmp > 0) && (pd_ymin < pd_black) && (pd_ymin > 0)) {
			pd_s0 = ((tmp << 8) + 128) / pd_ymin;
			pd_ymin_fix = pd_ymin;
		} else if (pd_ymin >= pd_black)	{
			pd_s0 = 256;
			pd_ymin_fix = 16;
		  /* do noting use s0 */
		} else {
			pd_s0 = 0;
			pd_ymin_fix =
			    -((pd_black << 8) - 128) / pd_s1 + pd_black;
		}

		if (pd_s0 == 0)
			pd_s1 = 256 * pd_black / (pd_black - pd_ymin_fix);

		if (pd_s1 == 256)
			pd_s0 = 256;

		tmp = pd_white + ((pd_s2 * (pd_ymax - pd_white)) >> 8);
		if ((tmp < 255) && (pd_ymax > pd_white) && (pd_ymax < 255)) {
			pd_s3 = (((255 - tmp) << 8) + 128) / (255 - pd_ymax);
			pd_ymax_fix = pd_ymax;
		} else if (pd_ymax <= pd_white) {
			pd_s3 = 256;
			pd_ymax_fix = 255;
		  /* do noting use s3 */
		} else {
			pd_s3 = 0;
			pd_ymax_fix =
			    (((255 - pd_white) << 8) - 128) / pd_s2 + pd_white;
		}

		if (pd_s3 == 256)
			pd_s2 = 256;
	} else {
	  /* no enough pixel for auto bws */
		pd_ymin_fix = 16;
		pd_black = 32;
		pd_white = 224;
		pd_ymax_fix = 240;
		pd_s0 = 0x100;
		pd_s1 = 0x100;
		pd_s2 = 0x100;
		pd_s3 = 0x100;
	}

	if (mean < 0 || pd_ymin_fix < 0 || pd_black < 0 || pd_white > 255
	    || pd_white < 0 || pd_ymax_fix < 0 || pd_ymax_fix > 255) {
		mean = 0;
		pd_ymin_fix = 16;
		pd_black = 32;
		pd_white = 224;
		pd_ymax_fix = 240;
		pd_s0 = 0x100;
		pd_s1 = 0x100;
		pd_s2 = 0x100;
		pd_s3 = 0x100;
	}

	p_hist_data->old_hist_mean = p_hist_data->hist_mean;
	p_hist_data->hist_mean = mean;	/* add by zly 2014-8-19 16:12:14 */

	p_hist_data->avg_mean_saved[p_hist_data->avg_mean_idx] = mean;
	p_hist_data->avg_mean_idx++;
	if (p_hist_data->avg_mean_idx == AVG_NUM)
		p_hist_data->avg_mean_idx = 0;

	p_hist_data->avg_mean = 0;
	for (i = 0; i < AVG_NUM; i++)
		p_hist_data->avg_mean += p_hist_data->avg_mean_saved[i];

	p_hist_data->avg_mean = p_hist_data->avg_mean / AVG_NUM;

	p_hist_data->hist_mean_diff =
	    (int)p_hist_data->avg_mean - (int)p_hist_data->hist_mean;

	/* cal th */
	p_hist_data->black_thr0 = pd_ymin_fix;
	p_hist_data->black_thr1 = pd_black;
	p_hist_data->white_thr0 = pd_white;
	p_hist_data->white_thr1 = pd_ymax_fix;
	p_hist_data->black_slp0 = pd_s0;
	p_hist_data->black_slp1 = pd_s1;
	p_hist_data->white_slp0 = pd_s2;
	p_hist_data->white_slp1 = pd_s3;


}

int de_hist_tasklet(unsigned int sel, unsigned int chno, unsigned int frame_cnt)
{
	if ((g_hist_status[sel][chno]->isenable)
	    && ((HIST_FRAME_MASK == (frame_cnt % 2))
		|| (HIST_FRAME_MASK == 0x2))) {
		memcpy((unsigned char *)g_hist_p[sel][chno],
		       (unsigned char *)g_hist[sel][chno], 1024);
		de_fce_get_hist(sel, chno, g_hist[sel][chno],
				&g_sum[sel][chno]);

		if (g_hist_status[sel][chno]->runtime < 2)
			g_hist_status[sel][chno]->runtime++;
		else
			g_hist_status[sel][chno]->twohistready = 1;
	}
	return 0;

}

int de_ce_tasklet(unsigned int sel, unsigned int chno, unsigned int frame_cnt)
{
	unsigned int percent_black, percent_white;
	unsigned int lowest_black, highest_white, autoce_thr;

	percent_black = 5;
	percent_white = 4;
	lowest_black = 4;
	highest_white = 252;
	autoce_thr = 21;

	if (g_ce_status[sel][chno]->isenable
	    && g_ce_status[sel][chno]->b_automode
	    && ((CE_FRAME_MASK == (frame_cnt % 2)) || (CE_FRAME_MASK == 0x2))) {
		if (g_hist_status[sel][chno]->twohistready) {
			auto_bws_model(g_ce_status[sel][chno]->width,
				       g_ce_status[sel][chno]->height,
				       g_hist[sel][chno],
				       g_hist_p[sel][chno],
				       g_sum[sel][chno], 256, 256, 1, 90,
				       74, 8, 53, 235,
				       g_ce_status[sel][chno]->
				       slope_black_lmt,
				       g_ce_status[sel][chno]->
				       slope_white_lmt, percent_black,
				       percent_white, lowest_black,
				       highest_white,
				       hist_res[sel][chno]);

			auto_ce_model(g_ce_status[sel][chno]->width,
				      g_ce_status[sel][chno]->height,
				      g_sum[sel][chno],
				      g_hist[sel][chno],
				      g_ce_status[sel][chno]->
				      up_precent_thr,
				      g_ce_status[sel][chno]->
				      down_precent_thr,
				      g_ce_status[sel][chno]->
				      brightness,
				      g_ce_status[sel][chno]->bls_lvl,
				      g_ce_status[sel][chno]->wls_lvl,
				      g_ce_status[sel][chno]->
				      update_diff_thr, lowest_black,
				      highest_white, autoce_thr,
				      g_celut[sel][chno],
				      hist_res[sel][chno]);
		}
		de_fce_set_ce(sel, chno, g_celut[sel][chno]);
	}

	return 0;
}
#endif
