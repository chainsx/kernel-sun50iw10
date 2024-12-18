/**
 *  All Winner Tech, All Right Reserved. 2006-2016 Copyright (c)
 *
 *  File name   :       di_type_v23.h
 *
 *  Description :
 *
 *  History     :2016/01/12        zhengwj        initial version for DI_V2.0
 *               2016/10/09        zhengwj        modify for DI_V2.3
 *               2016/12/27        yulangheng     code of DI_V2.2 added
 *
 *  This file is licensed under the terms of the GNU General Public
 *  License version 2.  This program is licensed "as is" without any
 *  warranty of any kind, whether express or implied.
 */

#ifndef __DI_TYPE_V2X__H__
#define __DI_TYPE_V2X__H__
#include <linux/types.h>
#include "../sunxi-di.h"

struct __di_buf_size_t {
	unsigned int width;
	unsigned int height;
	unsigned int fb_width;
	unsigned int fb_height;
};

struct __di_buf_addr_t {
	unsigned long long ch0_addr;
	unsigned long long ch1_addr;
	unsigned long long ch2_addr;
};

enum __di_fmt_t {
	DI_FMT_PLANAR420 = 0x0,
	DI_FMT_UVCOMB420 = 0x1,
	DI_FMT_PLANAR422 = 0x2,
	DI_FMT_UVCOMB422 = 0x3,
};

union DI_CTRL_REG {
	unsigned int dwval;
	struct {
		unsigned int start:1;
		unsigned int res0:15;
		unsigned int dma_rand_access_en:1; /* res in DI220 */
		unsigned int res1:14;
		unsigned int reset:1;
	} bits;
};

union DI_INT_CTRL_REG {
	unsigned int dwval;
	struct {
		unsigned int int_en:1;
		unsigned int res0:31;
	} bits;
};

union DI_STATUS_REG {
	unsigned int dwval;
	struct {
		unsigned int finish_sts:1;
		unsigned int res0:7;
		unsigned int busy:1;
		unsigned int res1:7;
		unsigned int cur_line:11;
		unsigned int res2:1;
		unsigned int cur_plane:2;
		unsigned int res3:2;
	} bits;
};

union DI_SIZE_REG {
	unsigned int dwval;
	struct {
		unsigned int width:11;
		unsigned int res0:5;
		unsigned int height:11;
		unsigned int res1:5;
	} bits;
};

union DI_FORMAT_REG {
	unsigned int dwval;
	struct {
		unsigned int fmt:2;
		unsigned int res0:30;
	} bits;
};

union DI_POLAR_REG {
	unsigned int dwval;
	struct {
		unsigned int polar:1;
		unsigned int res0:31;
	} bits;
};

union DI_PITCH_REG {
	unsigned int dwval;
	struct {
		unsigned int pitch:16;
		unsigned int res0:16;
	} bits;
};

union DI_ADD_REG {
	unsigned int dwval;
	struct {
		unsigned int addr:32;
	} bits;
};

union DI_ADDHB_REG {
	unsigned int dwval;
	struct {
		unsigned int addhb0:8;
		unsigned int addhb1:8;
		unsigned int addhb2:8;
		unsigned int res0:8;
	} bits;
};

union DI_MODE_REG {
	unsigned int dwval;
	struct {
		unsigned int di_mode_luma:1;
		unsigned int res0:3;
		unsigned int motion_detc_en:1;
		unsigned int diag_intp_en:1;
#if defined DI_V23
		unsigned int res1:6;
		unsigned int flag_auto_update_mode:2;
		unsigned int res2:2;
#elif defined DI_V22
		unsigned int res1:2;
		unsigned int flag_update_mode:1;
		unsigned int res2:7;
#endif
		unsigned int di_mode_chroma:1;
		unsigned int res3:14;
		unsigned int in_field_mode:1;
	} bits;
};

union DI_MD_PARA0_REG {
	unsigned int dwval;
	struct {
		unsigned int minlumath:8; /* Default: 0x4 */
		unsigned int maxlumath:8; /* Default: 0xc */
		unsigned int avglumashifter:4; /* Default: 0x6 */
		unsigned int res0:4; /* Default: */
		unsigned int th_shift:4; /* Default: 0x1 */
		unsigned int res1:4; /* Default: */
	} bits;
};

union DI_MD_PARA1_REG {
	unsigned int dwval;
	struct {
		unsigned int f_port_th:8;/* Default:0x80 res in DI_V23 */
		unsigned int f_port_factor:3;/* Default:0x4 res in DI_V23 */
		unsigned int res0:5;
		unsigned int edge_th:8;/* Default:0x20 res in DI_V23 */
		unsigned int mov_fac_edge:2;/* Default:0x3 res in DI_V23 */
		unsigned int res1:2;
		unsigned int mov_fac_nonedge:2; /* Default: 0x1 */
		unsigned int res2:2; /* Default: */
	} bits;
};


