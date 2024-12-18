/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

/**
 *All Winner Tech, All Right Reserved. 2014-2015 Copyright (c)
 *
 *File name   :de_rtmx.h
 *
 *Description :display engine 2.0 realtime
 *              mixer processing base functions implement
 *History     :2014/02/08  iptang  v0.1  Initial version
 *
 */

#ifndef __DE_RTMX_H__
#define __DE_RTMX_H__

#include "de_feat.h"
#include "../include.h"

enum de_irq_flag {
	DE_IRQ_FLAG_FRAME_END  = 0x1 << 4,
	DE_IRQ_FLAG_ERROR      = 0,
	DE_IRQ_FLAG_RCQ_FINISH = 0x1 << 6,
	DE_IRQ_FLAG_RCQ_ACCEPT = 0x1 << 7,
	DE_IRQ_FLAG_MASK =
		DE_IRQ_FLAG_FRAME_END
		| DE_IRQ_FLAG_ERROR
		| DE_IRQ_FLAG_RCQ_FINISH
		| DE_IRQ_FLAG_RCQ_ACCEPT,
};

enum de_irq_state {
	DE_IRQ_STATE_FRAME_END  = 0x1 << 0,
	DE_IRQ_STATE_ERROR      = 0,
	DE_IRQ_STATE_RCQ_FINISH = 0x1 << 2,
	DE_IRQ_STATE_RCQ_ACCEPT = 0x1 << 3,
	DE_IRQ_STATE_MASK =
		DE_IRQ_STATE_FRAME_END
		| DE_IRQ_STATE_ERROR
		| DE_IRQ_STATE_RCQ_FINISH
		| DE_IRQ_STATE_RCQ_ACCEPT,
};

enum de_wb_irq_flag {
	DE_WB_IRQ_FLAG_RCQ_FINISH = 0x1 << 6,
	DE_WB_IRQ_FLAG_RCQ_ACCEPT = 0x1 << 7,
	DE_WB_IRQ_FLAG_MASK =
		DE_WB_IRQ_FLAG_RCQ_FINISH
		| DE_WB_IRQ_FLAG_RCQ_ACCEPT,
};

enum de_wb_irq_state {
	DE_WB_IRQ_STATE_RCQ_FINISH = 0x1 << 2,
	DE_WB_IRQ_STATE_RCQ_ACCEPT = 0x1 << 3,
	DE_WB_IRQ_STATE_MASK =
		DE_WB_IRQ_STATE_RCQ_FINISH
		| DE_WB_IRQ_STATE_RCQ_ACCEPT,
};

#define max2(s, t) (((s) >= (t)) ? (s) : (t))
#define min2(s, t) (((s) <= (t)) ? (s) : (t))
#define N2_POWER(a, power)	(((unsigned long long)a)<<power)

enum de_color_space {
	DE_GBR = 0,
	DE_BT709 = 1,
	DE_FCC = 2,
	DE_BT470BG = 3,
	DE_BT601 = 4,
	DE_SMPTE240M = 5,
	DE_YCGCO = 6,
	DE_BT2020NC = 7,
	DE_BT2020C = 8,
};

enum AFBD_FMT {
	AFB_RGBA4444    =  0X0e,
	AFB_RGB565      =  0X0a,
	AFB_RGBA5551    =  0X12,
	AFB_RGB888      =  0X08,
	AFB_RGBA8888    =  0X02,
	AFB_RGBA1010102 =  0X16,

	AFB_YUV420      =  0X2a,
	AFB_YUV422      =  0X26,
	AFB_YUV420B10   =  0X30,
	AFB_YUV422B10   =  0X32,
};

