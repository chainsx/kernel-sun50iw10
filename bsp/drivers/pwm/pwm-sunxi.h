/*
 * Allwinnertech pulse-width-modulation controller driver
 *
 * Copyright (C) 2018 AllWinner
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#ifndef PWM_SUNXI_H
#define PWM_SUNXI_H

#define PWM_PIER	(0x0000)
#define PWM_PISR	(0x0004)
#define	PWM_CIER	(0x0010)
#define	PWM_CISR	(0x0014)
#define PWM_PCCR01	(0x0020)
#define PWM_PCCR23	(0x0024)
#define PWM_PCCR45	(0x0028)
#define PWM_PCCR67	(0x002c)
#define PWM_PCCR89	(0x0030)
#define PWM_PCCRAB	(0x0034)
#define PWM_PCCRCD	(0x0038)
#define PWM_PCCREF	(0x003c)

#define PWM_PCGR	(0x0040)

#define	PWM_PDZCR67	(0x006c)
#define	PWM_PDZCR89	(0x0070)
#define	PWM_PDZCRAB	(0x0074)
#define	PWM_PDZCRCD	(0x0078)
#define	PWM_PDZCREF	(0x007c)

#define PWM_PGR0	(0x0090)
#define PWM_PGR1	(0x0094)

#define PPCNTP_BASE     (0x0100 + 0x000c)
/* #define PWM_PCCR8	(0x0300) */

#define PWMG_CS_SHIFT           0
#define PWMG_CS_WIDTH           16
#define PWMG_EN_SHIFT           16
#define PWMG_START_SHIFT        17
#define PWM_COUNTER_START_SHIFT 16
#define PWM_COUNTER_START_WIDTH 16
#define PWM_PUL_START_SHIFT     10
#define PWM_PUL_START_WIDTH     1
#define PWM_PUL_NUM_SHIFT       16
#define PWM_PUL_NUM_WIDTH       16
#define PWM_MODE_ACTS_SHIFT	8
#define PWM_MODE_ACTS_WIDTH     2
#define PWM_ACT_STA_SHIFT	0x8
#define PWM_ACT_STA_WIDTH	0x1
#define PWM_CLK_SRC_SHIFT	0x7
#define PWM_CLK_SRC_WIDTH	0x2
#define PWM_DIV_M_SHIFT		0x0
#define PWM_DIV_M_WIDTH		0x4
#define PWM_PRESCAL_SHIFT	0x0
#define PWM_PRESCAL_WIDTH	0x8
#define PWM_ACT_CYCLES_SHIFT	0x0
#define PWM_ACT_CYCLES_WIDTH	0x10
#define PWM_PERIOD_CYCLES_SHIFT	0x10
#define PWM_PERIOD_CYCLES_WIDTH	0x10
#define PWM_DZ_EN_SHIFT		0x0
#define PWM_DZ_EN_WIDTH		0x1
#define PWM_PDZINTV_SHIFT	0x8
#define PWM_PDZINTV_WIDTH	0x8
#define PWM_BYPASS_SHIFT	0x5
#define PWM_BYPASS_WIDTH	0x1
#define PWM_CLK_GATING_SHIFT	0x4
#define PWM_CLK_GATING_WIDTH	0x1
#define PWM_REG_UNIFORM_OFFSET	0x20 /* multiple registers use uniform offset */

#endif
