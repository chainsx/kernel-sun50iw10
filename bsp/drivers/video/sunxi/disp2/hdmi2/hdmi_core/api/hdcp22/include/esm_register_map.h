/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#ifndef _ESM_REGISTERS_H_
#define _ESM_REGISTERS_H_


/* global memory regions */
#define ESM_EXTERNAL_TCM         0x00800000 /* internal SRAM memory */

#ifdef TARGET_ENSILICA_BSP
#define ESM_EXTERNAL_RAM         0x00900000 /* external SRAM memory */
#endif
#define ESM_PERIPHERAL_MAP_START 0x80000000
#define ESM_PERIPHERAL_MAP_END   0x800003FF
#define ESM_TIMERS_START         0x80001000
#define ESM_TIMER0_START         (ESM_TIMERS_START + 0x00) /* multi timer offset is 0x10 */
#define ESM_TIMER1_START         (ESM_TIMERS_START + 0x10)
#define ESM_TIMER2_START         (ESM_TIMERS_START + 0x20)
#define ESM_TIMER3_START         (ESM_TIMERS_START + 0x30)
#define ESM_TIMER4_START         (ESM_TIMERS_START + 0x40)
#define ESM_TIMER5_START         (ESM_TIMERS_START + 0x50)
#define ESM_TIMERS_END           0x800010FF
#define ESM_WATCHDOG_START       0x80002000
#define ESM_WATCHDOG_END         0x800020FF
#define ESM_I2C_ESI_SL_MST_START 0x80003000
#define ESM_I2C_ESI_SL_MST_END   0x800030FF
#define ESM_UART_START           0x80004000
#define ESM_UART_END             0x800040FF
#define ESM_I2C_ELP_MASTER_START 0x80020000
#define ESM_I2C_ELP_MASTER_END   0x800200FF
#define ESM_HPI_START            0x80020100
#define ESM_HPI_END              0x800201FF
#define ESM_CPI_START            0x80020200
#define ESM_CPI_END              0x800202FF
#define ESM_SKP_START            ESM_CPI_START
#define ESM_SKP_END              ESM_CPI_END
#define ESM_AIC_START            0x80020300
#define ESM_AIC_END              0x800203FF
#define ESM_SYS_START            0x80020400
#define ESM_SYS_END              0x800204FF
#define ESM_SHALE_AUX_TX_START   0x80020500
#define ESM_SHALE_AUX_TX_END     0x800205FF
#define ESM_SHALE_I2C_TX_START   0x80020600
#define ESM_SHALE_I2C_TX_END     0x800206FF
#define ESM_ELP_I2C_MST_START    0x80020700
#define ESM_ELP_I2C_MST_END      0x800207FF
#define ESM_CEE0_START           0x80020800
#define ESM_CEE0_END             0x800208FF
#define ESM_CEE1_START           0x80020900
#define ESM_CEE1_END             0x800209FF
#define ESM_CEE2_START           0x80020A00
#define ESM_CEE2_END             0x80020AFF
#define ESM_CEE3_START           0x80020B00
#define ESM_CEE3_END             0x80020BFF
#define ESM_CEE4_START           0x80020C00
#define ESM_CEE4_END             0x80020CFF
#define ESM_CEE5_START           0x80020D00
#define ESM_CEE5_END             0x80020DFF
#define ESM_CEE6_START           0x80020E00
#define ESM_CEE6_END             0x80020EFF
#define ESM_CEE7_START           0x80020F00
#define ESM_CEE7_END             0x80020FFF
#define ESM_AES0_START           0x80021000
#define ESM_AES0_END             0x800210FF
#define ESM_AES1_START           0x80021100
#define ESM_AES1_END             0x800211FF
#define ESM_AES2_START           0x80021200
#define ESM_AES2_END             0x800212FF
#define ESM_AES3_START           0x80021300
#define ESM_AES3_END             0x800213FF
#define ESM_AES4_START           0x80021400
#define ESM_AES4_END             0x800214FF
#define ESM_AES5_START           0x80021500
#define ESM_AES5_END             0x800215FF
#define ESM_AES6_START           0x80021600
#define ESM_AES6_END             0x800216FF
#define ESM_AES7_START           0x80021700
#define ESM_AES7_END             0x800217FF



