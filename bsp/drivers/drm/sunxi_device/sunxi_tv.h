/*
 * drivers/gpu/drm/sunxi/sunxi_device/sunxi_tv/sunxi_tv.h
 *
 * Copyright (c) 2007-2019 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 * Author: zhengwanyu <zhengwanyu@allwinnertech.com>
 *
 * sunxi tvout driver include cvbs out, YPbPr and VGA out
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
#ifndef _SUNXI_TV_H
#define _SUNXI_TV_H

#include <linux/clk.h>
#include <de_tvec.h>
#include "de/include.h"

#define MAX_TV_COUNT	3

struct sunxi_tv_work_mode {
	unsigned int color_fmt;
};

/**
 * sunxi_tv device class
 */
struct sunxi_tv {
	struct platform_device *pdev;

	/**
	 * @id:index of tve (hardware)
	 * */
	unsigned int id;

	/**
	 * @tv_mode:support resolution id
	 * */
	enum disp_tv_mode tv_mode;

	/**
	 * @base_addr:sunxi tve phyical address
	 * */
	void __iomem *base_addr;

	/* tve clk in ccmu */
	struct clk *mclk;

	/* tve's parent clk in ccmu */
	struct clk *mclk_parent;

	/**
	 * @top_addr:sunxi tve top phyical address
	 * */
	void __iomem *top_addr;

	/* tve top clk in ccmu */
	struct clk *top_clk;

	/**
	 * @dac_no: index of dac
	 * */
	unsigned int dac_no[TVE_DAC_NUM];

	/**
	 * @dac_type:data source type of dac.
	 * i.e. 0:composite,1:luma,2:chroma,3:reserved,4:y/green,
	 * 5:u/pb/blue,6:v/pr/red
	 * */
	enum disp_tv_dac_source dac_type[TVE_DAC_NUM];

	/**
	 * @dac_num: total number of dac to be used
	 * for current device.
	 * i.e. 1 for CVBS, 3 for VGA and YPbPr
	 * */
	unsigned int dac_num;

	/**
	 * @cali: cali value for calibrating dac
	 * */
	unsigned int cali[TVE_DAC_NUM];

	/**
	 * @cali: cali value for calibrating dac
	 * */
	int cali_offset[TVE_DAC_NUM];

	/**
	 * @enable: enable status of current tv device
	 * */
	bool enable;

	/**
	 * @mlcok:mutex lock for setting mode, enable or
	 * disable tv
	 * */
	struct mutex mlock;

	/**
	 * @tv_type:output interface of current device
	 * decided by [tv0] & [tv1] in sys_config.fex
	 * */
	enum disp_tv_output tv_type;

	/**
	 * @tv_power:name of axp power
	 * */
	const char *p_tv_power;

	struct disp_video_timings *video_info;

};


/**
 * @set_mode:call back for setting current mode
 * */
int sunxi_tv_set_mode(int tv_id, enum disp_tv_mode tv_mode);

/**
 * @enable:call back for enabling tv
 * */
int sunxi_tv_enable(int tv_id);

/**
 * @enable:call back for disabling tv
 * */
int sunxi_tv_disable(int tv_id);

int  sunxi_tv_get_working_mode(int tv_id,
		struct sunxi_tv_work_mode *work_mode);

/**
 * @get_input_csc:call back for setting deflick function
 * @mode: 0-->deflick off, 1-->deflick mode 1, 2-->deflick mode 2
 */
int sunxi_tv_set_enhance_mode(int tv_id, unsigned int mode);

/**
 * @get_connect_status:call back for getting tv connect status
 * @return:0:unconnected; 1:connected
 */
int sunxi_tv_get_connect_status(int tv_id);

/**
 * @get_video_mode_info:call back for getting disp video timing
 * info of specified mode
 */
int sunxi_tv_get_video_mode_info(int tv_id,
			   struct disp_video_timings **video_info,
			   enum disp_tv_mode tv_mode);
/**
 * @mode_support:call back for determining if a mode is supported or
 * not.
 */
int sunxi_tv_mode_support(int tv_id, enum disp_tv_mode tv_mode);

/**
 * @name       :get_video_timing_info
 * @brief      :call back for getting disp video timing info
 * @param[OUT] :num: store the totoal number of support mode
 * @param[OUT] :video_info:pointer of first item in timing_info array
 * @return     :0 if success, -1 else
 */
int sunxi_tv_get_video_timing_info(int tv_id, unsigned int *num,
			     struct disp_video_timings **video_info);
struct disp_video_timings *
	sunxi_tv_get_video_timing(
			int tv_id, unsigned int tv_mode);


/**
 * @name       :sunxi_tv_get_tv_count
 * @brief      :get the number of sunxi tv
 * @return     :the number of sunxi tv
 */
int sunxi_tv_get_tv_count(void);

void sunxi_tv_module_exit(void);
int sunxi_tv_module_init(void);

#define TV_DBG(fmt, arg...)   DRM_DEBUG_KMS("[SUNXI-TV] %s()%d - "fmt, __func__, __LINE__, ##arg)
#define TV_INFO(fmt, arg...)   DRM_INFO("[SUNXI-TV] %s()%d - "fmt, __func__, __LINE__, ##arg)
#define TV_ERR(fmt, arg...)   DRM_ERROR("[SUNXI-TV] %s()%d - "fmt, __func__, __LINE__, ##arg)

#endif /* End of file */
