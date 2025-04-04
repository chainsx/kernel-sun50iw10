/*
 * linux-4.9/drivers/media/platform/sunxi-vfe/csi/bsp_csi.c
 *
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/*
 * sunxi csi bsp interface
 * Author:raymonxiu
 */

#include "bsp_csi.h"

#define CHANNEL_OUTPUT_COLUMN

static struct frame_arrange frm_arrange_gbl[MAX_CSI];
static unsigned int line_stride_y_ch_gbl[MAX_CSI][MAX_CH_NUM];
static unsigned int line_stride_c_ch_gbl[MAX_CSI][MAX_CH_NUM];
static unsigned int buf_height_y_ch_gbl[MAX_CSI][MAX_CH_NUM];
static unsigned int buf_height_cb_ch_gbl[MAX_CSI][MAX_CH_NUM];
static unsigned int buf_height_cr_ch_gbl[MAX_CSI][MAX_CH_NUM];
static unsigned int line_stride_y_row_gbl[MAX_CSI][MAX_CH_NUM];
static unsigned int line_stride_c_row_gbl[MAX_CSI][MAX_CH_NUM];


int bsp_csi_set_base_addr(unsigned int sel, unsigned long addr)
{
	return csi_set_base_addr(sel, addr);
}

void bsp_csi_enable(unsigned int sel)
{
	csi_enable(sel);
}

void bsp_csi_disable(unsigned int sel)
{
	csi_disable(sel);
}

void bsp_csi_reset(unsigned int sel)
{
	csi_disable(sel);
	csi_enable(sel);
}


/* bsp_csi_set_fmt
 * function:
 * set csi timing/format/size, return 0(ok) or -1(error)
 *
 * struct bus_info
 * {
 *  enum   bus_if       bus_if;
 *  struct  bus_timing      bus_tmg;
 *  enum   bus_pixelcode    bus_ch_fmt[MAX_CH_NUM];
 *  unsigned int        ch_total_num;
 * };
 *
 * struct frame_info
 * {
 *   struct frame_arrange  arrange;
 *   struct frame_size     ch_size[MAX_CH_NUM];
 *   struct frame_offset   ch_offset[MAX_CH_NUM];
 *   enum pixel_fmt    pix_ch_fmt[MAX_CH_NUM];
 *   enum field      ch_field[MAX_CH_NUM];
 *   unsigned int      frm_byte_size;
 * };
 *
 * input parameters:
 * bus_if,
 * bus_tmg,
 * bus_ch_fmt,
 * ch_total_num,
 * pix_ch_fmt,
 * ch_field,
 *
 * output parameters:
 * none
 */

