/*
 * Copyright (c) 2007-2018 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include "di300.h"
#include "di300_reg.h"
#include "di300_alg.h"
#include "../di_client.h"
#include "../di_utils.h"
#include "../di_debug.h"
#include "../sunxi_di.h"

#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/slab.h>

#define TAG "[DI_DEV]"

struct di_dev_vof_buf {
	u8 buf_a[DI_VOF_BUF_SIZE_MAX];
	u8 buf_b[DI_VOF_BUF_SIZE_MAX];
	u8 buf_c[DI_VOF_BUF_SIZE_MAX];
	u8 buf_d[DI_VOF_BUF_SIZE_MAX];
};

/* each client has its owned di_dev_cdata */
struct di_dev_cdata {
	struct di_dev_proc_result proc_rst;

	u32 vof_buf_sel;
	struct di_dev_vof_buf *vof_buf;
};

static struct di_reg *di_reg_base;

static inline struct di_reg *di_dev_get_reg_base(void)
{
	return di_reg_base;
}

void di_dev_set_reg_base(void __iomem *reg_base)
{
	di_reg_base = (struct di_reg *)reg_base;
}

void di_dev_exit(void)
{
	di_reg_base = 0;
}

/*
* convert format to the format that indecated in DI HW
*/
static u32 di_dev_convert_fmt(u32 format)
{
	switch (format) {
	/* YV12 YV21 */
	case DRM_FORMAT_YUV420:
	case DRM_FORMAT_YVU420:
		return 0;
	case DRM_FORMAT_NV12:
	case DRM_FORMAT_NV21:
		return 1;
	/* YV16 YV61 */
	case DRM_FORMAT_YUV422:
	case DRM_FORMAT_YVU422:
		return 2;
	case DRM_FORMAT_NV16:
	case DRM_FORMAT_NV61:
		return 3;
	default:
		DI_ERR(TAG"unknown format=%s(0x%x)\n",
			di_format_to_string(format), format);
		return 0;
	}
}

/* set output address
* set out put fb y/cb/cr address
*/
static void di_dev_set_out_addr(struct di_fb *fb,
	struct di_addr *addr_reg)
{
	u64 addr[3] = {0, 0, 0};

	addr[0] = fb->buf.y_addr;
	if (fb->buf.cb_addr == 0) {
		addr[1] = fb->buf.cr_addr;
		addr[2] = 0;
	} else if (fb->buf.cr_addr == 0) {
		addr[1] = fb->buf.cb_addr;
		addr[2] = 0;
	} else {
		addr[1] = fb->buf.cb_addr;
		addr[2] = fb->buf.cr_addr;
	}

	addr_reg->laddr[0] = (u32)addr[0];
	addr_reg->laddr[1] = (u32)addr[1];
	addr_reg->laddr[2] = (u32)addr[2];
	addr_reg->haddr.bits.addr0 = (u32)(addr[0] >> 32);
	addr_reg->haddr.bits.addr1 = (u32)(addr[1] >> 32);
	addr_reg->haddr.bits.addr2 = (u32)(addr[2] >> 32);

}

/* set input address
* set input fb y/cb/cr address of  top filed and bottom filed
*/
#ifdef CONFIG_SUNXI_DI_SINGEL_FILE
static void di_dev_set_in_addr(struct di_fb *fb,
	struct di_in_addr *addr_reg)
{
	u64 addr[3] = {0, 0, 0};
	u64 addr_b[3] = {0, 0, 0};

	if (fb->buf.y_addr == 0)
		return;

	if (fb->field_type == DI_FIELD_TYPE_COMBINE_FIELD) {
		addr[0] = fb->buf.y_addr;
		if (fb->buf.cb_addr == 0) {
			addr[1] = fb->buf.cr_addr;
			addr[2] = 0;
		} else if (fb->buf.cr_addr == 0) {
			addr[1] = fb->buf.cb_addr;
			addr[2] = 0;
		} else {
			addr[1] = fb->buf.cb_addr;
			addr[2] = fb->buf.cr_addr;
		}
		addr_b[0] = addr[0] + fb->buf.ystride;
		addr_b[1] = addr[1] ? (addr[1] + fb->buf.cstride) : 0;
		addr_b[2] = addr[2] ? (addr[2] + fb->buf.cstride) : 0;

		addr_reg->top.laddr[0] = (u32)addr[0];
		addr_reg->top.laddr[1] = (u32)addr[1];
		addr_reg->top.laddr[2] = (u32)addr[2];
		addr_reg->top.haddr.bits.addr0 = (u32)(addr[0] >> 32);
		addr_reg->top.haddr.bits.addr1 = (u32)(addr[1] >> 32);
		addr_reg->top.haddr.bits.addr2 = (u32)(addr[2] >> 32);
		addr_reg->bot.laddr[0] = (u32)addr_b[0];
		addr_reg->bot.laddr[1] = (u32)addr_b[1];
		addr_reg->bot.laddr[2] = (u32)addr_b[2];
		addr_reg->bot.haddr.bits.addr0 = (u32)(addr_b[0] >> 32);
		addr_reg->bot.haddr.bits.addr1 = (u32)(addr_b[1] >> 32);
		addr_reg->bot.haddr.bits.addr2 = (u32)(addr_b[2] >> 32);
	} else if (fb->field_type == DI_FIELD_TYPE_TOP_FIELD) {
		addr[0] = fb->buf.y_addr;
		if (fb->buf.cb_addr == 0) {
			addr[1] = fb->buf.cr_addr;
			addr[2] = 0;
		} else if (fb->buf.cr_addr == 0) {
			addr[1] = fb->buf.cb_addr;
			addr[2] = 0;
		} else {
			addr[1] = fb->buf.cb_addr;
			addr[2] = fb->buf.cr_addr;
		}

		addr_reg->top.laddr[0] = (u32)addr[0];
		addr_reg->top.laddr[1] = (u32)addr[1];
		addr_reg->top.laddr[2] = (u32)addr[2];
		addr_reg->top.haddr.bits.addr0 = (u32)(addr[0] >> 32);
		addr_reg->top.haddr.bits.addr1 = (u32)(addr[1] >> 32);
		addr_reg->top.haddr.bits.addr2 = (u32)(addr[2] >> 32);
	} else if (fb->field_type == DI_FIELD_TYPE_BOTTOM_FIELD) {
		addr_b[0] = fb->buf.y_addr;
		if (fb->buf.cb_addr == 0) {
			addr_b[1] = fb->buf.cr_addr;
			addr_b[2] = 0;
		} else if (fb->buf.cr_addr == 0) {
			addr_b[1] = fb->buf.cb_addr;
			addr_b[2] = 0;
		} else {
			addr_b[1] = fb->buf.cb_addr;
			addr_b[2] = fb->buf.cr_addr;
		}

		addr_reg->bot.laddr[0] = (u32)addr_b[0];
		addr_reg->bot.laddr[1] = (u32)addr_b[1];
		addr_reg->bot.laddr[2] = (u32)addr_b[2];
		addr_reg->bot.haddr.bits.addr0 = (u32)(addr_b[0] >> 32);
		addr_reg->bot.haddr.bits.addr1 = (u32)(addr_b[1] >> 32);
		addr_reg->bot.haddr.bits.addr2 = (u32)(addr_b[2] >> 32);
	} else {
		DI_ERR("invalid field type:%d\n", fb->field_type);
	}
}
#else
static void di_dev_set_in_addr(struct di_fb *fb,
	struct di_in_addr *addr_reg)
{
	u64 addr[3] = {0, 0, 0};
	u64 addr_b[3] = {0, 0, 0};

