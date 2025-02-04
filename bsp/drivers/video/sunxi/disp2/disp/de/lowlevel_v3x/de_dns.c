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
 *  File name   :  display engine 3.0 dns basic function definition
 *
 *  History     :  2016-3-3 zzz  v0.1  Initial version
 ******************************************************************************/

#include "de_dns_type.h"
#include "de_rtmx.h"
#include "de_enhance.h"
#include "de_vep_table.h"

static struct __dns_reg_t *dns_dev[DE_NUM][CHN_NUM];
static struct __iqa_reg_t *iqa_dev[DE_NUM][CHN_NUM];

static struct de_reg_blocks dns_reg_block[DE_NUM][CHN_NUM];
static struct de_reg_blocks iqa_reg_block[DE_NUM][CHN_NUM];

static struct __dns_para_t dns_para[DE_NUM][CHN_NUM];

static unsigned int regs_read[18] = {0};

/* float AUTO_LV_R[11] = {0.2f, 0.4f, 0.7f, 0.8f, 0.9f, */
/* 				1.0f, 1.1f, 1.2f, 1.3f, 1.4f, 1.5f}; */
int AUTO_LV_R[11] = {2, 4, 7, 8, 9, 10, 11, 12, 13, 14, 15};

/*******************************************************************************
 * function       : de_dns_set_reg_base(unsigned int sel, unsigned int chno,
 *                  void *base, void *base1)
 * description    : set dns reg base
 * parameters     :
 *                  sel         <rtmx select>
 *                  chno        <overlay select>
 *                  base        <reg base>
 * return         :
 *                  success
 ******************************************************************************/
int de_dns_set_reg_base(unsigned int sel, unsigned int chno,
		void *base, void *base1)
{
	dns_dev[sel][chno] = (struct __dns_reg_t *) base;

	iqa_dev[sel][chno] = (struct __iqa_reg_t *) base1;

	return 0;
}

int de_dns_update_regs(unsigned int sel, unsigned int chno)
{
	if (dns_reg_block[sel][chno].dirty == 0x1) {
		memcpy((void *)dns_reg_block[sel][chno].off,
			     dns_reg_block[sel][chno].val,
			     dns_reg_block[sel][chno].size);
		dns_reg_block[sel][chno].dirty = 0x0;
	}

	if (iqa_reg_block[sel][chno].dirty == 0x1) {
		memcpy((void *)iqa_reg_block[sel][chno].off,
			     iqa_reg_block[sel][chno].val,
			     iqa_reg_block[sel][chno].size);
		iqa_reg_block[sel][chno].dirty = 0x0;
	}
	return 0;
}

int de_dns_init(unsigned int sel, unsigned int chno, uintptr_t reg_base)
{
	uintptr_t base;
	void *memory;
	void *memory1;

	base = reg_base + (sel + 1) * 0x00100000 + DNS_OFST;

	memory = kmalloc(sizeof(struct __dns_reg_t), GFP_KERNEL | __GFP_ZERO);
	if (memory == NULL) {
		__wrn("malloc dns[%d][%d] memory fail! size=0x%x\n", sel, chno,
			    (unsigned int)sizeof(struct __dns_reg_t));
		return -1;
	}

	dns_reg_block[sel][chno].off = base;
	dns_reg_block[sel][chno].val = memory;
	dns_reg_block[sel][chno].size = 0x20;
	dns_reg_block[sel][chno].dirty = 0;

	memory1 = kmalloc(sizeof(struct __iqa_reg_t), GFP_KERNEL | __GFP_ZERO);
	if (memory1 == NULL) {
		__wrn("malloc dns_iqa[%d][%d] memory fail! size=0x%x\n",
		sel, chno, (unsigned int)sizeof(struct __iqa_reg_t));
		return -1;
	}

	iqa_reg_block[sel][chno].off = base + 0x100;
	iqa_reg_block[sel][chno].val = memory1;
	iqa_reg_block[sel][chno].size = 0x48;
	iqa_reg_block[sel][chno].dirty = 0;

	de_dns_set_reg_base(sel, chno, memory, memory1);

	return 0;
}

int de_dns_double_init(unsigned int sel, unsigned int chno, uintptr_t reg_base)
{
	uintptr_t base;

	base = reg_base + (sel + 1) * 0x00100000 + DNS_OFST;

	de_dns_set_reg_base(sel, chno, (void *)base, (void *)(base + 0x100));

	return 0;
}

int de_dns_exit(unsigned int sel, unsigned int chno)
{
	kfree(dns_reg_block[sel][chno].val);
	kfree(iqa_reg_block[sel][chno].val);
	return 0;
}

int de_dns_double_exit(unsigned int sel, unsigned int chno)
{
	return 0;
}

