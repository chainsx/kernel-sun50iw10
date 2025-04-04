/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

/* standard Lib */
/* #include <stdlib.h> */
/* #include <string.h> */
#include <linux/module.h>
#include <asm/uaccess.h>
#include <asm/memory.h>
#include <asm/unistd.h>
#include "asm-generic/int-ll64.h"
#include "linux/kernel.h"
#include "linux/mm.h"
#include "linux/semaphore.h"
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/dma-mapping.h>
#include <linux/sched.h>	/* wake_up_process() */
#include <linux/kthread.h>	/* kthread_create()??|kthread_run() */
#include <linux/kthread.h>	/* kthread_create()??|kthread_run() */
#include <linux/err.h>		/* IS_ERR()??|PTR_ERR() */
#include <linux/err.h>		/* IS_ERR()??|PTR_ERR() */
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/init.h>
/* #include <mach/sys_config.h> */
/* #include <mach/platform.h> */

/* includes */
#include "HDCP.h"
#include "EP952_If.h"
#include "EP952api.h"
#include "DDC_If.h"

/*  */
/* Global Data */
/*  */
unsigned char HDCP_Status;
unsigned char RI_Check;
unsigned char *pKSVList, *pBKSV_BCAPS3, *pSHA_M0;
unsigned char KSVListNumber;
unsigned char Temp_HDCP[10];
unsigned int HDCP_TimeCount;
unsigned long Temp_SHA_H[5];
HDCP_STATE HDCP_State;

/*  */
/* Private Functions */
/*  */
unsigned char HDCP_validate_RI(void);
unsigned char HDCP_compute_SHA_message_digest(unsigned char hdcp_dev_count,
					      unsigned char hdcp_depth);
void SHA_Initial(void);
void SHA_Push_Data(unsigned char *pData, unsigned char Size);
unsigned long *SHA_Get_SHA_Digest(void);
void SHA_Calculation(unsigned long pSHA_H[5], unsigned long pSHA_W1[16]);

/* --------------------------------------------------------------------------------------------------------------------------- */

void HDCP_Extract_BKSV_BCAPS3(unsigned char *pBKSV_BCaps3)
{
	pBKSV_BCAPS3 = pBKSV_BCaps3;
}

void HDCP_Extract_FIFO(unsigned char *pFIFOList, unsigned char ListNumber)
{
	pKSVList = pFIFOList;
	KSVListNumber = ListNumber;
}

void HDCP_Extract_SHA_M0(unsigned char *pSha_M0)
{
	pSHA_M0 = pSha_M0;
}

void HDCP_Stop(void)
{
	HDCP_TimeCount = 1000 / HDCP_TIMER_PERIOD;	/* No delay for next startup */
	HDCP_Status = 0;
	HDCP_State = 0;
	RI_Check = 0;

	/* Disable the HDCP Engine */
	HDMI_Tx_HDCP_Disable();
}

unsigned char HDCP_Get_Status(void)
{
	return HDCP_Status;
}

void HDCP_Timer(void)
{
	++HDCP_TimeCount;
}

void HDCP_Ext_Ri_Trigger(void)
{
	RI_Check = 1;
}