	addr[0] = fb->buf.y_addr;
	if (fb->buf.cb_addr == 0) {
		addr[1] = fb->buf.cr_addr;
		addr[2] = 0;
	} else if (fb->buf.cr_addr == 0) {
		addr[1] = fb->buf.cb_addr;
		addr[2] = 0;
	} else {
		addr[1] = fb->buf.cb_addr;
		addr[2] = fb->buf.cr_addr;
	}
	addr_b[0] = addr[0] + fb->buf.ystride;
	addr_b[1] = addr[1] ? (addr[1] + fb->buf.cstride) : 0;
	addr_b[2] = addr[2] ? (addr[2] + fb->buf.cstride) : 0;

	addr_reg->top.laddr[0] = (u32)addr[0];
	addr_reg->top.laddr[1] = (u32)addr[1];
	addr_reg->top.laddr[2] = (u32)addr[2];
	addr_reg->top.haddr.bits.addr0 = (u32)(addr[0] >> 32);
	addr_reg->top.haddr.bits.addr1 = (u32)(addr[1] >> 32);
	addr_reg->top.haddr.bits.addr2 = (u32)(addr[2] >> 32);
	addr_reg->bot.laddr[0] = (u32)addr_b[0];
	addr_reg->bot.laddr[1] = (u32)addr_b[1];
	addr_reg->bot.laddr[2] = (u32)addr_b[2];
	addr_reg->bot.haddr.bits.addr0 = (u32)(addr_b[0] >> 32);
	addr_reg->bot.haddr.bits.addr1 = (u32)(addr_b[1] >> 32);
	addr_reg->bot.haddr.bits.addr2 = (u32)(addr_b[2] >> 32);

}
#endif

/* set fb
* 1.set format, uvseq,pitch,phy address of di/pre/cur fb to reg
* 2.set format, pitch,phy address of dit_out0/dit_out1/tnr_out0 fb to reg
*/
#ifdef CONFIG_SUNXI_DI_SINGEL_FILE
static void di_dev_set_fb(struct di_client *c)
{
	struct di_reg *reg = di_dev_get_reg_base();
	struct di_fb *fb, *fb_nf;

	if (c->dma_di) {
		fb = c->dma_di->fb;

		/* di300 Only Support planner and uv/vu combine */
		reg->fmt.bits.in0fmt = di_dev_convert_fmt(fb->format);
		if (di_format_get_planar_num(fb->format) == 2)
			reg->fmt.bits.uvseq = di_format_get_uvseq(fb->format);
		/* set pitch of field0 and field1 */
		if (fb->field_type == DI_FIELD_TYPE_COMBINE_FIELD) {
			reg->in_f01_pitch[0].bits.f01 = fb->buf.ystride << 1;
			reg->in_f01_pitch[1].bits.f01 = fb->buf.cstride << 1;
			reg->in_f01_pitch[2].bits.f01 = fb->buf.cstride << 1;
		} else {
			reg->in_f01_pitch[0].bits.f01 = fb->buf.ystride;
			reg->in_f01_pitch[1].bits.f01 = fb->buf.cstride;
			reg->in_f01_pitch[2].bits.f01 = fb->buf.cstride;
		}

		di_dev_set_in_addr(fb, &reg->in_f0_addr);

		if (fb->field_type != DI_FIELD_TYPE_COMBINE_FIELD
			&& c->dma_di_nf) {
			fb_nf = c->dma_di_nf->fb;
			di_dev_set_in_addr(fb_nf, &reg->in_f0_addr);
		}
	}

	if (c->dma_p) {
		fb = c->dma_p->fb;
		reg->fmt.bits.in1fmt = di_dev_convert_fmt(fb->format);
		if (di_format_get_planar_num(fb->format) == 2)
			reg->fmt.bits.uvseq = di_format_get_uvseq(fb->format);

		if (fb->field_type == DI_FIELD_TYPE_COMBINE_FIELD) {
			reg->in_f01_pitch[0].bits.f23 = fb->buf.ystride << 1;
			reg->in_f01_pitch[1].bits.f23 = fb->buf.cstride << 1;
			reg->in_f01_pitch[2].bits.f23 = fb->buf.cstride << 1;
		} else {
			reg->in_f01_pitch[0].bits.f23 = fb->buf.ystride;
			reg->in_f01_pitch[1].bits.f23 = fb->buf.cstride;
			reg->in_f01_pitch[2].bits.f23 = fb->buf.cstride;
		}
		di_dev_set_in_addr(fb, &reg->in_f1_addr);

		if (fb->field_type != DI_FIELD_TYPE_COMBINE_FIELD
			&& c->dma_p_nf) {
			fb_nf = c->dma_p_nf->fb;
			di_dev_set_in_addr(fb_nf, &reg->in_f1_addr);
		}
	}

	if (c->dma_c) {
		fb = c->dma_c->fb;
		reg->fmt.bits.in2fmt = di_dev_convert_fmt(fb->format);
		if (di_format_get_planar_num(fb->format) == 2)
			reg->fmt.bits.uvseq = di_format_get_uvseq(fb->format);
		if (fb->field_type == DI_FIELD_TYPE_COMBINE_FIELD) {
			reg->in_f2_pitch[0].bits.val = fb->buf.ystride << 1;
			reg->in_f2_pitch[1].bits.val = fb->buf.cstride << 1;
			reg->in_f2_pitch[2].bits.val = fb->buf.cstride << 1;
		} else {
			reg->in_f2_pitch[0].bits.val = fb->buf.ystride;
			reg->in_f2_pitch[1].bits.val = fb->buf.cstride;
			reg->in_f2_pitch[2].bits.val = fb->buf.cstride;
		}

		di_dev_set_in_addr(fb, &reg->in_f2_addr);

		if (fb->field_type != DI_FIELD_TYPE_COMBINE_FIELD
			&& c->dma_c_nf) {
			fb_nf = c->dma_c_nf->fb;
			di_dev_set_in_addr(fb_nf, &reg->in_f2_addr);
		}
	}

	if (c->di_w0) {
		fb = c->di_w0->fb;
		reg->fmt.bits.ditfmt = di_dev_convert_fmt(fb->format);
		reg->out_dit_pitch[0].bits.val = fb->buf.ystride;
		reg->out_dit_pitch[1].bits.val = fb->buf.cstride;
		reg->out_dit_pitch[2].bits.val = fb->buf.cstride;
		di_dev_set_out_addr(fb, &reg->out_dit0_addr);
	}

	if (c->di_w1) {
		fb = c->di_w1->fb;
		reg->fmt.bits.ditfmt = di_dev_convert_fmt(fb->format);
		reg->out_dit_pitch[0].bits.val = fb->buf.ystride;
		reg->out_dit_pitch[1].bits.val = fb->buf.cstride;
		reg->out_dit_pitch[2].bits.val = fb->buf.cstride;
		di_dev_set_out_addr(fb, &reg->out_dit1_addr);
	}

	if (c->tnr_w && c->tnr_en) {
		fb = c->tnr_w->fb;
		reg->fmt.bits.tnrfmt = di_dev_convert_fmt(fb->format);
		reg->out_tnr_pitch[0].bits.val = fb->buf.ystride;
		reg->out_tnr_pitch[1].bits.val = fb->buf.cstride;
		reg->out_tnr_pitch[2].bits.val = fb->buf.cstride;
		di_dev_set_out_addr(fb, &reg->out_tnr_addr);
	}

}
#else
static void di_dev_set_fb(struct di_client *c)
{
	struct di_reg *reg = di_dev_get_reg_base();
	struct di_fb *fb;

	if (c->dma_di) {
		fb = c->dma_di->fb;

		/* di300 Only Support planner and uv/vu combine */
		reg->fmt.bits.in0fmt = di_dev_convert_fmt(fb->format);
		if (di_format_get_planar_num(fb->format) == 2)
			reg->fmt.bits.uvseq = di_format_get_uvseq(fb->format);
		/* set pitch of field0 and field1 */
		reg->in_f01_pitch[0].bits.f01 = fb->buf.ystride << 1;
		reg->in_f01_pitch[1].bits.f01 = fb->buf.cstride << 1;
		reg->in_f01_pitch[2].bits.f01 = fb->buf.cstride << 1;
		di_dev_set_in_addr(fb, &reg->in_f0_addr);
	}

	if (c->dma_p) {
		fb = c->dma_p->fb;
		reg->fmt.bits.in1fmt = di_dev_convert_fmt(fb->format);
		if (di_format_get_planar_num(fb->format) == 2)
			reg->fmt.bits.uvseq = di_format_get_uvseq(fb->format);
		reg->in_f01_pitch[0].bits.f23 = fb->buf.ystride << 1;
		reg->in_f01_pitch[1].bits.f23 = fb->buf.cstride << 1;
		reg->in_f01_pitch[2].bits.f23 = fb->buf.cstride << 1;
		di_dev_set_in_addr(fb, &reg->in_f1_addr);
	}

	if (c->dma_c) {
		fb = c->dma_c->fb;
		reg->fmt.bits.in2fmt = di_dev_convert_fmt(fb->format);
		if (di_format_get_planar_num(fb->format) == 2)
			reg->fmt.bits.uvseq = di_format_get_uvseq(fb->format);
		reg->in_f2_pitch[0].bits.val = fb->buf.ystride << 1;
		reg->in_f2_pitch[1].bits.val = fb->buf.cstride << 1;
		reg->in_f2_pitch[2].bits.val = fb->buf.cstride << 1;
		di_dev_set_in_addr(fb, &reg->in_f2_addr);
	}

	if (c->di_w0) {
		fb = c->di_w0->fb;
		reg->fmt.bits.ditfmt = di_dev_convert_fmt(fb->format);
		reg->out_dit_pitch[0].bits.val = fb->buf.ystride;
		reg->out_dit_pitch[1].bits.val = fb->buf.cstride;
		reg->out_dit_pitch[2].bits.val = fb->buf.cstride;
		di_dev_set_out_addr(fb, &reg->out_dit0_addr);
	}

	if (c->di_w1) {
		fb = c->di_w1->fb;
		reg->fmt.bits.ditfmt = di_dev_convert_fmt(fb->format);
		reg->out_dit_pitch[0].bits.val = fb->buf.ystride;
		reg->out_dit_pitch[1].bits.val = fb->buf.cstride;
		reg->out_dit_pitch[2].bits.val = fb->buf.cstride;
		di_dev_set_out_addr(fb, &reg->out_dit1_addr);
	}

	if (c->tnr_w && c->tnr_en) {
		fb = c->tnr_w->fb;
		reg->fmt.bits.tnrfmt = di_dev_convert_fmt(fb->format);
		reg->out_tnr_pitch[0].bits.val = fb->buf.ystride;
		reg->out_tnr_pitch[1].bits.val = fb->buf.cstride;
		reg->out_tnr_pitch[2].bits.val = fb->buf.cstride;
		di_dev_set_out_addr(fb, &reg->out_tnr_addr);
	}

}
#endif