enum de_pixel_format {
	DE_FORMAT_ARGB_8888 = 0x00, /* MSB  A-R-G-B  LSB */
	DE_FORMAT_ABGR_8888 = 0x01,
	DE_FORMAT_RGBA_8888 = 0x02,
	DE_FORMAT_BGRA_8888 = 0x03,
	DE_FORMAT_XRGB_8888 = 0x04,
	DE_FORMAT_XBGR_8888 = 0x05,
	DE_FORMAT_RGBX_8888 = 0x06,
	DE_FORMAT_BGRX_8888 = 0x07,
	DE_FORMAT_RGB_888 = 0x08,
	DE_FORMAT_BGR_888 = 0x09,
	DE_FORMAT_RGB_565 = 0x0a,
	DE_FORMAT_BGR_565 = 0x0b,
	DE_FORMAT_ARGB_4444 = 0x0c,
	DE_FORMAT_ABGR_4444 = 0x0d,
	DE_FORMAT_RGBA_4444 = 0x0e,
	DE_FORMAT_BGRA_4444 = 0x0f,
	DE_FORMAT_ARGB_1555 = 0x10,
	DE_FORMAT_ABGR_1555 = 0x11,
	DE_FORMAT_RGBA_5551 = 0x12,
	DE_FORMAT_BGRA_5551 = 0x13,
	DE_FORMAT_ARGB_2101010 = 0x14,
	DE_FORMAT_ABGR_2101010 = 0x15,
	DE_FORMAT_RGBA_1010102 = 0x16,
	DE_FORMAT_BGRA_1010102 = 0x17,

	/* SP: semi-planar, P:planar, I:interleaved
	 * UVUV: U in the LSBs;     VUVU: V in the LSBs
	 */
	DE_FORMAT_YUV444_I_AYUV = 0x40,
	DE_FORMAT_YUV444_I_VUYA = 0x41,
	DE_FORMAT_YUV422_I_YVYU = 0x42,
	DE_FORMAT_YUV422_I_YUYV = 0x43,
	DE_FORMAT_YUV422_I_UYVY = 0x44,
	DE_FORMAT_YUV422_I_VYUY = 0x45,
	DE_FORMAT_YUV444_P = 0x46,
	DE_FORMAT_YUV422_P = 0x47,
	DE_FORMAT_YUV420_P = 0x48,
	DE_FORMAT_YUV411_P = 0x49,
	DE_FORMAT_YUV422_SP_UVUV = 0x4a,
	DE_FORMAT_YUV422_SP_VUVU = 0x4b,
	DE_FORMAT_YUV420_SP_UVUV = 0x4c,
	DE_FORMAT_YUV420_SP_VUVU = 0x4d,
	DE_FORMAT_YUV411_SP_UVUV = 0x4e,
	DE_FORMAT_YUV411_SP_VUVU = 0x4f,
	DE_FORMAT_8BIT_GRAY                    = 0x50,
	DE_FORMAT_YUV444_I_AYUV_10BIT          = 0x51,
	DE_FORMAT_YUV444_I_VUYA_10BIT          = 0x52,
	DE_FORMAT_YUV422_I_YVYU_10BIT          = 0x53,
	DE_FORMAT_YUV422_I_YUYV_10BIT          = 0x54,
	DE_FORMAT_YUV422_I_UYVY_10BIT          = 0x55,
	DE_FORMAT_YUV422_I_VYUY_10BIT          = 0x56,
	DE_FORMAT_YUV444_P_10BIT               = 0x57,
	DE_FORMAT_YUV422_P_10BIT               = 0x58,
	DE_FORMAT_YUV420_P_10BIT               = 0x59,
	DE_FORMAT_YUV411_P_10BIT               = 0x5a,
	DE_FORMAT_YUV422_SP_UVUV_10BIT         = 0x5b,
	DE_FORMAT_YUV422_SP_VUVU_10BIT         = 0x5c,
	DE_FORMAT_YUV420_SP_UVUV_10BIT         = 0x5d,
	DE_FORMAT_YUV420_SP_VUVU_10BIT         = 0x5e,
	DE_FORMAT_YUV411_SP_UVUV_10BIT         = 0x5f,
	DE_FORMAT_YUV411_SP_VUVU_10BIT         = 0x60,

};

