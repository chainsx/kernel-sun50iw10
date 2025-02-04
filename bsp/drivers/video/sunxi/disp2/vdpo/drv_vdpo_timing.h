/*
 * Allwinner SoCs timing table
 *
 * Copyright (C) 2017 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#ifndef _DRV_VDPO_TIMING_H
#define _DRV_VDPO_TIMING_H

#include "drv_vdpo_common.h"

/**
 * @name       :vdpo_get_timing_num
 * @brief      :get the number of timing that is support
 * @param[IN]  :none
 * @param[OUT] :none
 * @return     :the number of timing that is support
 */
u32 vdpo_get_timing_num(void);

/**
 * @name       :vdpo_get_timing_info
 * @brief      :get timing info of specified mode
 * @param[IN]  :mode:output resolution
 * @param[OUT] :info:pointer that store the timing info
 * @return     :0 if success
 */
s32 vdpo_get_timing_info(enum disp_tv_mode mode,
			 struct disp_video_timings **info);

/**
 * @name       vdpo_is_mode_support
 * @brief      if a specified mode is support
 * @param[IN]  mode:resolution output mode
 * @param[OUT] none
 * @return     1 if support, 0 if not support
 */
s32 vdpo_is_mode_support(enum disp_tv_mode mode);

#endif /* End of file */
