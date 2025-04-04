/*
 *  All Winner Tech, All Right Reserved. 2006-2016 Copyright (c)
 *
 *  File name   :        di_v23.c
 *
 *  Description :
 *
 *  History     :2016/01/18        zhengwj        initial version for DI_V2.0
 *               2016/10/09        zhengwj        modify for DI_V2.3
 *               2016/12/27        yulangheng     code of DI_V2.2 added
 *
 *  This file is licensed under the terms of the GNU General Public
 *  License version 2.  This program is licensed "as is" without any
 *  warranty of any kind, whether express or implied.
 */

#include "../di.h"
#include "../sunxi-di.h"
#include "di_type_v2x.h"
#include <linux/slab.h>
#include <asm/io.h>

volatile struct __di_dev_t *di_dev;
#if defined SUPPORT_DNS
volatile struct __dns_dev_t *dns_dev;
#endif

static struct __di_mode_t g_di_mode;

#define SETMASK(width, shift) ((width ? ((-1U) >> (32-width)) : 0)  << (shift))
#define CLRMASK(width, shift)   (~(SETMASK(width, shift)))
#define SET_BITS(shift, width, reg, val) \
		(((reg) & CLRMASK(width, shift)) | (val << (shift)))
/*
 * function: di_set_reg_base(void *base)
 * description: set di module register base
 * parameters: base <di module AHB memory mapping >
 * return   :
 */
s32 di_set_reg_base(void *base)
{
	di_dev = (struct __di_dev_t *)base;

#if defined SUPPORT_DNS
	dns_dev = (struct __dns_dev_t *)(base + 0x10000);
#endif

	return 0;
}

/*
 * function: di_get_reg_base(void)
 * description: get di module register base
 * parameters:
 * return   : di module AHB memory mapping
 */
void *di_get_reg_base(void)
{
	void *ret = NULL;

	ret = (void *)(di_dev);

	return ret;
}

/*
 * function: di_set_init(void)
 * description: set di module default register to ready de-interlace
 * parameters:
 * return   :
 */
#if defined DI_V23
#define IOMMU_ENABLE 1 /* DI_V2.3 : Enable IOMMU function */
#endif

