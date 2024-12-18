/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#ifndef _ESM_HOST_LIB_INTERNAL_H_
#define _ESM_HOST_LIB_INTERNAL_H_

#include "ESMHostTypes.h"
#include "ESMError.h"
#include "ESMHostLibDriverErrors.h"


/* Import relevant definitions from the ESM */
#include "esm_hdcp_hdmi_image_read.h"
#include "esm_errors.h"
#include "esm_register_map.h"
#include "esm_cmd.h"
#include "esm_hdcp_hdmi_tx_cmd.h"
#include "esm_config.h"
#include "esm_system.h"
#include "esm_exceptions.h"
#include "esm_hdcp_hdmi_id.h"

/* #define ESM_APP_MODE  ESM_HDCP_RX_TX_APP */
#include "esm_hdcp_hdmi_config.h"
#include "esm_hdcp_hdmi_image_common.h"

/* Import relevant definitions from the COMMON */

/* ESM FIRMWARE which is supported */
#define HOSTLIB_ESM_MAJOR            0x02
#define HOSTLIB_ESM_MINOR            0x00

#define HOSTLIB_ESM_APP_RX_VERSION   0x09
#define HOSTLIB_ESM_APP_TX_VERSION   0x0C
#define HOSTLIB_ESM_APP_RPRX_VERSION 0x08
#define HOSTLIB_ESM_APP_RPTX_VERSION 0x09

#define CMD_DEFAULT_TIMEOUT -1

#define ESM_DATA_MEMORY_SIZE (128*1024)

#define HPI_HOST_OFF                 0

#define HP_CTRL                      0x34

#define HP_IRQ_EN                    0x20

#define HP_IRQ_GLBL_EN_BIT		0 /* Globally enables the AE to HP interrupt subsystem */
					/* available in the HP_IRQ_STAT register. */
#define HP_IRQ_MB_MSG_EN_BIT		1 /* Enables the individual AE to HP */
#define HP_IRQ_MB_RTN_EN_BIT		2
#define HP_IRQ_STAT_UPDATED_EN_BIT	3
#define HP_IRQ_LOG_SIZE_STAT_BIT	4
#define HP_IRQ_SYNC_LOST_EN_BIT		5
#define HP_IRQ_AUTH_PASS_EN_BIT		6
#define HP_IRQ_AUTH_FAIL_EN_BIT		7

#define ESM_REG_HP_IRQ_BIT_SET(_val_, _bit_)  (((_val_)&(1<<(_bit_))) == (1<<(_bit_)))

#define ESM_REG_HP_IRQ_EN(__base__)  ((__base__) + HP_IRQ_EN)

#define HP_IRQ_STAT                  0x24

#define HP_IRQ_MB_MSG_STAT_BIT		1 /* Indicates that the AE has written a message to */
					/* the AE to HP mailbox. */
#define HP_IRQ_MB_RTN_STAT_BIT		2 /* Indicates that the AE has returned ownership of */
					/* the HP to AE mailbox backto the HP. */
#define HP_IRQ_STAT_UPDATED_STAT_BIT	3 /* Indicates that the AE_ERR_STAT register has been updated */
#define HP_IRQ_LOG_SIZE_STAT_BIT	4 /* User defined indicator. */
#define HP_IRQ_SYNC_LOST_STAT_BIT	5 /* Indicates that the synchronization lost. */
#define HP_IRQ_AUTH_PASS_STAT_BIT	6 /* Indicates that the AKE passed. */
#define HP_IRQ_AUTH_FAIL_STAT_BIT	7 /* Indicates that the AKE failed. */

/* This is only for polling mode */
#ifndef ESM_HOSTLIB_RESPONSE_CMD_TIMEOUT
#define ESM_HOSTLIB_RESPONSE_CMD_TIMEOUT 2000
#endif
#ifndef ESM_HOSTLIB_RESPONSE_SLEEP
#define ESM_HOSTLIB_RESPONSE_SLEEP       20
#endif

#define ESM_HOST_LIB_HANDLES_MAX  1
#define ESM_HOST_LIB_UNINITIALIZED_HANDLE (-1)

#define ESM_HOST_LIB_RX   1
#define ESM_HOST_LIB_TX   2
#define ESM_HOST_LIB_RPTX 3
#define ESM_HOST_LIB_RPRX 4

#define ESM_LOG_HEADER_SIZE       36
#define ESM_LOG_HEADER_INVALID_ID 0xFFFF

#define ESM_TOTAL_CONFIG_SIZE (ESM_CONFIG_MAX_SIZE*2) /* R/O and R/W data */

#define EX_BUFFER_SIZE 10