/* Set in/out flag phy address to reg */
static void di_dev_set_md_buf(struct di_client *c)
{
	struct di_reg *reg = di_dev_get_reg_base();
	struct di_mapped_buf *in_buf = NULL;
	struct di_mapped_buf *out_buf = NULL;

	in_buf = c->md_buf.mbuf[c->md_buf.dir & 1];
	c->md_buf.dir ^= 1;
	out_buf = c->md_buf.mbuf[c->md_buf.dir & 1];
	reg->in_flag_laddr = (u32)(in_buf->dma_addr);
	reg->out_flag_laddr = (u32)(out_buf->dma_addr);
	reg->flag_haddr.bits.in_addr =
		(u32)((u64)in_buf->dma_addr >> 32);
	reg->flag_haddr.bits.out_addr =
		(u32)((u64)out_buf->dma_addr >> 32);
}

/*
* set to para
* top means it is the module topped on DIT,MD,TNR module
* 1. set top_filed_first para
* 2. set in/out flag addr para
* 3. set format, uvseq,pitch,phy address of fbs
*/
static s32 di_dev_set_top_para(struct di_client *c)
{
	struct di_dev_cdata *cdata;
	struct di_dev_proc_result *proc_rst;
	struct __alg_hist *alg_hist;
	struct __di_para_t *di_para;
	struct __alg_para_t *alg_para;
	u8 tff;
	struct di_reg *reg = di_dev_get_reg_base();

	if (!c) {
		DI_ERR("%s di_client is NULL\n", __func__);
		return -1;
	}
	cdata = (struct di_dev_cdata *)c->dev_cdata;

	if (!cdata) {
		DI_ERR("%s cdata is NULL\n", __func__);
		return -1;
	}

	proc_rst = &cdata->proc_rst;
	alg_hist = &proc_rst->alg_hist;
	di_para = &proc_rst->di_para;
	alg_para = &di_para->alg_para;

	c->fb_arg.top_field_first =
			di_para->bff ? 0 : 1;
	DI_INFO("top_field_first:%d di_para->bff:%d\n", c->fb_arg.top_field_first, di_para->bff);
	tff = c->fb_arg.top_field_first ? 1 : 0;
	if (c->dit_mode.intp_mode == DI_DIT_INTP_MODE_BOB)
		tff ^= (c->fb_arg.base_field ? 1 : 0);
	DI_INFO("set tff:%d\n", tff);
	reg->forder.bits.bff = tff ? 0 : 1;

	if (c->md_en)
		di_dev_set_md_buf(c);

	di_dev_set_fb(c);

	return 0;
}

/* set params of MD Module
* set left/right value of window horizon crop
* set top/bottom value of window vetical crop
*/
static s32 di_dev_set_md_para(struct di_client *c)
{
	struct di_reg *reg = di_dev_get_reg_base();

	/* reg->md_croph.dwval = (c->md_out_crop.left & 0xfffffffc) |
		((((c->md_out_crop.right + 1) & 0xfffffffc) - 1)
		<< 16);
	reg->md_cropv.dwval = (c->md_out_crop.top & 0xfffffffc) |
		((((c->md_out_crop.bottom + 1) & 0xfffffffc) - 1)
		<< 16); */
	if ((c->md_out_crop.left % 4 != 0)
		|| (c->md_out_crop.right % 4 != 0)
		|| (c->md_out_crop.top % 4 != 0)
		|| (c->md_out_crop.bottom % 4 != 0)) {
		DI_ERR("crop value error, DI300 only support the crop that 4bytes aligned\n");
		return -1;
	}
	reg->md_croph.dwval = c->md_out_crop.left | ((c->md_out_crop.right - 1) << 16);
	reg->md_cropv.dwval = c->md_out_crop.top | ((c->md_out_crop.bottom - 1) << 16);
	return 0;
}