union DI_MD_PARA2_REG {
	unsigned int dwval;
	struct {
		unsigned int luma_spatial_th:8; /* Default:5 res in DI230 */
		unsigned int chroma_spatical_th:8; /* Default: 0x80 */
		unsigned int chroma_diff_th:8; /* Default: 0x5 */
		unsigned int erosion_bob_th:4; /* Default: 0x7 res in DI230 */
		unsigned int pix_static_th:2; /* Default: 0x3 */
		unsigned int res2:2; /* Default: */
	} bits;
};

union DI_INTP_PARA_REG {
	unsigned int dwval;
	struct {
		unsigned int angle_limit:5; /* Default: 0x14 */
		unsigned int res0:3; /* Default: */
		unsigned int angle_const_th:3; /* Default: 0x5 */
		unsigned int res1:5; /* Default: */
		unsigned int luma_cur_fac_mod:3; /* Default: 0x1 */
		unsigned int res2:1; /* Default: */
		unsigned int chroma_cur_fac_mod:3; /* Default: 0x1 */
		unsigned int res3:9; /* Default: */
	} bits;
};

union DI_MD_CH_PARA_REG {
	unsigned int dwval;
	struct {
		unsigned int blend_mode:4; /* Default: 0x0 */
		unsigned int res0:4; /* Default: */
		unsigned int font_pro_en:1; /* Default: 0x0 */
		unsigned int res1:7; /* Default: */
		unsigned int font_pro_th:8; /* Default: 0x30 */
		unsigned int font_pro_fac:5;
						 /* Default: 0x4
					      * 4 bits width in DI230
					      */
		unsigned int res2:3; /* Default: */
	} bits;
};

union DI_INTP_PARA1_REG {
	unsigned int dwval;
	struct {
		unsigned int a:3; /* Default: 0x4 */
		unsigned int en:1; /* Default: 0x1 */
		unsigned int c:4; /* Default: 0xa */
		unsigned int cmax:8; /* Default: 0x40 */
		unsigned int maxrat:2; /* Default: 0x2 */
		unsigned int res0:14; /* Default: */
	} bits;
};

union DI_OUTPUT_PATH_REG {
	unsigned int dwval;
	struct {
		unsigned int output_path:1;
		unsigned int res0:31;
	} bits;
};

/*
 * DI_V23 does not contain DNS-related regisgter
 */
union DNS_CTL {
	unsigned int dwval;
	struct {
		unsigned int dns_en:1;
		unsigned int winsz_sel:1;
		unsigned int res0:29;
		unsigned int win_en:1;
	} bits;
}; /* default: 0x0000_0000 */

union DNS_SIZE {
	unsigned int dwval;
	struct {
		unsigned int width:13;
		unsigned int res0:3;
		unsigned int height:13;
		unsigned int res1:3;
	} bits;
}; /* default: 0x0000_0000 */

union DNS_WIN0 {
	unsigned int dwval;
	struct {
		unsigned int win_left:13;
		unsigned int res0:3;
		unsigned int win_top:13;
		unsigned int res1:3;
	} bits;
}; /* default: 0x0000_0000 */

union DNS_WIN1 {
	unsigned int dwval;
	struct {
		unsigned int win_right:13;
		unsigned int res0:3;
		unsigned int win_bot:13;
		unsigned int res1:3;
	} bits;
}; /* default: 0x0000_0000 */

union DNS_LFT_PARA0 {
	unsigned int dwval;
	struct {
		unsigned int lsig:3;
		unsigned int res0:5;
		unsigned int lsig2:8;
		unsigned int lsig3:8;
		unsigned int ldir_rsig_gain2:8;
	} bits;
}; /* default: 0x0000_0000 */

union DNS_LFT_PARA1 {
	unsigned int dwval;
	struct {
		unsigned int ldir_cen:8;
		unsigned int ldir_rsig_gain:8;
		unsigned int ldir_thrlow:8;
		unsigned int ldir_thrhigh:8;
	} bits;
}; /* default: 0x0000_0000 */

union DNS_LFT_PARA2 {
	unsigned int dwval;
	struct {
		unsigned int lbben:1;
		unsigned int res0:7;
		unsigned int lbgain:8;
		unsigned int lbyst:4;
		unsigned int lbxst:4;
		unsigned int lbw:4;
		unsigned int lbh:4;
	} bits;
}; /* default:0x7700_0000 */

union DNS_LFT_PARA3 {
	unsigned int dwval;
	struct {
		unsigned int lsig_cen:8;
		unsigned int res0:24;
	} bits;
}; /* default:0x0000_0000 */

union IQA_CTL {
	unsigned int dwval;
	struct {
		unsigned int no_name:1;
		unsigned int res0:31;
	} bits;
}; /* default: 0x0000_0000 */