int bsp_csi_set_fmt(unsigned int sel, struct bus_info *bus_info, struct frame_info *frame_info)
{
	struct csi_if_cfg if_cfg;
	struct csi_timing_cfg tmg_cfg;
	struct csi_fmt_cfg fmt_cfg[MAX_CH_NUM];
	unsigned int is_buf_itl[MAX_CH_NUM];
	enum bus_pixeltype bus_pix_type[MAX_CH_NUM];
	enum bit_width bus_width[MAX_CH_NUM];
	enum bit_width bus_precision[MAX_CH_NUM];
	unsigned int ch;

	/* csi_disable(sel);//add for csi change fmt@20130111
	set csi interface
	depends on bus interface */
	switch (bus_info->bus_if) {
	case PARALLEL:
		if_cfg.interface = CSI_IF_INTLV;
		break;
	case BT656:
#if defined CONFIG_ARCH_SUN8IW11
		if (bus_info->ch_total_num == 1) {
			if (find_bus_width(bus_info->bus_ch_fmt[0]) == W_16BIT)
				if_cfg.interface = CSI_IF_CCIR656_16BIT;
			else
				if_cfg.interface = CSI_IF_CCIR656_1CH;
		} else if (bus_info->ch_total_num == 2)
			if_cfg.interface = CSI_IF_CCIR656_2CH;
		else if (bus_info->ch_total_num == 4)
			if_cfg.interface = CSI_IF_CCIR656_4CH;
#else
		if_cfg.interface = CSI_IF_CCIR656_1CH;
#endif
		break;
	case CSI2:
		if_cfg.interface = CSI_IF_MIPI;
		break;
	default:
		return -1;
	}

	for (ch = 0; ch < bus_info->ch_total_num; ch++) {
		/* get bus pixel type, bus width and bus data precision
		depends on bus format */
		bus_pix_type[ch] = find_bus_type(bus_info->bus_ch_fmt[ch]);
		bus_width[ch] = find_bus_width(bus_info->bus_ch_fmt[ch]);
		bus_precision[ch] = find_bus_precision(bus_info->bus_ch_fmt[ch]);

		if (if_cfg.interface != CSI_IF_MIPI)
			if_cfg.data_width = bus_width[ch];

		/*set csi field info
		depends on field format */
		switch (frame_info->ch_field[ch]) {
		case FIELD_ANY:
		case FIELD_NONE:
			if_cfg.src_type = CSI_PROGRESSIVE;
			fmt_cfg[ch].field_sel = CSI_EITHER;
			break;
		case FIELD_TOP:
			if_cfg.src_type = CSI_INTERLACE;
			fmt_cfg[ch].field_sel = CSI_ODD;
			break;
		case FIELD_BOTTOM:
			if_cfg.src_type = CSI_INTERLACE;
			fmt_cfg[ch].field_sel = CSI_EVEN;
			break;
		case FIELD_INTERLACED:
		case FIELD_INTERLACED_TB:
			if_cfg.src_type = CSI_INTERLACE;
			tmg_cfg.field = CSI_FIELD_TF;
			fmt_cfg[ch].field_sel = CSI_EITHER;
			break;
		case FIELD_INTERLACED_BT:
			if_cfg.src_type = CSI_INTERLACE;
			tmg_cfg.field = CSI_FIELD_BF;
			fmt_cfg[ch].field_sel = CSI_EITHER;
			break;
		default:
			return -1;
		}

		/* if the target frame buffer is interlaced
		depends on field format */
		if (frame_info->ch_field[ch] == FIELD_INTERLACED ||
		  frame_info->ch_field[ch] == FIELD_INTERLACED_TB ||
		  frame_info->ch_field[ch] == FIELD_INTERLACED_BT)
			is_buf_itl[ch] = 1;
		else
			is_buf_itl[ch] = 0;

		/* set input/output format and size/line stride/offset
		depends on bus format, bus precision, target frame format, field format */
		switch (frame_info->pix_ch_fmt[ch]) {
		case PIX_FMT_RGB565:
			if (bus_pix_type[ch] == BUS_FMT_RGB565) {
				fmt_cfg[ch].input_fmt = CSI_RAW;
				fmt_cfg[ch].output_fmt = (is_buf_itl[ch] == 1) ? CSI_FRAME_RGB565:CSI_FIELD_RGB565;
			} else
				return -1;
			break;
		case PIX_FMT_RGB888:
			if (bus_pix_type[ch] == BUS_FMT_RGB888) {
				fmt_cfg[ch].input_fmt = CSI_RAW;
				fmt_cfg[ch].output_fmt = (is_buf_itl[ch] == 1) ? CSI_FRAME_RGB888:CSI_FIELD_RGB888;
			} else
				return -1;

			break;
		case PIX_FMT_PRGB888:
			if (bus_pix_type[ch] == BUS_FMT_RGB888) {
				fmt_cfg[ch].input_fmt = CSI_RAW;
				fmt_cfg[ch].output_fmt = (is_buf_itl[ch] == 1) ? CSI_FRAME_PRGB888:CSI_FIELD_PRGB888;
			} else
				return -1;

			break;
		case PIX_FMT_YUYV:
			if (bus_pix_type[ch] == BUS_FMT_YUYV) {
				fmt_cfg[ch].input_fmt = CSI_RAW;
				fmt_cfg[ch].output_fmt = (is_buf_itl[ch] == 1) ? CSI_FRAME_RAW_8:CSI_FIELD_RAW_8;
			} else
				return -1;

			break;
		case PIX_FMT_YVYU:
			if (bus_pix_type[ch] == BUS_FMT_YVYU) {
				fmt_cfg[ch].input_fmt = CSI_RAW;
				fmt_cfg[ch].output_fmt = (is_buf_itl[ch] == 1) ? CSI_FRAME_RAW_8:CSI_FIELD_RAW_8;
			} else
				return -1;

			break;
		case PIX_FMT_UYVY:
			if (bus_pix_type[ch] == BUS_FMT_UYVY) {
				fmt_cfg[ch].input_fmt = CSI_RAW;
				fmt_cfg[ch].output_fmt = (is_buf_itl[ch] == 1) ? CSI_FRAME_RAW_8:CSI_FIELD_RAW_8;
			} else
				return -1;

			break;
		case PIX_FMT_VYUY:
			if (bus_pix_type[ch] == BUS_FMT_VYUY) {
				fmt_cfg[ch].input_fmt = CSI_RAW;
				fmt_cfg[ch].output_fmt = (is_buf_itl[ch] == 1) ? CSI_FRAME_RAW_8:CSI_FIELD_RAW_8;
			} else
				return -1;

			break;
		case PIX_FMT_YUV420P_8:
		case PIX_FMT_YVU420P_8:
		case PIX_FMT_YUV420SP_8:
		case PIX_FMT_YVU420SP_8:
		case PIX_FMT_YUV420MB_8:
		case PIX_FMT_YVU420MB_8:
			if (bus_pix_type[ch] == BUS_FMT_YUYV ||
					bus_pix_type[ch] == BUS_FMT_YVYU ||
					bus_pix_type[ch] == BUS_FMT_UYVY ||
					bus_pix_type[ch] == BUS_FMT_VYUY)
				fmt_cfg[ch].input_fmt = CSI_YUV422;
			else if (bus_pix_type[ch] == BUS_FMT_YY_YUYV ||
					bus_pix_type[ch] == BUS_FMT_YY_YVYU ||
					bus_pix_type[ch] == BUS_FMT_YY_UYVY ||
					bus_pix_type[ch] == BUS_FMT_YY_VYUY)
				fmt_cfg[ch].input_fmt = CSI_YUV420;
			else if (bus_pix_type[ch] == BUS_FMT_SBGGR ||
					bus_pix_type[ch] == BUS_FMT_SGBRG ||
					bus_pix_type[ch] == BUS_FMT_SRGGB ||
					bus_pix_type[ch] == BUS_FMT_SGRBG)
				fmt_cfg[ch].input_fmt = CSI_RAW;  /* parse to isp */
			if (fmt_cfg[ch].input_fmt == CSI_YUV422 ||
					fmt_cfg[ch].input_fmt == CSI_YUV420) {
				if (frame_info->pix_ch_fmt[ch] == PIX_FMT_YUV420P_8 ||
						frame_info->pix_ch_fmt[ch] == PIX_FMT_YVU420P_8)
					fmt_cfg[ch].output_fmt = (is_buf_itl[ch] == 1) ? CSI_FRAME_PLANAR_YUV420:CSI_FIELD_PLANAR_YUV420;
				else if (frame_info->pix_ch_fmt[ch] == PIX_FMT_YUV420SP_8 ||
						frame_info->pix_ch_fmt[ch] == PIX_FMT_YVU420SP_8)
					fmt_cfg[ch].output_fmt = (is_buf_itl[ch] == 1) ? CSI_FRAME_UV_CB_YUV420:CSI_FIELD_UV_CB_YUV420;
				else if (frame_info->pix_ch_fmt[ch] == PIX_FMT_YUV420MB_8 ||
						frame_info->pix_ch_fmt[ch] == PIX_FMT_YVU420MB_8)
					fmt_cfg[ch].output_fmt = (is_buf_itl[ch] == 1) ? CSI_FRAME_MB_YUV420:CSI_FIELD_MB_YUV420;
			} else {
				if (bus_precision[ch] == W_8BIT)
					fmt_cfg[ch].output_fmt = (is_buf_itl[ch] == 1) ? CSI_FRAME_RAW_8:CSI_FIELD_RAW_8;
				else if (bus_precision[ch] == W_10BIT)
					fmt_cfg[ch].output_fmt = (is_buf_itl[ch] == 1) ? CSI_FRAME_RAW_10:CSI_FIELD_RAW_10;
				else if (bus_precision[ch] == W_12BIT)
					fmt_cfg[ch].output_fmt = (is_buf_itl[ch] == 1) ? CSI_FRAME_RAW_12:CSI_FIELD_RAW_12;
			}
			break;
		case PIX_FMT_YUV422P_8:
		case PIX_FMT_YVU422P_8:
		case PIX_FMT_YUV422SP_8:
		case PIX_FMT_YVU422SP_8:
		case PIX_FMT_YUV422MB_8:
		case PIX_FMT_YVU422MB_8:
			if (bus_pix_type[ch] == BUS_FMT_YUYV ||
					bus_pix_type[ch] == BUS_FMT_YVYU ||
					bus_pix_type[ch] == BUS_FMT_UYVY ||
					bus_pix_type[ch] == BUS_FMT_VYUY)
				fmt_cfg[ch].input_fmt = CSI_YUV422;
			else if (bus_pix_type[ch] == BUS_FMT_SBGGR ||
					bus_pix_type[ch] == BUS_FMT_SGBRG ||
					bus_pix_type[ch] == BUS_FMT_SRGGB ||
					bus_pix_type[ch] == BUS_FMT_SGRBG)
				fmt_cfg[ch].input_fmt = CSI_RAW;  /* parse to isp */
			if (fmt_cfg[ch].input_fmt == CSI_YUV422) {
				if (frame_info->pix_ch_fmt[ch] == PIX_FMT_YUV422P_8 || frame_info->pix_ch_fmt[ch] == PIX_FMT_YVU422P_8)
					fmt_cfg[ch].output_fmt = (is_buf_itl[ch] == 1) ? CSI_FRAME_PLANAR_YUV422:CSI_FIELD_PLANAR_YUV422;
				else if (frame_info->pix_ch_fmt[ch] == PIX_FMT_YUV422SP_8 || frame_info->pix_ch_fmt[ch] == PIX_FMT_YVU422SP_8)
					fmt_cfg[ch].output_fmt = (is_buf_itl[ch] == 1) ? CSI_FRAME_UV_CB_YUV422:CSI_FIELD_UV_CB_YUV422;
				else if (frame_info->pix_ch_fmt[ch] == PIX_FMT_YUV422MB_8 || frame_info->pix_ch_fmt[ch] == PIX_FMT_YVU422MB_8)
					fmt_cfg[ch].output_fmt = (is_buf_itl[ch] == 1) ? CSI_FRAME_MB_YUV422:CSI_FIELD_MB_YUV422;
			} else {
				if (bus_precision[ch] == W_8BIT)
					fmt_cfg[ch].output_fmt = (is_buf_itl[ch] == 1) ? CSI_FRAME_RAW_8:CSI_FIELD_RAW_8;
				else if (bus_precision[ch] == W_10BIT)
					fmt_cfg[ch].output_fmt = (is_buf_itl[ch] == 1) ? CSI_FRAME_RAW_10:CSI_FIELD_RAW_10;
				else if (bus_precision[ch] == W_12BIT)
					fmt_cfg[ch].output_fmt = (is_buf_itl[ch] == 1) ? CSI_FRAME_RAW_12:CSI_FIELD_RAW_12;
			}
			break;
		case PIX_FMT_YUV420SP_10:/* for debug */
		case PIX_FMT_YVU420SP_10:
			fmt_cfg[ch].input_fmt = CSI_YUV420;
			fmt_cfg[ch].output_fmt = CSI_FIELD_UV_CB_YUV420_10;
			break;
		case PIX_FMT_YUV422SP_10:/* for debug */
		case PIX_FMT_YVU422SP_10:
			fmt_cfg[ch].input_fmt = CSI_YUV422;
			fmt_cfg[ch].output_fmt = CSI_FIELD_UV_CB_YUV422_10;
			break;
		case PIX_FMT_SBGGR_8:/* all below are for debug */
		case PIX_FMT_SGBRG_8:
		case PIX_FMT_SGRBG_8:
		case PIX_FMT_SRGGB_8:
		case PIX_FMT_SBGGR_10:
		case PIX_FMT_SGBRG_10:
		case PIX_FMT_SGRBG_10:
		case PIX_FMT_SRGGB_10:
		case PIX_FMT_SBGGR_12:
		case PIX_FMT_SGBRG_12:
		case PIX_FMT_SGRBG_12:
		case PIX_FMT_SRGGB_12:
			fmt_cfg[ch].input_fmt = CSI_RAW;
			if (bus_precision[ch] == W_8BIT)
				fmt_cfg[ch].output_fmt = (is_buf_itl[ch] == 1) ? CSI_FRAME_RAW_8:CSI_FIELD_RAW_8;
			else if (bus_precision[ch] == W_10BIT)
				fmt_cfg[ch].output_fmt = (is_buf_itl[ch] == 1) ? CSI_FRAME_RAW_10:CSI_FIELD_RAW_10;
			else if (bus_precision[ch] == W_12BIT)
				fmt_cfg[ch].output_fmt = (is_buf_itl[ch] == 1) ? CSI_FRAME_RAW_12:CSI_FIELD_RAW_12;
			break;
		default:
			return -1;
		}

		/* change input sequence
		depends on bus format and target frame format */
		switch (frame_info->pix_ch_fmt[ch]) {
		case PIX_FMT_YUV420P_8:
		case PIX_FMT_YUV420SP_8:
		case PIX_FMT_YUV420MB_8:
		case PIX_FMT_YUV422P_8:
		case PIX_FMT_YUV422SP_8:
		case PIX_FMT_YUV422MB_8:
		case PIX_FMT_YUV420SP_10:
		case PIX_FMT_YUV422SP_10:
			if (bus_pix_type[ch] == BUS_FMT_YUYV)
				fmt_cfg[ch].input_seq = CSI_YUYV;
			else if (bus_pix_type[ch] == BUS_FMT_YVYU)
				fmt_cfg[ch].input_seq = CSI_YVYU;
			else if (bus_pix_type[ch] == BUS_FMT_UYVY)
				fmt_cfg[ch].input_seq = CSI_UYVY;
			else if (bus_pix_type[ch] == BUS_FMT_VYUY)
				fmt_cfg[ch].input_seq = CSI_VYUY;
			break;
		case PIX_FMT_YVU420P_8:
		case PIX_FMT_YVU420SP_8:
		case PIX_FMT_YVU420MB_8:
		case PIX_FMT_YVU422P_8:
		case PIX_FMT_YVU422SP_8:
		case PIX_FMT_YVU422MB_8:
		case PIX_FMT_YVU420SP_10:
		case PIX_FMT_YVU422SP_10:
			if (bus_pix_type[ch] == BUS_FMT_YUYV)
				fmt_cfg[ch].input_seq = CSI_YVYU;
			else if (bus_pix_type[ch] == BUS_FMT_YVYU)
				fmt_cfg[ch].input_seq = CSI_YUYV;
			else if (bus_pix_type[ch] == BUS_FMT_UYVY)
				fmt_cfg[ch].input_seq = CSI_VYUY;
			else if (bus_pix_type[ch] == BUS_FMT_VYUY)
				fmt_cfg[ch].input_seq = CSI_UYVY;
			break;
		default:
			fmt_cfg[ch].input_seq = CSI_UYVY;
			break;
		}
		csi_fmt_cfg(sel, ch, &fmt_cfg[ch]);
	}
	csi_if_cfg(sel, &if_cfg);
	/* set csi timing parameter */
	tmg_cfg.href = bus_info->bus_tmg.href_pol;
	tmg_cfg.vref = bus_info->bus_tmg.vref_pol;
	tmg_cfg.sample = bus_info->bus_tmg.pclk_sample;
	tmg_cfg.field = 0; /* todo for 16bits. */
	csi_timing_cfg(sel, &tmg_cfg);
	/* csi_enable(sel);//add for csi change fmt@20130111 */
	return 0;
}