HDCP_STATE HDCP_Authentication_Task(unsigned char ReceiverRdy)
{
	if ((HDCP_State > A0_Wait_for_Active_Rx) && !(ReceiverRdy)) {
		DBG_printf(("WARNING: No RSEN or Hot-Plug in Authentication\r\n"));

		/* Enable mute for transmiter video and audio */
		HDMI_Tx_Mute_Enable();

		/* Confirm, Disable the HDCP Engine (actived from Downstream) */
		HDMI_Tx_HDCP_Disable();

		/* Restart HDCP authentication */
		HDCP_TimeCount = 0;
		HDCP_State = A0_Wait_for_Active_Rx;
		HDCP_Status |= HDCP_ERROR_RSEN;
		return HDCP_State;
	}

	switch (HDCP_State) {
		/* HDCP authentication */

		/* A0 state -- Wait for Active RX */
		/* -- read and validate BKSV for every 1 second. */
	case A0_Wait_for_Active_Rx:

		if (HDCP_TimeCount > 1000 / HDCP_TIMER_PERIOD) {
			HDCP_TimeCount = 0;

			if (Downstream_Rx_read_BKSV(Temp_HDCP)) {
				DBG_printf(("\r\n============================\r\n"));
				DBG_printf(("HDCP Authentication start...\r\n"));
				HDCP_State = A1_Exchange_KSVs;
			} else {	/* HDCP not support */
				DBG_printf(("HDCP might not be supported\r\n"));
				Downstream_Rx_write_AINFO(0x00);	/* Make TE sense the retry to pass ATC HDCP test */
				HDCP_Status |= HDCP_ERROR_BKSV;
			}
		}
		break;

		/* A1 state -- Exchange KSVs */
		/* -- retrieve the random number */
		/* -- write AN to RX and TX */
		/* -- read and write AKSV, BKSV */
	case A1_Exchange_KSVs:

		/* Write AINFO */
		Downstream_Rx_write_AINFO(0x00);

		/* Check Repeater Bit */
		Temp_HDCP[0] = Downstream_Rx_BCAPS();
		if (Temp_HDCP[0] & 0x40) {	/* REPEATER */
			HDMI_Tx_RPTR_Set();
		} else {	/* NON-REPEATER */
			HDMI_Tx_RPTR_Clear();
		}

		/* Exange AN */
		for (Temp_HDCP[8] = 0; Temp_HDCP[8] < 8; ++Temp_HDCP[8]) {
			Temp_HDCP[Temp_HDCP[8]] = 0;	/* rand()%256; */
		}
		HDMI_Tx_write_AN(Temp_HDCP);
		Downstream_Rx_write_AN(Temp_HDCP);

		/* Exange AKSV */
		if (!HDMI_Tx_read_AKSV(Temp_HDCP)) {
			HDCP_State = A0_Wait_for_Active_Rx;
			HDCP_Status |= HDCP_ERROR_AKSV;
			break;
		}
		Downstream_Rx_write_AKSV(Temp_HDCP);

		/* Exange BKSV */
		if (!Downstream_Rx_read_BKSV(Temp_HDCP)) {
			HDCP_State = A0_Wait_for_Active_Rx;
			HDCP_Status |= HDCP_ERROR_BKSV;
			break;
		}
		HDMI_Tx_write_BKSV(Temp_HDCP);
		if (pBKSV_BCAPS3)
			memcpy(&pBKSV_BCAPS3[0], Temp_HDCP, 5);

		HDCP_State = A2_Computations;
		break;

		/* A2 state -- Computations */
		/* -- Wait 150ms for R0 update (min 100ms) */
	case A2_Computations:

		if (HDCP_TimeCount > 150 / HDCP_TIMER_PERIOD) {
			if (HDMI_Tx_RI_RDY()) {
				HDCP_TimeCount = 0;
				HDCP_State = A3_Validate_Receiver;
			}
		}
		break;

		/* A3 state -- Validate Receiver */
		/* -- read and compare R0 from TX and RX */
		/* -- allow IIC traffic or R0 compare error in 200ms */
	case A3_Validate_Receiver:

		if (!HDCP_validate_RI()) {
			if (HDCP_TimeCount > 200 / HDCP_TIMER_PERIOD) {
				HDCP_TimeCount = 0;

				DBG_printf(("HDCP ERROR: R0 check failed\r\n"));

				HDCP_State = A0_Wait_for_Active_Rx;
				HDCP_Status |= HDCP_ERROR_R0;
			}
		} else {
			HDCP_TimeCount = 0;
			HDCP_State = A6_Test_for_Repeater;
		}
		break;

		/* A4 state -- Authenticated */
		/* -- Disable mute */
	case A4_Authenticated:

		/* Start the HDCP Engine */
		HDMI_Tx_HDCP_Enable();

		/* Disable mute for transmiter video */
		HDMI_Tx_Mute_Disable();

		DBG_printf(("HDCP Authenticated\r\n"));
		DBG_printf(("============================\r\n"));

		HDCP_State = A5_Link_Integrity_Check;
		break;

		/* A5 state -- Link Integrity Check every second */
		/* -- HDCP Engine must be started */
		/* -- read and compare RI from RX and TX */
	case A5_Link_Integrity_Check:

		if (RI_Check) {
			RI_Check = 0;

			if (!HDCP_validate_RI()) {
				if (!HDCP_validate_RI()) {

					/* Enable mute for transmiter video and audio */
					HDMI_Tx_Mute_Enable();

					/* Disable the HDCP Engine */
					HDMI_Tx_HDCP_Disable();

					DBG_printf(("HDCP ERROR: Ri check failed\r\n"));

					HDCP_State = A0_Wait_for_Active_Rx;
					HDCP_Status |= HDCP_ERROR_Ri;
				}
			}
		}
		break;

		/* A6 state -- Test For Repeater */
		/* -- REPEATER     : Enter the WAIT_RX_RDY state; */
		/* -- NON-REPEATER : Enter the AUTHENTICATED state */
	case A6_Test_for_Repeater:

		Temp_HDCP[0] = Downstream_Rx_BCAPS();
		if (pBKSV_BCAPS3)
			pBKSV_BCAPS3[5] = Temp_HDCP[0];

		if (Temp_HDCP[0] & 0x40) {	/* REPEATER */
			HDCP_State = A8_Wait_for_READY;
		} else {	/* NON-REPEATER */
			HDCP_State = A4_Authenticated;
		}
		break;

		/* A8 state -- Wait for READY */
		/* -- read BCAPS and check READY bit continuously */
		/* -- time out while 5-second period exceeds */
	case A8_Wait_for_READY:

		Temp_HDCP[0] = Downstream_Rx_BCAPS();
		if (pBKSV_BCAPS3)
			pBKSV_BCAPS3[5] = Temp_HDCP[0];

		if (Temp_HDCP[0] & 0x20) {
			HDCP_TimeCount = 0;
			HDCP_State = A9_Read_KSV_List;
		} else {
			if (HDCP_TimeCount > 5000 / HDCP_TIMER_PERIOD) {
				HDCP_TimeCount = 0;

				DBG_printf(("HDCP ERROR: Repeater check READY bit time-out\r\n"));

				HDCP_State = A0_Wait_for_Active_Rx;
				HDCP_Status |= HDCP_ERROR_RepeaterRdy;
			}
		}
		break;

		/* A9 state -- Read KSV List */
		/* -- compute and validate SHA-1 values */
	case A9_Read_KSV_List:

		Downstream_Rx_read_BSTATUS(Temp_HDCP);
		if (pBKSV_BCAPS3)
			memcpy(&pBKSV_BCAPS3[6], Temp_HDCP, 2);

		if (!(Temp_HDCP[0] & 0x80) && !(Temp_HDCP[1] & 0x08)) {
			if (HDCP_compute_SHA_message_digest
			    (Temp_HDCP[0], Temp_HDCP[1])) {
				HDCP_State = A4_Authenticated;
				break;
			} else {
				HDCP_Status |= HDCP_ERROR_RepeaterSHA;
			}
		} else {
			HDCP_Status |= HDCP_ERROR_RepeaterMax;
		}

		DBG_printf(("HDCP ERROR: Repeater HDCP SHA check failed\r\n"));

		HDCP_State = A0_Wait_for_Active_Rx;
		break;
	}

	return HDCP_State;
}

