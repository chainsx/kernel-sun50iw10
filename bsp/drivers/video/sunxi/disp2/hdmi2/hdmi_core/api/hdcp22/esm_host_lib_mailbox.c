/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include "include/ESMHost.h"
#include "include/ESMHostTypes.h"

static ESM_STATUS esm_hostlib_read_mb_response(esm_instance_t *esm,
							char *msg,
							uint32_t *response);

/**
*@msg_param: show if count of param(is an array) bigger than 1
*@param: params that want to write to esm register
*/
ELP_STATUS esm_hostlib_mb_cmd(esm_instance_t *esm, uint32_t cmd_req,
				uint32_t req_msg_param, uint32_t *req_param,
				uint32_t cmd_resp, uint32_t resp_msg_param,
				uint32_t *resp_param, int32_t cmd_timeout)
{
	uint32_t reg = 0;
	ESM_STATUS err;

	if (esm == NULL)
		return ESM_HL_NO_INSTANCE;
	if (esm->driver == NULL)
		return ESM_HL_FAILED;
	if (esm->driver->hpi_write == NULL)
		return ESM_HL_FAILED;

	/* write command parameters */
	if (req_msg_param && req_param) {
		if (esm->driver->hpi_write(esm->driver->instance,
				ESM_REG_HP_MB_P00(HPI_HOST_OFF), req_param[0])
				!= ESM_HL_DRIVER_SUCCESS) {
			pr_info("HPI CMD Data 1 %d\n", err);
			return ESM_HL_DRIVER_HPI_WRITE_FAILED;
		}

		if (req_msg_param > 1) {
			if (esm->driver->hpi_write(esm->driver->instance,
						ESM_REG_HP_MB_P10(HPI_HOST_OFF),
						req_param[1]) != ESM_HL_DRIVER_SUCCESS) {
				pr_info("HPI CMD Data 2 %d", err);
				return ESM_HL_DRIVER_HPI_WRITE_FAILED;
			}
		}
	}

	/* Command request */
	if (esm->driver->hpi_write(esm->driver->instance,
						ESM_REG_HP_2AE_MB(HPI_HOST_OFF),
						cmd_req) != ESM_HL_DRIVER_SUCCESS) {
		pr_info("HPI MB CMD Failed %d", err);
		return ESM_HL_DRIVER_HPI_WRITE_FAILED;
	}

	/* Wait for the reply from the ESM */
	if (esm_hostlib_wait_mb_response(esm, cmd_timeout) != ESM_HL_SUCCESS) {
		if (cmd_req != ESM_CMD_SYSTEM_ON_EXIT_REQ) {
			pr_info("Wait For MB Failed %d / %d", err, cmd_timeout);
			return err;
		} else {
			/* This is expected for a ESM_Kill */
			return ESM_HL_COMMAND_TIMEOUT;
		}
	}

	/* Check for the ESM command response */
	if (esm_hostlib_read_mb_response(esm, "MB_CMD", &reg) != ESM_HL_SUCCESS) {
		pr_info("Read MB CMD Failed %d", err);
		return err;
	}
	if (reg != cmd_resp) {
		/* Not what we have expected */
		pr_info("MAILBOX: EXPECTED ESM RESPONSE %x != %x", cmd_resp, reg);
		return ESM_HL_REQ_RESP_MISMATCH;
	}

	/* Return the response */
	/* req_param is not 0 means that req_param is not a null address */
	/* use req_param to restore the esm state */
	if (resp_msg_param && resp_param) {
		if (esm->driver->hpi_read(esm->driver->instance,
					ESM_REG_AE_MB_P00(HPI_HOST_OFF),
					resp_param) != ESM_HL_DRIVER_SUCCESS) {
			pr_info("Read MB CMD Response Failed %d", err);
			return ESM_HL_DRIVER_HPI_READ_FAILED;
		}
	}

	/* Return mailbox to AE. */
	err = esm->driver->hpi_write(esm->driver->instance,
			ESM_REG_AE_MB_STAT(HPI_HOST_OFF),
			1ul << ESM_REG_AE_MB_STAT_MB_RTN_BIT);
	if (err != ESM_HL_DRIVER_SUCCESS) {
		pr_info("Failed to write AE_MB_STAT [%d]", err);
		return ESM_HL_DRIVER_HPI_WRITE_FAILED;
	}

	return ESM_HL_SUCCESS;
}

ELP_STATUS esm_hostlib_wait_mb_response(esm_instance_t *esm, uint32_t cmd_tmo)
{
	uint32_t i;
	uint32_t err;
	uint32_t reg0 = 0;

	if (esm == 0)
		return ESM_HL_NO_INSTANCE;

	if (!esm->irq_supported) {
		for (i = 0; i < (ESM_HOSTLIB_RESPONSE_CMD_TIMEOUT / ESM_HOSTLIB_RESPONSE_SLEEP); i++) {
			/* Check the interrupt status */
			if (esm->driver->hpi_read(esm->driver->instance,
						ESM_REG_HP_IRQ_STAT_MSG(HPI_HOST_OFF),
						&reg0) != ESM_HL_DRIVER_SUCCESS) {
				pr_info("Read HPI IRQ Failed %d", err);
				return ESM_HL_DRIVER_HPI_READ_FAILED;
			}

			/* Is there a pending mailbox interrupt */
			if (ESM_REG_HP_IRQ_BIT_SET(reg0, HP_IRQ_MB_MSG_STAT_BIT)) {
				/* Yes, clear it */
				if (esm->driver->hpi_write(esm->driver->instance,
							ESM_REG_HP_IRQ_STAT_MSG(HPI_HOST_OFF),
							(1 << HP_IRQ_MB_MSG_STAT_BIT))
						!= ESM_HL_DRIVER_SUCCESS) {
					pr_info("Read HPI IRQ Clear Failed %d", err);
					return ESM_HL_DRIVER_HPI_WRITE_FAILED;
				}

				return ESM_HL_SUCCESS;
			}
		}
	} else {
		/* Block on the semaphore to be signaled by the ISR */
		/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
		/* if (ELP_SEMAPHORE_WAIT(esm->esm_mb_msg_wait, 1) == ELP_SYSTEM_OK)
		   {
		Got the signal from the ISR
		return ESM_HL_SUCCESS;
		} */
		return ESM_HL_FAILED;
	}

	/* Timeout */
	pr_info("Wait for mailbox message timeout.");
	return ESM_HL_COMMAND_TIMEOUT;
}

static ESM_STATUS esm_hostlib_read_mb_response(esm_instance_t *esm,
							char *msg,
							uint32_t *response)
{
	ESM_STATUS err;

	if (esm == 0)
		return ESM_HL_NO_INSTANCE;

	if (esm->driver->hpi_read(esm->driver->instance,
					ESM_REG_AE_2HP_MB(HPI_HOST_OFF),
					response) != ESM_HL_DRIVER_SUCCESS) {
		pr_info("Read HPI IRQ Clear Failed %d", err);
		return ESM_HL_DRIVER_HPI_READ_FAILED;
	}

	return ESM_HL_SUCCESS;
}
