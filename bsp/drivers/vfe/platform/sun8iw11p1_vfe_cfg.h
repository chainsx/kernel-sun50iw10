/*
 * linux-4.9/drivers/media/platform/sunxi-vfe/platform/sun8iw11p1_vfe_cfg.h
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
 ***************************************************************************************
 *
 * sun8iw10p1_vfe_cfg.h
 *
 * Hawkview ISP - sun8iw11p1_vfe_cfg.h module
 *
 * Copyright (c) 2014 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Version		  Author         Date		    Description
 *
 *   2.0		  Yang Feng	2014/07/24	      Second Version
 *
 ****************************************************************************************
 */

#ifndef _SUN8IW11P1_VFE_CFG_H_
#define _SUN8IW11P1_VFE_CFG_H_

#define MIPI_CSI_NOT_EXIST
#define VFE_ISP_REGULATOR				""
#define VFE_CSI_REGULATOR				""
/* #define USE_SPECIFIC_CCI */

#define CSI0_REGS_BASE				0x01c09000 /* 0x01c09000 */
#define CSI1_REGS_BASE				0X01c1D000 /* 0X01c1D000 */

/* #define CSI0_CCI_REG_BASE				0x01cb3000 */
#define MIPI_CSI0_REGS_BASE			0x01cb1000

#define ISP_REGS_BASE			0x01cb8000
#define GPIO_REGS_VBASE					0x01c20800
#define CCMU_REGS_VBASE					0x01c20000
#define CPU_DRAM_PADDR_ORG			    0x40000000
#define HW_DMA_OFFSET					0x00000000
#define MAX_VFE_INPUT					2     /* the maximum number of input source of video front end */


/* set vfe core clk base on sensor size */
#define CORE_CLK_RATE_FOR_2M (108*1000*1000)
#define CORE_CLK_RATE_FOR_3M (216*1000*1000)
#define CORE_CLK_RATE_FOR_5M (324*1000*1000)
#define CORE_CLK_RATE_FOR_8M (432*1000*1000)
#define CORE_CLK_RATE_FOR_16M (432*1000*1000)


/* CSI & ISP size configs */

#define CSI0_REG_SIZE               0x1000
#define MIPI_CSI_REG_SIZE           0x1000
#define MIPI_DPHY_REG_SIZE          0x1000
#define CSI0_CCI_REG_SIZE          0x1000
#define CSI1_REG_SIZE               0x1000
#define CSI1_CCI_REG_SIZE          0x1000
#define ISP_REG_SIZE                0x1000
#define ISP_LOAD_REG_SIZE           0x1000
#define ISP_SAVED_REG_SIZE          0x1000

/* ISP size configs */

/* stat size configs */

#define ISP_STAT_TOTAL_SIZE         0x1700

#define ISP_STAT_HIST_MEM_SIZE      0x0200
#define ISP_STAT_AE_MEM_SIZE        0x0c00
#define ISP_STAT_AWB_MEM_SIZE       0x0500
#define ISP_STAT_AF_MEM_SIZE        0x0200
#define ISP_STAT_AFS_MEM_SIZE       0x0200
#define ISP_STAT_AWB_WIN_MEM_SIZE   0x0000

#define ISP_STAT_HIST_MEM_OFS       0x0
#define ISP_STAT_AE_MEM_OFS         (ISP_STAT_HIST_MEM_OFS + ISP_STAT_HIST_MEM_SIZE)
#define ISP_STAT_AWB_MEM_OFS        (ISP_STAT_AE_MEM_OFS   + ISP_STAT_AE_MEM_SIZE)
#define ISP_STAT_AF_MEM_OFS         (ISP_STAT_AWB_MEM_OFS  + ISP_STAT_AWB_MEM_SIZE)
#define ISP_STAT_AFS_MEM_OFS        (ISP_STAT_AF_MEM_OFS   + ISP_STAT_AF_MEM_SIZE)
#define ISP_STAT_AWB_WIN_MEM_OFS    (ISP_STAT_AFS_MEM_OFS   + ISP_STAT_AFS_MEM_SIZE)

/* table size configs */

#define ISP_LINEAR_LUT_LENS_GAMMA_MEM_SIZE 0x1000
#define ISP_LUT_MEM_SIZE            0x0400
#define ISP_LENS_MEM_SIZE           0x0600
#define ISP_GAMMA_MEM_SIZE          0x0200
#define ISP_LINEAR_MEM_SIZE          0x0


#define ISP_DRC_DISC_MEM_SIZE            0x0200
#define ISP_DRC_MEM_SIZE            0x0200
#define ISP_DISC_MEM_SIZE          0


#define MAX_CH_NUM      4
#define MAX_INPUT_NUM   2     /* the maximum number of device connected to the same bus */
#define MAX_ISP_STAT_BUF  5   /* the maximum number of isp statistic buffer */
#define MAX_SENSOR_DETECT_NUM  3   /* the maximum number of detect sensor on the same bus. */

#define MAX_AF_WIN_NUM 1
#define MAX_AE_WIN_NUM 1

#endif /*_SUN8IW11P1_VFE_CFG_H_*/