/* set para to dit module
* 1. set left/right/top/bottom value of dit and demo window crop
* 2. set field_weave_phase_f1/2, video_exist_f1/2  through itd_alg/fmd_alg/vof_alg
*/
static s32 di_dev_set_dit_para(struct di_client *c)
{
	struct di_reg *reg = di_dev_get_reg_base();
	struct di_dev_cdata *cdata =
			(struct di_dev_cdata *)c->dev_cdata;
	struct di_dev_proc_result *proc_rst = &cdata->proc_rst;
	struct __alg_hist *alg_hist = &proc_rst->alg_hist;
	struct __di_para_t *di_para = &proc_rst->di_para;
	struct __alg_para_t *alg_para = &di_para->alg_para;

	/* reg->dit_croph.dwval = (c->dit_out_crop.left & 0xfffffffe) |
		((((c->dit_out_crop.right + 1) & 0xfffffffe) - 1)
		<< 16);
	reg->dit_cropv.dwval = (c->dit_out_crop.top & 0xfffffffc) |
		((((c->dit_out_crop.bottom + 1) & 0xfffffffc) - 1)
		<< 16); */
	if ((c->dit_out_crop.left % 4 != 0)
		|| (c->dit_out_crop.right % 4 != 0)
		|| (c->dit_out_crop.top % 4 != 0)
		|| (c->dit_out_crop.bottom % 4 != 0)) {
		DI_ERR("dit_out crop value error, DI300 only support the crop that 4bytes aligned\n");
		return -1;
	}

	if (c->dit_out_crop.right > 0)
		reg->dit_croph.dwval = c->dit_out_crop.left
			| ((c->dit_out_crop.right - 1) << 16);
	else
		reg->dit_croph.dwval = c->dit_out_crop.left;

	if (c->dit_out_crop.bottom > 0)
		reg->dit_cropv.dwval = c->dit_out_crop.top
			| ((c->dit_out_crop.bottom - 1) << 16);
	else
		reg->dit_cropv.dwval = c->dit_out_crop.top;

	/* reg->dit_demoh.dwval = (c->dit_demo_crop.left & 0xfffffffe) |
	    ((((c->dit_demo_crop.right + 1) & 0xfffffffe) - 1)
	     << 16);
	reg->dit_demov.dwval = (c->dit_demo_crop.top & 0xfffffffc) |
	    ((((c->dit_demo_crop.bottom + 1) & 0xfffffffc) - 1)
	     << 16); */
	if ((c->dit_demo_crop.left % 4 != 0)
		|| (c->dit_demo_crop.right % 4 != 0)
		|| (c->dit_demo_crop.top % 4 != 0)
		|| (c->dit_demo_crop.bottom % 4 != 0)) {
		DI_ERR("dit demo crop value error, DI300 only support the crop that 4bytes aligned\n");
		return -1;
	}

	if (c->dit_demo_crop.right > 0)
		reg->dit_demoh.dwval = c->dit_demo_crop.left
			| ((c->dit_demo_crop.right - 1) << 16);
	else
		reg->dit_demoh.dwval = c->dit_demo_crop.left;

	if (c->dit_demo_crop.bottom > 0)
		reg->dit_demov.dwval = c->dit_demo_crop.top
			| ((c->dit_demo_crop.bottom - 1) << 16);
	else
		reg->dit_demov.dwval = c->dit_demo_crop.top;

	if (alg_para->alg_en == 0)
		return 0;

	/* film mode and vof */
	reg->dit_inter_para.bits.field_weave_f1 = 0;
	reg->dit_inter_para.bits.field_weave_chroma_f1 = 0;
	reg->dit_inter_para.bits.field_weave_phase_f1 = 1;
	reg->dit_inter_para.bits.field_weave_f2 = 0;
	reg->dit_inter_para.bits.field_weave_chroma_f2 = 0;
	reg->dit_inter_para.bits.field_weave_phase_f2 = 1;
	reg->dit_inter_para.bits.video_exist_f1 = 0;
	reg->dit_inter_para.bits.video_exist_f2 = 0;

	/* TODO: itd_alg parameters feedback */
	if (alg_para->itd_alg_para.itd_alg_en) {
		if (alg_hist->itd_alg_hist.is_progressive_lock == 1 &&
		    alg_hist->itd_alg_hist.is_temp_di_f3 == 0 &&
		    alg_hist->vof_alg_hist.text_field_exist_f3 == 0) {
			reg->dit_inter_para.bits.field_weave_f1 = 1;
			/* For safety, DONOT enable field weave for chroma */
			/* reg->dit_inter_para.bits.field_weave_chroma_f1 = 0; */
			reg->dit_inter_para.bits.field_weave_phase_f1 =
			    alg_hist->itd_alg_hist.weave_phase_f3;
		}

		if (alg_hist->itd_alg_hist.is_progressive_lock == 1 &&
		    alg_hist->itd_alg_hist.is_temp_di_f4 == 0 &&
		    alg_hist->vof_alg_hist.text_field_exist_f4 == 0) {
			reg->dit_inter_para.bits.field_weave_f2 = 1;
			/* For safety, DONOT enable field weave for chroma */
			/* reg->dit_inter_para.bits.field_weave_chroma_f2 = 0; */
			reg->dit_inter_para.bits.field_weave_phase_f2 =
			    alg_hist->itd_alg_hist.weave_phase_f4;
		}

	}

	if (alg_para->fmd_alg_para.fmd_alg_en) {
		if (alg_hist->fmd_alg_hist.is_fm_lock_f3 == 1 &&
			alg_hist->fmd_alg_hist.is_temp_di_f3 == 0 &&
			alg_hist->vof_alg_hist.text_field_exist_f3 == 0) {
			reg->dit_inter_para.bits.field_weave_f1 = 1;
			/* For safety, DONOT enable field weave for chroma */
			/* reg->dit_inter_para.bits.field_weave_chroma_f1 = 0; */
			reg->dit_inter_para.bits.field_weave_phase_f1 =
				alg_hist->fmd_alg_hist.weave_phase_f3;
		}

		if (alg_hist->fmd_alg_hist.is_fm_lock_f4 == 1 &&
			alg_hist->fmd_alg_hist.is_temp_di_f4 == 0 &&
			alg_hist->vof_alg_hist.text_field_exist_f4 == 0) {
			reg->dit_inter_para.bits.field_weave_f2 = 1;
			/* For safety, DONOT enable field weave for chroma */
			/* reg->dit_inter_para.bits.field_weave_chroma_f2 = 0; */
			reg->dit_inter_para.bits.field_weave_phase_f2 =
				alg_hist->fmd_alg_hist.weave_phase_f4;
		}
	}

	if (alg_para->vof_alg_para.vof_alg_en) {
		if (alg_hist->vof_alg_hist.video_field_exist_f3)
			reg->dit_inter_para.bits.video_exist_f1 = 1;

		if (alg_hist->vof_alg_hist.video_field_exist_f4)
			reg->dit_inter_para.bits.video_exist_f2 = 1;
	}
	return 0;
}