enum de_3d_in_mode {
	DE_3D_SRC_NORMAL = 0x0,
	/* not 3d mode */
	DE_3D_SRC_MODE_TB = 0x1 << 0,
	/* top bottom */
	DE_3D_SRC_MODE_FP = 0x1 << 1,
	/* frame packing */
	DE_3D_SRC_MODE_SSF = 0x1 << 2,
	/* side by side full */
	DE_3D_SRC_MODE_SSH = 0x1 << 3,
	/* side by side half */
	DE_3D_SRC_MODE_LI = 0x1 << 4,
	/* line interleaved */
};

enum de_3d_out_mode {
	DE_3D_OUT_MODE_CI_1 = 0x5,
	/* column interleaved 1 */
	DE_3D_OUT_MODE_CI_2 = 0x6,
	/* column interleaved 2 */
	DE_3D_OUT_MODE_CI_3 = 0x7,
	/* column interleaved 3 */
	DE_3D_OUT_MODE_CI_4 = 0x8,
	/* column interleaved 4 */
	DE_3D_OUT_MODE_LIRGB = 0x9,
	/* line interleaved rgb */

	DE_3D_OUT_MODE_TB = 0x0,
	/* top bottom */
	DE_3D_OUT_MODE_FP = 0x1,
	/* frame packing */
	DE_3D_OUT_MODE_SSF = 0x2,
	/* side by side full */
	DE_3D_OUT_MODE_SSH = 0x3,
	/* side by side half */
	DE_3D_OUT_MODE_LI = 0x4,
	/* line interleaved */
	DE_3D_OUT_MODE_FA = 0xa,
	/* field alternative */
};

enum de_dirty_flags {
	DE_LAYER_ATTR_DIRTY = 0x00000001,
	DE_LAYER_VI_FC_DIRTY = 0x00000002,
	DE_LAYER_HADDR_DIRTY = 0x00000004,
	DE_LAYER_SIZE_DIRTY = 0x00000008,
	DE_BLEND_ENABLE_DIRTY = 0x00000010,
	DE_BLEND_ATTR_DIRTY = 0x00000020,
	DE_BLEND_CTL_DIRTY = 0x00000040,
	DE_BLEND_OUT_DIRTY = 0x00000080,
	DE_LAYER_ALL_DIRTY = 0x000000ff,

};

enum de_bld_mode {
	/*
	 * pixel color = sc * sa * cfs + dc * da * cfd
	 * pixel alpha = sa * afs + da * afd
	 * sc = source color
	 * sa = source alpha
	 * dc = destination color
	 * da = destination alpha
	 * cfs = source color factor for blend function
	 * cfd = destination color factor for blend function
	 * afs = source alpha factor for blend function
	 * afd = destination alpha factor for blend function
	 */
	DE_BLD_CLEAR = 0x00,
	/* cfs/afs:    0     cfd/afd:    0 */
	DE_BLD_SRC = 0x01,
	/* cfs/afs:    1     cfd/afd:    0 */
	DE_BLD_DST = 0x02,
	/* cfs/afs:    0     cfd/afd:    1 */
	DE_BLD_SRCOVER = 0x03,
	/* cfs/afs:    1     cfd/afd:    1-sa */
	DE_BLD_DSTOVER = 0x04,
	/* cfs/afs:    1-da  cfd/afd:    1 */
	DE_BLD_SRCIN = 0x05,
	/* cfs/afs:    da    cfd/afd:    0 */
	DE_BLD_DSTIN = 0x06,
	/* cfs/afs:    0     cfd/afd:    sa */
	DE_BLD_SRCOUT = 0x07,
	/* cfs/afs:    1-da  cfd/afd:    0 */
	DE_BLD_DSTOUT = 0x08,
	/* cfs/afs:    0     cfd/afd:    1-sa */
	DE_BLD_SRCATOP = 0x09,
	/* cfs/afs:    da    cfd/afd:    1-sa */
	DE_BLD_DSTATOP = 0x0a,
	/* cfs/afs:    1-da  cfd/afd:    sa */
	DE_BLD_XOR = 0x0b,
	/* cfs/afs:    1-da  cfd/afd:    1-sa */
};

enum de_ck_mode {
	DE_CK_SRC = 0x0,
	/* the pixel value match source image,
	 * it displays the destination image pixel
	 */
	DE_CK_DST = 0x1,
	/* the pixel value match destination image,
	 * it displays the source image pixel
	 */
};

