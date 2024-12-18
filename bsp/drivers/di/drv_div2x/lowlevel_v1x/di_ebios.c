/*
 * Allwinner SoCs de-interlace driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include "di_ebios.h"
#include "di_ebios_data.h"
#include <linux/slab.h>
#include <asm/io.h>

extern volatile __di_dev_t *di_dev;

__u32 DI_VAtoPA(__u32 va)
{
	if ((va) > 0x40000000)
		return (va) - 0x40000000;
	return va;
}

/* should initial some registers for memory-to-memory de-interlace used */
__s32 DI_Init(void)
{
#if defined CONFIG_ARCH_SUN9IW1
	di_dev->trd_ctrl.dwval = 0;
	di_dev->ch0_horzphase.bits.phase = 0;
	di_dev->ch1_horzphase.bits.phase = 0;
	di_dev->ch0_vertphase0.bits.phase = 0;
	di_dev->ch0_vertphase1.bits.phase = 0;
	di_dev->ch1_vertphase0.bits.phase = 0;
	di_dev->ch1_vertphase1.bits.phase = 0;
	di_dev->output_fmt.bits.byte_seq = 0;
	di_dev->input_fmt.bits.byte_seq = 0;

#endif

	di_dev->bypass.bits.csc_bypass_en = 1; /* bypass CSC */
	di_dev->wb_linestrd_en.dwval = 0x1;
	di_dev->frm_ctrl.bits.out_ctrl = 1;
	di_dev->output_fmt.bits.alpha_en = 0x0;
	di_dev->bypass.bits.sram_map_sel = 0; /* normal mode */
	di_dev->agth_sel.bits.linebuf_agth = 1;

	DI_Set_Di_Ctrl(1, 3, 1, 1);

	return 0;
}

__s32 DI_Config_Src(__di_buf_addr_t *addr, __di_src_size_t *size,
			__di_src_type_t *type)
{
/* __u8 w_shift, h_shift; */
	__u32 image_w0, image_w1;
/* __u32 x_off0, y_off0, x_off1, y_off1; */
	__u32 in_w0, in_h0, in_w1, in_h1;
/* __u8 rgb16mode = 0; */

	image_w0 = size->src_width;
	in_w0 = size->scal_width;
	in_h0 = size->scal_height;

	image_w1 = (image_w0 + 0x1)>>1;
	in_w1 = (in_w0 + 0x1)>>1;
	in_h1 = (in_h0 + 0x1)>>1;

	/* added no-zero limited */
	in_h0 = (in_h0 != 0) ? in_h0 : 1;
	in_h1 = (in_h1 != 0) ? in_h1 : 1;
	in_w0 = (in_w0 != 0) ? in_w0 : 1;
	in_w1 = (in_w1 != 0) ? in_w1 : 1;

	if (type->mod == DI_UVCOMBINED) {
		di_dev->linestrd0.dwval = image_w0;
		di_dev->linestrd1.dwval = image_w1<<1;
		di_dev->linestrd2.dwval = 0x0;

		di_dev->buf_addr0.dwval = addr->ch0_addr;
		di_dev->buf_addr1.dwval = addr->ch1_addr;
		di_dev->buf_addr2.dwval = 0x0;
	} else if (type->mod == DI_UVCOMBINEDMB) {
		image_w0 = (image_w0 + 0x1f)&0xffffffe0;
		image_w1 = (image_w1 + 0x0f)&0xfffffff0;
		/* block offset */
		di_dev->tb_off0.bits.x_offset0 = 0;
		di_dev->tb_off0.bits.y_offset0 = 0;
		di_dev->tb_off0.bits.x_offset1 = (in_w0 + 0x1f) & 0x1f;
		di_dev->tb_off1.bits.x_offset0 = 0;
		di_dev->tb_off1.bits.y_offset0 = 0;
		di_dev->tb_off1.bits.x_offset1 = (((in_w1)<<1) + 0x1f) & 0x1f;

		di_dev->linestrd0.dwval =
			(((image_w0 + 0x1f)&0xffe0) - 0x1f)<<0x05;
		di_dev->linestrd1.dwval =
			(((((image_w1)<<1)+0x1f)&0xffe0) - 0x1f) << 0x05;
		di_dev->linestrd2.dwval = 0x00;

		di_dev->buf_addr0.dwval = addr->ch0_addr;
		di_dev->buf_addr1.dwval = addr->ch1_addr;
		di_dev->buf_addr2.dwval = 0x0;
	}

	di_dev->input_fmt.bits.data_mod = type->mod;
	di_dev->input_fmt.bits.data_fmt = type->fmt;
	di_dev->input_fmt.bits.data_ps = type->ps;

	di_dev->ch0_insize.bits.in_width = in_w0 - 1;
	di_dev->ch0_insize.bits.in_height = in_h0 - 1;
	di_dev->ch1_insize.bits.in_width = in_w1 - 1;
	di_dev->ch1_insize.bits.in_height = in_h1 - 1;

	return 0;
}