/* secure key port */
#define ESM_REG_IV_WATERMARK0     (ESM_SKP_START + 0x00)
#define ESM_REG_IV_WATERMARK1     (ESM_SKP_START + 0x04)
#define ESM_REG_IV_WATERMARK2     (ESM_SKP_START + 0x08)
#define ESM_REG_IV_WATERMARK_SIZE 12

#define ESM_REG_FUSE_PLAT_KEY0     (ESM_SKP_START + 0x10)
#define ESM_REG_FUSE_PLAT_KEY1     (ESM_SKP_START + 0x14)
#define ESM_REG_FUSE_PLAT_KEY2     (ESM_SKP_START + 0x18)
#define ESM_REG_FUSE_PLAT_KEY3     (ESM_SKP_START + 0x1C)
#define ESM_REG_FUSE_PLAT_KEY_SIZE 16

#define ESM_REG_NONCE0    (ESM_SKP_START + 0x20)
#define ESM_REG_NONCE1    (ESM_SKP_START + 0x24)
#define ESM_REG_NONCE2    (ESM_SKP_START + 0x28)
#define ESM_REG_NONCE3    (ESM_SKP_START + 0x2C)
#define ESM_REG_NONCE_SIZE 16

#define ESM_REG_DUK0    (ESM_SKP_START + 0x40)
#define ESM_REG_DUK1    (ESM_SKP_START + 0x44)
#define ESM_REG_DUK2    (ESM_SKP_START + 0x48)
#define ESM_REG_DUK3    (ESM_SKP_START + 0x4C)
#define ESM_REG_DUK_SIZE 16

#define ESM_REG_SKP_IRQ_EN              (ESM_SKP_START + 0x50)
#define ESM_REG_SKP_IRQ_EN_IN_BIT        0
#define ESM_REG_SKP_IRQ_EN_NONCE_VLD_BIT 30
#define ESM_REG_SKP_IRQ_EN_GLBL_BIT      31
#define ESM_REG_SKP_IRQ_EN_SIZE          4

#define ESM_REG_SKP_IRQ_STAT               (ESM_SKP_START + 0x54)
#define ESM_REG_SKP_IRQ_STAT_IN_BIT        0
#define ESM_REG_SKP_IRQ_STAT_NONCE_VLD_BIT 30
#define ESM_REG_SKP_IRQ_STAT_SIZE          4

/* 8-bit interface for Samsung */
#define ESM_REG_SKP_DCP_DIRECT_RD_ADDR      (ESM_SKP_START + 0x84)
#define ESM_REG_SKP_DCP_DIRECT_RD_ADDR_SIZE 4
#define ESM_REG_SKP_DCP_DIRECT_RD_DATA      (ESM_SKP_START + 0x88)
#define ESM_REG_SKP_DCP_DIRECT_RD_DATA_SIZE 4

/* CPI bits starting at ESM_REG_SKP_IRQ_*_IN_BIT */
#define ESM_REG_SKP_IRQ_HPD_BIT        (ESM_REG_SKP_IRQ_STAT_IN_BIT + 0)
#define ESM_REG_SKP_IRQ_I2C_GRANT_BIT  (ESM_REG_SKP_IRQ_STAT_IN_BIT + 1)

/* PIN[2..3] are reserved for application use */
#define ESM_REG_SKP_IRQ_EXIT_BIT       (ESM_REG_SKP_IRQ_STAT_IN_BIT + 29)

#define ESM_REG_SKP_STAT               (ESM_SKP_START + 0x58)
#define ESM_REG_SKP_STAT_NONCE_VLD_BIT 30
#define ESM_REG_SKP_STAT_SIZE          4

