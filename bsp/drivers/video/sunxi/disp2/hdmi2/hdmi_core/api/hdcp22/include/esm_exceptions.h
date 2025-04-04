/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#ifndef _ESM_EXCEPTIONS_H_
#define _ESM_EXCEPTIONS_H_

/* exception structure (80 Bytes) */
#define ESM_SYSTEM_EXCEPT_MAXFILE_NAME 16

typedef struct _esm_exception_info_ {
	uint32_t id;
	uint32_t exception_flag;
	uint32_t exception_value;
	uint32_t action;
	uint32_t file_line;
	uint8_t file[ESM_SYSTEM_EXCEPT_MAXFILE_NAME];
} esm_exception_info;

#define ESM_SYSEXCEPT_START_ABORT  0
#define ESM_SYSEXCEPT_START_NOTIFY 32

/* establish starting points for application code */
#define ESM_SYSEXCEPT_START    64
#define ESM_SYSEXCEPT_SC_START 256
#define ESM_SYSEXCEPT_SA_START 320
#define ESM_SYSEXCEPT_MASK  0x7fffffff

#endif
