/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef API_HDCP_H_
#define API_HDCP_H_

#include "hdmitx_dev.h"
#include "core/video.h"
#include "identification.h"
#include "log.h"
#include "access.h"

#define KSV_MSK	0x7F
#define VRL_LENGTH	0x05
#define VRL_HEADER	5
#define VRL_NUMBER	3
#define HEADER		10
#define SHAMAX		20
#define DSAMAX		20

#define BSTATUS_HDMI_MODE_MASK			0x1000
#define BSTATUS_MAX_CASCADE_EXCEEDED_MASK	0x0800
#define BSTATUS_DEPTH_MASK			0x0700
#define BSTATUS_MAX_DEVS_EXCEEDED_MASK		0x0080
#define BSTATUS_DEVICE_COUNT_MASK		0x007F

/*****************************************************************************
 *                                                                           *
 *                               HDCP Registers                              *
 *                                                                           *
 *****************************************************************************/

/* HDCP Enable and Functional Control Configuration Register 0 */
#define A_HDCPCFG0  0x00014000
#define A_HDCPCFG0_HDMIDVI_MASK  0x00000001 /* Configures the transmitter to operate with a HDMI capable device or with a DVI device */
#define A_HDCPCFG0_EN11FEATURE_MASK  0x00000002 /* Enable the use of features 1 */
#define A_HDCPCFG0_RXDETECT_MASK  0x00000004 /* Information that a sink device was detected connected to the HDMI port */
#define A_HDCPCFG0_AVMUTE_MASK  0x00000008 /* This register holds the current AVMUTE state of the DWC_hdmi_tx controller, as expected to be perceived by the connected HDMI/HDCP sink device */
#define A_HDCPCFG0_SYNCRICHECK_MASK  0x00000010 /* Configures if the Ri check should be done at every 2s even or synchronously to every 128 encrypted frame */
#define A_HDCPCFG0_BYPENCRYPTION_MASK  0x00000020 /* Bypasses all the data encryption stages */
#define A_HDCPCFG0_I2CFASTMODE_MASK  0x00000040 /* Enable the I2C fast mode option from the transmitter's side */
#define A_HDCPCFG0_ELVENA_MASK  0x00000080 /* Enables the Enhanced Link Verification from the transmitter's side */

/* HDCP Software Reset and Functional Control Configuration Register 1 */
#define A_HDCPCFG1  0x00014004
#define A_HDCPCFG1_SWRESET_MASK  0x00000001 /* Software reset signal, active by writing a zero and auto cleared to 1 in the following cycle */
#define A_HDCPCFG1_ENCRYPTIONDISABLE_MASK  0x00000002 /* Disable encryption without losing authentication */
#define A_HDCPCFG1_PH2UPSHFTENC_MASK  0x00000004 /* Enables the encoding of packet header in the tmdsch0 bit[0] with cipher[2] instead of the tmdsch0 bit[2] Note: This bit must always be set to 1 for all PHYs (PHY GEN1, PHY GEN2, and non-Synopsys PHY) */
#define A_HDCPCFG1_DISSHA1CHECK_MASK  0x00000008 /* Disables the request to the API processor to verify the SHA1 message digest of a received KSV List */
#define A_HDCPCFG1_HDCP_LOCK_MASK  0x00000010 /* Lock the HDCP bypass and encryption disable mechanisms: - 1'b0: The default 1'b0 value enables you to bypass HDCP through bit 5 (bypencryption) of the A_HDCPCFG0 register or to disable the encryption through bit 1 (encryptiondisable) of A_HDCPCFG1 */
#define A_HDCPCFG1_SPARE_MASK  0x000000E0 /* This is a spare register with no associated functionality */

/* HDCP Observation Register 0 */
#define A_HDCPOBS0  0x00014008
#define A_HDCPOBS0_HDCPENGAGED_MASK  0x00000001 /* Informs that the current HDMI link has the HDCP protocol fully engaged */
#define A_HDCPOBS0_SUBSTATEA_MASK  0x0000000E /* Observability register informs in which sub-state the authentication is on */
#define A_HDCPOBS0_STATEA_MASK  0x000000F0 /* Observability register informs in which state the authentication machine is on */