s32 di_set_init(void)
{
#if defined DI_V23
	/* di_dev->output_path.bits.output_path = 0; */
	di_dev->outpath.dwval = 0x00000000;

	/* default setting of di */

	/* di_dev->ctrl.bits.dma_rand_access_en = 1 (iommu_enable) */
	di_dev->ctrl.dwval = 0x00000000 | (IOMMU_ENABLE << 16);

	/* int di_mode = 1;
	 * int motion_detc_en = 1;
	 * int diag_intp_en = 1;
	 * int flag_update_mode = 0;
	 * di_dev->mode.bits.di_mode_luma = di_mode;
	 * di_dev->mode.bits.di_mode_chroma = di_mode;
	 * di_dev->mode.bits.motion_detc_en = motion_detc_en;
	 * di_dev->mode.bits.diag_intp_en = diag_intp_en;
	 * di_dev->mode.bits.flag_update_mode = flag_update_mode;
	 * di_dev->mode.bits.in_field_mode =
	 *         (di_mode == DI_MODE_MOTION)? 0: 1;
	 */
	di_dev->mode.dwval = 0x00010031;

	/* di_dev->mdpara0.bits.minlumath = 0x4;
	 * di_dev->mdpara0.bits.maxlumath = 0xc;
	 * di_dev->mdpara0.bits.avglumashifter = 0x6;
	 * di_dev->mdpara0.bits.th_shift = 0x1;
	 */
	di_dev->mdpara0.dwval = 0x01060c04;

	/* di_dev->mdpara1.bits.mov_fac_nonedge = 0x2;
	 */
	di_dev->mdpara1.dwval = 0x20000000;

	/* di_dev->mdpara2.bits.chroma_spatical_th = 0x80;
	 * di_dev->mdpara2.bits.chroma_diff_th = 0x5;
	 * di_dev->mdpara2.bits.pix_static_th = 0x3;
	 */
	di_dev->mdpara2.dwval = 0x30058000;

	/* di_dev->dipara.bits.angle_limit = 0x14;
	 * di_dev->dipara.bits.angle_const_th = 0x5;
	 * di_dev->dipara.bits.luma_cur_fac_mod = 0x1;
	 * di_dev->dipara.bits.chroma_cur_fac_mod = 0x1;
	 */
	di_dev->dipara0.dwval = 0x00110514;

	/* di_dev->mdchpara.bits.blend_mode = 0x1;
	 * di_dev->mdchpara.bits.font_pro_en = 0x1;
	 * di_dev->mdchpara.bits.font_pro_th = 0x30;
	 * di_dev->mdchpara.bits.font_pro_fac = 0x4;
	 */
	di_dev->mdchpara.dwval = 0x04300101;

	/* di_dev->dipara1.bits.a = 0x4;
	 * di_dev->dipara1.bits.en = 0x1;
	 * di_dev->dipara1.bits.c = 0xa;
	 * di_dev->dipara1.bits.cmax = 0x40;
	 * di_dev->dipara1.bits.maxrat = 0x2
	 */
	di_dev->dipara1.dwval = 0x000240ac;
#elif defined DI_V22
	/* di_dev->output_path.bits.output_path = 0; */
	di_dev->outpath.dwval = 0x00000000;

	/* default setting of di */
	/* di_dev->ctrl.bits.dma_rand_access_en = 1 (iommu_enable) */
	di_dev->ctrl.dwval = 0x00000000;

	/* int di_mode = 1;
	 * int motion_detc_en = 1;
	 * int diag_intp_en = 1;
	 * int flag_update_mode = 0;
	 * di_dev->mode.bits.di_mode_luma = di_mode;
	 * di_dev->mode.bits.di_mode_chroma = di_mode;
	 * di_dev->mode.bits.motion_detc_en = motion_detc_en;
	 * di_dev->mode.bits.diag_intp_en = diag_intp_en;
	 * di_dev->mode.bits.flag_update_mode = flag_update_mode;
	 * di_dev->mode.bits.in_field_mode =
	 *         (di_mode == DI_MODE_MOTION)? 0: 1;
	 */
	di_dev->mode.dwval = 0x00010031;

	/* di_dev->mdpara0.bits.minlumath = 0x4;
	 * di_dev->mdpara0.bits.max_luma_th= 0x18;
	 * di_dev->mdpara0.bits.avglumashifter = 0x6;
	 * di_dev->mdpara0.bits.th_shift = 0x0;
	 */
	/* di_dev->mdpara0.dwval = 0x01060c04; */
	di_dev->mdpara0.dwval = 0x00061804;

	/* di_dev->f_prot_th = 0x80
	 * di_dev->f_prot_factor = 0x4
	 * di_dev->edge_th = 0x20
	 * di_dev->mov_fac_edge = 0x03
	 * di_dev->mov_fac_nonedge = 0x01
	 */
	di_dev->mdpara1.dwval = 0x13200480;

	/* di_dev->luma_spatial_th = 0x5
	 * di_dev->chroma_spatical_th = 0x80
	 * di_dev->chroma_diff_th = 0x5
	 * di_dev->erosion_bob_th = 0x7
	 * di_dev->pix_static_th = 0x3
	 */
	di_dev->mdpara2.dwval = 0x37058005;

	/* di_dev->dipara.bits.angle_limit = 0x14;
	 * di_dev->dipara.bits.angle_const_th = 0x5;
	 * di_dev->dipara.bits.luma_cur_fac_mod = 0x1;
	 * di_dev->dipara.bits.chroma_cur_fac_mod = 0x1;
	 */
	di_dev->dipara0.dwval = 0x00110514;

	/* di_dev->mdchpara.bits.blend_mode = 0x0;
	 * di_dev->mdchpara.bits.font_pro_en = 0x0;
	 * di_dev->mdchpara.bits.font_pro_th = 0x30;
	 * di_dev->mdchpara.bits.font_pro_fac = 0x4;
	 */
	di_dev->mdchpara.dwval = 0x04300000;
#endif

#if defined SUPPORT_DNS
	dns_dev->dns_ctl.dwval = 0x00000001; /* Enable Denoising */
	dns_dev->dns_lft_para0.dwval = 0x00ff9601;
	dns_dev->dns_lft_para1.dwval = 0xb24e3c80;
	dns_dev->dns_lft_para2.dwval = 0x77153a00;
	dns_dev->dns_lft_para3.dwval = 0x000000ff;
	dns_dev->iqa_blkdt_para0.dwval = 0x0000c810;

#endif

	g_di_mode.di_mode = DI_MODE_MOTION;
	g_di_mode.update_mode = DI_UPDMODE_FIELD;

	return 0;
}