enum de_coarsescale_sts {
	DE_NO_CS = 0x0,
	/* no coarse scale in both direction */
	DE_CS_HORZ = 0x1,
	/* coarse scale only in horizon */
	DE_CS_VERT = 0x2,
	/* coarse scale only in vertical */
	DE_CS_BOTHDIR = 0x3
	/* coarse scale in both direction */
};

struct de_fb {
	unsigned int w;
	unsigned int h;
};

struct de_rect {
	int x;
	int y;
	unsigned int w;
	unsigned int h;
};

struct de_rect64 {
	long long x;
	long long y;
	unsigned long long w;
	unsigned long long h;
};

struct de_reg_blocks {
	void *val;
	/* address from memory */
	uintptr_t off;
	/* register offset */
	unsigned int size;
	/* unit:byte */
	unsigned char dirty;
};

struct __lay_para_t {
	bool en;
	bool fcolor_en;
	bool top_bot_en;
	unsigned char fmt;
	unsigned char alpha_mode;
	unsigned char premul_ctl;
	unsigned char alpha;

	unsigned char haddr_t[3];
	unsigned char haddr_b[3];
	unsigned int pitch[3];
	struct de_rect layer;
	unsigned int laddr_t[3];
	unsigned int laddr_b[3];

	/* add para for afbc */
	unsigned int blk_wid;
	unsigned int blk_hei;
	unsigned int top_crop;
	unsigned int left_crop;
	unsigned int yuv_trans;
	unsigned int fbd_en;
};

/* DATA typedef structURE */
struct scaler_para {
	int hphase;
	/* initial phase of vsu/gsu in horizon */
	int vphase;
	/* initial phase of vsu/gsu in vertical */
	unsigned int hstep;
	/* scale step of vsu/gsu in horizon */
	unsigned int vstep;
	/* scale step of vsu/gsu in vertical */
};

int de_rtmx_init(unsigned int sel, uintptr_t reg_base);
int de_rtmx_double_init(unsigned int sel, uintptr_t reg_base);
int de_rtmx_exit(unsigned int sel);
int de_rtmx_double_exit(unsigned int sel);
int de_rtmx_update_regs(unsigned int sel);
int de_rtmx_set_gld_reg_base(unsigned int sel, void *base);
int de_rtmx_set_bld_reg_base(unsigned int sel, void *base);
int de_rtmx_set_overlay_reg_base(unsigned int sel, unsigned int chno,
				 void *base);
int de_rtmx_set_lay_cfg(unsigned int sel, unsigned int chno, unsigned int layno,
			struct __lay_para_t *cfg);
int de_rtmx_set_lay_haddr(unsigned int sel, unsigned int chno,
			  unsigned int layno, unsigned char top_bot_en,
			  unsigned char *haddr_t, unsigned char *haddr_b);
int de_rtmx_set_lay_laddr(unsigned int sel, unsigned int chno,
		  unsigned int layno, unsigned char fmt, struct de_rect crop,
		  unsigned int *size, unsigned int *align,
		  enum de_3d_in_mode trdinmode, unsigned int *addr,
		  unsigned char *haddr, unsigned int fbd_en);
int de_rtmx_get_3d_in(unsigned char fmt, struct de_rect crop,
			struct de_fb *size,
		  unsigned int *align, enum de_3d_in_mode trdinmode,
		  unsigned int *addr, unsigned int *trd_addr,
		  unsigned int *pitch, unsigned int *pitchr,
		  unsigned int *lay_laddr);
int de_rtmx_get_3d_in_single_size(enum de_3d_in_mode inmode,
	struct de_rect64 *size);
int de_rtmx_get_3d_out(struct de_rect frame0, unsigned int scn_w,
	unsigned int scn_h, enum de_3d_out_mode trdoutmode,
	struct de_rect *frame1);
int de_rtmx_get_li_addr_offset(unsigned int size, unsigned int align,
			unsigned int x, unsigned int y,
			unsigned int cnt);