#define ESM_REG_SKP_CTRL                (ESM_SKP_START + 0x5C)
#define ESM_REG_SKP_CTRL_SIZE            4
#define ESM_REG_SKP_CTRL_NONCE_RFRSH_BIT 0

/* HPI Port */
#define ESM_REG_DESIGN_ID0(__base__)  ((__base__) + 0x00)
#define ESM_REG_DESIGN_ID1(__base__)  ((__base__) + 0x01)
#define ESM_REG_DESIGN_ID2(__base__)  ((__base__) + 0x02)
#define ESM_REG_DESIGN_ID3(__base__)  ((__base__) + 0x03)
#define ESM_REG_DESIGN_ID_HW_REVISION_BIT  2
#define ESM_REG_DESIGN_ID_HW_REVISION_SIZE 6
#define ESM_REG_DESIGN_ID_HW_REVISION_MASK 0x000000fc
#define ESM_REG_DESIGN_ID_SIZE             4

#define ESM_REG_DESIGN_ID_HW_REVISION_INITIAL_RELEASE_VALUE 1
#define ESM_REG_DESIGN_ID_HW_REVISION_CEE_REDUCED_VALUE     2
#define ESM_REG_DESIGN_ID_HW_REVISION_CEE_SW_RESET          3
#define ESM_REG_DESIGN_ID_HW_REVISION_CEE_PROG_WOO_VALUE    4

#define ESM_REG_PRODUCT_ID0(__base__) ((__base__) + 0x04)
#define ESM_REG_PRODUCT_ID1(__base__) ((__base__) + 0x05)
#define ESM_REG_PRODUCT_ID2(__base__) ((__base__) + 0x06)
#define ESM_REG_PRODUCT_ID3(__base__) ((__base__) + 0x07)
#define ESM_REG_PRODUCT_ID_MINOR_ID_BIT  0
#define ESM_REG_PRODUCT_ID_MINOR_ID_SIZE 16
#define ESM_REG_PRODUCT_ID_MINOR_ID_MASK 0x0000ffff
#define ESM_REG_PRODUCT_ID_MAJOR_ID_BIT  16
#define ESM_REG_PRODUCT_ID_MAJOR_ID_SIZE 16
#define ESM_REG_PRODUCT_ID_MAJOR_ID_MASK 0xffff0000
#define ESM_REG_PRODUCT_ID_SIZE          4

#define ESM_REG_PRODUCT_ID_MINOR_ID_HG_VALUE               1
#define ESM_REG_PRODUCT_ID_MINOR_ID_MERLIN_VALUE           2
#define ESM_REG_PRODUCT_ID_MINOR_ID_FRANKIE_VALUE          3
#define ESM_REG_PRODUCT_ID_MINOR_ID_CUSTOM_DCP_INTF_VALUE  4

#define ESM_REG_PRODUCT_ID_MAJOR_ID_HGF_VALUE              1
#define ESM_REG_PRODUCT_ID_MAJOR_ID_GRIFFIN_VALUE          2



#define ESM_REG_FW_INFO00(__base__) ((__base__) + 0x08)
#define ESM_REG_FW_INFO01(__base__) ((__base__) + 0x09)
#define ESM_REG_FW_INFO02(__base__) ((__base__) + 0x0A)
#define ESM_REG_FW_INFO03(__base__) ((__base__) + 0x0B)
#define ESM_REG_FW_INFO_SIZE         4

#define ESM_REG_AE_IRQ_EN(__base__)  ((__base__) + 0x10)
#define ESM_REG_AE_IRQ_EN_GLBL_BIT   0
#define ESM_REG_AE_IRQ_EN_MB_MSG_BIT 1
#define ESM_REG_AE_IRQ_EN_MB_RTN_BIT 2
#define ESM_REG_AE_IRQ_EN_AE_GO_BIT  7
#define ESM_REG_AE_IRQ_EN_SIZE       4