/*******************************************************************************
 * function       : de_dns_enable(unsigned int sel, unsigned int chno,
 *                    unsigned int en)
 * description    : enable/disable dns
 * parameters     :
 *                  sel         <rtmx select>
 *                  chno        <overlay select>
 *                  en          <enable: 0-diable; 1-enable>
 * return         :
 *                  success
 ******************************************************************************/
int de_dns_enable(unsigned int sel, unsigned int chno, unsigned int en)
{
	de_set_bits(&dns_dev[sel][chno]->ctrl.dwval, en, 0, 1);
	dns_reg_block[sel][chno].dirty = 1;
	return 0;
}

/*******************************************************************************
 * function       : de_dns_set_size(unsigned int sel, unsigned int chno,
 *                   unsigned int width, unsigned int height)
 * description    : set dns size
 * parameters     :
 *                  sel         <rtmx select>
 *                  chno        <overlay select>
 *                  width       <input width>
 *                                      height  <input height>
 * return         :
 *                  success
 ******************************************************************************/
int de_dns_set_size(unsigned int sel, unsigned int chno, unsigned int width,
			  unsigned int height)
{
	dns_dev[sel][chno]->size.dwval = (height - 1) << 16 | (width - 1);
	dns_para[sel][chno].winsz_sel = (width > 2048) ? 1 : 0;
	de_set_bits(&dns_dev[sel][chno]->ctrl.dwval,
			dns_para[sel][chno].winsz_sel, 1, 1);
	dns_reg_block[sel][chno].dirty = 1;
	return 0;
}

/*******************************************************************************
 * function       : de_dns_set_window(unsigned int sel, unsigned int chno,
 *                     unsigned int win_enable, de_rect window)
 * description    : set dns window
 * parameters     :
 *                  sel         <rtmx select>
 *                  chno        <overlay select>
 *                  win_enable  <enable: 0-window mode diable;
 *                                       1-window mode enable>
 *                  window  <window rectangle>
 * return         :
 *                  success
 ******************************************************************************/
int de_dns_set_window(unsigned int sel, unsigned int chno,
			    unsigned int win_enable, struct de_rect window)
{
	/* save win en para */
	dns_para[sel][chno].win_en = win_enable;

	de_set_bits(&dns_dev[sel][chno]->ctrl.dwval, win_enable, 31, 1);
	if (win_enable) {
		dns_dev[sel][chno]->win0.dwval = window.y << 16 | window.x;
		dns_dev[sel][chno]->win1.dwval =
			  (window.y + window.h - 1) << 16 |
			  (window.x + window.w - 1);
	}
	dns_reg_block[sel][chno].dirty = 1;
	return 0;
}

/*******************************************************************************
 * function       : de_dns_init_para(unsigned int sel, unsigned int chno)
 * description    : set dns para
 * parameters     :
 *                  sel         <rtmx select>
 *                  chno        <overlay select>
 * return         :
 *                  success
 ******************************************************************************/
int de_dns_init_para(unsigned int sel, unsigned int chno)
{
	int SIG = 1;
	int SIG2 = 10;
	int SIG3 = 255;
	int DIR_RSIG_GAIN = 100;
	int DIR_THRLOW = 64;
	int DIR_THRHIGH = 192;
	int DIR_CEN = 128;
	int DIR_CEN_RAD = (DIR_THRHIGH - DIR_THRLOW + 1) / 2;
	int RSIG_CEN = SIG2 + DIR_RSIG_GAIN * (DIR_CEN - DIR_THRLOW) / 16;
	int DIR_RSIG_GAIN2;

	RSIG_CEN = min(SIG3, RSIG_CEN);
	DIR_RSIG_GAIN2 = (SIG3 - RSIG_CEN + DIR_CEN_RAD / 2) / DIR_CEN_RAD;

	dns_dev[sel][chno]->lpara0.dwval = DIR_RSIG_GAIN2 << 24 |
				SIG3 << 16 | SIG2 << 8 | SIG;
	dns_dev[sel][chno]->lpara1.dwval = DIR_THRHIGH << 24 |
				DIR_THRLOW << 16 |
				DIR_RSIG_GAIN << 8 |
				DIR_CEN;
	dns_dev[sel][chno]->lpara2.dwval = 0x77 << 24 |
				0 << 20 |
				0 << 16 |
				0 << 8 | 1;
	dns_dev[sel][chno]->lpara3.dwval = RSIG_CEN;
	dns_reg_block[sel][chno].dirty = 1;

	iqa_dev[sel][chno]->blk_dt_para.dwval = 0xc810;
	iqa_reg_block[sel][chno].dirty = 1;
	return 0;
}

/*******************************************************************************
 * function       : de_dns_read_iqa_regs(unsigned int sel, unsigned int chno,
 *                  unsigned int regs[18])
 * description    : get histogram result
 * parameters     :
 *                  sel         <rtmx select>
 *                  chno        <overlay select>
 *                  regs[18]   <regs to read>
 * return         :
 *                  success
 ******************************************************************************/