/* ---------------------------------------------------------------------------------------------------------------------- */

unsigned char HDCP_validate_RI(void)
{
	unsigned short Temp_RI_Tx, Temp_RI_Rx;
	if (!HDMI_Tx_read_RI((unsigned char *)&Temp_RI_Tx))
		return 0;	/* Read form Tx is fast, do it first */
	if (!Downstream_Rx_read_RI((unsigned char *)&Temp_RI_Rx))
		return 0;	/* Read form Rx is slow, do it second */

	if (Temp_RI_Tx != Temp_RI_Rx) {
		DBG_printf(("HDCP ERROR : Ri_Tx=0x%0.4X, Ri_Rx=0x%0.4X\r\n",
			    (int)Temp_RI_Tx, (int)Temp_RI_Rx));
	}

	return (Temp_RI_Tx == Temp_RI_Rx);
}

/* -------------------------------------------------------------------------------------------------- */

unsigned char HDCP_compute_SHA_message_digest(unsigned char hdcp_dev_count,
					      unsigned char hdcp_depth)
{
	int i;
	unsigned long *SHA_H;

	/* //////////////////////////////////////////////////////////////////////////////////////////// */
	/* Calculate SHA Value */
	/*  */

	SHA_Initial();

	/*  */
	/* Step 1 */
	/* Push all KSV FIFO to SHA caculation */
	/*  */

	/* Read KSV (5 byte) one by one and check the revocation list */
	for (i = 0; i < hdcp_dev_count; ++i) {

		/* Get KSV from FIFO */
		if (!Downstream_Rx_read_KSV_FIFO(Temp_HDCP, i, hdcp_dev_count)) {
			return 0;
		}
		/* Save FIFO */
		if (pKSVList && KSVListNumber) {
			if (i < KSVListNumber)
				memcpy(pKSVList + (i * 5), Temp_HDCP, 5);
		}
		/* Push KSV to the SHA block buffer (Total 5 bytes) */
		SHA_Push_Data(Temp_HDCP, 5);
	}
	if (hdcp_dev_count == 0) {
		Downstream_Rx_read_KSV_FIFO(Temp_HDCP, 0, 1);
	}
	/*  */
	/* Step 2 */
	/* Push BSTATUS, M0, and EOF to SHA caculation */
	/*  */

	/* Get the BSTATUS, M0, and EOF */
	Temp_HDCP[0] = hdcp_dev_count;	/* Temp_HDCP[0]   = BStatus, LSB */
	Temp_HDCP[1] = hdcp_depth;	/* Temp_HDCP[1]   = BStatus, MSB */
	HDMI_Tx_read_M0(Temp_HDCP + 2);	/* Temp_HDCP[2:9] = Read M0 from TX */
	if (pSHA_M0)
		memcpy(pSHA_M0 + 20, (unsigned char *)Temp_HDCP + 2, 8);

	/* Push the BSTATUS, and M0 to the SHA block buffer (Total 10 bytes) */
	SHA_Push_Data(Temp_HDCP, 10);

	/*  */
	/* Step 3 */
	/* Push the final block with length to SHA caculation */
	/*  */

	SHA_H = SHA_Get_SHA_Digest();

	/*  */
	/* SHA complete */
	/* //////////////////////////////////////////////////////////////////////////////////////////// */

	/* //////////////////////////////////////////////////////////////////////////////////////////// */
	/* Compare the SHA value */
	/*  */

	/* read RX SHA value */
	Downstream_Rx_read_SHA1_HASH((unsigned char *)Temp_SHA_H);
	if (pSHA_M0)
		memcpy(pSHA_M0, (unsigned char *)Temp_SHA_H, 20);
	DBG_printf(("Rx_SHA_H: "));
#ifdef DBG
	for (i = 0; i < 20; i += 4) {
		DBG_printf(("0x%0.2X%0.2X%0.2X%0.2X ",
			    (int)(((PBYTE) Temp_SHA_H)[i + 3]),
			    (int)(((PBYTE) Temp_SHA_H)[i + 2]),
			    (int)(((PBYTE) Temp_SHA_H)[i + 1]),
			    (int)(((PBYTE) Temp_SHA_H)[i + 0])));
	}
	DBG_printf(("\r\n"));
#endif
	/* compare the TX/RX SHA value */
	if ((hdcp_dev_count & 0x80) || (hdcp_depth & 0x08)) {
		DBG_printf(("Max Cascade or Max Devs exceeded\r\n"));
		return 0;
	} else if ((SHA_H[0] != Temp_SHA_H[0]) || (SHA_H[1] != Temp_SHA_H[1])
		   || (SHA_H[2] != Temp_SHA_H[2]) || (SHA_H[3] != Temp_SHA_H[3])
		   || (SHA_H[4] != Temp_SHA_H[4])) {
		DBG_printf(("SHA Digit Unmatch\r\n"));
		return 0;
	} else {
		DBG_printf(("SHA Digit Match\r\n"));
		return 1;
	}

	/*  */
	/* Return the compared result */
	/* //////////////////////////////////////////////////////////////////////////////////////////// */

}