#define di_writel(val, addr) writel(val, (void __iomem *)(addr))
#define di_readl(addr) readl((void __iomem *)(addr))

void DI_SET_BITS(unsigned int *reg_addr, unsigned int bits_val,
			unsigned int shift, unsigned int width)
{
	unsigned int reg_val;

	reg_val = di_readl(reg_addr);
	reg_val = SET_BITS(shift, width, reg_val, bits_val);
	di_writel(reg_val, reg_addr);
}

/*
 * function: di_reset(void)
 * description: stop di module
 * parameters:
 * return   :
 */
s32 di_reset(void)
{
	unsigned int reset = 0x1;

	di_dev->ctrl.bits.reset = reset;
	/* DI_SET_BITS(&di_dev->ctrl.dwval, reset, 0, 1); */
	while (reset < 20)
		reset++;
	reset = 0;
	di_dev->ctrl.bits.reset = reset;
	/* DI_SET_BITS(&di_dev->ctrl.dwval, reset, 0, 1); */
	return 0;
}

/*
 * function: di_start(void)
 * description: start a de-interlace function
 * parameters:
 * return   :
 */
s32 di_start(void)
{
	di_dev->ctrl.bits.start = 0x1;

	return 0;
}

/*
 * function: di_irq_enable(unsigned int enable)
 * description: enable/disable di irq
 * parameters: enable <0-disable; 1-enable>
 * return   :
 */
s32 di_irq_enable(unsigned int enable)
{
	di_dev->intr.bits.int_en = (enable & 0x1);

	return 0;
}

/*
 * function: di_get_status(void)
 * description: get status of di module
 * parameters:
 * return  :  <0-Writeback finish; 1-Writeback no start; 2-Writeback-ing;
 *            (-1)-Undefined
 */
s32 di_get_status(void)
{
	int ret;
	unsigned int busy;
	unsigned int finish;

	finish = di_dev->status.bits.finish_sts;
	busy = di_dev->status.bits.busy;

	if (busy)
		ret = 2;
	else if (finish == 0 && busy == 0)
		ret = 1;
	else if (finish)
		ret = 0;
	else
		ret = -1;

	return ret;
}

/*
 * function: di_irq_clear()
 * description: clear irq status
 * parameters:
 * return   :
 */
s32 di_irq_clear(void)
{
	di_dev->status.bits.finish_sts = 0x1;

	return 0;
}

void DI_SET_FORMAT(unsigned char fmt)
{
	di_dev->fmt.bits.fmt = fmt;
}

void DI_SET_SIZE(unsigned int width, unsigned int height)
{
	di_dev->size.dwval = ((height - 1) & 0x7ff) << 16 |
			     ((width - 1) & 0x7ff);
#if defined SUPPORT_DNS
	dns_dev->dns_size.dwval = ((height - 1) & 0x7ff) << 16 |
			     ((width - 1) & 0x7ff);
#endif

}