/* HDCP Observation Register 1 */
#define A_HDCPOBS1  0x0001400C
#define A_HDCPOBS1_STATER_MASK  0x0000000F /* Observability register informs in which state the revocation machine is on */
#define A_HDCPOBS1_STATEOEG_MASK  0x00000070 /* Observability register informs in which state the OESS machine is on */

/* HDCP Observation Register 2 */
#define A_HDCPOBS2  0x00014010
#define A_HDCPOBS2_STATEEEG_MASK  0x00000007 /* Observability register informs in which state the EESS machine is on */
#define A_HDCPOBS2_STATEE_MASK  0x00000038 /* Observability register informs in which state the cipher machine is on */

/* HDCP Observation Register 3 */
#define A_HDCPOBS3  0x00014014
#define A_HDCPOBS3_FAST_REAUTHENTICATION_MASK  0x00000001 /* Register read from attached sink device: Bcap(0x40) bit 0 */
#define A_HDCPOBS3_FEATURES_1_1_MASK  0x00000002 /* Register read from attached sink device: Bcap(0x40) bit 1 */
#define A_HDCPOBS3_HDMI_MODE_MASK  0x00000004 /* Register read from attached sink device: Bstatus(0x41) bit 12 */
#define A_HDCPOBS3_FAST_I2C_MASK  0x00000010 /* Register read from attached sink device: Bcap(0x40) bit 4 */
#define A_HDCPOBS3_KSV_FIFO_READY_MASK  0x00000020 /* Register read from attached sink device: Bcap(0x40) bit 5 */
#define A_HDCPOBS3_REPEATER_MASK  0x00000040 /* Register read from attached sink device: Bcap(0x40) bit 6 */

/* HDCP Interrupt Clear Register Write only register, active high and auto cleared, cleans the respective interruption in the interrupt status register */
#define A_APIINTCLR  0x00014018
#define A_APIINTCLR_KSVACCESSINT_MASK  0x00000001 /* Clears the interruption related to KSV memory access grant for Read-Write access */
#define A_APIINTCLR_KSVSHA1CALCINT_MASK  0x00000002 /* Clears the interruption related to KSV list update in memory that needs to be SHA1 verified */
#define A_APIINTCLR_KEEPOUTERRORINT_MASK  0x00000004 /* Clears the interruption related to keep out window error */
#define A_APIINTCLR_LOSTARBITRATION_MASK  0x00000008 /* Clears the interruption related to I2C arbitration lost */
#define A_APIINTCLR_I2CNACK_MASK  0x00000010 /* Clears the interruption related to I2C NACK reception */
#define A_APIINTCLR_HDCP_FAILED_MASK  0x00000040 /* Clears the interruption related to HDCP authentication process failed */
#define A_APIINTCLR_HDCP_ENGAGED_MASK  0x00000080 /* Clears the interruption related to HDCP authentication process successful */

/* HDCP Interrupt Status Register Read only register, reports the interruption which caused the activation of the interruption output pin */
#define A_APIINTSTAT  0x0001401C
#define A_APIINTSTAT_KSVACCESSINT_MASK  0x00000001 /* Notifies that the KSV memory access as been guaranteed for Read-Write access */
#define A_APIINTSTAT_KSVSHA1CALCINT_MASK  0x00000002 /* Notifies that the HDCP13TCTRL core as updated a KSV list in memory that needs to be SHA1 verified */
#define A_APIINTSTAT_KEEPOUTERRORINT_MASK  0x00000004 /* Notifies that during the keep out window, the ctlout[3:0] bus was used besides control period */
#define A_APIINTSTAT_LOSTARBITRATION_MASK  0x00000008 /* Notifies that the I2C lost the arbitration to communicate */
#define A_APIINTSTAT_I2CNACK_MASK  0x00000010 /* Notifies that the I2C received a NACK from slave device */
#define A_APIINTSTAT_KSVSHA1CALCDONEINT_MASK  0x00000020
#define A_APIINTSTAT_HDCP_FAILED_MASK  0x00000040 /* Notifies that the HDCP authentication process was failed */
#define A_APIINTSTAT_HDCP_ENGAGED_MASK  0x00000080 /* Notifies that the HDCP authentication process was successful */