/* -------------------------------------------------------------------------------------------------- */
/* SHA Implementation */
/* -------------------------------------------------------------------------------------------------- */

unsigned long SHA_H[5];
unsigned char SHA_Block[64];	/* 16*4 */
unsigned char SHA_Block_Index;
unsigned char CopySize;
unsigned int SHA_Length;

void SHA_Initial(void)
{
	/* //////////////////////////////////////////////////////////////////////////////////////////// */
	/* Calculate SHA Value */
	/*  */

	/* initial the SHA variables */
	SHA_H[0] = 0x67452301;
	SHA_H[1] = 0xEFCDAB89;
	SHA_H[2] = 0x98BADCFE;
	SHA_H[3] = 0x10325476;
	SHA_H[4] = 0xC3D2E1F0;

	/* Clean the SHA Block buffer */
	memset(SHA_Block, 0, 64);
	SHA_Block_Index = 0;

	SHA_Length = 0;
}

void SHA_Push_Data(unsigned char *pData, unsigned char Size)
{
	/* int i; */
	SHA_Length += Size;

	while (Size) {
		/* Push Data to the SHA block buffer */
		CopySize = min((unsigned char)(64 - SHA_Block_Index), Size);
		memcpy(SHA_Block + SHA_Block_Index, pData, CopySize);
		SHA_Block_Index += CopySize;
		pData += CopySize;
		Size -= CopySize;

		if (SHA_Block_Index >= 64) {	/* The SHA block buffer Full */
			/* Do SHA caculation for this SHA block buffer */
			SHA_Calculation(SHA_H, (unsigned long *)SHA_Block);
			memset(SHA_Block, 0, 64);

			SHA_Block_Index = 0;	/* Reset the Index */
		}
	}
}