void DI_CALC_INADDR(struct __di_buf_addr_t *pre_addr,
		    struct __di_buf_addr_t *cur_addr,
		    struct __di_buf_addr_t *nxt_addr,
		    struct __di_buf_size_t *size, unsigned int field,
		    unsigned char fmt, unsigned int top_field_first,
		    unsigned long long addr[4][3], unsigned int fieldpitch[3])
{
	unsigned int pitch[3];

	pitch[0] = size->fb_width;

	if (fmt == DI_FMT_PLANAR420 || fmt == DI_FMT_PLANAR422) {
		pitch[1] = ((size->fb_width + 1) >> 1);
		pitch[2] = pitch[1];
	} else {
		pitch[1] = (((size->fb_width + 1) >> 1) << 1);
		pitch[2] = 0;
	}

	fieldpitch[0] = pitch[0] << 1;
	fieldpitch[1] = pitch[1] << 1;
	fieldpitch[2] = pitch[2] << 1;

	if (top_field_first == 1) {
		if (field == 0) {
			addr[0][0] = pre_addr->ch0_addr;
			addr[0][1] = pre_addr->ch1_addr;
			addr[0][2] = pre_addr->ch2_addr;
			addr[1][0] = pre_addr->ch0_addr + pitch[0];
			addr[1][1] = pre_addr->ch1_addr + pitch[1];
			addr[1][2] = pre_addr->ch2_addr + pitch[2];
			addr[2][0] = cur_addr->ch0_addr;
			addr[2][1] = cur_addr->ch1_addr;
			addr[2][2] = cur_addr->ch2_addr;
			addr[3][0] = cur_addr->ch0_addr + pitch[0];
			addr[3][1] = cur_addr->ch1_addr + pitch[1];
			addr[3][2] = cur_addr->ch2_addr + pitch[2];
		} else {
			addr[0][0] = pre_addr->ch0_addr + pitch[0];
			addr[0][1] = pre_addr->ch1_addr + pitch[1];
			addr[0][2] = pre_addr->ch2_addr + pitch[2];
			addr[1][0] = cur_addr->ch0_addr;
			addr[1][1] = cur_addr->ch1_addr;
			addr[1][2] = cur_addr->ch2_addr;
			addr[2][0] = cur_addr->ch0_addr + pitch[0];
			addr[2][1] = cur_addr->ch1_addr + pitch[1];
			addr[2][2] = cur_addr->ch2_addr + pitch[2];
			addr[3][0] = nxt_addr->ch0_addr;
			addr[3][1] = nxt_addr->ch1_addr;
			addr[3][2] = nxt_addr->ch2_addr;
		}
	} else {
		if (field == 0) {
			addr[0][0] = pre_addr->ch0_addr;
			addr[0][1] = pre_addr->ch1_addr;
			addr[0][2] = pre_addr->ch2_addr;
			addr[1][0] = cur_addr->ch0_addr + pitch[0];
			addr[1][1] = cur_addr->ch1_addr + pitch[1];
			addr[1][2] = cur_addr->ch2_addr + pitch[2];
			addr[2][0] = cur_addr->ch0_addr;
			addr[2][1] = cur_addr->ch1_addr;
			addr[2][2] = cur_addr->ch2_addr;
			addr[3][0] = nxt_addr->ch0_addr + pitch[0];
			addr[3][1] = nxt_addr->ch1_addr + pitch[1];
			addr[3][2] = nxt_addr->ch2_addr + pitch[2];
		} else {
			addr[0][0] = pre_addr->ch0_addr + pitch[0];
			addr[0][1] = pre_addr->ch1_addr + pitch[1];
			addr[0][2] = pre_addr->ch2_addr + pitch[2];
			addr[1][0] = pre_addr->ch0_addr;
			addr[1][1] = pre_addr->ch1_addr;
			addr[1][2] = pre_addr->ch2_addr;
			addr[2][0] = cur_addr->ch0_addr + pitch[0];
			addr[2][1] = cur_addr->ch1_addr + pitch[1];
			addr[2][2] = cur_addr->ch2_addr + pitch[2];
			addr[3][0] = cur_addr->ch0_addr;
			addr[3][1] = cur_addr->ch1_addr;
			addr[3][2] = cur_addr->ch2_addr;
		}
	}
}