__s32 DI_Set_Scaling_Factor(__di_src_size_t *in_size,
				__di_out_size_t *out_size)
{
	__s32 in_w0, in_h0, out_w0, out_h0;
	__s32 ch0_hstep, ch0_vstep;

	in_w0 = in_size->scal_width;
	in_h0 = in_size->scal_height;

	out_w0 = out_size->width;
	out_h0 = out_size->height;

	/* added no-zero limited */
	in_h0 = (in_h0 != 0) ? in_h0 : 1;
	in_w0 = (in_w0 != 0) ? in_w0 : 1;
	out_h0 = (out_h0 != 0) ? out_h0 : 1;
	out_w0 = (out_w0 != 0) ? out_w0 : 1;

	/* step factor */
	ch0_hstep = (in_w0<<16)/out_w0;
	ch0_vstep = (in_h0<<16)/out_h0;

		di_dev->ch0_horzfact.dwval = ch0_hstep;
	di_dev->ch0_vertfact.dwval = ch0_vstep;
	di_dev->ch1_horzfact.dwval = ch0_hstep;
	di_dev->ch1_vertfact.dwval = ch0_vstep;

	return 0;
}

__s32 DI_Set_Scaling_Coef(__di_src_size_t *in_size, __di_out_size_t *out_size,
			__di_src_type_t *in_type,  __di_out_type_t *out_type)
{
	__s32 in_w0, in_h0, out_w0, out_h0;
	__u32 int_part, float_part;
	__u32 zoom0_size, zoom1_size, zoom2_size, zoom3_size, zoom4_size;
	__u32 zoom5_size, al1_size;
	__u32 ch0h_sc, ch0v_sc;
	__u32 ch0v_fir_coef_addr, ch0h_fir_coef_addr;
	__u32 ch1v_fir_coef_addr, ch1h_fir_coef_addr;
	__u32 ch0v_fir_coef_ofst, ch0h_fir_coef_ofst;
#if defined CONFIG_ARCH_SUN9IW1
	__u32 ch3h_fir_coef_addr, ch3v_fir_coef_addr;
#else
	__u32 loop_count = 0;
#endif
#if defined SCALE_NO_SUPPORT
	__s32 i;
#endif
	in_w0 = in_size->scal_width;
	in_h0 = in_size->scal_height;
	out_w0 = out_size->width;
	out_h0 = out_size->height;

	zoom0_size = 1;
	zoom1_size = 2;
	zoom2_size = 2;
	zoom3_size = 1;
	zoom4_size = 1;
	zoom5_size = 1;
	al1_size = zoom0_size + zoom1_size + zoom2_size + zoom3_size
		+ zoom4_size + zoom5_size;

	/* added no-zero limited */
	in_h0 = (in_h0 != 0) ? in_h0 : 1;
	in_w0 = (in_w0 != 0) ? in_w0 : 1;
	out_h0 = (out_h0 != 0) ? out_h0 : 1;
	out_w0 = (out_w0 != 0) ? out_w0 : 1;

	ch0h_sc = (in_w0<<1)/out_w0;
	ch0v_sc = (in_h0<<1)/out_h0;

	/* comput the fir coefficient offset in coefficient table */
	int_part = ch0h_sc>>1;
	float_part = ch0h_sc & 0x1;

	ch0h_fir_coef_ofst = (int_part == 0)  ? zoom0_size :
		 (int_part == 1)  ? zoom0_size + float_part :
		(int_part == 2)  ? zoom0_size + zoom1_size + float_part :
		(int_part == 3)  ? zoom0_size + zoom1_size + zoom2_size :
	(int_part == 4)  ? zoom0_size + zoom1_size + zoom2_size + zoom3_size :
		zoom0_size + zoom1_size + zoom2_size + zoom3_size + zoom4_size;

	int_part = ch0v_sc>>1;
	float_part = ch0v_sc & 0x1;
	ch0v_fir_coef_ofst = (int_part == 0)  ? zoom0_size :
		(int_part == 1)  ? zoom0_size + float_part :
		(int_part == 2)  ? zoom0_size + zoom1_size + float_part :
		(int_part == 3)  ? zoom0_size + zoom1_size + zoom2_size :
	(int_part == 4)  ? zoom0_size + zoom1_size + zoom2_size + zoom3_size :
		zoom0_size + zoom1_size + zoom2_size + zoom3_size + zoom4_size;

#if defined CONFIG_ARCH_SUN9IW1
	/* compute the fir coeficient address for each channel in horizontal
	 * and vertical direction
	 */
	ch0h_fir_coef_addr = ch0h_fir_coef_ofst<<5;
	ch0v_fir_coef_addr = ch0v_fir_coef_ofst<<5;
	ch1h_fir_coef_addr = ch0h_fir_coef_ofst<<5;
	ch1v_fir_coef_addr = ch0v_fir_coef_ofst<<5;
	ch3h_fir_coef_addr = ch0h_fir_coef_addr<<5;
	ch3v_fir_coef_addr = ch0v_fir_coef_addr<<5;

	memcpy(&di_dev->ch0_horzcoef0,
			lan3coefftab32_left + ch0h_fir_coef_addr, 256);
	memcpy(&di_dev->ch0_horzcoef1,
			lan3coefftab32_right + ch0h_fir_coef_addr, 256);
	memcpy(&di_dev->ch0_vertcoef,
			lan2coefftab32 + ch0v_fir_coef_addr, 256);

	if ((out_type->fmt == DI_OUTUVCYUV420) ||
			(in_type->fmt == DI_INYUV420)) {
		memcpy(&di_dev->ch1_horzcoef0,
			bicubic8coefftab32_left + ch1h_fir_coef_addr, 256);
		memcpy(&di_dev->ch1_horzcoef1,
			bicubic8coefftab32_right + ch1h_fir_coef_addr, 256);
		memcpy(&di_dev->ch1_vertcoef,
			bicubic4coefftab32 + ch1v_fir_coef_addr, 256);
	} else {
		memcpy(&di_dev->ch1_horzcoef0,
			lan3coefftab32_left + ch1h_fir_coef_addr, 256);
		memcpy(&di_dev->ch1_horzcoef1,
			lan3coefftab32_right + ch1h_fir_coef_addr, 256);
		memcpy(&di_dev->ch1_vertcoef,
			lan2coefftab32 + ch1v_fir_coef_addr, 256);
	}

	if (out_type->alpha_en && (in_type->mod == DI_INTERLEAVED)) {
		memcpy(&di_dev->ch3_horzcoef0,
			bicubic8coefftab32_left + ch3h_fir_coef_addr, 256);
		memcpy(&di_dev->ch3_horzcoef1,
			bicubic8coefftab32_right + ch3h_fir_coef_addr, 256);
		memcpy(&di_dev->ch3_vertcoef,
			bicubic4coefftab32 + ch3v_fir_coef_addr, 256);
	}

	di_dev->frm_ctrl.bits.coef_rdy_en = 0x1;
#else
	/* for  single buffer */
	ch0h_fir_coef_addr = (ch0h_fir_coef_ofst<<5);
	ch0v_fir_coef_addr = (ch0v_fir_coef_ofst<<5);
	ch1h_fir_coef_addr = (ch0h_fir_coef_ofst<<5);
	ch1v_fir_coef_addr = (ch0v_fir_coef_ofst<<5);

	di_dev->frm_ctrl.bits.coef_access_ctrl = 1;
	while ((di_dev->status.bits.coef_access_status == 0) &&
			(loop_count < 40)) {
		loop_count++;
	}

#if defined SCALE_NO_SUPPORT
	for (i = 0; i < 32; i++) {
		di_dev->ch0_horzcoef0[i].dwval = 0x00004000;
		di_dev->ch0_vertcoef[i].dwval  = 0x00004000;
		di_dev->ch1_horzcoef0[i].dwval = 0x00004000;
		di_dev->ch1_vertcoef[i].dwval  = 0x00004000;
	}
#else
	memcpy(&di_dev->ch0_horzcoef0,
		lan2coefftab32 + ch0h_fir_coef_addr, 128);
	memcpy(&di_dev->ch0_vertcoef,
		lan2coefftab32 + ch0v_fir_coef_addr, 128);

	if ((out_type->fmt == DI_OUTUVCYUV420) ||
		(in_type->fmt == DI_INYUV420)) {
		memcpy(&di_dev->ch1_horzcoef0,
			bicubic4coefftab32 + ch1h_fir_coef_addr, 128);
		memcpy(&di_dev->ch1_vertcoef,
			bicubic4coefftab32 + ch1v_fir_coef_addr, 128);

		if (di_dev->ch1_horzcoef0[0].dwval != 0xfd0d290d) {
			pr_warn("DIDIDIDIDIDI wrong! di_dev->ch1_horzcoef0[0] = 0x%x.\n  bicubic4coefftab32[64] = 0x%x, ch1h_fir_coef_addr = %d.\n",
				di_dev->ch1_horzcoef0[0].dwval,
				bicubic4coefftab32[64], ch1h_fir_coef_addr);
		}
	} else {
		memcpy(&di_dev->ch1_horzcoef0,
				lan2coefftab32 + ch1h_fir_coef_addr, 128);
		memcpy(&di_dev->ch1_vertcoef,
				lan2coefftab32 + ch1v_fir_coef_addr, 128);
	}

#endif
	di_dev->frm_ctrl.bits.coef_access_ctrl = 0x0;
#endif
	return 0;
}

