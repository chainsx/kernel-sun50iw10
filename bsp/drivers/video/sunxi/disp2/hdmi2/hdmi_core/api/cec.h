/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef CEC_H
#define CEC_H

#include <linux/delay.h>

#include "hdmitx_dev.h"
#include "log.h"
#include "access.h"

/* * Broadcast logical address */
#define BCST_ADDR   (0x0FUL)
/* * Control register bitfield */
#define    FRAME_TYP1   (0x1 << 2)
/* * Control register bitfield */
#define    FRAME_TYP0   (0x1 << 1)


/*****************************************************************************
 *                                                                           *
 *                               CEC Registers                               *
 *                                                                           *
 *****************************************************************************/

/* CEC Control Register This register handles the main control of the CEC initiator */
#define CEC_CTRL  0x0001F400
#define CEC_CTRL_SEND_MASK  0x00000001 /* - 1'b1: Set by software to trigger CEC sending a frame as an initiator */
#define CEC_CTRL_FRAME_TYP_MASK  0x00000006 /* - 2'b00: Signal Free Time = 3-bit periods */
#define CEC_CTRL_BC_NACK_MASK  0x00000008 /* - 1'b1: Set by software to NACK the received broadcast message */
#define CEC_CTRL_STANDBY_MASK  0x00000010 /* - 1: CEC controller responds with a NACK to all messages and generates a wakeup status for opcode */

/* CEC Interrupt Mask Register This read/write register masks/unmasks the interrupt events */
#define CEC_MASK  0x0001F408
#define CEC_MASK_DONE_MASK  0x00000001 /* The current transmission is successful (for initiator only) */
#define CEC_MASK_EOM_MASK  0x00000002 /* EOM is detected so that the received data is ready in the receiver data buffer (for follower only) */
#define CEC_MASK_NACK_MASK  0x00000004 /* A frame is not acknowledged in a directly addressed message */
#define CEC_MASK_ARB_LOST_MASK  0x00000008 /* The initiator losses the CEC line arbitration to a second initiator */
#define CEC_MASK_ERROR_INITIATOR_MASK  0x00000010 /* An error is detected on a CEC line (for initiator only) */
#define CEC_MASK_ERROR_FLOW_MASK  0x00000020 /* An error is notified by a follower */
#define CEC_MASK_WAKEUP_MASK  0x00000040 /* Follower wake-up signal mask */

/* CEC Logical Address Register Low This register indicates the logical address(es) allocated to the CEC device */
#define CEC_ADDR_L  0x0001F414
#define CEC_ADDR_L_CEC_ADDR_L_0_MASK  0x00000001 /* Logical address 0 - Device TV */
#define CEC_ADDR_L_CEC_ADDR_L_1_MASK  0x00000002 /* Logical address 1 - Recording Device 1 */
#define CEC_ADDR_L_CEC_ADDR_L_2_MASK  0x00000004 /* Logical address 2 - Recording Device 2 */
#define CEC_ADDR_L_CEC_ADDR_L_3_MASK  0x00000008 /* Logical address 3 - Tuner 1 */
#define CEC_ADDR_L_CEC_ADDR_L_4_MASK  0x00000010 /* Logical address 4 - Playback Device 1 */
#define CEC_ADDR_L_CEC_ADDR_L_5_MASK  0x00000020 /* Logical address 5 - Audio System */
#define CEC_ADDR_L_CEC_ADDR_L_6_MASK  0x00000040 /* Logical address 6 - Tuner 2 */
#define CEC_ADDR_L_CEC_ADDR_L_7_MASK  0x00000080 /* Logical address 7 - Tuner 3 */

/* CEC Logical Address Register High This register indicates the logical address(es) allocated to the CEC device */
#define CEC_ADDR_H  0x0001F418
#define CEC_ADDR_H_CEC_ADDR_H_0_MASK  0x00000001 /* Logical address 8 - Playback Device 2 */
#define CEC_ADDR_H_CEC_ADDR_H_1_MASK  0x00000002 /* Logical address 9 - Playback Device 3 */
#define CEC_ADDR_H_CEC_ADDR_H_2_MASK  0x00000004 /* Logical address 10 - Tuner 4 */
#define CEC_ADDR_H_CEC_ADDR_H_3_MASK  0x00000008 /* Logical address 11 - Playback Device 3 */
#define CEC_ADDR_H_CEC_ADDR_H_4_MASK  0x00000010 /* Logical address 12 - Reserved */
#define CEC_ADDR_H_CEC_ADDR_H_5_MASK  0x00000020 /* Logical address 13 - Reserved */
#define CEC_ADDR_H_CEC_ADDR_H_6_MASK  0x00000040 /* Logical address 14 - Free use */
#define CEC_ADDR_H_CEC_ADDR_H_7_MASK  0x00000080 /* Logical address 15 - Unregistered (as initiator address), Broadcast (as destination address) */

/* CEC TX Frame Size Register This register indicates the size of the frame in bytes (including header and data blocks), which are available in the transmitter data buffer */
#define CEC_TX_CNT  0x0001F41C
#define CEC_TX_CNT_CEC_TX_CNT_MASK  0x0000001F /* CEC Transmitter Counter register 5'd0: No data needs to be transmitted 5'd1: Frame size is 1 byte */