/* bsp_csi_set_size
 * function:
 * set csi timing/format/size, return 0(ok) or -1(error)
 *
 * struct bus_info
 * {
 *  enum   bus_if           bus_if;
 *  struct  bus_timing      bus_tmg;
 *  enum   bus_pixelcode    bus_ch_fmt[MAX_CH_NUM];
 *  unsigned int            ch_total_num;
 * };
 *
 * struct frame_info
 * {
 *   struct frame_arrange  arrange;
 *   struct frame_size     ch_size[MAX_CH_NUM];
 *   struct frame_offset   ch_offset[MAX_CH_NUM];
 *   enum pixel_fmt        pix_ch_fmt[MAX_CH_NUM];
 *   enum field            ch_field[MAX_CH_NUM];
 *   unsigned int          frm_byte_size;
 * };
 *
 * input parameters:
 * bus_ch_fmt,
 * ch_total_num,
 * arrange,
 * ch_size,
 * ch_offset,
 * pix_ch_fmt,
 * ch_field,
 *
 * output parameters:
 * frm_byte_size;
 */

int bsp_csi_set_size(unsigned int sel, struct bus_info *bus_info, struct frame_info *frame_info)
{
	enum bit_width bus_width[MAX_CH_NUM];
	enum bit_width bus_precision[MAX_CH_NUM];
	unsigned int is_buf_itl[MAX_CH_NUM];
	unsigned int input_len_h[MAX_CH_NUM], input_len_v[MAX_CH_NUM];
	unsigned int start_h[MAX_CH_NUM], start_v[MAX_CH_NUM];
	unsigned int buf_height_y_ch[MAX_CH_NUM], buf_height_cb_ch[MAX_CH_NUM], buf_height_cr_ch[MAX_CH_NUM];
	unsigned int line_stride_y_ch[MAX_CH_NUM], line_stride_c_ch[MAX_CH_NUM];
	unsigned int line_stride_y_row[MAX_CH_NUM], line_stride_c_row[MAX_CH_NUM];
	unsigned int ch, i, j, row, column;

#ifdef CHANNEL_OUTPUT_COLUMN
	frame_info->arrange.row = bus_info->ch_total_num;
	frame_info->arrange.column = 1;
#endif

	row = frame_info->arrange.row;
	column = frame_info->arrange.column;
	frm_arrange_gbl[sel].row = row;
	frm_arrange_gbl[sel].column = column;

	for (ch = 0; ch < bus_info->ch_total_num; ch++) {
		/* if the target frame buffer is interlaced
		depends on field format */
		if (frame_info->ch_field[ch] == FIELD_INTERLACED ||
		       frame_info->ch_field[ch] == FIELD_INTERLACED_TB ||
		       frame_info->ch_field[ch] == FIELD_INTERLACED_BT)
			is_buf_itl[ch] = 1;
		else
			is_buf_itl[ch] = 0;

		bus_width[ch] = find_bus_width(bus_info->bus_ch_fmt[ch]);
		bus_precision[ch] = find_bus_precision(bus_info->bus_ch_fmt[ch]);

		/* common initial value */
		buf_height_cb_ch[ch] = 0;
		buf_height_cr_ch[ch] = 0;
		input_len_h[ch] = frame_info->ch_size[ch].width;
		input_len_v[ch] = frame_info->ch_size[ch].height >> ((is_buf_itl[ch] == 1) ? 1:0);
		start_h[ch] = frame_info->ch_offset[ch].hoff;
		start_v[ch] = frame_info->ch_offset[ch].voff;

		switch (frame_info->pix_ch_fmt[ch]) {
		case PIX_FMT_RGB565:
			line_stride_y_ch[ch] = frame_info->ch_size[ch].width << 1;
			line_stride_y_ch[ch] = frame_info->ch_size[ch].width << 1;
			line_stride_c_ch[ch] = 0;
			buf_height_y_ch[ch] = frame_info->ch_size[ch].height;
			break;
		case PIX_FMT_RGB888:
			line_stride_y_ch[ch] = frame_info->ch_size[ch].width * 3;
			line_stride_c_ch[ch] = 0;
			buf_height_y_ch[ch] = frame_info->ch_size[ch].height;
			break;
		case PIX_FMT_PRGB888:
			line_stride_y_ch[ch] = frame_info->ch_size[ch].width << 2;
			line_stride_c_ch[ch] = 0;
			buf_height_y_ch[ch] = frame_info->ch_size[ch].height;
			break;
		case PIX_FMT_YUYV:
		case PIX_FMT_YVYU:
		case PIX_FMT_UYVY:
		case PIX_FMT_VYUY:
			input_len_h[ch] = frame_info->ch_size[ch].width << 1;
			start_h[ch] = frame_info->ch_offset[ch].hoff >> 1 << 3;
			start_v[ch] = frame_info->ch_offset[ch].voff >> 1 << 3;
			line_stride_y_ch[ch] = frame_info->ch_size[ch].width << 1;
			line_stride_c_ch[ch] = 0;
			buf_height_y_ch[ch] = frame_info->ch_size[ch].height;
			break;
		case PIX_FMT_YUV420P_8:
		case PIX_FMT_YVU420P_8:
			line_stride_y_ch[ch] = CSI_ALIGN_16B(frame_info->ch_size[ch].width);
			line_stride_c_ch[ch] = CSI_ALIGN_16B(line_stride_y_ch[ch] >> 1);
			buf_height_y_ch[ch] = frame_info->ch_size[ch].height;
			buf_height_cb_ch[ch] = buf_height_y_ch[ch] >> 1;
			buf_height_cr_ch[ch] = buf_height_y_ch[ch] >> 1;
			break;
		case PIX_FMT_YUV422P_8:
		case PIX_FMT_YVU422P_8:
			line_stride_y_ch[ch] = CSI_ALIGN_16B(frame_info->ch_size[ch].width);
			line_stride_c_ch[ch] = CSI_ALIGN_16B(line_stride_y_ch[ch] >> 1);
			buf_height_y_ch[ch] = frame_info->ch_size[ch].height;
			buf_height_cb_ch[ch] = buf_height_y_ch[ch];
			buf_height_cr_ch[ch] = buf_height_y_ch[ch];
			break;
		case PIX_FMT_YUV420SP_8:
		case PIX_FMT_YVU420SP_8:
		case PIX_FMT_YUV420MB_8:
		case PIX_FMT_YVU420MB_8:
#ifdef CONFIG_ARCH_SUN3IW1P1
			line_stride_y_ch[ch] = CSI_ALIGN_32B(frame_info->ch_size[ch].width);
			line_stride_c_ch[ch] = CSI_ALIGN_32B(line_stride_y_ch[ch]);
			buf_height_y_ch[ch] = CSI_ALIGN_32B(frame_info->ch_size[ch].height);
			buf_height_cb_ch[ch] = CSI_ALIGN_64B(buf_height_y_ch[ch]) >> 1;
#else
			line_stride_y_ch[ch] = CSI_ALIGN_16B(frame_info->ch_size[ch].width);
			line_stride_c_ch[ch] = line_stride_y_ch[ch];
			buf_height_y_ch[ch] = frame_info->ch_size[ch].height;
			buf_height_cb_ch[ch] = buf_height_y_ch[ch] >> 1;
#endif
			break;
		case PIX_FMT_YUV422SP_8:
		case PIX_FMT_YVU422SP_8:
		case PIX_FMT_YUV422MB_8:
		case PIX_FMT_YVU422MB_8:
			line_stride_y_ch[ch] = CSI_ALIGN_16B(frame_info->ch_size[ch].width);
			line_stride_c_ch[ch] = line_stride_y_ch[ch];
			buf_height_y_ch[ch] = frame_info->ch_size[ch].height;
			buf_height_cb_ch[ch] = buf_height_y_ch[ch];
			break;
		case PIX_FMT_YUV420SP_10: /* for debug */
		case PIX_FMT_YVU420SP_10:
			line_stride_y_ch[ch] = frame_info->ch_size[ch].width << 1;
			line_stride_c_ch[ch] = line_stride_y_ch[ch];
			buf_height_y_ch[ch] = frame_info->ch_size[ch].height;
			buf_height_cb_ch[ch] = buf_height_y_ch[ch] >> 1;
			break;
		case PIX_FMT_YUV422SP_10:/* for debug */
		case PIX_FMT_YVU422SP_10:
			line_stride_y_ch[ch] = frame_info->ch_size[ch].width << 1;
			line_stride_c_ch[ch] = line_stride_y_ch[ch];
			buf_height_y_ch[ch] = frame_info->ch_size[ch].height;
			buf_height_cb_ch[ch] = buf_height_y_ch[ch];
			break;
		case PIX_FMT_SBGGR_8:/* all below are for debug */
		case PIX_FMT_SGBRG_8:
		case PIX_FMT_SGRBG_8:
		case PIX_FMT_SRGGB_8:
		case PIX_FMT_SBGGR_10:
		case PIX_FMT_SGBRG_10:
		case PIX_FMT_SGRBG_10:
		case PIX_FMT_SRGGB_10:
		case PIX_FMT_SBGGR_12:
		case PIX_FMT_SGBRG_12:
		case PIX_FMT_SGRBG_12:
		case PIX_FMT_SRGGB_12:
			line_stride_y_ch[ch] = frame_info->ch_size[ch].width << ((bus_precision[ch] == W_8BIT) ? 0:1);
			buf_height_y_ch[ch] = frame_info->ch_size[ch].height;
			break;
		default:
			break;
		}
		line_stride_y_ch_gbl[sel][ch] = line_stride_y_ch[ch];
		line_stride_c_ch_gbl[sel][ch] = line_stride_c_ch[ch];
		buf_height_y_ch_gbl[sel][ch]  = buf_height_y_ch[ch];
		buf_height_cb_ch_gbl[sel][ch] = buf_height_cb_ch[ch];
		buf_height_cr_ch_gbl[sel][ch] = buf_height_cr_ch[ch];
		csi_set_offset(sel, ch, start_h[ch], start_v[ch]);
	}
	/* assume channels at the same row has the same height
	assume channels at the same column has the same width */
#ifdef CONFIG_ARCH_SUN3IW1P1
	for (i = 0; i < row; i++) {
		line_stride_y_row[i] = 0;
		line_stride_c_row[i] = 0;
		for (j = 0; j < column; j++) {
			ch = i*column + j;
			line_stride_y_row[i] += line_stride_y_ch[ch];
			line_stride_c_row[i] += line_stride_c_ch[ch];
			line_stride_y_row_gbl[sel][i] = line_stride_y_row[i];
			line_stride_c_row_gbl[sel][i] = line_stride_c_row[i];
			csi_set_size(sel, ch, input_len_h[ch], input_len_v[ch],
				line_stride_y_row[i], line_stride_c_row[i]);
		}
	}
#else
	for (i = 0; i < row; i++) {
		for (j = 0; j < column; j++) {
			ch = i * column + j;
			line_stride_y_row[i] = line_stride_y_ch[ch] * column;
			line_stride_c_row[i] = line_stride_c_ch[ch] * column;
			line_stride_y_row_gbl[sel][i] = line_stride_y_row[i];
			line_stride_c_row_gbl[sel][i] = line_stride_c_row[i];
			csi_set_size(sel, ch, input_len_h[ch], input_len_v[ch],
				line_stride_y_row[i], line_stride_c_row[i]);
		}
	}
#endif
	for (ch = 0; ch < row * column; ch++) {
		frame_info->frm_byte_size[ch] = 0;
		frame_info->frm_byte_size[ch] += line_stride_y_ch[ch] * buf_height_y_ch[ch] +
								line_stride_c_ch[ch] * buf_height_cb_ch[ch] +
								line_stride_c_ch[ch] * buf_height_cr_ch[ch];
	}

	return 0;
}