void DI_SET_INPUT(unsigned int pitch[3], unsigned long long addr[4][3])
{
	di_dev->inpicth0.dwval = pitch[0] & 0xffff;
	di_dev->inpicth1.dwval = pitch[1] & 0xffff;
	di_dev->inpicth2.dwval = pitch[2] & 0xffff;

	di_dev->in0add0.dwval = (unsigned int)(addr[0][0] & 0xffffffffLL);
	di_dev->in0add1.dwval = (unsigned int)(addr[0][1] & 0xffffffffLL);
	di_dev->in0add2.dwval = (unsigned int)(addr[0][2] & 0xffffffffLL);
	di_dev->in0addhb.dwval =
		((unsigned int)((addr[0][0] & 0xff00000000LL)>>32) |
		 (unsigned int)((addr[0][1] & 0xff00000000LL)>>24) |
		 (unsigned int)((addr[0][2] & 0xff00000000LL)>>16));

	di_dev->in1add0.dwval = (unsigned int)(addr[1][0] & 0xffffffffLL);
	di_dev->in1add1.dwval = (unsigned int)(addr[1][1] & 0xffffffffLL);
	di_dev->in1add2.dwval = (unsigned int)(addr[1][2] & 0xffffffffLL);
	di_dev->in1addhb.dwval =
		((unsigned int)((addr[1][0] & 0xff00000000LL)>>32) |
		 (unsigned int)((addr[1][1] & 0xff00000000LL)>>24) |
		 (unsigned int)((addr[1][2] & 0xff00000000LL)>>16));

	di_dev->in2add0.dwval = (unsigned int)(addr[2][0] & 0xffffffffLL);
	di_dev->in2add1.dwval = (unsigned int)(addr[2][1] & 0xffffffffLL);
	di_dev->in2add2.dwval = (unsigned int)(addr[2][2] & 0xffffffffLL);
	di_dev->in2addhb.dwval =
		((unsigned int)((addr[2][0] & 0xff00000000LL)>>32) |
		 (unsigned int)((addr[2][1] & 0xff00000000LL)>>24) |
		 (unsigned int)((addr[2][2] & 0xff00000000LL)>>16));

	di_dev->in3add0.dwval = (unsigned int)(addr[3][0] & 0xffffffffLL);
	di_dev->in3add1.dwval = (unsigned int)(addr[3][1] & 0xffffffffLL);
	di_dev->in3add2.dwval = (unsigned int)(addr[3][2] & 0xffffffffLL);
	di_dev->in3addhb.dwval =
		((unsigned int)((addr[3][0] & 0xff00000000LL)>>32) |
		 (unsigned int)((addr[3][1] & 0xff00000000LL)>>24) |
		 (unsigned int)((addr[3][2] & 0xff00000000LL)>>16));

}

void DI_SET_OUTPUT(struct __di_buf_size_t *size, unsigned char fmt,
			unsigned long long addr[3])
{
	unsigned int pitch[3];

	pitch[0] = size->fb_width;

	if (fmt == DI_FMT_PLANAR420 || fmt == DI_FMT_PLANAR422) {
		pitch[1] = ((size->fb_width + 1) >> 1);
		pitch[2] = pitch[1];
	} else {
		pitch[1] = (((size->fb_width + 1) >> 1) << 1);
		pitch[2] = 0;
	}

	di_dev->outpicth0.dwval = pitch[0] & 0xffff;
	di_dev->outpicth1.dwval = pitch[1] & 0xffff;
	di_dev->outpicth2.dwval = pitch[2] & 0xffff;

	di_dev->outadd0.dwval = (unsigned int)(addr[0] & 0xffffffffLL);
	di_dev->outadd1.dwval = (unsigned int)(addr[1] & 0xffffffffLL);
	di_dev->outadd2.dwval = (unsigned int)(addr[2] & 0xffffffffLL);
	di_dev->outaddhb.dwval =
		((unsigned int)((addr[0] & 0xff00000000LL)>>32) |
		 (unsigned int)((addr[1] & 0xff00000000LL)>>24) |
		 (unsigned int)((addr[2] & 0xff00000000LL)>>16));

}