/* set window params of FMD Module
* set left/right value of window horizon crop
* set top/bottom value of window vetical crop
*/
static s32 di_dev_set_fmd_para(struct di_client *c)
{
	struct di_reg *reg = di_dev_get_reg_base();

	if (di_device_get_debug_mode())
		return 0;

	if (c->mode == DI_MODE_60HZ) {
		u32 vof_buf_sel = reg->fmd_glb.bits.vof_buf_sel;

		vof_buf_sel ^= 1;
		reg->fmd_glb.bits.vof_buf_sel = vof_buf_sel;
	}

	/* reg->fmd_croph.dwval = (c->fmd_out_crop.left & 0xfffffffc) |
		((((c->fmd_out_crop.right + 1) & 0xfffffffc) - 1)
		<< 16);
	reg->fmd_cropv.dwval = (c->fmd_out_crop.top & 0xfffffffc) |
		((((c->fmd_out_crop.bottom + 1) & 0xfffffffc) - 1)
		<< 16); */

	if ((c->fmd_out_crop.left % 4 != 0)
		|| (c->fmd_out_crop.right % 4 != 0)
		|| (c->fmd_out_crop.top % 4 != 0)
		|| (c->fmd_out_crop.bottom % 4 != 0)) {
		DI_ERR("crop value error, DI300 only support the crop that 4bytes aligned\n");
		return -1;
	}
	reg->fmd_croph.dwval = c->fmd_out_crop.left | ((c->fmd_out_crop.right - 1) << 16);
	reg->fmd_cropv.dwval = c->fmd_out_crop.top | ((c->fmd_out_crop.bottom - 1) << 16);

	return 0;
}

static s32 di_dev_set_tnr_para(struct di_client *c)
{
	struct di_reg *reg = di_dev_get_reg_base();
	struct di_dev_cdata *cdata =
			(struct di_dev_cdata *)c->dev_cdata;
	struct di_dev_proc_result *proc_rst = &cdata->proc_rst;
	struct __alg_hist *alg_hist = &proc_rst->alg_hist;
	struct __di_para_t *di_para = &proc_rst->di_para;
	struct __alg_para_t *alg_para = &di_para->alg_para;

	if (di_device_get_debug_mode())
		return 0;
	/* reg->tnr_croph.dwval = (c->tnr_out_crop.left & 0xfffffffc) |
		((((c->tnr_out_crop.right + 1) & 0xfffffffc) - 1)
		<< 16);
	reg->tnr_cropv.dwval = (c->tnr_out_crop.top & 0xfffffffc) |
		((((c->tnr_out_crop.bottom + 1) & 0xfffffffc) - 1)
		<< 16); */

	if ((c->tnr_out_crop.left % 4 != 0)
		|| (c->tnr_out_crop.right % 4 != 0)
		|| (c->tnr_out_crop.top % 4 != 0)
		|| (c->tnr_out_crop.bottom % 4 != 0)) {
		DI_ERR("tnr_out crop value error, DI300 only support the crop that 4bytes aligned\n");
		return -1;
	}

	if (c->tnr_out_crop.right > 0)
		reg->tnr_croph.dwval = c->tnr_out_crop.left
			| ((c->tnr_out_crop.right - 1) << 16);
	else
		reg->tnr_croph.dwval = c->tnr_out_crop.left;

	if (c->tnr_out_crop.bottom > 0)
		reg->tnr_cropv.dwval = c->tnr_out_crop.top
			| ((c->tnr_out_crop.bottom - 1) << 16);
	else
		reg->tnr_cropv.dwval = c->tnr_out_crop.top;

	/* reg->tnr_demoh.dwval = (c->tnr_demo_crop.left & 0xfffffffc) |
	    ((((c->tnr_demo_crop.right + 1) & 0xfffffffc) - 1)
	     << 16);
	reg->tnr_demov.dwval = (c->tnr_demo_crop.top & 0xfffffffc) |
	    ((((c->tnr_demo_crop.bottom + 1) & 0xfffffffc) - 1)
	     << 16); */
	if ((c->tnr_demo_crop.left % 4 != 0)
		|| (c->tnr_demo_crop.right % 4 != 0)
		|| (c->tnr_demo_crop.top % 4 != 0)
		|| (c->tnr_demo_crop.bottom % 4 != 0)) {
		DI_ERR("tnr_demo crop value error, DI300 only support the crop that 4bytes aligned\n");
		return -1;
	}

	if (c->tnr_demo_crop.right > 0)
		reg->tnr_demoh.dwval = c->tnr_demo_crop.left
			| ((c->tnr_demo_crop.right - 1) << 16);
	else
		reg->tnr_demoh.dwval = c->tnr_demo_crop.left;

	if (c->tnr_demo_crop.bottom > 0)
		reg->tnr_demov.dwval = c->tnr_demo_crop.top
			| ((c->tnr_demo_crop.bottom - 1) << 16);
	else
		reg->tnr_demov.dwval = c->tnr_demo_crop.top;

	if (alg_para->alg_en == 0)
		return 0;

	if (alg_para->fmd_alg_para.fmd_alg_en) {
		/* TNR : is_scene_changed */
		reg->tnr_md_result.bits.is_scene_changed =
			(alg_hist->fmd_alg_hist.is_scenechange_f3 == 1) ||
			(alg_hist->fmd_alg_hist.is_scenechange_f4 == 1);

		/* TNR : is_contain_bad_frame */
		if ((alg_hist->fmd_alg_hist.film_mode_f4 == FM_32 ||
			alg_hist->fmd_alg_hist.film_mode_f4 == FM_2332 ||
			alg_hist->fmd_alg_hist.film_mode_f4 == FM_32322 ||
			alg_hist->fmd_alg_hist.film_mode_f4 == FM_55 ||
			alg_hist->fmd_alg_hist.film_mode_f4 == FM_87) &&
			alg_hist->fmd_alg_hist.is_fm_lock_f4 == 1)
			reg->tnr_md_result.bits.is_contain_bad_frame = 1;
		else
			reg->tnr_md_result.bits.is_contain_bad_frame = 0;
	}

	/* TNR : reset accumulation regs */
	reg->tnr_sum_weight_y = 0x0;
	reg->tnr_sum_weight_u = 0x0;
	reg->tnr_sum_weight_v = 0x0;
	reg->tnr_sum_gain_y = 0x0;
	reg->tnr_sum_gain_u = 0x0;
	reg->tnr_sum_gain_v = 0x0;
	reg->tnr_sum_still_out = 0x0;
	reg->tnr_sum_weight_y_cnt = 0x0;
	reg->tnr_sum_gain_y_cnt = 0x0;

	if (alg_para->tnr_alg_para.tnr_alg_en
		&& alg_para->tnr_alg_para.tnr_mode == DI_TNR_MODE_ADAPTIVE) {
		/* fixme: get the correct gain value */
		reg->tnr_strength.bits.y = alg_hist->tnr_alg_hist.gain;
		reg->tnr_strength.bits.u = alg_hist->tnr_alg_hist.gain;
		reg->tnr_strength.bits.v = alg_hist->tnr_alg_hist.gain;
	}

	return 0;
}

/*
 * di_dev_get_proc_result wil be called in finished-irq.
 * It just get some paras of current fb-process.
 */