/* bsp_csi_set_addr
 * function:
 * set csi output address, no return
 * must be called after bsp_csi_set_size
 *
 * input parameters:
 * buffer base address
 *
 */

void bsp_csi_set_addr(unsigned int sel, u64 buf_base_addr)
{
	u64 buf_addr_plane0[MAX_CH_NUM];
	u64 buf_addr_plane1[MAX_CH_NUM];
	u64 buf_addr_plane2[MAX_CH_NUM];
	unsigned int ch, i, j, k, l, row, column;

	row = frm_arrange_gbl[sel].row;
	column = frm_arrange_gbl[sel].column;
#ifdef CHANNEL_OUTPUT_COLUMN
#ifdef CONFIG_ARCH_SUN3IW1P1
	for (i = 0; i < row; i++) {
		for (j = 0; j < column; j++) {
			ch = i*column+j;
			buf_addr_plane0[ch] = buf_base_addr;
			buf_addr_plane1[ch] = buf_addr_plane0[ch]
							+ line_stride_y_row_gbl[sel][i] * buf_height_y_ch_gbl[sel][ch];
			buf_addr_plane2[ch] = buf_addr_plane1[ch]
							+ line_stride_c_row_gbl[sel][i] * buf_height_cb_ch_gbl[sel][ch];
			for (k = 0; k < j; k++) {
				buf_addr_plane0[ch] += line_stride_y_ch_gbl[sel][i * column+k - 1];
				buf_addr_plane1[ch] += line_stride_c_ch_gbl[sel][i * column+k - 1];
				buf_addr_plane2[ch] += line_stride_c_ch_gbl[sel][i * column+k - 1];
			}

			for (l = 1; l < i; l++) {
				buf_addr_plane0[ch] += ((line_stride_y_row_gbl[sel][l-1] * buf_height_y_ch_gbl[sel][l*column+j-1])
							+ (line_stride_c_row_gbl[sel][l-1] * buf_height_cb_ch_gbl[sel][l*column+j-1])
							+ (line_stride_c_row_gbl[sel][l-1] * buf_height_cr_ch_gbl[sel][l*column+j-1]));
				buf_addr_plane1[ch] += ((line_stride_c_row_gbl[sel][l-1] * buf_height_cb_ch_gbl[sel][l*column+j-1])
							+ (line_stride_c_row_gbl[sel][l-1] * buf_height_cr_ch_gbl[sel][l*column+j-1])
							+ (line_stride_y_row_gbl[sel][l] * buf_height_y_ch_gbl[sel][l*column+j]));
				buf_addr_plane2[ch] += ((line_stride_c_row_gbl[sel][l-1] * buf_height_cr_ch_gbl[sel][l*column+j-1])
							+ (line_stride_y_row_gbl[sel][l] * buf_height_y_ch_gbl[sel][l*column+j])
							+ (line_stride_c_row_gbl[sel][l] * buf_height_cb_ch_gbl[sel][l*column+j]));
			}
			csi_set_buffer_address(sel, ch, CSI_BUF_0_A, buf_addr_plane0[ch]);
			csi_set_buffer_address(sel, ch, CSI_BUF_1_A, buf_addr_plane1[ch]);
			csi_set_buffer_address(sel, ch, CSI_BUF_2_A, buf_addr_plane2[ch]);
		}
	}
#else
	for (i = 0; i < row; i++) {
		for (j = 0; j < column; j++) {
			ch = i*column+j;
			buf_addr_plane0[ch] = buf_base_addr;
			buf_addr_plane1[ch] = buf_addr_plane0[ch]
				  + line_stride_y_row_gbl[sel][i] * buf_height_y_ch_gbl[sel][ch];
			buf_addr_plane2[ch] = buf_addr_plane1[ch]
				  + line_stride_c_row_gbl[sel][i] * buf_height_cb_ch_gbl[sel][ch];
			for (k = 1; k <= j; k++) {
				buf_addr_plane0[ch] += line_stride_y_ch_gbl[sel][i*column+k-1];
				buf_addr_plane1[ch] += line_stride_c_ch_gbl[sel][i*column+k-1];
				buf_addr_plane2[ch] += line_stride_c_ch_gbl[sel][i*column+k-1];
			}

			for (l = 1; l <= i; l++) {
				buf_addr_plane0[ch] += ((line_stride_y_row_gbl[sel][l-1] * buf_height_y_ch_gbl[sel][l*column+j-1])
					+ (line_stride_c_row_gbl[sel][l-1] * buf_height_cb_ch_gbl[sel][l*column+j-1])
					+ (line_stride_c_row_gbl[sel][l-1] * buf_height_cr_ch_gbl[sel][l*column+j-1]));
				buf_addr_plane1[ch] += ((line_stride_c_row_gbl[sel][l-1] * buf_height_cb_ch_gbl[sel][l*column+j-1])
					+ (line_stride_c_row_gbl[sel][l-1] * buf_height_cr_ch_gbl[sel][l*column+j-1])
					+ (line_stride_y_row_gbl[sel][l-1] * buf_height_y_ch_gbl[sel][l*column+j-1]));
				buf_addr_plane2[ch] += ((line_stride_c_row_gbl[sel][l-1] * buf_height_cr_ch_gbl[sel][l*column+j-1])
					+ (line_stride_y_row_gbl[sel][l-1] * buf_height_y_ch_gbl[sel][l*column+j-1])
					+ (line_stride_c_row_gbl[sel][l-1] * buf_height_cb_ch_gbl[sel][l*column+j-1]));
			}
			csi_set_buffer_address(sel, ch, CSI_BUF_0_A, buf_addr_plane0[ch]);
			csi_set_buffer_address(sel, ch, CSI_BUF_1_A, buf_addr_plane1[ch]);
			csi_set_buffer_address(sel, ch, CSI_BUF_2_A, buf_addr_plane2[ch]);
		}
	}
#endif
#else
	for (i = 0; i < row; i++) {
		for (j = 0; j < column; j++) {
			ch = i * column + j;

			buf_addr_plane0[ch] = buf_base_addr;
			buf_addr_plane1[ch] = buf_addr_plane0[ch]
				+ line_stride_y_row_gbl[sel][i] * buf_height_y_ch_gbl[sel][ch] * row;
			buf_addr_plane2[ch] = buf_addr_plane1[ch]
				+ line_stride_c_row_gbl[sel][i] * buf_height_cb_ch_gbl[sel][ch] * row;

			for (k = j; k > 0; k--) {
				buf_addr_plane0[ch] += line_stride_y_ch_gbl[sel][k - 1];
				buf_addr_plane1[ch] += line_stride_c_ch_gbl[sel][k - 1];
				buf_addr_plane2[ch] += line_stride_c_ch_gbl[sel][k - 1];
			}

			for (l = i; l > 0; l--) {
				buf_addr_plane0[ch] += line_stride_y_row_gbl[sel][l - 1] * buf_height_y_ch_gbl[sel][ch];
				buf_addr_plane1[ch] += line_stride_c_row_gbl[sel][l - 1] * buf_height_cb_ch_gbl[sel][ch];
				buf_addr_plane2[ch] += line_stride_c_row_gbl[sel][l - 1] * buf_height_cr_ch_gbl[sel][ch];
			}
			csi_set_buffer_address(sel, ch, CSI_BUF_0_A, buf_addr_plane0[ch]);
			csi_set_buffer_address(sel, ch, CSI_BUF_1_A, buf_addr_plane1[ch]);
			csi_set_buffer_address(sel, ch, CSI_BUF_2_A, buf_addr_plane2[ch]);
		}
	}
#endif
}

