/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include "de_wb_type.h"
#include "de_wb.h"
#define WB_MODULE_NUMBER 1
#define WB_OFFSET 0x010000

static volatile __wb_reg_t *wb_dev[WB_MODULE_NUMBER] = { NULL };

static struct disp_capture_config wb_config;

unsigned int wb_lan2coefftab16[16] = {
	0x00004000, 0x00033ffe, 0x00063efc, 0x000a3bfb,
	0xff0f37fb, 0xfe1433fb, 0xfd192ffb, 0xfd1f29fb,
	0xfc2424fc, 0xfb291ffd, 0xfb2f19fd, 0xfb3314fe,
	0xfb370fff, 0xfb3b0a00, 0xfc3e0600, 0xfe3f0300
};

unsigned int wb_lan2coefftab16_down[16] = {
	0x000e240e, 0x0010240c, 0x0013230a, 0x00142309,
	0x00162208, 0x01182106, 0x011a2005, 0x021b1f04,
	0x031d1d03, 0x041e1c02, 0x05201a01, 0x06211801,
	0x07221601, 0x09231400, 0x0a231300, 0x0c231100
};

/* ********************************************************************************** */
/* function       : WB_EBIOS_Set_Reg_Base(__u32 sel, __u32 base) */
/* description    : setup write-back controller register base */
/* parameters     : */
/* sel <controller select> */
/* base <register base> */
/* return         : */
/* success */
/* *********************************************************************************** */
__s32 WB_EBIOS_Set_Reg_Base(__u32 sel, __u32 base)
{
	__inf("sel=%d, base=0x%x\n", sel, base + WB_OFFSET);
	wb_dev[sel] = (__wb_reg_t *) (base + WB_OFFSET);

	return 0;
}

/* ********************************************************************************** */
/* function       : WB_EBIOS_Get_Reg_Base(__u32 sel) */
/* description    : get write-back controller register base */
/* parameters     : */
/* sel <controller select> */
/*  */
/* return         : */
/* registers base */
/* *********************************************************************************** */
__u32 WB_EBIOS_Get_Reg_Base(__u32 sel)
{
	__u32 ret = 0;

	ret = (__u32) wb_dev[sel];

	return ret;
}

/* ********************************************************************************** */
/* function       : WB_EBIOS_Init(__u32 sel) */
/* description    : initial write-back controller registers */
/* parameters     : */
/* sel <controller select> */
/*  */
/* return         : */
/* success */
/* *********************************************************************************** */
__s32 WB_EBIOS_Init(struct disp_bsp_init_para *para)
{
	WB_EBIOS_Set_Reg_Base(0, para->reg_base[DISP_MOD_DE]);
	/* FIXME, need? */
	/* memset((void*)wb_dev[0], 0, sizeof(__wb_reg_t)); */
	return 0;
}

/* ********************************************************************************** */
/* function       : WB_EBIOS_Writeback_Enable(__u32 sel) */
/* description    : enable write-back once */
/* parameters     : */
/* sel <controller select>, en<0:disable; 1:enable> */
/* return         : */
/* success */
/* *********************************************************************************** */
__s32 WB_EBIOS_Writeback_Enable(__u32 sel, bool en)
{
	wb_dev[sel]->gctrl.dwval |= ((0 == en) ? 0x0 : 0x00000001);

	return 0;
}

/* ********************************************************************************** */
/* function       : WB_EBIOS_Apply(__u32 sel, struct disp_capture_config *config) */
/* description    : save user config */
/* parameters     : */
/* sel <controller select> */
/* wb_config <write-back information,include input_fb and output_fb information> */
/* return         : */
/* 0   --      success */
/* -1      --      fail */
/* note                   : Don't support YUV input yet 14-02-28 */
/* when write-back format is yuv, default 16-235 output */
/* *********************************************************************************** */
s32 WB_EBIOS_Apply(__u32 sel, struct disp_capture_config *cfg)
{
	memcpy(&wb_config, cfg, sizeof(struct disp_capture_config));
	return 0;
}