int de_dns_read_iqa_regs(unsigned int sel, unsigned int chno,
			   unsigned int regs[18])
{
	/* Read regs */
	memcpy((unsigned char *)regs,
		     (unsigned char *)iqa_reg_block[sel][chno].off,
		     iqa_reg_block[sel][chno].size);

	return 0;
}

/*******************************************************************************
 * function       : de_dns_info2para(unsigned int sel, unsigned int chno,
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
int de_dns_info2para(unsigned int sel, unsigned int chno,
			   unsigned int fmt, unsigned int dev_type,
			   struct __dns_config_data *para,
			   unsigned int bypass)
{
	unsigned int en;
	/* parameters */
	en = ((para->level == 0) || (bypass == 1)) ? 0 : 1;
	para->mod_en = en; /* return enable info */

	if (en == 0 || fmt == 1 || para->inw > 4096 || para->inw < 32) {
		/*
		 * if level=0, module will be disabled,
		 * no need to set para register
		 * if rgb fmt, dont support
		 */
		dns_para[sel][chno].en = 0;
	} else {
		/* save info, and apply in tasklet */
		dns_para[sel][chno].en = 1;
		dns_para[sel][chno].autolvl = para->level;
		dns_para[sel][chno].xst = para->croprect.x & 0x7;
		dns_para[sel][chno].yst = para->croprect.y & 0x7;
		dns_para[sel][chno].w = para->inw;
		dns_para[sel][chno].h = para->inh;
	}
	de_set_bits(&dns_dev[sel][chno]->ctrl.dwval,
			dns_para[sel][chno].en, 0, 1);
	dns_reg_block[sel][chno].dirty = 1;

	return 0;
}

int de_dns_para_apply(unsigned int screen_id, unsigned int chno,
		struct __dns_para_t *para)
{
	struct __dns_reg_t *dev = dns_dev[screen_id][chno];

	dev->lpara0.dwval = para->dir_rsig_gain2 << 24 |
			para->sig3 << 16 |
			para->sig2 << 8 | para->sig;
	dev->lpara1.dwval = para->dir_thrhigh << 24 |
			para->dir_thrlow << 16 |
			para->dir_rsig_gain << 8 |
			para->dir_center;
	dev->lpara2.dwval = 0x77 << 24 |
			para->xst << 20 |
			para->yst << 16 |
			para->bgain << 8 | 1;
	dev->lpara3.dwval = para->rsig_cen;

	dns_reg_block[screen_id][chno].dirty = 1;
	return 0;
}

int cal_blks(int w, int h, int DB_XST, int DB_YST)
{
	int blk_num;
	int yres = (h + DB_YST - 8) % 8;
	int yend = (8 - DB_YST) + ((h + DB_YST - 8) / 8) * 8;
	int xres = (w + DB_XST - 8) % 8;
	int xend = (8 - DB_XST) + ((w + DB_XST - 8) / 8) * 8;

	if (yres == 0)
		yend = yend - 8;

	if (xres == 0)
		xend = xend - 8;

	blk_num = ((yend - DB_YST) / 8)*((xend - DB_XST) / 8);

	return blk_num;
}