void bsp_csi_set_ch_addr(unsigned int sel, unsigned int ch, u64 buf_base_addr)
{
	u64 buf_addr_plane0[MAX_CH_NUM];
	u64 buf_addr_plane1[MAX_CH_NUM];
	u64 buf_addr_plane2[MAX_CH_NUM];

	buf_addr_plane0[ch] = buf_base_addr;
	buf_addr_plane1[ch] = buf_addr_plane0[ch] + line_stride_y_row_gbl[sel][ch] * buf_height_y_ch_gbl[sel][ch];
	buf_addr_plane2[ch] = buf_addr_plane1[ch] + line_stride_c_row_gbl[sel][ch] * buf_height_cb_ch_gbl[sel][ch];

	csi_set_buffer_address(sel, ch, CSI_BUF_0_A, buf_addr_plane0[ch]);
	csi_set_buffer_address(sel, ch, CSI_BUF_1_A, buf_addr_plane1[ch]);
	csi_set_buffer_address(sel, ch, CSI_BUF_2_A, buf_addr_plane2[ch]);
}

void bsp_csi_cap_start(unsigned int sel, unsigned int ch_total_num, enum csi_cap_mode csi_cap_mode)
{
	csi_capture_start(sel, ch_total_num, csi_cap_mode);
}

void bsp_csi_cap_stop(unsigned int sel, unsigned int ch_total_num, enum csi_cap_mode csi_cap_mode)
{
	csi_capture_stop(sel, ch_total_num, csi_cap_mode);
}

void bsp_csi_int_enable(unsigned int sel, unsigned int ch, enum csi_int_sel interrupt)
{
	csi_int_enable(sel, ch, interrupt);
}

void bsp_csi_int_disable(unsigned int sel, unsigned int ch, enum csi_int_sel interrupt)
{
	csi_int_disable(sel, ch, interrupt);
}

void bsp_csi_int_get_status(unsigned int sel, unsigned int ch, struct csi_int_status *status)
{
	csi_int_get_status(sel, ch, status);
}

void bsp_csi_int_clear_status(unsigned int sel, unsigned int ch, enum csi_int_sel interrupt)
{
	csi_int_clear_status(sel, ch, interrupt);
}
