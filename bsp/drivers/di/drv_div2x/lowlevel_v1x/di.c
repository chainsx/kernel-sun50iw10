/*
 * Allwinner SoCs de-interlace driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */


#include "../di.h"
#include "di_ebios.h"
#include <linux/slab.h>

volatile __di_dev_t *di_dev;

/* function: di_set_reg_base(u32 base)
 * description: set di module register base
 * parameters: base <di module AHB memory mapping >
 * return   :
 */
s32 di_set_reg_base(void *base)
{
	di_dev = (__di_dev_t *)base;

	return 0;
}

/* function: di_get_reg_base(void)
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

/* function: di_set_init(void)
 * description: set di module default register to ready de-interlace
 * parameters:
 * return   :
 */
s32 di_set_init(void)
{
	DI_Init();

	return 0;
}

/* function: di_reset(void)
 * description: stop di module
 * parameters:
 * return   :
 */
s32 di_reset(void)
{
	DI_Set_Reset();

	return 0;
}

/* function: di_start(void)
 * description: start a de-interlace function
 * parameters:
 * return   :
 */
s32 di_start(void)
{
	DI_Set_Writeback_Start();

	return 0;
}

/* function: di_irq_enable(u32 enable)
 * description: enable/disable di irq
 * parameters: enable <0-disable; 1-enable>
 * return   :
 */
s32 di_irq_enable(u32 enable)
{
	DI_Set_Irq_Enable(enable);

	return 0;
}

/*
* function: di_get_status(void)
* description: get status of di module
* parameters:
* return  :  <0-Writeback finish; 1-Writeback no start;
*			 2-Writeback-ing; (-1)-Undefined
*/
s32 di_get_status(void)
{
	s32 ret;

	ret = DI_Get_Irq_Status();
	return ret;
}

/* function: di_irq_clear()
 * description: clear irq status
 * parameters:
 * return   :
 */
s32 di_irq_clear(void)
{
	DI_Clear_irq();

	return 0;
}

/* function: di_set_para(__disp_scaler_para_t *para)
 * description: set parameters to ready a de-interlace function
 * parameters: para <parameters which set from ioctrl>
 * in_flag_add/out_flag_add <flag address malloc in driver>
 * field <0 - select even line for source line;
 * 1 - select odd line for source line>
 * return  :   <0 - set OK; 1 - para NULL>
 */
s32 di_set_para(struct __di_para_t2 *para, void *in_flag_add,
		void *out_flag_add, u32 field)
{
	__di_buf_addr_t in_addr;
	__di_buf_addr_t out_addr;
	__di_src_size_t in_size;
	__di_out_size_t out_size;
	__di_src_type_t in_type;
	__di_out_type_t out_type;
	__di_buf_addr_t pre_addr;
	unsigned long in_address = 0;
	unsigned long out_address = 0;
	unsigned long ch0_addr = 0;
	unsigned long ch1_addr = 0;

	if (para == NULL) {
		/* DE_WRN("input parameter can't be null!\n"); */
		return -1;
	}

	in_type.fmt = di_sw_para_to_reg(0, para->input_fb.format);
	in_type.mod = di_sw_para_to_reg(1, para->input_fb.format);
	in_type.ps = di_sw_para_to_reg(2, para->input_fb.format);

	out_type.fmt = di_sw_para_to_reg(3, para->output_fb.format);
	out_type.ps = di_sw_para_to_reg(4, para->output_fb.format);
	out_type.alpha_en = 0;

	out_size.width  = para->out_regn.width;
	out_size.height = para->out_regn.height;
	out_size.fb_width = para->output_fb.size.width;
	out_size.fb_height = para->output_fb.size.height;

	ch0_addr = (unsigned long)(para->input_fb.addr[0]);
	ch1_addr = (unsigned long)(para->input_fb.addr[1]);
	in_addr.ch0_addr = (u32)(ch0_addr);
	in_addr.ch1_addr = (u32)(ch1_addr);
	in_addr.ch0_addr = (u32)DI_VAtoPA(in_addr.ch0_addr);
	in_addr.ch1_addr = (u32)DI_VAtoPA(in_addr.ch1_addr);

	in_size.src_width = para->input_fb.size.width;
	in_size.src_height = para->input_fb.size.height;
	in_size.scal_width = para->source_regn.width;
	in_size.scal_height = para->source_regn.height;

	ch0_addr = (unsigned long)(para->output_fb.addr[0]);
	ch1_addr = (unsigned long)(para->output_fb.addr[1]);
	out_addr.ch0_addr = (u32)(ch0_addr);
	out_addr.ch1_addr = (u32)(ch1_addr);

	ch0_addr = (unsigned long)(para->pre_fb.addr[0]);
	ch1_addr = (unsigned long)(para->pre_fb.addr[1]);
	pre_addr.ch0_addr = (u32)(ch0_addr);
	pre_addr.ch1_addr = (u32)(ch1_addr);

	out_addr.ch0_addr = DI_VAtoPA(out_addr.ch0_addr);
	out_addr.ch1_addr = DI_VAtoPA(out_addr.ch1_addr);

	pre_addr.ch0_addr = DI_VAtoPA(pre_addr.ch0_addr);
	pre_addr.ch1_addr = DI_VAtoPA(pre_addr.ch1_addr);

	DI_Module_Enable();

	DI_Config_Src(&in_addr, &in_size, &in_type);
	DI_Set_Scaling_Factor(&in_size, &out_size);
	DI_Set_Scaling_Coef(&in_size, &out_size, &in_type, &out_type);
	DI_Set_Out_Format(&out_type);
	DI_Set_Out_Size(&out_size);
	DI_Set_Writeback_Addr_ex(&out_addr, &out_size, &out_type);

	DI_Set_Di_PreFrame_Addr(pre_addr.ch0_addr, pre_addr.ch1_addr);
	in_address = (unsigned long)(in_flag_add);
	out_address = (unsigned long)(out_flag_add);
	in_address = DI_VAtoPA(in_address);
	out_address = DI_VAtoPA(out_address);
	DI_Set_Di_MafFlag_Src((u32)(in_address), (u32)(out_address), 0x200);
	DI_Set_Di_Field(field);

	DI_Enable();
	DI_Set_Reg_Rdy();

	return 0;
}