__s32 DI_Set_Out_Format(__di_out_type_t *out_type)
{
	di_dev->output_fmt.bits.data_fmt = out_type->fmt;
	di_dev->output_fmt.bits.data_ps  = out_type->ps;

	return 0;
}

__s32 DI_Set_Out_Size(__di_out_size_t *out_size)
{
	__u32 out_w1, out_h1, out_w0, out_h0;

	out_h0 = out_size->height;
	out_w0 = out_size->width;
	out_w1 = (out_size->width + 0x1) >> 1;
	out_h1 = (out_size->height + 0x1) >> 1;

	/* added no-zero limited */
	out_h0 = (out_h0 != 0) ? out_h0 : 1;
	out_h1 = (out_h1 != 0) ? out_h1 : 1;
	out_w0 = (out_w0 != 0) ? out_w0 : 1;
	out_w1 = (out_w1 != 0) ? out_w1 : 1;

	di_dev->ch0_outsize.bits.out_height = out_h0 - 1;
	di_dev->ch0_outsize.bits.out_width = out_w0 - 1;
	di_dev->ch1_outsize.bits.out_height = out_h1 - 1;
	di_dev->ch1_outsize.bits.out_width = out_w1 - 1;
	return 0;
}

__s32 DI_Set_Writeback_Addr(__di_buf_addr_t *addr)
{
	di_dev->wb_addr0.dwval = addr->ch0_addr;
	di_dev->wb_addr1.dwval = addr->ch1_addr;
	di_dev->wb_addr2.dwval = addr->ch2_addr;

	return 0;
}