void DI_SET_FLAG(unsigned long long in_flag_add,
		 unsigned long long out_flag_add, unsigned int pitch)
{
	di_dev->flagpitch.dwval = pitch & 0xffff;

	di_dev->inflagadd.dwval = (unsigned int)(in_flag_add & 0xffffffffLL);
	di_dev->outflagadd.dwval = (unsigned int)(out_flag_add & 0xffffffffLL);
	di_dev->flagaddhb.dwval =
		((unsigned int)((in_flag_add & 0xff00000000LL)>>32) |
		 (unsigned int)((out_flag_add & 0xff00000000LL)>>24));

}

void DI_SET_CURFAC(unsigned int curfac)
{
	di_dev->dipara0.bits.luma_cur_fac_mod = curfac;
	di_dev->dipara0.bits.chroma_cur_fac_mod = curfac;
}

void DI_SET_POLAR(unsigned int polar)
{
	di_dev->polar.dwval = polar & 0x1;
}

void DI_SET_FLAG_AUTO_UPD_MODE(unsigned int mode)
{
#if defined DI_V23
	di_dev->mode.bits.flag_auto_update_mode = mode;
#endif
}

int DI_SW_PARA_TO_REG(unsigned char format)
{
	/* deinterlace input pixel format */
	if (format <= DI_FORMAT_NV21)
		return DI_FMT_UVCOMB420;
	else if (format == DI_FORMAT_YV12)
		return DI_FMT_PLANAR420;
	else if (format == DI_FORMAT_YUV422_SP_UVUV ||
		   format == DI_FORMAT_YUV422_SP_VUVU)
		return DI_FMT_UVCOMB422;
	else if (format == DI_FORMAT_YUV422P)
		return DI_FMT_PLANAR422;
	/* DE_INF("not supported de-interlace input pixel format :%d
	 *		in di_sw_para_to_reg\n",format);
	 */
	return -1;
}

/*
 * function: di_set_para(__di_para_t *para)
 * description: set parameters to ready a de-interlace function
 * parameters: para <parameters which set from ioctrl>
 *             in_flag_add/out_flag_add <flag address malloc in driver>
 *             polar <0 - select even line for source line;
 *                    1 - select odd line for source line>
 *             di_mode <di mode set by app>
 * return  :   <0 - set OK; -1 - para NULL>
 */