/* 0:deinterlace input pixel format
 * 1:deinterlace input yuv mode
 * 2:deinterlace input pixel sequence
 * 3:deinterlace output format
 * 4:deinterlace output pixel sequence
 */
s32 di_sw_para_to_reg(u8 type, u8 format)
{
	/* deinterlace input  pixel format */
	if (type == 0)	{
		if (format <= DI_FORMAT_MB32_21)
			return DI_INYUV420;
		else {
			/* DE_INF("not supported de-interlace input pixel
			 * format:%d in di_sw_para_to_reg\n",format);
			 */
		}
	}
	/* deinterlace input mode */
	else if (type == 1) {
		if ((format == DI_FORMAT_MB32_21) ||
				(format == DI_FORMAT_MB32_12))
			return DI_UVCOMBINEDMB;
		else if ((format == DI_FORMAT_NV21) ||
				(format == DI_FORMAT_NV12))
			return DI_UVCOMBINED;
		else {
			/* DE_INF("not supported de-interlace input mode:%d
			 * in di_sw_para_to_reg\n", format);
			 */
		}
	}
	/* deinterlace input pixel sequence */
	else if (type == 2) {
		if ((format == DI_FORMAT_MB32_12) ||
				(format == DI_FORMAT_NV12))
			return DI_UVUV;
		else if ((format == DI_FORMAT_MB32_21) ||
				(format == DI_FORMAT_NV21))
			return DI_VUVU;
		else {
			/* DE_INF("not supported de-interlace input
			 * pixel sequence:%d in di_sw_para_to_reg\n",format);
			 */
		}
	}
	/* deinterlace output format */
	else if (type == 3) {
		if ((format == DI_FORMAT_NV12) || (format == DI_FORMAT_NV21))
			return DI_OUTUVCYUV420;
		else {
			/* DE_INF("not supported de-interlace output format :%d
			 * in di_sw_para_to_reg\n", format);
			 */
		}
	}
	/* deinterlace output pixel sequence */
	else if (type == 4) {
		if (format == DI_FORMAT_NV12)
			return DI_UVUV;
		else if (format == DI_FORMAT_NV21)
			return DI_VUVU;
		else {
			/* DE_INF("not supported de-interlace output pixel
			 * sequence:%d in di_sw_para_to_reg\n", format);
			 */
		}
	}
	/* DE_INF("not supported type:%d in di_sw_para_to_reg\n", type); */
	return -1;
}


s32 di_internal_clk_enable(void)
{
	return DI_Internal_Set_Clk(1);
}

s32 di_internal_clk_disable(void)
{
	return DI_Internal_Set_Clk(0);
}