s32 di_dev_get_proc_result(void *client)
{
	struct di_reg *reg = di_dev_get_reg_base();
	struct di_client *c = (struct di_client *)client;
	struct di_dev_cdata *cdata = (struct di_dev_cdata *)c->dev_cdata;
	struct di_dev_proc_result *proc_rst = &cdata->proc_rst;
	struct __hw_hist *hw_hist = &proc_rst->hw_hist;
	struct __fmd_hist *fmd_hist = &hw_hist->fmd_hist;
	struct __tnr_hist *tnr_hist = &hw_hist->tnr_hist;
	struct __itd_hist *itd_hist = &hw_hist->itd_hist;

	fmd_hist->FMD_FID12 = reg->fmd_fid12.bits.cnt;
	fmd_hist->FMD_FID23 = reg->fmd_fid23.bits.cnt;
	fmd_hist->FOD_FID30 = reg->fod_fid30.bits.cnt;
	fmd_hist->FOD_FID32 = reg->fod_fid32.bits.cnt;
	fmd_hist->FOD_FID10 = reg->fod_fid10.bits.cnt;
	fmd_hist->FOD_FID12 = reg->fod_fid12.bits.cnt;
	fmd_hist->FMD_FRD02 = reg->fmd_frd02.bits.cnt;
	fmd_hist->FMD_FRD13 = reg->fmd_frd13.bits.cnt;

	DI_INFO("di_dev_get_proc_result, FOD_FID10:%d FOD_FID32:%d\n", fmd_hist->FOD_FID10, fmd_hist->FOD_FID32);

	fmd_hist->FIELD_MAX_VIDEO_NUM_F3 =
		reg->fmd_field_hist0.bits.max_video_num;
	fmd_hist->FIELD_MAX_TEXT_NUM_F3 =
		reg->fmd_field_hist0.bits.max_text_num;
	fmd_hist->FIELD_MAX_TEXT_POS_F3 =
		reg->fmd_field_hist0.bits.max_text_pos;
	fmd_hist->FIELD_TEXT_ROW_NUM_F3 =
		reg->fmd_field_hist0.bits.text_row_num;

	fmd_hist->FIELD_MAX_VIDEO_NUM_F4 =
		reg->fmd_field_hist1.bits.max_video_num;
	fmd_hist->FIELD_MAX_TEXT_NUM_F4 =
		reg->fmd_field_hist1.bits.max_text_num;
	fmd_hist->FIELD_MAX_TEXT_POS_F4 =
		reg->fmd_field_hist1.bits.max_text_pos;
	fmd_hist->FIELD_TEXT_ROW_NUM_F4 =
		reg->fmd_field_hist1.bits.text_row_num;

	itd_hist->FMD_FID12 = reg->fmd_fid12.bits.cnt;
	itd_hist->FMD_FID23 = reg->fmd_fid23.bits.cnt;

	tnr_hist->tnr_sum_weight_y = reg->tnr_sum_weight_y;
	tnr_hist->tnr_sum_weight_u = reg->tnr_sum_weight_u;
	tnr_hist->tnr_sum_weight_v = reg->tnr_sum_weight_v;
	tnr_hist->tnr_sum_gain_y = reg->tnr_sum_gain_y;
	tnr_hist->tnr_sum_gain_u = reg->tnr_sum_gain_u;
	tnr_hist->tnr_sum_gain_v = reg->tnr_sum_gain_v;
	tnr_hist->tnr_sum_still_out = reg->tnr_sum_still_out;
	tnr_hist->tnr_sum_weight_y_cnt = reg->tnr_sum_weight_y_cnt;
	tnr_hist->tnr_sum_gain_y_cnt = reg->tnr_sum_gain_y_cnt;

	return 0;
}

/* figure out current fb-process paras by using pre fb-process result paras. */
static s32 di_dev_calc_proc_result(struct di_client *c)
{
	struct di_dev_cdata *cdata = (struct di_dev_cdata *)c->dev_cdata;
	struct di_dev_proc_result *proc_rst = &cdata->proc_rst;
	struct __alg_hist *alg_hist = &proc_rst->alg_hist;

	/* Run Alg */
	di_alg(proc_rst);

	/* Print result */
	/* FOD */
	if (alg_hist->fod_alg_hist.is_fieldorderchange) {
		DI_DEBUG(TAG"[FOD]Field order changed.\n");
	}

	if (alg_hist->fod_alg_hist.bff_fix)
		DI_DEBUG(TAG"[FOD]Current field order:BFF.\n");
	else
		DI_DEBUG(TAG"[FOD]Current field order:TFF.\n");

	/* ITD */
	if (alg_hist->itd_alg_hist.is_progressive_lock) {
		c->di_detect_result = DI_DETECT_PROGRESSIVE;
		DI_DEBUG(TAG"[ITD]Progressive locked.\n");
	} else {
		c->di_detect_result = DI_DETECT_INTERLACE;
		DI_DEBUG(TAG"[ITD]Interlaced detected.\n");
	}

	/* FMD */
	if (alg_hist->fmd_alg_hist.is_non22_lock) {
		switch (alg_hist->fmd_alg_hist.film_mode_non22) {
		case 1:
			DI_FMD(TAG"[FMD]Film mode 32 locked.\n");
			break;
		case 2:
			DI_FMD(TAG"[FMD]Film mode 2332 locked.\n");
			break;
		case 3:
			DI_FMD(TAG"[FMD]Film mode 2224 locked.\n");
			DI_FMD(TAG"[FMD]Film mode 22 locked. is actually a progressive video\n");
			c->di_detect_result = DI_DETECT_PROGRESSIVE;
			break;
		case 4:
			DI_FMD(TAG"[FMD]Film mode 32322 locked.\n");
			break;
		case 5:
			DI_FMD(TAG"[FMD]Film mode 55 locked.\n");
			break;
		case 6:
			DI_FMD(TAG"[FMD]Film mode 64 locked.\n");
			break;
		case 7:
			DI_FMD(TAG"[FMD]Film mode 87 locked.\n");
			break;
		default:
			DI_FMD(TAG"[FMD]Film mode 22 locked. is actually a progressive video\n");
			c->di_detect_result = DI_DETECT_PROGRESSIVE;
			break;
		}
	} else if (alg_hist->fmd_alg_hist.is_22_lock) {
		DI_FMD(TAG"[FMD]Film mode 22 locked. is actually a progressive video\n");
		c->di_detect_result = DI_DETECT_PROGRESSIVE;
	} else {
		DI_DEBUG(TAG"[FMD]Film mode unlocked.\n");
	}

	/* VOF */
	if (alg_hist->vof_alg_hist.video_field_exist_f3
		|| alg_hist->vof_alg_hist.video_field_exist_f4) {
		DI_DEBUG(TAG"[VOF]Video field exited.\n");
	} else {
		DI_DEBUG(TAG"[VOF]Video field not exited.\n");
	}

	if (alg_hist->vof_alg_hist.text_field_exist_f3
		|| alg_hist->vof_alg_hist.text_field_exist_f4) {
		DI_DEBUG(TAG"[VOF]Text field exited.\n");
	} else {
		DI_DEBUG(TAG"[VOF]Text field not exited.\n");
	}

	/* Modify Hardware Parameters */
	di_alg_hist_to_hardware(c, proc_rst);
	return 0;
}

