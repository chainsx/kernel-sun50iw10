/* de_edp_config.h
 *
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
 * some macron definition
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef _DE_EDP_CONFIG_H
#define _DE_EDP_CONFIG_H

#define BR_5P4G ((unsigned long long)5400 * 1000 * 1000)
#define BR_2P7G ((unsigned long long)2700 * 1000 * 1000)
#define BR_1P62G ((unsigned long long)1620 * 1000 * 1000)

#define SRC_VIDEO ((unsigned long long)600 * 1000 * 1000)

#define TU_SIZE 32
#define STA_DLY 5
#define MAX_LANE_NUM 4
#define TRAIN_CNT 4

/* SRAM TEST */
/* #define SRAM_TEST */
#define SRAM_SIZE (0x2000 * 2)

#define COLOR_MODE 8

/* Pixel Encoding/Colorimetry Format Indicationa */
/* Note:depend on COLOR_MODE---0:6bpc;1:8bpc;2:10bpc;3:12bpc;4:16bpc */
#define COLOR_FORMAT 1
#define CLOCK_MODE 1 /* 1:SYNC;0:ASYNC */
#define VSC_SDP 0    /* Video Stream Configuration SDP */

/* #define QUALITY_TEST */
#define SYMBOL_ERROR_PERMIT 32767

#define BIST_MODE_CLK_SEL 0 /* 0:vclk,1:dpclk */

#define TOTAL_SYMBOL_PER_LINE 7000 /* 7000 */

/* the VOL_SWING_LEVEL_NUM shall larger to 3 */
#define VOL_SWING_LEVEL_NUM 4
/* the PRE_EMPHASIS_LEVEL_NUM shall larger to 3 */
#define PRE_EMPHASIS_LEVEL_NUM 4

#endif /* End of file */