void dns_para_adapt(struct __dns_para_t *para, int regs[18])
{
	int auto_lvl = para->autolvl;
	int auto_level_ratio;
	int sum, avg;
	int blkm, blkn, blkr, blks;
	int SIG, SIG2, SIG3;
	int DIR_RSIG_GAIN, DIR_RSIG_GAIN2, DIR_CEN;
	int DIR_THRLOW;
	int RSIG_CEN;
	int BGAIN;
	int w, h, num;
	int dir_cen_rad;

	w = para->w;
	h = para->h;

	if (w >= 32 && h >= 32) {
		para->dir_center = 128;
		para->dir_rsig_gain = 100;
		para->sig3 = 255;
		para->dir_thrlow = 64;
		para->dir_thrhigh = 192;
		dir_cen_rad = (para->dir_thrhigh - para->dir_thrlow + 1) / 2;
		SIG3 = para->sig3;

		num = w * h;
		sum = regs[1];
		avg = sum / num;

		blks = regs[16];
		blkn = regs[17];
		blkn = (blkn == 0) ? 1 : blkn;
		blkm = blks * 100 / blkn;
		blkr = blkn * 100 / cal_blks(w, h, para->xst, para->yst);

		/* set default para for different size */
		if (abs(w - 1280) < 192 ||
			 abs(w - 1920) < 288 ||
			 abs(w - 3840) < 576) {
			SIG = 0;
			SIG2 = 25; /* 10 */
		}	else if ((w == 720 || w == 704 || w == 640) &&
			(h == 576 || h == 480)) {
			SIG = 1;
			SIG2 = 75; /* 10 */
		} else if (w >= 480) {
			SIG = 1;
			SIG2 = 30;
		}	else {
			SIG = 2;
			SIG2 = 40;
		}

		DIR_THRLOW = para->dir_thrlow;
		DIR_RSIG_GAIN = para->dir_rsig_gain;
		DIR_CEN = para->dir_center;
		RSIG_CEN = SIG2 + DIR_RSIG_GAIN * (DIR_CEN - DIR_THRLOW) / 16;
		RSIG_CEN = min(SIG3, RSIG_CEN);
		DIR_RSIG_GAIN2 = (SIG3 - RSIG_CEN + dir_cen_rad / 2) /
				dir_cen_rad;

		if (avg < 35) {
			SIG2 = max(0, min(255, SIG2 * 3 / 2));
			DIR_RSIG_GAIN += 20;
			DIR_CEN = max(0, min(255, DIR_CEN - 32));
		} else if (avg >= 35 && avg < 65) {
			/* remain */
		} else if (avg >= 65 && avg <= 140) {
			if (w >= (1280)) {
				SIG = max(0, SIG - 1);
				SIG2 = max(0, min(255, SIG2 * 2 / 3));
			} else {
				SIG2 = max(0, min(255, SIG2 * 3 / 4));
			}

			DIR_RSIG_GAIN  = max(0, DIR_RSIG_GAIN - 20);
			DIR_CEN  = max(0, min(255, DIR_CEN + 32));
		}

		if (w <= 2048) {
			if (blkm >= 20)
				DIR_RSIG_GAIN = min(255,
					DIR_RSIG_GAIN * (blkm - 20) / 20);
			if (blkr >= 5) {
				SIG2 = SIG2 + (blkr - 5) * 8;
				SIG2 = max(15, min(30, SIG2));
				if (blkm < 30)
					SIG2 = SIG2 * (blkm + 15) / 30;
				else if (blkm < 60)
					SIG2 = SIG2 * (blkm + 15) / 30;
				else
					SIG2 = SIG2 * 2;
				SIG2 = min(100, SIG2);
			}
#if 0
			if (blkr < 5)
				BGAIN = 10 + 5 * (blkr + 1) / 2;
			else if (blkr < 10)
				BGAIN = 22 + 9 * (blkr - 5) / 2;
			else if (blkr < 15)
				BGAIN = 43 + 3 * (blkr - 10);
			else if (blkr < 30)
				BGAIN = 58 + 6 * (blkr - 15) / 5;
			else if (blkr < 60)
				BGAIN = 76 + 1 * (blkr - 30) / 3;
			else
				BGAIN = 85;
#else
			if (blkr < 5)
				BGAIN = 10 + (16 - 10) * blkr / (10 - 5);
			else if (blkr < 10)
				BGAIN = 16 +
					(32 - 16) * (blkr - 10) / (15 - 10);
			else if (blkr < 15)
				BGAIN = 32 +
					(36 - 32) * (blkr - 15) / (30 - 15);
			else if (blkr < 30)
				BGAIN = 36 +
					(40 - 36) * (blkr - 30) / (60 - 30);
			else if (blkr < 60)
				BGAIN = 40;
			else
				BGAIN = 50;
#endif
		} else {
			BGAIN = 0x10;
			/* default low gain in 4k size : 0x14 is original */
		}
		/* for level control */
		auto_lvl = max(0, min(auto_lvl, 10));
		auto_level_ratio = AUTO_LV_R[auto_lvl];

		SIG2 = min(255, (5 + SIG2 * auto_level_ratio)/10);
		BGAIN = min(127, (5 + BGAIN * auto_level_ratio)/10);
		DIR_RSIG_GAIN = min(255,
			(5 + DIR_RSIG_GAIN * auto_level_ratio)/10);
		para->bgain = BGAIN;
		para->sig = SIG;
		para->sig2 = SIG2;
		para->dir_rsig_gain = DIR_RSIG_GAIN;
		para->dir_rsig_gain2 = DIR_RSIG_GAIN2;
		para->rsig_cen = RSIG_CEN;

		__inf("dns set sig/sig2/bgain: %d, %d, %d\n", SIG, SIG2, BGAIN);

		if (SIG2 == 0) {
			SIG2 = 1;
			__wrn("%s: SIG2==0!\n", __func__);
		}

	}
}

int de_dns_tasklet(unsigned int screen_id, unsigned int chno,
			unsigned int frame_cnt)
{
	struct __dns_para_t *para = &dns_para[screen_id][chno];

	if (para->en) {
		__inf("dns tasklet start\n");
		de_dns_read_iqa_regs(screen_id, chno, regs_read);
		dns_para_adapt(para, regs_read);
		de_dns_para_apply(screen_id, chno, para);
		__inf("dns tasklet end\n");
	}
	return 0;
}