/* apply fb-process para every process. */
s32 di_dev_apply_para(void *client)
{
	struct di_client *c = (struct di_client *)client;

	DI_DEBUG(TAG"%s: c=%p  seqno:%llu\n", __func__, c, c->proc_fb_seqno);

	if (c->proc_fb_seqno > 0)
		di_dev_calc_proc_result(c);
	di_dev_set_top_para(c);
	di_dev_set_md_para(c);/* crop */
	di_dev_set_dit_para(c);
	if (c->fmd_en.en)
		di_dev_set_fmd_para(c);
	if (c->tnr_en)
		di_dev_set_tnr_para(c);

	return 0;
}

/*
 * di_dev_apply_fixed_para will be called in these conditions:
 * a) at 1st time of continuous fb-process of a const-mode client; or
 * b) at 1st time of fb-process after resume.
 */
s32 di_dev_apply_fixed_para(void *client)
{
	struct di_client *c = (struct di_client *)client;
	struct di_dev_cdata *cdata =
		(struct di_dev_cdata *)c->dev_cdata;
	struct di_reg *reg = di_dev_get_reg_base();
	u32 dwval = 0;
	u32 vof_blk_size_sel = 0;

	if (di_device_get_debug_mode())
		return 0;

	DI_DEBUG(TAG"%s: c=%p\n", __func__, c);

	/* apply top fixed para */
	dwval = 0;
	if (c->dit_mode.intp_mode != DI_DIT_INTP_MODE_INVALID)
		dwval |= (1 << 0);
	if (c->md_en)
		dwval |= (1 << 1);
	if (c->tnr_en)
		dwval |= (1 << 2);
	if (c->fmd_en.en)
		dwval |= (1 << 3);
	reg->func_en.dwval = dwval;

	dwval = (c->video_size.width - 1) & 0x7FF;
	dwval |= (((c->video_size.height - 1) & 0x7FF) << 16);
	reg->size.dwval = dwval;

	dwval = 0;
	if (c->md_en) {
		/* TODO: check flag_pitch = ((width + 127) >> 2) & 0xffffffe0 */
		reg->flag_pitch.dwval = c->md_buf.w_stride;
		dwval |= ((1 << 12) | (1 << 28));
	} else {
		reg->flag_pitch.dwval = 0;
	}
	if (c->dma_c)
		dwval |= 1;
	if (c->dma_p)
		dwval |= (1 << 4);
	if (c->dma_di)
		dwval |= (1 << 8);
	if (c->di_w0)
		dwval |= (1 << 16);
	if (c->di_w1)
		dwval |= (1 << 20);
	if (c->tnr_w)
		dwval |= (1 << 24);

	dwval |= (1 << 31);
	reg->dma_ctl.dwval = dwval;
	/* reg->dma_ctl.bits.mclk_gate = 1; */

	/* apply md fixed para */
	reg->md_para.dwval = 0x21360c04;

	/* apply dit fixed para */
	dwval = reg->dit_setting.dwval;
	dwval &= 0x700;
	dwval |= (1 << 5);
	switch (c->dit_mode.intp_mode) {
	case DI_DIT_INTP_MODE_MOTION:
		dwval |= (1 | (1 << 4) | (1 << 16));
		if (c->dit_mode.out_frame_mode == DI_DIT_OUT_2FRAME)
			dwval |= (1 << 20);
		break;
	case DI_DIT_INTP_MODE_BOB:
		dwval |= (1 | (1 << 16));
		break;
	case DI_DIT_INTP_MODE_WEAVE:
		break;
	case DI_DIT_INTP_MODE_INVALID:
		break;
	default:
		DI_ERR(TAG"bad dit intp mode %d\n", c->dit_mode.intp_mode);
		break;
	}
	if (c->dit_mode.out_frame_mode == DI_DIT_OUT_2FRAME) {
		dwval &= ~(1 << 24);
	} else {
		dwval &= ~(1 << 20);
		dwval |= (1 << 24);
	}
	reg->dit_setting.dwval = dwval;
	reg->dit_setting.bits.ela_demo_win_en = 1;
	reg->dit_setting.bits.field_weave_demo_win_en = 1;
	reg->dit_setting.bits.video_blend_demo_win_en = 1;

	reg->dit_chr_para0.dwval = 0x30058000;
	dwval = 0x04300000;
	/* want to reduce flicker of chroma text when noisy video */
	/* dwval |= (1 << 8); */
	/* want to reduce flicker of chorma when noisy video */
	/* dwval |= 1; */
	reg->dit_chr_para1.dwval = dwval;
	reg->dit_intra_para.dwval = 0x514240ac;
	reg->dit_inter_para.dwval = 0x22000000;

	/* apply fmd fixed para */
	/* block size of VOF : 0-8x8(default); 1-16x16 */
	vof_blk_size_sel = 0;
	if (c->mode == DI_MODE_60HZ)
		reg->fmd_glb.dwval = (1 << 31) | vof_blk_size_sel;
	else
		reg->fmd_glb.dwval = (0 << 31) | vof_blk_size_sel;
	c->vof_blk_size_sel = vof_blk_size_sel;

	reg->fmd_diff_th0.dwval = 0xff03ff00;
	reg->fmd_diff_th1.dwval = 0x0003ff00;
	reg->fmd_diff_th2.dwval = 0x0220ff00;
	reg->fmd_feat_th0.dwval = 0x05200405;
	reg->fmd_feat_th1.dwval = 0x030100c0;
	reg->fmd_feat_th2.dwval = 0x00020609;
	reg->fmd_mot_th.dwval = 0x02090010;
	reg->fmd_text_th.dwval = 0x000000c0;
	if (vof_blk_size_sel) {
		reg->fmd_blk_th.dwval = 0x00555520;
		dwval = 3 * c->video_size.width * c->video_size.height /
			(720 * 480 * 4);
		dwval |= ((2 * c->video_size.width * c->video_size.height /
			(720 * 480 * 4)) << 8);
	} else {
		reg->fmd_blk_th.dwval = 0x00151508;
		dwval = 3 * c->video_size.width * c->video_size.height /
			(720 * 480);
		dwval |= ((2 * c->video_size.width * c->video_size.height /
			(720 * 480)) << 8);
	}
	reg->fmd_row_th.dwval = dwval | (60 << 16);

	/* apply tnr fixed para */
	reg->tnr_strength.dwval = 0x003a3a3a;
	reg->tnr_dark_th.dwval = 0x00010101;
	reg->tnr_dark_protect.dwval = 0x00030303;
	reg->tnr_dark_para_y.dwval = 0x01000100;
	reg->tnr_dark_para_u.dwval = 0x01000100;
	reg->tnr_dark_para_v.dwval = 0x01000100;
	reg->tnr_fth_detect.dwval = 0x00640101;
	reg->tnr_dt_filter.dwval = 0x0040000f;
	reg->tnr_weight_lbound.dwval = 0x3f201610;
	reg->tnr_abn_detect.dwval = 0x00780101;
	reg->tnr_th_sum.dwval = 0x00000000;
	reg->tnr_sum_weight_y = 0x0;
	reg->tnr_sum_weight_u = 0x0;
	reg->tnr_sum_weight_v = 0x0;
	reg->tnr_sum_gain_y = 0x0;
	reg->tnr_sum_gain_u = 0x0;
	reg->tnr_sum_gain_v = 0x0;
	reg->tnr_sum_still_out = 0x0;
	reg->tnr_sum_weight_y_cnt = 0x0;
	reg->tnr_sum_gain_y_cnt = 0x0;
	reg->tnr_th_dark_dth.dwval = 0x00000202;
	reg->tnr_random_cfg.dwval = 0x02031c01;
	reg->tnr_random_gen = 0x0;
	reg->tnr_md_result.dwval = 0x00000000;

	/* apply alg fixed para */
	di_alg_fixed_para(c, &(cdata->proc_rst));

	return 0;
}