#define ESM_REG_AE_IRQ_STAT(__base__) ((__base__) + 0x14)
#define ESM_REG_AE_IRQ_STAT_MB_MSG_BIT 1
#define ESM_REG_AE_IRQ_STAT_MB_RTN_BIT 2
#define ESM_REG_AE_IRQ_STAT_AE_GO_BIT  7
#define ESM_REG_AE_IRQ_STAT_SIZE       4 /* 1 byte valid */

#define ESM_REG_AE_2HP_MB(__base__) ((__base__) + 0x18)
#define ESM_REG_AE_2HP_MB_SIZE      4 /* 1 byte valid */

#define ESM_REG_AE_MB_STAT(__base__)  ((__base__) + 0x1C)
#define ESM_REG_AE_MB_STAT_MB_RTN_BIT 2
#define ESM_REG_AE_MB_STAT_SIZE       4 /* 1 byte valid */

#define ESM_REG_HP_2AE_MB(__base__) ((__base__) + 0x28)
#define ESM_REG_HP_2AE_MB_SIZE       4 /* 1 byte valid */

#define ESM_REG_HP_IRQ_STAT_MSG(__base__)       ((__base__) + 0x24)
#define ESM_REG_HP_IRQ_STAT_MSG_MB_MSG_BIT       1
#define ESM_REG_HP_IRQ_STAT_MSG_MB_RTN_BIT       2
#define ESM_REG_HP_IRQ_STAT_MSG_STAT_UPDATED_BIT 3
#define ESM_REG_HP_IRQ_STAT_MSG_USER_DEF_BIT     4
#define ESM_REG_HP_IRQ_STAT_MSG_SYNC_LOST_BIT    5
#define ESM_REG_HP_IRQ_STAT_MSG_AUTH_PASS_BIT    6
#define ESM_REG_HP_IRQ_STAT_MSG_AUTH_FAIL_BIT    7
#define ESM_REG_HP_IRQ_STAT_MSG_SIZE             4

#define ESM_REG_HP_2AE_MB_STAT(__base__)  ((__base__) + 0x2c)
#define ESM_REG_HP_2AE_MB_STAT_MB_RTN_BIT 2
#define ESM_REG_HP_2AE_MB_STAT_SIZE       4 /* 1 byte valid */

#define ESM_REG_AE_STAT_MSG(__base__)     ((__base__) + 0x30)
#define ESM_REG_AE_STAT_MSG_USER_DEF_BIT  4
#define ESM_REG_AE_STAT_MSG_SYNC_LOST_BIT 5
#define ESM_REG_AE_STAT_MSG_AUTH_PASS_BIT 6
#define ESM_REG_AE_STAT_MSG_AUTH_FAIL_BIT 7
#define ESM_REG_AE_STAT_MSG_SIZE          4

#define ESM_REG_HP_FW_BASE0(__base__) ((__base__) + 0x40)
#define ESM_REG_HP_FW_BASE1(__base__) ((__base__) + 0x41)
#define ESM_REG_HP_FW_BASE2(__base__) ((__base__) + 0x42)
#define ESM_REG_HP_FW_BASE3(__base__) ((__base__) + 0x43)
#define ESM_REG_HP_FW_BASE_SIZE       4

#define ESM_REG_HP_FW_TOP0(__base__) ((__base__) + 0x44)
#define ESM_REG_HP_FW_TOP1(__base__) ((__base__) + 0x45)
#define ESM_REG_HP_FW_TOP2(__base__) ((__base__) + 0x46)
#define ESM_REG_HP_FW_TOP3(__base__) ((__base__) + 0x47)
#define ESM_REG_HP_FW_TOP_SIZE        4

#define ESM_REG_HP_DATA_BASE0(__base__) ((__base__) + 0x48)
#define ESM_REG_HP_DATA_BASE1(__base__) ((__base__) + 0x49)
#define ESM_REG_HP_DATA_BASE2(__base__) ((__base__) + 0x4A)
#define ESM_REG_HP_DATA_BASE3(__base__) ((__base__) + 0x4B)
#define ESM_REG_HP_DATA_BASE_SIZE       4