/* CEC RX Frame Size Register This register indicates the size of the frame in bytes (including header and data blocks), which are available in the receiver data buffer */
#define CEC_RX_CNT  0x0001F420
#define CEC_RX_CNT_CEC_RX_CNT_MASK  0x0000001F /* CEC Receiver Counter register: 5'd0: No data received 5'd1: 1-byte data is received */

/* CEC TX Data Register Array Address offset: i = 0 to 15 These registers (8 bits each) are the buffers used for storing the data waiting for transmission (including header and data blocks) */
#define CEC_TX_DATA  0x0001F440
#define CEC_TX_DATA_SIZE  16

/* CEC RX Data Register Array Address offset: i =0 to 15 These registers (8 bit each) are the buffers used for storing the received data (including header and data blocks) */
#define CEC_RX_DATA  0x0001F480
#define CEC_RX_DATA_SIZE  16

/* CEC Buffer Lock Register */
#define CEC_LOCK  0x0001F4C0
#define CEC_LOCK_LOCKED_BUFFER_MASK  0x00000001 /* When a frame is received, this bit would be active */

/* CEC Wake-up Control Register After receiving a message in the CEC_RX_DATA1 (OPCODE) registers, the CEC engine verifies the message opcode[7:0] against one of the previously defined values to generate the wake-up status: Wakeupstatus is 1 when: received opcode is 0x04 and opcode0x04en is 1 or received opcode is 0x0D and opcode0x0Den is 1 or received opcode is 0x41 and opcode0x41en is 1 or received opcode is 0x42 and opcode0x42en is 1 or received opcode is 0x44 and opcode0x44en is 1 or received opcode is 0x70 and opcode0x70en is 1 or received opcode is 0x82 and opcode0x82en is 1 or received opcode is 0x86 and opcode0x86en is 1 Wakeupstatus is 0 when none of the previous conditions are true */
#define CEC_WAKEUPCTRL  0x0001F4C4
#define CEC_WAKEUPCTRL_OPCODE0X04EN_MASK  0x00000001 /* OPCODE 0x04 wake up enable */
#define CEC_WAKEUPCTRL_OPCODE0X0DEN_MASK  0x00000002 /* OPCODE 0x0D wake up enable */
#define CEC_WAKEUPCTRL_OPCODE0X41EN_MASK  0x00000004 /* OPCODE 0x41 wake up enable */
#define CEC_WAKEUPCTRL_OPCODE0X42EN_MASK  0x00000008 /* OPCODE 0x42 wake up enable */
#define CEC_WAKEUPCTRL_OPCODE0X44EN_MASK  0x00000010 /* OPCODE 0x44 wake up enable */
#define CEC_WAKEUPCTRL_OPCODE0X70EN_MASK  0x00000020 /* OPCODE 0x70 wake up enable */
#define CEC_WAKEUPCTRL_OPCODE0X82EN_MASK  0x00000040 /* OPCODE 0x82 wake up enable */
#define CEC_WAKEUPCTRL_OPCODE0X86EN_MASK  0x00000080 /* OPCODE 0x86 wake up enable */

int cec_send_poll(hdmi_tx_dev_t *dev, char src);
int cec_get_nack_state(hdmi_tx_dev_t *dev);
int cec_Init(hdmi_tx_dev_t *dev);
int cec_Disable(hdmi_tx_dev_t *dev, int wakeup);
int cec_CfgLogicAddr(hdmi_tx_dev_t *dev, unsigned addr, int enable);
unsigned int cec_get_log_addr(hdmi_tx_dev_t *dev);
int cec_CfgStandbyMode(hdmi_tx_dev_t *dev, int enable);
int cec_CfgSignalFreeTime(hdmi_tx_dev_t *dev, int time);
int cec_GetSend(hdmi_tx_dev_t *dev);
int cec_SetSend(hdmi_tx_dev_t *dev);
int cec_IntClear(hdmi_tx_dev_t *dev, unsigned char mask);
void cec_wakeupctrl_clear(hdmi_tx_dev_t *dev);
int cec_IntDisable(hdmi_tx_dev_t *dev, unsigned char mask);
int cec_IntEnable(hdmi_tx_dev_t *dev, unsigned char mask);
int cec_IntStatus(hdmi_tx_dev_t *dev, unsigned char mask);
int cec_IntStatusClear(hdmi_tx_dev_t *dev, unsigned char mask);
int cec_CfgTxBuf(hdmi_tx_dev_t *dev, char *buf, unsigned size);
int cec_CfgRxBuf(hdmi_tx_dev_t *dev, char *buf, unsigned size);
int cec_GetLocked(hdmi_tx_dev_t *dev);
int cec_SetLocked(hdmi_tx_dev_t *dev);
int cec_CfgBroadcastNAK(hdmi_tx_dev_t *dev, int enable);
int cec_ctrlReceiveFrame(hdmi_tx_dev_t *dev, char *buf, unsigned size);
int cec_ctrlSendFrame(hdmi_tx_dev_t *dev, char *buf, unsigned size, unsigned src, unsigned dst);
/******************************************************************************
 *
 *****************************************************************************/

#endif /* CEC_H */