__s32 DI_Set_Writeback_Addr_ex(__di_buf_addr_t *addr, __di_out_size_t *size,
				__di_out_type_t *type)
{
	__u32 image_w0, image_w1;

	image_w0 = size->fb_width;
	image_w1 = (image_w0 + 0x1)>>1;

	if (type->fmt == DI_OUTUVCYUV420) {
		di_dev->wb_linestrd0.dwval = image_w0;
		di_dev->wb_linestrd1.dwval = (image_w1<<1);
		di_dev->wb_linestrd2.dwval = 0;

		/* addr->ch0_addr = addr->ch0_addr;
		addr->ch1_addr = addr->ch1_addr; */
		addr->ch2_addr = 0x0;

		DI_Set_Writeback_Addr(addr);
	}
	return 0;
}


__s32 DI_Set_Di_Ctrl(__u8 en, __u8 mode, __u8 diagintp_en, __u8 tempdiff_en)
{
	di_dev->di_ctrl.bits.en = en;
	di_dev->di_ctrl.bits.flag_out_en = (mode == 3)?0:1;
	di_dev->di_ctrl.bits.mod = mode;
	di_dev->di_ctrl.bits.diagintp_en = diagintp_en;
	di_dev->di_ctrl.bits.tempdiff_en = tempdiff_en;
#if defined CONFIG_ARCH_SUN9IW1
	di_dev->di_spatcomp.bits.th2 = 0;
	di_dev->di_lumath.bits.avglumashifter = 8;
#else
	di_dev->di_lumath.bits.minlumath = 4;
	di_dev->di_spatcomp.bits.th2 = 5;
	di_dev->di_tempdiff.bits.ambiguity_th = 5;
	di_dev->di_diagintp.bits.th0 = 60;
	di_dev->di_diagintp.bits.th1 = 0;
	di_dev->di_diagintp.bits.th3 = 30;
	di_dev->di_chromadiff.bits.chroma_diff_th = 31;
#endif

	return 0;
}