unsigned long *SHA_Get_SHA_Digest(void)
{
	int i;
	SHA_Block[SHA_Block_Index++] = 0x80;	/* Set EOF */

	if ((64 - SHA_Block_Index) < 2) {
		memset(SHA_Block, 0, 64);
	}
	SHA_Length *= 8;
	SHA_Block[62] = (SHA_Length >> 8) & 0xFF;	/* Pad with Length MSB */
	SHA_Block[63] = SHA_Length & 0xFF;	/* Pad with Length LSB */

	/* Do SHA caculation for final SHA block */
	SHA_Calculation(SHA_H, (unsigned long *)SHA_Block);

	/* Swap the sequence of SHA_H (The big-endian to little-endian) */
	DBG_printf(("SHA_H:    "));
	for (i = 0; i < 20; i += 4) {

		Temp_HDCP[0] = ((unsigned char *)SHA_H)[i + 0];
		((unsigned char *)SHA_H)[i + 0] =
		    ((unsigned char *)SHA_H)[i + 3];
		((unsigned char *)SHA_H)[i + 3] = Temp_HDCP[0];

		Temp_HDCP[0] = ((unsigned char *)SHA_H)[i + 1];
		((unsigned char *)SHA_H)[i + 1] =
		    ((unsigned char *)SHA_H)[i + 2];
		((unsigned char *)SHA_H)[i + 2] = Temp_HDCP[0];

		DBG_printf(("0x%02X%02X%02X%02X ",
			    (int)(((unsigned char *)SHA_H)[i + 3]),
			    (int)(((unsigned char *)SHA_H)[i + 2]),
			    (int)(((unsigned char *)SHA_H)[i + 1]),
			    (int)(((unsigned char *)SHA_H)[i + 0])));
	}
	DBG_printf(("\r\n"));

	return SHA_H;
}