/* HDCP Interrupt Mask Register The configuration of this register mask a given setup of interruption, disabling them from generating interruption pulses in the interruption output pin */
#define A_APIINTMSK  0x00014020
#define A_APIINTMSK_KSVACCESSINT_MASK  0x00000001 /* Masks the interruption related to KSV memory access grant for Read-Write access */
#define A_APIINTMSK_KSVSHA1CALCINT_MASK  0x00000002 /* Masks the interruption related to KSV list update in memory that needs to be SHA1 verified */
#define A_APIINTMSK_KEEPOUTERRORINT_MASK  0x00000004 /* Masks the interruption related to keep out window error */
#define A_APIINTMSK_LOSTARBITRATION_MASK  0x00000008 /* Masks the interruption related to I2C arbitration lost */
#define A_APIINTMSK_I2CNACK_MASK  0x00000010 /* Masks the interruption related to I2C NACK reception */
#define A_APIINTMSK_SPARE_MASK  0x00000020 /* This is a spare bit and has no associated functionality */
#define A_APIINTMSK_HDCP_FAILED_MASK  0x00000040 /* Masks the interruption related to HDCP authentication process failed */
#define A_APIINTMSK_HDCP_ENGAGED_MASK  0x00000080 /* Masks the interruption related to HDCP authentication process successful */

/* HDCP Video Polarity Configuration Register */
#define A_VIDPOLCFG  0x00014024
#define A_VIDPOLCFG_SPARE_1_MASK  0x00000001 /* This is a spare bit and has no associated functionality */
#define A_VIDPOLCFG_HSYNCPOL_MASK  0x00000002 /* Configuration of the video Horizontal synchronism polarity */
#define A_VIDPOLCFG_SPARE_2_MASK  0x00000004 /* This is a spare bit and has no associated functionality */
#define A_VIDPOLCFG_VSYNCPOL_MASK  0x00000008 /* Configuration of the video Vertical synchronism polarity */
#define A_VIDPOLCFG_DATAENPOL_MASK  0x00000010 /* Configuration of the video data enable polarity */
#define A_VIDPOLCFG_UNENCRYPTCONF_MASK  0x00000060 /* Configuration of the color sent when sending unencrypted video data For a complete table showing the color results (RGB), refer to the "Color Configuration When Sending Unencrypted Video Data" figure in Chapter 2, "Functional Description */

/* HDCP OESS WOO Configuration Register Pulse width of the encryption enable (CTL3) signal in the HDCP OESS mode */
#define A_OESSWCFG  0x00014028
#define A_OESSWCFG_A_OESSWCFG_MASK  0x000000FF /* HDCP OESS WOO Configuration Register */

/* HDCP Core Version Register LSB Design ID number */
#define A_COREVERLSB  0x00014050
#define A_COREVERLSB_A_COREVERLSB_MASK  0x000000FF /* HDCP Core Version Register LSB */

/* HDCP Core Version Register MSB Revision ID number */
#define A_COREVERMSB  0x00014054
#define A_COREVERMSB_A_COREVERMSB_MASK  0x000000FF /* HDCP Core Version Register MSB */

/* HDCP KSV Memory Control Register The KSVCTRLupd bit is a notification flag */
#define A_KSVMEMCTRL  0x00014058
#define A_KSVMEMCTRL_KSVMEMREQUEST_MASK  0x00000001 /* Request access to the KSV memory; must be de-asserted after the access is completed by the system */
#define A_KSVMEMCTRL_KSVMEMACCESS_MASK  0x00000002 /* Notification that the KSV memory access as been guaranteed */
#define A_KSVMEMCTRL_KSVCTRLUPD_MASK  0x00000004 /* Set to inform that the KSV list in memory has been analyzed and the response to the Message Digest as been updated */
#define A_KSVMEMCTRL_SHA1FAIL_MASK  0x00000008 /* Notification whether the KSV list message digest is correct */
#define A_KSVMEMCTRL_KSVSHA1STATUS_MASK  0x00000010

/* HDCP BStatus Register Array */
#define HDCP_BSTATUS  0x00014080
#define HDCP_BSTATUS_SIZE  2

/* HDCP M0 Register Array */
#define HDCP_M0  0x00014088
#define HDCP_M0_SIZE  8