#define ESM_REG_HP_DATA_TOP0(__base__) ((__base__) + 0x4C)
#define ESM_REG_HP_DATA_TOP1(__base__) ((__base__) + 0x4D)
#define ESM_REG_HP_DATA_TOP2(__base__) ((__base__) + 0x4E)
#define ESM_REG_HP_DATA_TOP3(__base__) ((__base__) + 0x4F)
#define ESM_REG_HP_DATA_TOP_SIZE       4

#define ESM_REG_HP_MB_P00(__base__) ((__base__) + 0x50)
#define ESM_REG_HP_MB_P01(__base__) ((__base__) + 0x51)
#define ESM_REG_HP_MB_P02(__base__) ((__base__) + 0x52)
#define ESM_REG_HP_MB_P03(__base__) ((__base__) + 0x53)
#define ESM_REG_AE_MB_P0x_SIZE      4

#define ESM_REG_HP_MB_P10(__base__) ((__base__) + 0x54)
#define ESM_REG_HP_MB_P11(__base__) ((__base__) + 0x55)
#define ESM_REG_HP_MB_P12(__base__) ((__base__) + 0x56)
#define ESM_REG_HP_MB_P13(__base__) ((__base__) + 0x57)
#define ESM_REG_AE_MB_P1x_SIZE      4

#define ESM_REG_HP_MB_P10(__base__) ((__base__) + 0x54)
#define ESM_REG_HP_MB_P11(__base__) ((__base__) + 0x55)
#define ESM_REG_HP_MB_P12(__base__) ((__base__) + 0x56)
#define ESM_REG_HP_MB_P13(__base__) ((__base__) + 0x57)
#define ESM_REG_AE_MB_P1x_SIZE      4

#define ESM_REG_AE_MB_P00(__base__) ((__base__) + 0x58)
#define ESM_REG_AE_MB_P01(__base__) ((__base__) + 0x59)
#define ESM_REG_AE_MB_P02(__base__) ((__base__) + 0x5A)
#define ESM_REG_AE_MB_P03(__base__) ((__base__) + 0x5B)
#define ESM_REG_AE_MB_P0_SIZE       4

#define ESM_REG_AE_ERR_STAT0(__base__) ((__base__) + 0x60)
#define ESM_REG_AE_ERR_STAT1(__base__) ((__base__) + 0x61)
#define ESM_REG_AE_ERR_STAT2(__base__) ((__base__) + 0x62)
#define ESM_REG_AE_ERR_STAT3(__base__) ((__base__) + 0x63)
#define ESM_REG_AE_ERR_STAT_SIZE       4

/* CPI Port */
#define ESM_REG_CPI_OUT      (ESM_CPI_START + 0x60)
#define ESM_REG_CPI_BOOTED_BIT 6
#define ESM_REG_CPI_OUT_SIZE   4

#define ESM_REG_CPI_STAT         (ESM_CPI_START + 0x64)
#define ESM_REG_CPI_STAT_BUSY_BIT 0
#define ESM_REG_CPI_STAT_SIZE     4

#define ESM_REG_CPI_IN_DIR      (ESM_CPI_START + 0x68)
#define ESM_REG_CPI_IN_DIR_SIZE 4

#define ESM_REG_CPI_RISING_EN      (ESM_CPI_START + 0x70)
#define ESM_REG_CPI_RISING_EN_SIZE 4

#define ESM_REG_CPI_FALLING_EN      (ESM_CPI_START + 0x74)
#define ESM_REG_CPI_FALLING_EN_SIZE 4

#define ESM_REG_SKP_MSTR_PROT       (ESM_CPI_START + 0x80)
#define ESM_REG_SKP_MSTR_PROT_I_PROT_BIT 0
#define ESM_REG_SKP_MSTR_PROT_D_PROT_BIT 1
#define ESM_REG_SKP_MSTR_PROT_SIZE 4


/* CEE */