void SHA_Calculation(unsigned long pSHA_H[5], unsigned long pSHA_W1[16])
{
	unsigned char i;
	unsigned long TEMP;

	/* ========================================================= */
	/*  */
	/* STEP (c) : Let A = H0, B = H1, C = H2, D = H3, E = H4 */
	/*  */
	Temp_SHA_H[0] = pSHA_H[0];
	Temp_SHA_H[1] = pSHA_H[1];
	Temp_SHA_H[2] = pSHA_H[2];
	Temp_SHA_H[3] = pSHA_H[3];
	Temp_SHA_H[4] = pSHA_H[4];
	/*  */
	/* ========================================================= */

	/* ========================================================= */
	/*  */
	/* STEP (d) : FOR t = 0 to 79 DO */
	/* 1. TEMP = S5(A) + Ft(B,C,D) + E + Wt + Kt */
	/* 2. E = D; D = C; C = S30(B); B = A; A = TEMP; */
	/*  */
	for (i = 0; i <= 79; i++) {
		/* Update the Message Word while loop time >= 16 */
		if (i >= 16) {
			/* tword = pSHA_W1[tm03] ^ pSHA_W1[tm08] ^ pSHA_W1[tm14] ^ pSHA_W1[tm16]; */
			TEMP =
			    pSHA_W1[(i + 13) % 16] ^ pSHA_W1[(i +
							      8) %
							     16] ^ pSHA_W1[(i +
									    2) %
									   16] ^
			    pSHA_W1[i % 16];
			pSHA_W1[i % 16] = (TEMP << 1) | (TEMP >> 31);
		}
		/* Calculate first equation */
		TEMP = pSHA_W1[i % 16];

		TEMP += ((Temp_SHA_H[0] << 5) | (Temp_SHA_H[0] >> 27));

		if (i <= 19)
			TEMP +=
			    ((Temp_SHA_H[1] & Temp_SHA_H[2]) |
			     (~Temp_SHA_H[1] & Temp_SHA_H[3])) + 0x5A827999;
		else if (i <= 39)
			TEMP +=
			    (Temp_SHA_H[1] ^ Temp_SHA_H[2] ^ Temp_SHA_H[3]) +
			    0x6ED9EBA1;
		else if (i <= 59)
			TEMP +=
			    ((Temp_SHA_H[1] & Temp_SHA_H[2]) |
			     (Temp_SHA_H[1] & Temp_SHA_H[3]) | (Temp_SHA_H[2] &
								Temp_SHA_H[3]))
			    + 0x8F1BBCDC;
		else
			TEMP +=
			    (Temp_SHA_H[1] ^ Temp_SHA_H[2] ^ Temp_SHA_H[3]) +
			    0xCA62C1D6;

		TEMP += Temp_SHA_H[4];

		/* Update the Value A/B/C/D/E */
		Temp_SHA_H[4] = Temp_SHA_H[3];
		Temp_SHA_H[3] = Temp_SHA_H[2];
		Temp_SHA_H[2] = ((Temp_SHA_H[1] << 30) | (Temp_SHA_H[1] >> 2));
		Temp_SHA_H[1] = Temp_SHA_H[0];
		Temp_SHA_H[0] = TEMP;
	}
	/*  */
	/* ========================================================= */

	/* ========================================================= */
	/*  */
	/* STEP (e) : H0 = H0 + A; H1 = H1 + B; H2 = H2 + C; H3 = H3 + D; H4 = H4 + E; */
	/*  */
	pSHA_H[0] += Temp_SHA_H[0];
	pSHA_H[1] += Temp_SHA_H[1];
	pSHA_H[2] += Temp_SHA_H[2];
	pSHA_H[3] += Temp_SHA_H[3];
	pSHA_H[4] += Temp_SHA_H[4];
	/*  */
	/* ========================================================= */
}
