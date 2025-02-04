/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#ifndef _ESMERROR_H_
#define _ESMERROR_H_

/* * Success criteria for each function */
#define ESM_HL_SUCCESS                      0
/* * General Failure for each function */
#define ESM_HL_FAILED                      (-1)
/* * Bad input parameter */
#define ESM_HL_INVALID_PARAMETERS          (-2)
/* * Mailbox failed */
#define ESM_HL_MB_FAILED                   (-3)
/* * REQ and RESP mismatch */
#define ESM_HL_REQ_RESP_MISMATCH           (-4)
/* * HPI command timeout */
#define ESM_HL_COMMAND_TIMEOUT             (-5)
/* * ESM code failed to load */
#define ESM_HL_FAILED_TO_LOAD_CODE         (-6)
/* * Failed to set code address */
#define ESM_HL_FAILED_TO_SET_CODE_ADDR     (-7)
/* * Failed to set data address */
#define ESM_HL_FAILED_TO_SET_DATA_ADDR     (-8)
/* * Failed to set valid bit on ESM */
#define ESM_HL_FAILED_TO_SET_VLD_BIT       (-9)
/* * Failed to read the configuration data */
#define ESM_HL_FAILED_TO_RD_CONFIG_DATA    (-10)
/* * Failed to parse the configuration data */
#define ESM_HL_FAILED_TO_PARSE_CONFIG_DATA (-11)
/* * Failed to enable IRQ */
#define ESM_HL_ENABLE_IRQ_FAILED           (-12)
/* * Failed to disable IRQ */
#define ESM_HL_DISABLE_IRQ_FAILED          (-13)
/* * Input buffer is too small */
#define ESM_HL_BUFFER_TOO_SMALL            (-14)
/* * Invalid command */
#define ESM_HL_INVALID_COMMAND             (-15)
/* * No ESM Host Lib instance passed into function */
#define ESM_HL_NO_INSTANCE                 (-16)
/* * HLD driver data write failed */
#define ESM_HL_DRIVER_DATA_WRITE_FAILED    (-17)
/* * HLD driver data read failed */
#define ESM_HL_DRIVER_DATA_READ_FAILED     (-18)
/* * HLD driver data set failed */
#define ESM_HL_DRIVER_DATA_SET_FAILED      (-19)
/* * HLD driver HPI write failed */
#define ESM_HL_DRIVER_HPI_WRITE_FAILED     (-20)
/* * HLD driver HPI read failed */
#define ESM_HL_DRIVER_HPI_READ_FAILED      (-21)
/* * Hostlib does not support esm fw version */
#define ESM_HL_INVALID_ESM_FIRMWARE_VERSION (-22)

#endif