#define ESM_REG_CEE_IRQ_EN(__cee__)     ((__cee__) + 0x00)
#define ESM_REG_CEE_IRQ_EN_DIAG_UPD_BIT 0
#define ESM_REG_CEE_IRQ_EN_LINK_ERR_BIT 1
#define ESM_REG_CEE_IRQ_EN_FIFO_ERR_BIT 2
#define ESM_REG_CEE_IRQ_EN_GLBL_BIT     31
#define ESM_REG_CEE_IRQ_EN_SIZE         4
#define ESM_REG_CEE_IRQ_STAT(__cee__)     ((__cee__) + 0x04)
#define ESM_REG_CEE_IRQ_STAT_DIAG_UPD_BIT  0
#define ESM_REG_CEE_IRQ_STAT_LINK_ERR_BIT  1
#define ESM_REG_CEE_IRQ_STAT_FIFO_ERR_BIT  2
#define ESM_REG_CEE_IRQ_STAT_SIZE          4
#define ESM_REG_CEE_CTRL(__cee__)       ((__cee__) + 0x08)
#define ESM_REG_CEE_CTRL_ENUM_NOP        0
#define ESM_REG_CEE_CTRL_ENUM_INIT       1 /* also known as "setup" */
#define ESM_REG_CEE_CTRL_ENUM_PRE_AUTH   2
#define ESM_REG_CEE_CTRL_ENUM_POST_AUTH  3
#define ESM_REG_CEE_CTRL_ENUM_LOST_AUTH  4
#define ESM_REG_CEE_CTRL_ENUM_EXIT       5 /* bypass mode */
#define ESM_REG_CEE_CTRL_CMD_MODE_MASK   0x00000007
#define ESM_REG_CEE_CTRL_CDO_CD_MASK     0x000000F8
#define ESM_REG_CEE_CTRL_CD_OVERRIDE_BIT 3
#define ESM_REG_CEE_CTRL_CDEPTH_BIT      4
#define ESM_REG_CEE_CTRL_CDEPTH_SIZE     4
#define ESM_REG_CEE_CTRL_SIZE            4

#define ESM_REG_CEE_BSOD(__cee__)        ((__cee__) + 0x10)
#define ESM_REG_CEE_BSOD_SIZE            4
#define ESM_REG_CEE_RAND_IV0(__cee__)    ((__cee__) + 0x20)
#define ESM_REG_CEE_RAND_IV1(__cee__)    ((__cee__) + 0x24)
#define ESM_REG_CEE_RAND_IV_SIZE         8

#define ESM_REG_CEE_SESS_KEY0(__cee__)   ((__cee__) + 0x30)
#define ESM_REG_CEE_SESS_KEY1(__cee__)   ((__cee__) + 0x34)
#define ESM_REG_CEE_SESS_KEY2(__cee__)   ((__cee__) + 0x38)
#define ESM_REG_CEE_SESS_KEY3(__cee__)   ((__cee__) + 0x3C)
#define ESM_REG_CEE_SESS_KEY_SIZE        16

#define ESM_REG_CEE_PREV_DAT_DIAG0(__cee__)  ((__cee__) + 0x40)
#define ESM_REG_CEE_PREV_DAT_DIAG0_SIZE      4

#define ESM_REG_CEE_FR_NUM_DIAG0(__cee__)    ((__cee__) + 0x44) /* FR_NUM_31_0 31:0 RO Frame number is composed of the following 38-bit concatenation: {FR_MUN_37_32, FR_NUM_31_0}. */
#define ESM_REG_CEE_FR_NUM_DIAG1(__cee__)    ((__cee__) + 0x48) /* FR_MUN_37_32 5:0 RO */
#define ESM_REG_CEE_FR_NUM_DIAG_SIZE         8