s32 di_set_para(struct __di_para_t2 *para, void *in_flag_add,
		void *out_flag_add, u32 field)
{
	int fmt;
	unsigned int width, height;
	unsigned long long inaddr[4][3], outaddr[3];
	unsigned int inpitch[3];
	/* FIXME: flagpitch can be changed according to source */
	unsigned int flagpitch = 0x200;
	unsigned int curfac = 0;
	unsigned int faum;

	struct __di_buf_addr_t pre_addr;
	struct __di_buf_addr_t cur_addr;
	struct __di_buf_addr_t nxt_addr;
	/* struct __di_buf_addr_t out_addr; */
	/* struct __di_buf_addr_t flg_addr; */
	struct __di_buf_size_t in_size;
	struct __di_buf_size_t out_size;

	if (para == NULL) {
		/* DE_WRN("input parameter can't be null!\n"); */
		return -1;
	}

	/* FORMAT and SIZE */
	fmt = DI_SW_PARA_TO_REG(para->input_fb.format);

	if (fmt < 0) {
		/* DE_WRN("para->input_fb.format=%d isnot supported!\n",
		 * para->input_fb.format);
		 */
		return -1;
	}

	width = para->source_regn.width;
	height = para->source_regn.height;
	DI_SET_FORMAT(fmt);
	DI_SET_SIZE(width, height);

	/* INPUT FRAMEBUFFER ADDRESS and PITCH */
	pre_addr.ch0_addr = para->pre_fb.addr[0];
	pre_addr.ch1_addr = para->pre_fb.addr[1];
	pre_addr.ch2_addr = para->pre_fb.addr[2];

	cur_addr.ch0_addr = para->input_fb.addr[0];
	cur_addr.ch1_addr = para->input_fb.addr[1];
	cur_addr.ch2_addr = para->input_fb.addr[2];

	nxt_addr.ch0_addr = para->next_fb.addr[0];
	nxt_addr.ch1_addr = para->next_fb.addr[1];
	nxt_addr.ch2_addr = para->next_fb.addr[2];

	/* in_size.width = para->source_regn.width; */
	/* in_size.height = para->source_regn.height; */
	in_size.fb_width = para->input_fb.size.width;
	in_size.fb_height = para->input_fb.size.height;

	DI_CALC_INADDR(&pre_addr, &cur_addr, &nxt_addr, &in_size, field, fmt,
			para->top_field_first, inaddr, inpitch);
	DI_SET_INPUT(inpitch, inaddr);

	/* OUTPUT FRAMEBUFFER ADDRESS and PITCH */
	outaddr[0] = para->output_fb.addr[0];
	outaddr[1] = para->output_fb.addr[1];
	outaddr[2] = para->output_fb.addr[2];

	/* out_size.width = para->out_regn.width; */
	/* out_size.height = para->out_regn.height; */
	out_size.fb_width = para->output_fb.size.width;
	out_size.fb_height = para->output_fb.size.height;

	DI_SET_OUTPUT(&out_size, fmt, outaddr);

	/* MOTION INFORMATION ADDRESS and PITCH */
	DI_SET_FLAG((unsigned long)in_flag_add,
			(unsigned long)out_flag_add, flagpitch);

	/* DI PARAMERTES */
#if defined DI_V23
	/* DI_V2.3 */
	curfac = (para->top_field_first == 0) ? 5 : 4;
#elif defined DI_V22
	curfac = (g_di_mode.di_mode != DI_MODE_WEAVE) ? 1 :
		 (para->top_field_first == 0) ? 5 : 4;
#endif

	DI_SET_CURFAC(curfac);
	DI_SET_POLAR(field);

	faum = (para->top_field_first == 0) ? 1 : 0;
	DI_SET_FLAG_AUTO_UPD_MODE(faum);

	return 0;
}

/*
 * function: di_set_mode(struct __di_mode_t *di_mode)
 * description: set DI mode
 * parameters: di_mode <update_mode and di_mode set by app>
 * return  :
 */
void di_set_mode(struct __di_mode_t *di_mode)
{
	unsigned char flag_update_mode;
	unsigned char motion_detc_en;
	unsigned char diag_intp_en;
	unsigned char mode;
	unsigned char in_field_mode;

	g_di_mode.di_mode = di_mode->di_mode;
	g_di_mode.update_mode = di_mode->update_mode;

	flag_update_mode = di_mode->update_mode & 0x1;
	motion_detc_en = (di_mode->di_mode < DI_MODE_MOTION) ? 0 : 1;
	diag_intp_en = (di_mode->di_mode < DI_MODE_INTP) ? 0 : 1;
	mode = (di_mode->di_mode < DI_MODE_INTP) ? 0 : 1;
	in_field_mode = (di_mode->di_mode == DI_MODE_MOTION) ? 0 : 1;

	di_dev->mode.dwval = mode |
			    (motion_detc_en << 4) |
			    (diag_intp_en << 5) |
			    (flag_update_mode << 8) |
			    (mode << 16) |
			    (in_field_mode << 31);

}

s32 di_internal_clk_enable(void)
{
	return 0;
}

s32 di_internal_clk_disable(void)
{
	return 0;
}