s32 di_dev_save_spot(void *client)
{
	struct di_reg *reg = di_dev_get_reg_base();
	struct di_client *c = (struct di_client *)client;
	struct di_dev_cdata *cdata =
		(struct di_dev_cdata *)c->dev_cdata;

	if (c->mode != DI_MODE_60HZ)
		return 0;

	if (cdata->vof_buf == NULL) {
		cdata->vof_buf = kzalloc(
			sizeof(*(cdata->vof_buf)), GFP_KERNEL);
		if (!cdata->vof_buf) {
			DI_ERR(TAG"kzalloc for vof_buf failed, size=%d\n",
				(u32)sizeof(*(cdata->vof_buf)));
			return -1;
		}
	}

	reg->bist_ctl.bits.bist_mode_en = 1;
	memcpy((void *)cdata->vof_buf,
		(void *)((u8 *)reg + DI_VOF_BUF_REG_ADDR_A),
		sizeof(*(cdata->vof_buf)));
	reg->bist_ctl.bits.bist_mode_en = 0;
	cdata->vof_buf_sel = reg->fmd_glb.bits.vof_buf_sel;

	return 0;
}

s32 di_dev_restore_spot(void *client)
{
	struct di_reg *reg = di_dev_get_reg_base();
	struct di_client *c = (struct di_client *)client;
	struct di_dev_cdata *cdata =
		(struct di_dev_cdata *)c->dev_cdata;

	if ((c->mode != DI_MODE_60HZ)
		|| (cdata->vof_buf == NULL))
		return 0;

	reg->bist_ctl.bits.bist_mode_en = 1;
	memcpy((void *)((u8 *)reg + DI_VOF_BUF_REG_ADDR_A),
		(void *)cdata->vof_buf,
		sizeof(*(cdata->vof_buf)));
	reg->bist_ctl.bits.bist_mode_en = 0;
	reg->fmd_glb.bits.vof_buf_sel = cdata->vof_buf_sel;

	return 0;
}

s32 di_dev_enable_irq(u32 irq_flag, u32 en)
{
	struct di_reg *reg = di_dev_get_reg_base();
	u32 reg_val = readl(&reg->int_ctl);

	if (en)
		reg_val |= irq_flag;
	else
		reg_val &= ~irq_flag;
	writel(reg_val, &reg->int_ctl);

	return 0;
}

/* clear di finish bit */
u32 di_dev_query_state_with_clear(u32 irq_state)
{
	struct di_reg *reg = di_dev_get_reg_base();
	u32 reg_val = readl(&reg->status);
	u32 state = reg_val & irq_state & DI_IRQ_STATE_MASK;

	reg_val &= ~DI_IRQ_STATE_MASK;
	reg_val |= state;
	writel(reg_val, &reg->status); /* w1c */

	return state;
}

void di_dev_start(u32 start)
{
	struct di_reg *reg = di_dev_get_reg_base();

	reg->start.dwval = start;
}

/* reset then stop reset */
void di_dev_reset(void)
{
	struct di_reg *reg = di_dev_get_reg_base();
	const u32 t = 1;

	reg->reset.bits.reset = 1;
	udelay(t);
	reg->reset.bits.reset = 0;
}

u32 di_dev_get_cdata_size(void)
{
	return (u32)sizeof(struct di_dev_cdata);
}

u32 di_dev_reset_cdata(void *dev_cdata)
{
	struct di_dev_cdata *cdata = (struct di_dev_cdata *)dev_cdata;

	if (cdata->vof_buf)
		kfree(cdata->vof_buf);

	memset((void *)cdata, 0, sizeof(*cdata));

	return 0;
}

u32 di_dev_get_ip_version(void)
{
	struct di_reg *reg = di_dev_get_reg_base();

	return reg->ip_version;
}

void di_dev_dump_reg_value(void)
{
	unsigned int *addr;
	unsigned long i;
	struct di_reg *reg = di_dev_get_reg_base();

	addr = (unsigned int *)reg;

	DI_INFO("GENERAL TOP REG:\n");
	for (i = 0x0; i <= 0x38; i += 4) {
		if ((i % 16) == 0)
			DI_INFO("0x%08lx:", i);
		DI_INFO("0x%08x ", *((unsigned int *)((unsigned long)addr + i)));
		if (((i + 4) % 16) == 0)
			DI_INFO("\n");
	}
	DI_INFO("\n");

	DI_INFO("TOP PITCH REG:\n");
	for (i = 0x40; i <= 0x78; i += 4) {
		if ((i % 16) == 0)
			DI_INFO("0x%08lx:", i);
		DI_INFO("0x%08x ", *((unsigned int *)((unsigned long)addr + i)));
		if (((i + 4) % 16) == 0)
			DI_INFO("\n");

	}
	DI_INFO("\n");

	DI_INFO("TOP ADDR REG:\n");
	for (i = 0x90; i <= 0x128; i += 4) {
		if ((i % 16) == 0)
			DI_INFO("0x%08lx:", i);
		DI_INFO("0x%08x ", *((unsigned int *)((unsigned long)addr + i)));
		if (((i + 4) % 16) == 0)
			DI_INFO("\n");

	}
	DI_INFO("\n");

	DI_INFO("MD REG:\n");
	for (i = 0x180; i <= 0x194; i += 4) {
		if ((i % 16) == 0)
			DI_INFO("0x%08lx:", i);
		DI_INFO("0x%08x ", *((unsigned int *)((unsigned long)addr + i)));
		if (((i + 4) % 16) == 0)
			DI_INFO("\n");

	}
	DI_INFO("\n");

	DI_INFO("DIT REG:\n");
	for (i = 0x1a0; i <= 0x1cc; i += 4) {
		if ((i % 16) == 0)
			DI_INFO("0x%08lx:", i);
		DI_INFO("0x%08x ", *((unsigned int *)((unsigned long)addr + i)));
		if (((i + 4) % 16) == 0)
			DI_INFO("\n");

	}
	DI_INFO("\n");

	DI_INFO("FMD REG:\n");
	for (i = 0x1d0; i <= 0x234; i += 4) {
		if ((i % 16) == 0)
			DI_INFO("0x%08lx:", i);
		DI_INFO("0x%08x ", *((unsigned int *)((unsigned long)addr + i)));
		if (((i + 4) % 16) == 0)
			DI_INFO("\n");

	}
	DI_INFO("\n");

	DI_INFO("TNR REG:\n");
	for (i = 0x240; i <= 0x2ac; i += 4) {
		if ((i % 16) == 0)
			DI_INFO("0x%08lx:", i);
		DI_INFO("0x%08x ", *((unsigned int *)((unsigned long)addr + i)));
		if (((i + 4) % 16) == 0)
			DI_INFO("\n");

	}
	DI_INFO("\n");

	DI_INFO("DEBUG REG:\n");
	for (i = 0x2d0; i <= 0x2e8; i += 4) {
		if ((i % 16) == 0)
			DI_INFO("0x%08lx:", i);
		DI_INFO("0x%08x ", *((unsigned int *)((unsigned long)addr + i)));
		if (((i + 4) % 16) == 0)
			DI_INFO("\n");

	}
	DI_INFO("\n");
}