/* ********************************************************************************** */
/* function       : WB_EBIOS_Upate_regs(__u32 sel) */
/* description    : transform user config into hw config */
/* parameters     : */
/* sel <controller select> */
/* return         : */
/* 0   --      success */
/* -1      --      fail */
/* *********************************************************************************** */
s32 WB_EBIOS_Update_Regs(__u32 sel)
{
	WB_EBIOS_Set_Para(sel, &wb_config);
	return 0;
}

/* ********************************************************************************** */
/* function       : WB_EBIOS_Set_Para(__u32 sel,disp_capture_config *cfg) */
/* description    : setup write-back controller parameters */
/* parameters     : */
/* sel <controller select> */
/* wb_config <write-back information,include input_fb and output_fb information> */
/* return         : */
/* 0   --      success */
/* -1      --      fail */
/* note                   : Don't support YUV input yet 14-02-28 */
/* when write-back format is yuv, default 16-235 output */
/* *********************************************************************************** */
__s32 WB_EBIOS_Set_Para(__u32 sel, struct disp_capture_config *cfg)
{
	u32 in_port;
	u32 in_w, in_h;
	u32 crop_x, crop_y, crop_w, crop_h;
	u32 out_addr[3];
	u32 out_buf_w, out_buf_h;
	u32 out_fmt;
	u32 out_window_w, out_window_h, out_window_x, out_window_y;
	u32 cs_out_w0, cs_out_h0, cs_out_w1, cs_out_h1;
	u32 fs_out_w0, fs_out_h0, fs_out_w1, fs_out_h1;
	u32 step_h, step_v;
	u32 v_intg, v_frac, h_intg, h_frac;
	u32 down_scale_y, down_scale_c;
	u32 i;
	/* get para */
	in_port = cfg->disp;
	in_w = cfg->in_frame.size[0].width;
	in_h = cfg->in_frame.size[0].height;
	crop_x = (u32) cfg->in_frame.crop.x;
	crop_y = (u32) cfg->in_frame.crop.y;
	crop_w = cfg->in_frame.crop.width;
	crop_h = cfg->in_frame.crop.height;
	out_addr[0] = cfg->out_frame.addr[0];
	out_addr[1] = cfg->out_frame.addr[1];
	out_addr[2] = cfg->out_frame.addr[2];
	out_buf_w = cfg->out_frame.size[0].width;
	out_buf_h = cfg->out_frame.size[0].height;
	out_fmt = cfg->out_frame.format;
	out_window_x = cfg->out_frame.crop.x;
	out_window_y = cfg->out_frame.crop.y;
	out_window_w = cfg->out_frame.crop.width;
	out_window_h = cfg->out_frame.crop.height;
	__inf
	    ("in_port:%d, in_size=<%d,%d>, crop=<%d,%d,%d,%d>, out_size=<%d,%d>,out_window=<%d,%d>, addr=<0x%x,0x%x,0x%x>\n",
	     in_port, in_w, in_h, crop_x, crop_y, crop_w, crop_h, out_buf_w,
	     out_buf_h, out_window_w, out_window_h, out_addr[0], out_addr[1],
	     out_addr[2]);
	if (in_port > 1)
		return -1;
	if ((in_w < MININWIDTH) || (in_w > MAXINWIDTH) || (in_h < MININHEIGHT)
	    || (in_h > MAXINHEIGHT))
		return -1;
	if ((crop_w < MININWIDTH) || (crop_w > MAXINWIDTH)
	    || (crop_h < MININHEIGHT) || (crop_h > MAXINHEIGHT))
		return -1;
	if ((crop_x + crop_w > in_w) || (crop_y + crop_h > in_h))
		return -1;
	if ((out_buf_w < out_window_w) || (out_buf_h < out_window_h))
		return -1;
	if ((out_window_w < MININWIDTH) || (out_window_w > MAXINWIDTH)
	    || (out_window_h < MININHEIGHT) || (out_window_h > MAXINHEIGHT))
		return -1;
	if ((out_window_w > crop_w) || (out_window_h > crop_h))
		return -1;
	if (((out_fmt == DISP_FORMAT_YUV420_P)
	     || (out_fmt == DISP_FORMAT_YUV420_SP_VUVU)
	     || (out_fmt == DISP_FORMAT_YUV420_SP_UVUV))
	    && (out_window_w > LINE_BUF_LEN))
		return -1;
	__inf("para check ok\n");
	/* gctrl */
	wb_dev[sel]->gctrl.dwval |= (0x10000000 | (in_port << 16));
	__inf("gctrl_adrr=0x%x, val=0x%x\n",
	      (unsigned int)(&(wb_dev[sel]->gctrl)), wb_dev[sel]->gctrl.dwval);
	/* input size */
	wb_dev[sel]->size.dwval = (in_w - 1) | ((in_h - 1) << 16);
	/* input crop window */
	wb_dev[sel]->crop_coord.dwval = crop_x | ((crop_y) << 16);
	wb_dev[sel]->crop_size.dwval = (crop_w - 1) | ((crop_h - 1) << 16);
	__inf("crop_size=0x%x, val=0x%x\n",
	      (unsigned int)(&wb_dev[sel]->crop_size),
	      wb_dev[sel]->crop_size.dwval);
	/* output fmt */
	if (out_fmt == DISP_FORMAT_ARGB_8888)
		wb_dev[sel]->fmt.dwval = WB_FORMAT_ARGB_8888;
	else if (out_fmt == DISP_FORMAT_ABGR_8888)
		wb_dev[sel]->fmt.dwval = WB_FORMAT_ABGR_8888;
	else if (out_fmt == DISP_FORMAT_RGBA_8888)
		wb_dev[sel]->fmt.dwval = WB_FORMAT_RGBA_8888;
	else if (out_fmt == DISP_FORMAT_BGRA_8888)
		wb_dev[sel]->fmt.dwval = WB_FORMAT_BGRA_8888;
	else if (out_fmt == DISP_FORMAT_RGB_888)
		wb_dev[sel]->fmt.dwval = WB_FORMAT_RGB_888;
	else if (out_fmt == DISP_FORMAT_BGR_888)
		wb_dev[sel]->fmt.dwval = WB_FORMAT_BGR_888;
	else if (out_fmt == DISP_FORMAT_YUV420_P)
		wb_dev[sel]->fmt.dwval = WB_FORMAT_YUV420_P;
	else if (out_fmt == DISP_FORMAT_YUV420_SP_VUVU)
		wb_dev[sel]->fmt.dwval = WB_FORMAT_YUV420_SP_VUVU;
	else if (out_fmt == DISP_FORMAT_YUV420_SP_UVUV)
		wb_dev[sel]->fmt.dwval = WB_FORMAT_YUV420_SP_UVUV;
	else {
		return -1;
	}
	/* output addr and pitch for different fmt */
	if ((out_fmt == DISP_FORMAT_ARGB_8888)
	    || (out_fmt == DISP_FORMAT_ABGR_8888)
	    || (out_fmt == DISP_FORMAT_RGBA_8888)
	    || (out_fmt == DISP_FORMAT_BGRA_8888)) {
		out_addr[0] += (out_window_y * out_buf_w + out_window_x) << 2;
		wb_dev[sel]->wb_addr_A0.dwval = out_addr[0];
		__inf("wb_addr_A0=0x%x, val=0x%x\n",
		      (unsigned int)(&wb_dev[sel]->wb_addr_A0),
		      wb_dev[sel]->wb_addr_A0.dwval);
		wb_dev[sel]->wb_pitch0.dwval = out_buf_w << 2;
		__inf("gctrl_adrr=0x%x, val=0x%x\n",
		      (unsigned int)(&wb_dev[sel]->wb_addr_A0),
		      wb_dev[sel]->wb_addr_A0.dwval);
	}
	if ((out_fmt == DISP_FORMAT_RGB_888)
	    || (out_fmt == DISP_FORMAT_BGR_888)) {
		out_addr[0] += (out_window_y * out_buf_w + out_window_x) * 3;
		wb_dev[sel]->wb_addr_A0.dwval = out_addr[0];
		wb_dev[sel]->wb_pitch0.dwval = 3 * out_buf_w;
	}
	if ((out_fmt == DISP_FORMAT_YUV420_SP_VUVU)
	    || (out_fmt == DISP_FORMAT_YUV420_SP_UVUV)) {
		out_addr[0] += (out_window_y * out_buf_w + out_window_x);
		out_addr[1] +=
		    (((out_window_y * out_buf_w) >> 1) + out_window_x);
		wb_dev[sel]->wb_addr_A0.dwval = out_addr[0];
		wb_dev[sel]->wb_addr_A1.dwval = out_addr[1];
		wb_dev[sel]->wb_pitch0.dwval = out_buf_w;
		wb_dev[sel]->wb_pitch1.dwval = out_buf_w;
	}
	if (out_fmt == DISP_FORMAT_YUV420_P) {
		out_addr[0] += (out_window_y * out_buf_w + out_window_x);
		out_addr[1] += (out_window_y * out_buf_w + out_window_x) >> 1;
		out_addr[2] += (out_window_y * out_buf_w + out_window_x) >> 1;
		wb_dev[sel]->wb_addr_A0.dwval = out_addr[0];
		wb_dev[sel]->wb_addr_A1.dwval = out_addr[1];
		wb_dev[sel]->wb_addr_A2.dwval = out_addr[2];
		wb_dev[sel]->wb_pitch0.dwval = out_buf_w;
		wb_dev[sel]->wb_pitch1.dwval = out_buf_w >> 1;
	}
	/* CSC */
	if ((out_fmt == DISP_FORMAT_ARGB_8888)
	    || (out_fmt == DISP_FORMAT_ABGR_8888)
	    || (out_fmt == DISP_FORMAT_RGBA_8888)
	    || (out_fmt == DISP_FORMAT_BGRA_8888)
	    || (out_fmt == DISP_FORMAT_RGB_888)
	    || (out_fmt == DISP_FORMAT_BGR_888))
		wb_dev[sel]->bypass.dwval &= 0xfffffffe;
	else {
		wb_dev[sel]->bypass.dwval |= 0x00000001;
	}
	/* Coarse scaling */
	if ((crop_w > (out_window_w << 1)) && (crop_h > (out_window_h << 1))) {
		wb_dev[sel]->bypass.dwval |= 0x00000002;
		wb_dev[sel]->cs_horz.dwval = crop_w | (out_window_w << 17);
		wb_dev[sel]->cs_vert.dwval = crop_h | (out_window_h << 17);
		cs_out_w0 = out_window_w << 1;
		cs_out_h0 = out_window_h << 1;
	}
	if ((crop_w > (out_window_w << 1)) && (crop_h <= (out_window_h << 1))) {
		wb_dev[sel]->bypass.dwval |= 0x00000002;
		wb_dev[sel]->cs_horz.dwval = crop_w | (out_window_w << 17);
		wb_dev[sel]->cs_vert.dwval = 0;
		cs_out_w0 = out_window_w << 1;
		cs_out_h0 = crop_h;
	}
	if ((crop_w <= (out_window_w << 1)) && (crop_h > (out_window_h << 1))) {
		wb_dev[sel]->bypass.dwval |= 0x00000002;
		wb_dev[sel]->cs_horz.dwval = 0;
		wb_dev[sel]->cs_vert.dwval = crop_h | (out_window_h << 17);
		cs_out_w0 = crop_w;
		cs_out_h0 = out_window_h << 1;
	}
	if ((crop_w <= (out_window_w << 1)) && (crop_h <= (out_window_h << 1))) {
		wb_dev[sel]->bypass.dwval &= 0xfffffffd;
		wb_dev[sel]->cs_horz.dwval = 0;
		wb_dev[sel]->cs_vert.dwval = 0;
		cs_out_w0 = crop_w;
		cs_out_h0 = crop_h;
	}
	/* Fine scaling */
	cs_out_w1 = cs_out_w0;
	cs_out_h1 = cs_out_h0;
	fs_out_w0 = out_window_w;
	fs_out_w1 =
	    ((out_fmt == DISP_FORMAT_YUV420_P) | (out_fmt ==
						  DISP_FORMAT_YUV420_SP_VUVU) |
	     (out_fmt ==
	      DISP_FORMAT_YUV420_SP_UVUV)) ? (out_window_w >> 1) : out_window_w;
	fs_out_h0 = out_window_h;
	fs_out_h1 =
	    ((out_fmt == DISP_FORMAT_YUV420_P) | (out_fmt ==
						  DISP_FORMAT_YUV420_SP_VUVU) |
	     (out_fmt ==
	      DISP_FORMAT_YUV420_SP_UVUV)) ? (out_window_h >> 1) : out_window_h;
	if ((cs_out_w0 == fs_out_w0) && (cs_out_h0 == fs_out_h0)
	    && (cs_out_w1 == fs_out_w1) && (cs_out_h1 == fs_out_h1)) {
		wb_dev[sel]->bypass.dwval &= 0xfffffffb;
		wb_dev[sel]->fs_hstep.dwval = 1 << 20;
		wb_dev[sel]->fs_vstep.dwval = 1 << 20;
	} else {
		unsigned long long tmp;
		wb_dev[sel]->bypass.dwval |= 0x00000004;
		tmp = ((long long)cs_out_w0 << LOCFRACBIT);
		do_div(tmp, (long long)out_window_w);
		step_h = (int)tmp;
		tmp = ((long long)cs_out_h0 << LOCFRACBIT);
		do_div(tmp, (long long)out_window_h);
		step_v = (int)tmp;
		h_intg = (step_h & (~((1 << LOCFRACBIT) - 1))) >> LOCFRACBIT;
		h_frac = step_h & ((1 << LOCFRACBIT) - 1);
		v_intg = (step_v & (~((1 << LOCFRACBIT) - 1))) >> LOCFRACBIT;
		v_frac = step_v & ((1 << LOCFRACBIT) - 1);
		wb_dev[sel]->fs_hstep.dwval = (h_frac << 2) | (h_intg << 20);
		wb_dev[sel]->fs_vstep.dwval = (v_frac << 2) | (v_intg << 20);
		if (cs_out_w0 <= fs_out_w0)
			down_scale_y = 0;
		else
			down_scale_y = 1;
		if (cs_out_w1 <= fs_out_w1)
			down_scale_c = 0;
		else
			down_scale_c = 1;
		for (i = 0; i < SCALERPHASE; i++) {
			wb_dev[sel]->yhcoeff[i].dwval =
			    down_scale_y ? wb_lan2coefftab16_down[i] :
			    wb_lan2coefftab16[i];
			wb_dev[sel]->chcoeff[i].dwval =
			    down_scale_c ? wb_lan2coefftab16_down[i] :
			    wb_lan2coefftab16[i];
		}
	}
	wb_dev[sel]->fs_insize.dwval =
	    (cs_out_w0 - 1) | ((cs_out_h0 - 1) << 16);
	wb_dev[sel]->fs_outsize.dwval =
	    (out_window_w - 1) | ((out_window_h - 1) << 16);
	return 0;
}