union IQA_BLKDT_PARA0 {
	unsigned int dwval;
	struct {
		unsigned int dt_thrlow:8;
		unsigned int dt_thrhigh:8;
		unsigned int res0:16;
	} bits;
}; /* default: 0x0000_c810 */

struct __di_dev_t {
	union DI_CTRL_REG ctrl; /* 0x0000 */
	union DI_INT_CTRL_REG intr; /* 0x0004 */
	union DI_STATUS_REG status;	/* 0x0008 */
	unsigned int res0; /* 0x000c */
	union DI_SIZE_REG size; /* 0x0010 */
	union DI_FORMAT_REG fmt; /* 0x0014 */
	union DI_POLAR_REG polar; /* 0x0018 */
	unsigned int res1; /* 0x001c */
	union DI_PITCH_REG inpicth0; /* 0x0020 */
	union DI_PITCH_REG inpicth1; /* 0x0024 */
	union DI_PITCH_REG inpicth2; /* 0x0028 */
	unsigned int res2; /* 0x002C */
	union DI_PITCH_REG outpicth0; /* 0x0030 */
	union DI_PITCH_REG outpicth1; /* 0x0034 */
	union DI_PITCH_REG outpicth2; /* 0x0038 */
	unsigned int res3; /* 0x003C */
	union DI_PITCH_REG flagpitch; /* 0x0040 */
	unsigned int res4[3]; /* 0x0044-4c */
	union DI_ADD_REG in0add0; /* 0x0050 */
	union DI_ADD_REG in0add1; /* 0x0054 */
	union DI_ADD_REG in0add2; /* 0x0058 */
	union DI_ADDHB_REG in0addhb; /* 0x005c */
	union DI_ADD_REG in1add0; /* 0x0060 */
	union DI_ADD_REG in1add1; /* 0x0064 */
	union DI_ADD_REG in1add2; /* 0x0068 */
	union DI_ADDHB_REG in1addhb; /* 0x006c */
	union DI_ADD_REG in2add0; /* 0x0070 */
	union DI_ADD_REG in2add1; /* 0x0074 */
	union DI_ADD_REG in2add2; /* 0x0078 */
	union DI_ADDHB_REG in2addhb; /* 0x007c */
	union DI_ADD_REG in3add0; /* 0x0080 */
	union DI_ADD_REG in3add1; /* 0x0084 */
	union DI_ADD_REG in3add2; /* 0x0088 */
	union DI_ADDHB_REG in3addhb; /* 0x008c */
	union DI_ADD_REG outadd0; /* 0x0090 */
	union DI_ADD_REG outadd1; /* 0x0094 */
	union DI_ADD_REG outadd2; /* 0x0098 */
	union DI_ADDHB_REG outaddhb; /* 0x009c */
	union DI_ADD_REG inflagadd; /* 0x00a0 */
	union DI_ADD_REG outflagadd; /* 0x00a4 */
	union DI_ADDHB_REG flagaddhb; /* 0x00a8 */
	unsigned int res5; /* 0x00ac */
	union DI_MODE_REG mode; /* 0x00b0 */
	union DI_MD_PARA0_REG mdpara0; /* 0x00b4 */
	union DI_MD_PARA1_REG mdpara1; /* 0x00b8 */
	union DI_MD_PARA2_REG mdpara2; /* 0x00bc */
	union DI_INTP_PARA_REG dipara0; /* 0x00c0 */
	union DI_MD_CH_PARA_REG mdchpara; /* 0x00c4 */

	union DI_INTP_PARA1_REG dipara1; /* 0x00c8 res in DI220 */
	unsigned int res6[77]; /* 0x00cc-0x01fc */

	union DI_OUTPUT_PATH_REG outpath; /* 0x0200 */
};

/*
 * DI_V23 does not contain DNS module
 */
struct __dns_dev_t {
	union DNS_CTL dns_ctl; /* 0x10000 */
	union DNS_SIZE dns_size; /* 0x10004 */
	union DNS_WIN0 dns_win0; /* 0x10008 */
	union DNS_WIN1 dns_win1; /* 0x1000c */
	union DNS_LFT_PARA0 dns_lft_para0; /* 0x10010 */
	union DNS_LFT_PARA1 dns_lft_para1; /* 0x10014 */
	union DNS_LFT_PARA2 dns_lft_para2; /* 0x10018 */
	union DNS_LFT_PARA3 dns_lft_para3; /* 0x1001c */
	unsigned int res8[56]; /* 0x10020-0x100fc */
	union IQA_CTL iqa_ctl; /* 0x10100 */
	unsigned int res9[14]; /* 0x10104-0x1c80c */
	union IQA_BLKDT_PARA0 iqa_blkdt_para0; /* 0x1013c */

};

#endif