/* HDCP KSV Registers */
#define HDCP_KSV  0x000140A8
#define HDCP_KSV_SIZE  635

/* HDCP SHA-1 VH Registers */
#define HDCP_VH  0x00014A94
#define HDCP_VH_SIZE  20

/* HDCP Revocation KSV List Size Register 0 */
#define HDCP_REVOC_SIZE_0  0x00014AE4
#define HDCP_REVOC_SIZE_0_HDCP_REVOC_SIZE_0_MASK  0x000000FF /* Register containing the LSB of KSV list size (ksv_list_size[7:0]) */

/* HDCP Revocation KSV List Size Register 1 */
#define HDCP_REVOC_SIZE_1  0x00014AE8
#define HDCP_REVOC_SIZE_1_HDCP_REVOC_SIZE_1_MASK  0x000000FF /* Register containing the MSB of KSV list size (ksv_list_size[15:8]) */

/* HDCP Revocation KSV Registers */
#define HDCP_REVOC_LIST  0x00014AEC
#define HDCP_REVOC_LIST_SIZE  5060

/* HDCP KSV Status Register 0 */
#define HDCPREG_BKSV0  0x0001E000
#define HDCPREG_BKSV0_HDCPREG_BKSV0_MASK  0x000000FF /* Contains the value of BKSV[7:0] */

/* HDCP KSV Status Register 1 */
#define HDCPREG_BKSV1  0x0001E004
#define HDCPREG_BKSV1_HDCPREG_BKSV1_MASK  0x000000FF /* Contains the value of BKSV[15:8] */

/* HDCP KSV Status Register 2 */
#define HDCPREG_BKSV2  0x0001E008
#define HDCPREG_BKSV2_HDCPREG_BKSV2_MASK  0x000000FF /* Contains the value of BKSV[23:16] */

/* HDCP KSV Status Register 3 */
#define HDCPREG_BKSV3  0x0001E00C
#define HDCPREG_BKSV3_HDCPREG_BKSV3_MASK  0x000000FF /* Contains the value of BKSV[31:24] */

/* HDCP KSV Status Register 4 */
#define HDCPREG_BKSV4  0x0001E010
#define HDCPREG_BKSV4_HDCPREG_BKSV4_MASK  0x000000FF /* Contains the value of BKSV[39:32] */

/* HDCP AN Bypass Control Register */
#define HDCPREG_ANCONF  0x0001E014
#define HDCPREG_ANCONF_OANBYPASS_MASK  0x00000001 /* - When oanbypass=1, the value of AN used in the HDCP engine comes from the hdcpreg_an0 to hdcpreg_an7 registers */

/* HDCP Forced AN Register 0 */
#define HDCPREG_AN0  0x0001E018
#define HDCPREG_AN0_HDCPREG_AN0_MASK  0x000000FF /* Contains the value of AN[7:0] */

/* HDCP Forced AN Register 1 */
#define HDCPREG_AN1  0x0001E01C
#define HDCPREG_AN1_HDCPREG_AN1_MASK  0x000000FF /* Contains the value of AN[15:8] */

/* HDCP forced AN Register 2 */
#define HDCPREG_AN2  0x0001E020
#define HDCPREG_AN2_HDCPREG_AN2_MASK  0x000000FF /* Contains the value of AN[23:16] */

/* HDCP Forced AN Register 3 */
#define HDCPREG_AN3  0x0001E024
#define HDCPREG_AN3_HDCPREG_AN3_MASK  0x000000FF /* Contains the value of AN[31:24] */

/* HDCP Forced AN Register 4 */
#define HDCPREG_AN4  0x0001E028
#define HDCPREG_AN4_HDCPREG_AN4_MASK  0x000000FF /* Contains the value of AN[39:32] */

/* HDCP Forced AN Register 5 */
#define HDCPREG_AN5  0x0001E02C
#define HDCPREG_AN5_HDCPREG_AN5_MASK  0x000000FF /* Contains the value of AN[47:40] */

/* HDCP Forced AN Register 6 */
#define HDCPREG_AN6  0x0001E030
#define HDCPREG_AN6_HDCPREG_AN6_MASK  0x000000FF /* Contains the value of AN[55:48] */

