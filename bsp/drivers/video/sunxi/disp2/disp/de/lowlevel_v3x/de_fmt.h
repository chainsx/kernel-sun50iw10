/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __DE_FMT_H__
#define __DE_FMT_H__

struct disp_fmt_config {
	unsigned int enable; /* return mod en info */

	/* parameter */
	unsigned int width; /* input size */
	unsigned int height; /* input height */

	/* output bitdepth compensation :
	 * 0-8bit mode (when output device is cvbs/YPbPr/hdmi8bit etc)
	 * 1-10bit mode (when output device is hdmi10bit etc)
	 */
	unsigned int bitdepth;

	/* output colorspace : 0-YUV444(RGB); 1-YUV422; 2-YUV420 */
	unsigned int colorspace;
	/* output pixel format :
	 *     colorspace = 0 :
	 *         0-YCbCr(RGB); 1-CbYCr(GRB);
	 *     colorspace = 1 :
	 *         0-CbY/CrY; 1-YCbCr/YCbCr;
	 *     colorspace = 2 :
	 *         0-CbYY/CrYY; 1-YYCb/YYCr;
	 */
	unsigned int pixelfmt;

	/* horizontal low-pass-filter coefficients selection for 2
	 * chroma channels :
	 * 0-subsample phase = 0, and 6-tap LPF (Recommended!)
	 * 1-subsample phase = 1, and 6-tap LPF
	 * 2-subsample phase = 0.5, and 6-tap LPF
	 * 3-subsample phase = 1.5, and 6-tap LPF
	 * 4-subsample phase = 0, and no LPF
	 * 5-subsample phase = 1, and no LPF
	 * 6-subsample phase = 0.5, and 2-tap average
	 * 7-subsample phase = 1.5, and 2-tap average
	 */
	unsigned int hcoef_sel_c0;
	unsigned int hcoef_sel_c1;
	/* vertical low-pass-filter coefficients selection for 2
	 * chroma channels :
	 * 0-subsample phase = 0.5, and 2-tap average (Recommended!)
	 * 1-subsample phase = 0, and no LPF
	 * 2-subsample phase = 1, and no LPF
	 */
	unsigned int vcoef_sel_c0;
	unsigned int vcoef_sel_c1;
	unsigned int swap_enable; /* swap Cb and Cr channel input data */

};

int de_fmt_init(unsigned int sel, uintptr_t reg_base);
int de_fmt_exit(unsigned int sel);
int de_fmt_double_init(unsigned int sel, uintptr_t reg_base);
int de_fmt_double_exit(unsigned int sel);
int de_fmt_apply(unsigned int sel, struct disp_fmt_config *config);
int de_fmt_update_regs(unsigned int sel);
#endif