#define ESM_REG_CEE_ETC_DIAG(__cee__)           ((__cee__) + 0x4C) /* LINK_ERR_CNT 6:0 RO COLOUR_DEPTH 11:8 RO BRG_PLANKS 14:12 RO ENC_DIS 16 RO ENC_EN 17 RO */
#define ESM_REG_CEE_ETC_DIAG_LINK_ERR_CNT_LSB   0
#define ESM_REG_CEE_ETC_DIAG_LINK_ERR_CNT_MASK  0x0000007f
#define ESM_REG_CEE_ETC_DIAG_COLOUR_DEPTH_LSB   8
#define ESM_REG_CEE_ETC_DIAG_COLOUR_DEPTH_MASK  0x00000f00
#define ESM_REG_CEE_ETC_DIAG_BRG_PLANKS_LSB     12
#define ESM_REG_CEE_ETC_DIAG_BRG_PLANKS_MASK    0x00007000
#define ESM_REG_CEE_ETC_DIAG_ENC_DIS_LSB        16
#define ESM_REG_CEE_ETC_DIAG_ENC_DIS_MASK       0x00010000
#define ESM_REG_CEE_ETC_DIAG_ENC_EN_LSB         17
#define ESM_REG_CEE_ETC_DIAG_ENC_EN_MASK        0x00020000
#define ESM_REG_CEE_ETC_DIAG_SIZE               4

#define ESM_REG_CEE_FSM_STATE(__cee__)        ((__cee__) + 0x50)
#define ESM_REG_CEE_FSM_STATE_ENC_BYPASS_MASK  0x00000001
#define ESM_REG_CEE_FSM_STATE_ENC_NOAUTH_MASK  0x00000002
#define ESM_REG_CEE_FSM_STATE_ENC_PREAUTH_MASK 0x00000004
#define ESM_REG_CEE_FSM_STATE_ENC_AUTH_MASK    0x00000008
#define ESM_REG_CEE_FSM_STATE_VIF_BYPASS_MASK  0x00000010
#define ESM_REG_CEE_FSM_STATE_VIF_NOAUTH_MASK  0x00000020
#define ESM_REG_CEE_FSM_STATE_VIF_PREAUTH_MASK 0x00000040
#define ESM_REG_CEE_FSM_STATE_VIF_AUTH_MASK    0x00000080
#define ESM_REG_CEE_FSM_STATE_ENC_MASK         0x0000ffff
#define ESM_REG_CEE_FSM_STATE_VIF_MASK         0xffff0000
#define ESM_REG_CEE_FSM_STATE_SIZE            4

#define ESM_REG_CEE_TMDS_CHK_ALIVE(__cee__)   ((__cee__) + 0x54)
#define ESM_REG_CEE_TMDS_CHK_ALIVE_SIZE       4

#define ESM_REG_CEE_RX_MODE(__cee__)          ((__cee__) + 0x5c)
#define ESM_REG_CEE_RX_MODE_RX_MODE           1
#define ESM_REG_CEE_RX_MODE_SIZE              4

#define ESM_REG_CEE_WOO_BEGINS(__cee__)  ((__cee__) + 0x78)
#define ESM_REG_CEE_WOO_BEGINS_SIZE      4

#define ESM_REG_CEE_WOO_ENDS(__cee__)    ((__cee__) + 0x7c)
#define ESM_REG_CEE_WOO_ENDS_SIZE        4

#define ESM_REG_CEE_WOO_CTLS(__cee__)    ((__cee__) + 0x80)
#define ESM_REG_CEE_WOO_CTLS_SIZE        4

/* System */
#define ESM_REG_SYS_CEE_RST      (ESM_SYS_START + 0x00)
#define ESM_REG_SYS_CEE_RST_SIZE 4

#define ESM_REG_GET_BIT(__reg__, __bit_pos__)   (((__reg__) >> (__bit_pos__)) & 0x00000001)
#define ESM_REG_SET_BIT(__reg__, __bit_pos__)   ((__reg__) = ((__reg__) | (0x00000001 << (__bit_pos__))))
#define ESM_REG_CLEAR_BIT(__reg__, __bit_pos__) ((__reg__) = ((__reg__) & (~(0x00000001 << (__bit_pos__)))))

#endif /* _ESM_REGISTERS_H_ */