/* HDCP Forced AN Register 7 */
#define HDCPREG_AN7  0x0001E034
#define HDCPREG_AN7_HDCPREG_AN7_MASK  0x000000FF /* Contains the value of BKSV[63:56] */

/* HDCP2.2 Identification Register */
#define HDCP22REG_ID  0x0001E400

/* HDCP2.2 Control Register */
#define HDCP22REG_CTRL  0x0001E410
#define HDCP22REG_CTRL_HDCP22_SWITCH_LCK_MASK  0x00000001 /*  */
#define HDCP22REG_CTRL_HDCP22_OVR_EN_MASK      0x00000002 /*  */
#define HDCP22REG_CTRL_HDCP22_OVR_VAL_MASK     0x00000004 /*  */
#define HDCP22REG_CTRL_HPD_OVR_EN_MASK         0x00000010 /*  */
#define HDCP22REG_CTRL_HPD_OVR_VAL_MASK        0x00000020 /*  */

/* HDCP2.2 Control Register */
#define HDCP22REG_CTRL1  0x0001E414
#define HDCP22REG_CTRL1_HDCP22_AVMUTE_OVR_EN_MASK  0x00000001
#define HDCP22REG_CTRL1_HDCP22_AVMUTE_OVR_VAL_MASK  0x00000002
#define HDCP22REG_CTRL1_HDCP22_CD_OVR_EN_MASK  0x00000008
#define HDCP22REG_CTRL1_HDCP22_CD_OVR_VAL_MASK  0x000000F0

/* HDCP2.2 Status Mask Register */
#define HDCP22REG_MASK  0x0001E430
#define HDCP22REG_MASK_CAPABLE_MASK                0x00000001 /*  */
#define HDCP22REG_MASK_NOT_CAPABLE_MASK        0x00000002 /*  */
#define HDCP22REG_MASK_AUTHEN_LOST_MASK        0x00000004 /*  */
#define HDCP22REG_MASK_AUTHEN_MASK             0x00000010 /*  */
#define HDCP22REG_MASK_AUTHEN_FAIL_MASK        0x00000020 /*  */
#define HDCP22REG_MASK_DECRYP_CHG_MASK         0x00000040 /*  */

/* HDCP2.2 Status Register */
#define HDCP22REG_STAT  0x0001E434
#define HDCP22REG_STAT_CAPABLE_MASK                0x00000001 /*  */
#define HDCP22REG_STAT_NOT_CAPABLE_MASK        0x00000002 /*  */
#define HDCP22REG_STAT_AUTHEN_LOST_MASK        0x00000004 /*  */
#define HDCP22REG_STAT_AUTHEN_MASK             0x00000008 /*  */
#define HDCP22REG_STAT_AUTHEN_FAIL_MASK        0x00000010 /*  */
#define HDCP22REG_STAT_DECRYP_CHG_MASK         0x00000020 /*  */


typedef enum {
	HDCP_IDLE = 0,
	HDCP_KSV_LIST_READY,
	HDCP_ERR_KSV_LIST_NOT_VALID,
	HDCP_KSV_LIST_ERR_DEPTH_EXCEEDED,
	HDCP_KSV_LIST_ERR_MEM_ACCESS,
	HDCP_ENGAGED,
	HDCP_FAILED
} hdcp_status_t;

typedef struct {
	u8 mLength[8];
	u8 mBlock[64];
	int mIndex;
	int mComputed;
	int mCorrupted;
	unsigned mDigest[5];
} sha_t;


void hdcp_init(hdmi_tx_dev_t *dev);
void hdcp_exit(void);
void hdcp22_data_enable(u8 enable);
void hdcp_configure_new(hdmi_tx_dev_t *dev,
					hdcpParams_t *hdcp,
					videoParams_t *video);


/**
 * Enter or exit AV mute mode
 * @param dev Device structure
 * @param enable the HDCP AV mute
 */
void hdcp_av_mute(hdmi_tx_dev_t *dev, int enable);

void hdcp_api_init(hdmi_tx_dev_t *dev, hdcpParams_t *hdcp,
					struct hdmi_dev_func *func);
#endif	/* API_HDCP_H_ */