__s32 DI_Set_Di_PreFrame_Addr(__u32 luma_addr, __u32 chroma_addr)
{
	di_dev->di_preluma.dwval = luma_addr;
	di_dev->di_prechroma.dwval = chroma_addr;

	return 0;
}


__s32 DI_Set_Di_MafFlag_Src(__u32 cur_addr, __u32 pre_addr, __u32 stride)
{
	di_dev->di_tileflag0.dwval = pre_addr;
	di_dev->di_tileflag1.dwval = cur_addr;
	di_dev->di_flaglinestrd.dwval = stride;

	return 0;
}

__s32 DI_Set_Di_Field(u32 field)
{
	di_dev->field_ctrl.bits.field_cnt = (field & 0x1);

	return 0;
}
__s32 DI_Set_Reg_Rdy(void)
{
	di_dev->frm_ctrl.bits.reg_rdy_en = 0x1;

	return 0;
}

__s32 DI_Enable(void)
{
	/* di_dev->modl_en.bits.en = 0x1; */
	di_dev->frm_ctrl.bits.frm_start = 0x1;

	return 0;
}

__s32 DI_Module_Enable(void)
{
	di_dev->modl_en.bits.en = 0x1;

	return 0;
}

__s32 DI_Set_Reset(void)
{
	di_dev->frm_ctrl.bits.frm_start = 0x0;
	di_dev->modl_en.bits.en = 0x0;

	return 0;
}


__s32 DI_Set_Irq_Enable(__u32 enable)
{
	di_dev->int_en.bits.wb_en = (enable & 0x1);
	return 0;
}

__s32 DI_Clear_irq(void)
{
	di_dev->int_status.bits.wb_status = 0x1;

	return 0;
}

__s32 DI_Get_Irq_Status(void)
{
	__u32 wb_finish;
	__u32 wb_processing;

	wb_finish = di_dev->int_status.bits.wb_status;
	wb_processing = di_dev->status.bits.wb_status;

	if (wb_processing)
		return 2;
	else if (wb_finish == 0 && wb_processing == 0)
		return 1;
	else if (wb_finish)
		return 0;
	else
		return 3;
}

__s32 DI_Set_Writeback_Start(void)
{
	di_dev->frm_ctrl.bits.wb_en = 0x1;

	return 0;
}

#if defined CONFIG_ARCH_SUN9IW1
__s32 DI_Internal_Set_Clk(__u32 enable)
{
	__u32 reg_val, base;

	base = 0xf3000000; /* FIXME */

	if (enable) {
		reg_val = readl(base + 0x0);
		reg_val |= 0x1;
		writel(reg_val, base + 0x0);

		reg_val = readl(base + 0x4);
		reg_val |= 0x1;
		writel(reg_val, base + 0x4);

		reg_val = readl(base + 0x8);
		reg_val |= 0x1;
		writel(reg_val, base + 0x8);

		reg_val = readl(base + 0xc);
		reg_val |= 0x1;
		writel(reg_val, base + 0xc);
	} else {
		reg_val = readl(base + 0x0);
		reg_val &= 0xfffffffe;
		writel(reg_val, base + 0x0);

		reg_val = readl(base + 0x4);
		reg_val &= 0xfffffffe;
		writel(reg_val, base + 0x4);

		reg_val = readl(base + 0x8);
		reg_val &= 0xfffffffe;
		writel(reg_val, base + 0x8);

		reg_val = readl(base + 0xc);
		reg_val &= 0xfffffffe;
		writel(reg_val, base + 0xc);
	}
	return 0;
}
#else
__s32 DI_Internal_Set_Clk(__u32 enable)
{
	return 0;
}
#endif