int de_rtmx_set_lay_fcolor(unsigned int sel, unsigned int chno,
			   unsigned int layno, unsigned char en,
			   unsigned char fmt, unsigned int color,
			   unsigned int fbd_en);
int de_rtmx_set_overlay_size(unsigned int sel, unsigned int chno,
			     unsigned int w, unsigned int h,
			     unsigned int fbd_en);
int de_rtmx_set_coarse_fac(unsigned int sel, unsigned char chno,
			   unsigned int fmt, unsigned int lcd_fps,
			   unsigned int lcd_height, unsigned int de_freq_mhz,
			   unsigned int ovl_w, unsigned int ovl_h,
			   unsigned int vsu_outw, unsigned int vsu_outh,
			   unsigned int *midyw, unsigned int *midyh,
			   unsigned int *midcw, unsigned int *midch,
			   unsigned int fbd_en);
int de_rtmx_set_pf_en(unsigned int sel, unsigned char *pen);
int de_rtmx_set_pipe_cfg(unsigned int sel, unsigned char pno,
			 unsigned int color, struct de_rect bldrc);
int de_rtmx_set_route(unsigned int sel, unsigned char pno, unsigned int zoder);
int de_rtmx_set_premul(unsigned int sel, unsigned char pno,
		       unsigned int pre_mul);
int de_rtmx_set_background_color(unsigned int sel, unsigned int color);
int de_rtmx_set_blend_size(unsigned int sel, unsigned int w, unsigned int h);
int de_rtmx_set_blend_mode(unsigned int sel, unsigned int bldno,
			   unsigned char mode);
int de_rtmx_set_blend_color(unsigned int sel, unsigned int bldno,
			    unsigned int color);
int de_rtmx_set_outitl(unsigned int sel, unsigned char interlace_en);
int de_rtmx_set_bld_color_space(unsigned int sel, unsigned char csc);
int de_rtmx_set_colorkey(unsigned int sel, unsigned char ck_no,
			 unsigned char ck_mode, unsigned char ck_red_match,
			 unsigned char ck_green_match,
			 unsigned char ck_blue_match, unsigned int ck_max,
			 unsigned int ck_min);
int de_rtmx_calc_chnrect(unsigned char *lay_en, int laynum,
	struct de_rect *frame, struct de_rect *crop, int gsu_sel,
	 struct scaler_para *step, struct de_rect *layer,
	 struct de_rect *bld_rect, unsigned int *ovlw, unsigned int *ovlh);
int de_rtmx_trimcoord(struct de_rect *frame, struct de_rect *crop,
		unsigned int outw, unsigned int outh, int xratio, int yratio);
int de_rtmx_get_premul_ctl(int laynum, unsigned char *premul);
struct de_rect de_rtmx_extend_rect(struct de_rect rc1, struct de_rect rc2);
int de_rtmx_set_dbuff_rdy(unsigned int sel);
int de_rtmx_set_enable(unsigned int sel, unsigned int en);
int de_rtmx_set_display_size(unsigned int sel, unsigned int width,
			     unsigned int height);
int de_rtmx_query_irq(unsigned int sel);
int de_rtmx_enable_irq(unsigned int sel, unsigned int en);

int de_rtmx_mux(unsigned int sel, unsigned int tcon_index);
int de_rtmx_get_mux(unsigned int sel);
int de_rtmx_sync_hw(unsigned int sel);
int de_rtmx_get_lay_enabled(unsigned int sel, unsigned int chno,
			    unsigned int layno);
int de_rtmx_get_lay_address(unsigned int sel, unsigned int chno,
			    unsigned int layno, unsigned long long *addr);
int de_rtmx_get_lay_size(unsigned int sel, unsigned int chno,
			 unsigned int layno, struct disp_rectsz *size);
int de_rtmx_get_lay_win(unsigned int sel, unsigned int chno, unsigned int layno,
			struct disp_rect *win);
int de_rtmx_get_lay_format(unsigned int sel, unsigned int chno,
			   unsigned int layno);
int de_rtmx_get_display_size(unsigned int sel, unsigned int *width,
			     unsigned int *height);

#endif