/* ********************************************************************************** */
/* function       : WB_EBIOS_Get_Status(__u32 sel) */
/* description    : get last frame write back status */
/* parameters     : */
/* sel <controller select> */
/* return         : */
/* writeback finish    0 */
/* overflow          1 */
/* timeout           2 */
/* not start         3 */
/* *********************************************************************************** */
__u32 WB_EBIOS_Get_Status(__u32 sel)
{
	__u32 status;

	status = wb_dev[sel]->status.dwval & 0x71;
	wb_dev[sel]->status.dwval = 0x71;

	if (status & 0x10) {
		return 0;
	} else if (status & 0x20) {
		return 1;
	} else if (status & 0x40) {
		return 2;
	} else {
		return 3;
	}
}

/* /INTERUPT */
__s32 WB_EBIOS_EnableINT(__u32 sel)
{
	wb_dev[sel]->intr.dwval |= 0x00000001;
	return 0;
}

__s32 WB_EBIOS_DisableINT(__u32 sel)
{
	wb_dev[sel]->intr.dwval &= 0xfffffffe;
	return 0;
}

__u32 WB_EBIOS_QueryINT(__u32 sel)
{
	return wb_dev[sel]->status.dwval;
}

/* write 1 to clear */
__u32 WB_EBIOS_ClearINT(__u32 sel)
{
	wb_dev[sel]->status.dwval |=
	    (WB_END_IE | WB_FINISH_IE | WB_FIFO_OVERFLOW_ERROR_IE |
	     WB_TIMEOUT_ERROR_IE);
	return 0;
}