/* Logging Header Structure (currently supports 4 fields):
 * [reserverd: 2 bytes][version: 1 byte][current ID: 1 byte] = 4 bytes
 * [id: 1 byte][offset: 4 bytes][num. bytes: 3 bytes]        = 8 bytes <- item 1
 * [id: 1 byte][offset: 4 bytes][num. bytes: 3 bytes]        = 8 bytes <- item 2
 * [id: 1 byte][offset: 4 bytes][num. bytes: 3 bytes]        = 8 bytes <- item 3
 * [id: 1 byte][offset: 4 bytes][num. bytes: 3 bytes]        = 8 bytes <- item 4
 * Total: 36 bytes. */
typedef struct {
	uint8_t ctrl[4];
	uint8_t item1[8];
	uint8_t item2[8];
	uint8_t item3[8];
	uint8_t item4[8];
} esm_log_hdr;

typedef struct {
	uint32_t buffer[EX_BUFFER_SIZE];
	uint8_t head;
	uint8_t tail;
} ring_buffer;

typedef struct {
	u32 code_base;
	u32 code_size;
	unsigned long vir_code_base;

	u32 data_base;
	u32 data_size;
	unsigned long vir_data_base;

	unsigned long hpi_base;
	u32 hpi_size;

	void *instance;

	int (*get_code_phys_addr)(void *instance, u32 *addr);
	int (*get_data_phys_addr)(void *instance, u32 *addr);
	int (*get_data_size)(void *instance, uint32_t *data_size);
	int (*hpi_read)(void *instance, uint32_t offset, uint32_t *data);
	int (*hpi_write)(void *instance, uint32_t offset, uint32_t data);
	int (*data_write)(void *instance, uint32_t offset,
			  uint8_t *src_buf, uint32_t nbytes);
	int (*data_read)(void *instance, uint32_t offset,
			 uint8_t *dest_buf, uint32_t nbytes);
	int (*data_set)(void *instance, uint32_t offset,
			uint8_t data, uint32_t nbytes);
	void (*idle)(void *instance);
} esm_host_driver_t;


/* ESM instance */
typedef struct _esm_instance_t {
	esm_host_driver_t *driver;

	int32_t status;              /* response from the ESM */
	u32 code_mem_phys_addr; /* From driver->get_code_phys_addr()=code_base */
	u32 data_mem_phys_addr; /* From driver->get_data_phys_addr()=data_base */
	uint32_t data_mem_size; /* From driver->get_data_size()=data_size */

	int32_t fw_version; /* Firmware version from the ESM register */
	int32_t fw_type;    /* 1-RX 2-TX 3-RPTR */

	int32_t loaded; /* Internal flag to set when ESL is loaded successfully */

	/* Interrupts */
	uint32_t irq_supported;

	/* Application */
	uint32_t app_id;

	/* ESM CPI */
	uint32_t esm_cpi_off;
	uint32_t esm_cpi_size;

	/* ESM Mailbox */
	uint32_t esm_mb_off;
	uint32_t esm_mb_size;

	/* ESM SRM */
	uint32_t esm_srm_off;
	uint32_t esm_srm_size;

	/* ESM Topology */
	uint32_t esm_topo_off;
	uint32_t esm_topo_size;
	uint8_t esm_topo_slots;
	uint32_t esm_topo_seed_off;
	uint8_t esm_topo_seed_size;

	/* ESM Pairing */
	uint32_t esm_pair_off;
	uint32_t esm_pair_size;

	/* ESM Exceptions */
	uint32_t esm_exc_off;
	uint32_t esm_exceptions_size;
	uint32_t esm_exceptions_last_id;
	ring_buffer exp_status_buffer;

	/* ESM Logs */
	uint32_t esm_log_header_off;
	uint32_t esm_log_off;
	int32_t  esm_log_size;
	int32_t  esm_log_hdr_last_id;   /* Last processed id; */
	uint32_t esm_log_hdr_last_item; /* Last processed id's item ; */

	int32_t log_read_ptr;           /* offset of the current read */
	int32_t esm_log_notified_last;  /* last notified size */
	int32_t esm_log_notified_total; /* total notified log size */

	/* Callbacks */
	void (*esm_GetLogCallback)(struct _esm_instance_t *esm, uint32_t BufferSize, uint8_t *Buffer);

	/* Status snapshots filled by the ISR and then passed to the user apps */
	uint32_t esm_sync_lost;
	uint32_t esm_auth_pass;
	uint32_t esm_auth_fail;
	uint32_t esm_exception;

	/* ESM Control and configuration structure */
	esm_controls esm_ctrl;
	esm_hdmi_hdcp_controls hdcp_c;

} esm_instance_t;

/*  */
/* Internal functions */
/*  */
ELP_STATUS esm_hostlib_wait_mb_response(esm_instance_t *esm, uint32_t cmd_tmo);
ESM_STATUS esm_hostlib_put_exceptions(esm_instance_t *esm);

ELP_STATUS esm_hostlib_mb_cmd(esm_instance_t *esm, uint32_t cmd_req,
				uint32_t req_msg_param, uint32_t *req_param,
				uint32_t cmd_resp, uint32_t resp_msg_param,
				uint32_t *resp_param, int32_t cmd_timeout);
#endif
