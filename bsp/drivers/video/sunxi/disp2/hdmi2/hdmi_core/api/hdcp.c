/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include <linux/delay.h>
#include <linux/workqueue.h>
#include "hdcp.h"
#include "log.h"
#include "scdc.h"
#include "core/fc_video.h"
#include "core/packets.h"
#include "hdcp22_tx.h"

static void _EnableAvmute(hdmi_tx_dev_t *dev, u8 bit);
static hdmi_tx_dev_t *hdmi_dev;
static hdcpParams_t *phdcp;
static videoParams_t *pvideo;


bool hdcp14_enable;
bool hdcp22_enable;

static u32 hdcp14_auth_enable;
static u32 hdcp14_auth_complete;

static u8 hdcp_status;
static u32 hdcp_engaged_count;
#define KSV_LEN  5 /* KSV value size */


/* HDCP Interrupt fields */
#define INT_KSV_ACCESS    (A_APIINTSTAT_KSVACCESSINT_MASK)
#define INT_KSV_SHA1      (A_APIINTSTAT_KSVSHA1CALCINT_MASK)
#define INT_KSV_SHA1_DONE (A_APIINTSTAT_KSVSHA1CALCDONEINT_MASK)
#define INT_HDCP_FAIL     (A_APIINTSTAT_HDCP_FAILED_MASK)
#define INT_HDCP_ENGAGED  (A_APIINTSTAT_HDCP_ENGAGED_MASK)

#define SIZE	(160/8)
#define KSIZE	(1024/8)

static void sha_process_block(hdmi_tx_dev_t *dev, sha_t *sha);
static void sha_pad_message(hdmi_tx_dev_t *dev, sha_t *sha);
static int hdcp_verify_ksv(hdmi_tx_dev_t *dev, const u8 *data, size_t size);

static int hdcp_interrupt_clear(hdmi_tx_dev_t *dev, u8 value);
static u8 hdcp_interrupt_status(hdmi_tx_dev_t *dev);
static u8 hdcp_event_handler(hdmi_tx_dev_t *dev, int *param,
								u32 irq_stat);

static void hdcp14_authenticate_work(struct work_struct *work);
static void hdcp14_disconfigure(hdmi_tx_dev_t *dev);
#ifdef CONFIG_AW_HDMI2_HDCP22_SUNXI
static void hdcp22_disconfigure(hdmi_tx_dev_t *dev);
#endif

static void _EnableFeature11(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, A_HDCPCFG0, A_HDCPCFG0_EN11FEATURE_MASK, bit);
}

static void hdcp_rxdetect(hdmi_tx_dev_t *dev, u8 enable)
{
	LOG_TRACE1(enable);
	dev_write_mask(dev, A_HDCPCFG0, A_HDCPCFG0_RXDETECT_MASK, enable);
}

static void _RiCheck(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, A_HDCPCFG0, A_HDCPCFG0_SYNCRICHECK_MASK, bit);
}

static void _BypassEncryption(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, A_HDCPCFG0, A_HDCPCFG0_BYPENCRYPTION_MASK, bit);
}

static void _EnableI2cFastMode(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, A_HDCPCFG0, A_HDCPCFG0_I2CFASTMODE_MASK, bit);
}

static void _EnhancedLinkVerification(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, A_HDCPCFG0, A_HDCPCFG0_ELVENA_MASK, bit);
}

static void hdcp_sw_reset(hdmi_tx_dev_t *dev)
{
	LOG_TRACE();
	/* Software reset signal, active by writing a zero
	and auto cleared to 1 in the following cycle */
	dev_write_mask(dev, A_HDCPCFG1, A_HDCPCFG1_SWRESET_MASK, 0);
}

static void _DisableEncryption(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, A_HDCPCFG1, A_HDCPCFG1_ENCRYPTIONDISABLE_MASK, bit);
}

static void _EncodingPacketHeader(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, A_HDCPCFG1, A_HDCPCFG1_PH2UPSHFTENC_MASK, bit);
}

static void _InterruptClear(hdmi_tx_dev_t *dev, u8 value)
{
	dev_write(dev, (A_APIINTCLR), value);
}

static u8 _InterruptStatus(hdmi_tx_dev_t *dev)
{
	return dev_read(dev, A_APIINTSTAT);
}

static void _InterruptMask(hdmi_tx_dev_t *dev, u8 value)
{
	dev_write(dev, (A_APIINTMSK), value);
}

static u8 _InterruptMaskStatus(hdmi_tx_dev_t *dev)
{
	return dev_read(dev, A_APIINTMSK);
}

static void _HSyncPolarity(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, A_VIDPOLCFG, A_VIDPOLCFG_HSYNCPOL_MASK, bit);
}


static void _VSyncPolarity(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, A_VIDPOLCFG, A_VIDPOLCFG_VSYNCPOL_MASK, bit);
}

static void _DataEnablePolarity(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, A_VIDPOLCFG, A_VIDPOLCFG_DATAENPOL_MASK, bit);
}

static void _UnencryptedVideoColor(hdmi_tx_dev_t *dev, u8 value)
{
	LOG_TRACE1(value);
	dev_write_mask(dev, A_VIDPOLCFG, A_VIDPOLCFG_UNENCRYPTCONF_MASK, value);
}

static void _OessWindowSize(hdmi_tx_dev_t *dev, u8 value)
{
	LOG_TRACE1(value);
	dev_write(dev, (A_OESSWCFG), value);
}


static void _MemoryAccessRequest(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE();
	dev_write_mask(dev, A_KSVMEMCTRL, A_KSVMEMCTRL_KSVMEMREQUEST_MASK, bit);
}

static u8 _MemoryAccessGranted(hdmi_tx_dev_t *dev)
{
	LOG_TRACE();
	return (u8)((dev_read(dev, A_KSVMEMCTRL)
		& A_KSVMEMCTRL_KSVMEMACCESS_MASK) >> 1);
}

static void _UpdateKsvListState(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, A_KSVMEMCTRL, A_KSVMEMCTRL_SHA1FAIL_MASK, bit);
	dev_write_mask(dev, A_KSVMEMCTRL, A_KSVMEMCTRL_KSVCTRLUPD_MASK, 1);
	dev_write_mask(dev, A_KSVMEMCTRL, A_KSVMEMCTRL_KSVCTRLUPD_MASK, 0);
}

static u16 _BStatusRead(hdmi_tx_dev_t *dev)
{
	u16 bstatus = 0;

	bstatus	= dev_read(dev, HDCP_BSTATUS);
	bstatus	|= dev_read(dev, HDCP_BSTATUS + ADDR_JUMP) << 8;
	return bstatus;
}

/* chose hdcp22_ovr_val to designed which hdcp version to be configured */
static void hdcp22_ovr_val_1p4(hdmi_tx_dev_t *dev)
{
	dev_write_mask(dev, (HDCP22REG_CTRL),
		HDCP22REG_CTRL_HDCP22_OVR_VAL_MASK, 0);
	dev_write_mask(dev, (HDCP22REG_CTRL),
		HDCP22REG_CTRL_HDCP22_OVR_EN_MASK, 1);
}

#ifdef CONFIG_AW_HDMI2_HDCP22_SUNXI
/* chose which way to enable hdcp22
* @enable: 0-chose to enable hdcp22 by dwc_hdmi inner signal ist_hdcp_capable
*              1-chose to enable hdcp22 by hdcp22_ovr_val bit
*/
static void hdcp22_ovr_val_2p2(hdmi_tx_dev_t *dev, u8 enable)
{
	dev_write_mask(dev, (HDCP22REG_CTRL),
		HDCP22REG_CTRL_HDCP22_OVR_VAL_MASK, enable);
	dev_write_mask(dev, (HDCP22REG_CTRL),
		HDCP22REG_CTRL_HDCP22_OVR_EN_MASK, enable);
}

void hdcp22_avmute_ovr_enable(hdmi_tx_dev_t *dev, u8 val, u8 enable)
{
	dev_write_mask(dev, HDCP22REG_CTRL1, HDCP22REG_CTRL1_HDCP22_AVMUTE_OVR_VAL_MASK, val);
	dev_write_mask(dev, HDCP22REG_CTRL1, HDCP22REG_CTRL1_HDCP22_AVMUTE_OVR_EN_MASK, enable);
}

/* chose what place hdcp22 hpd come from and config hdcp22 hpd enable or disable
* @val: 0-hdcp22 hpd come from phy: phy_stat0.HPD
*         1-hdcp22 hpd come from hpd_ovr_val
* @enable:hpd_ovr_val
*/
static void hdcp22_hpd_ovr_enable(hdmi_tx_dev_t *dev, u8 val, u8 enable)
{
	dev_write_mask(dev, (HDCP22REG_CTRL),
		HDCP22REG_CTRL_HPD_OVR_VAL_MASK, val);
	dev_write_mask(dev, (HDCP22REG_CTRL),
		HDCP22REG_CTRL_HPD_OVR_EN_MASK, enable);
}
#endif
/* lock hdcp22_ovr_en and hdcp22_ovr_val */
/* static void hdcp22_switch_lck(hdmi_tx_dev_t *dev, u8 lck)
{
	LOG_TRACE();
	dev_write_mask(dev, (HDCP22REG_CTRL),
		HDCP22REG_CTRL_HDCP22_SWITCH_LCK_MASK, lck);
} */
/* void hdcp_params_reset(hdmi_tx_dev_t *dev, hdcpParams_t *params)
{
	params->mEnable11Feature = 0;
	params->mRiCheck = 1;
	params->mI2cFastMode = 0;
	params->mEnhancedLinkVerification = 0;
	params->maxDevices = 0;

	if (params->mKsvListBuffer != NULL)
		kfree(params->mKsvListBuffer);
	params->mKsvListBuffer = NULL;
} */

static void sha_reset(hdmi_tx_dev_t *dev, sha_t *sha)
{
	size_t i = 0;

	sha->mIndex = 0;
	sha->mComputed = FALSE;
	sha->mCorrupted = FALSE;
	for (i = 0; i < sizeof(sha->mLength); i++)
		sha->mLength[i] = 0;

	sha->mDigest[0] = 0x67452301;
	sha->mDigest[1] = 0xEFCDAB89;
	sha->mDigest[2] = 0x98BADCFE;
	sha->mDigest[3] = 0x10325476;
	sha->mDigest[4] = 0xC3D2E1F0;
}

static int sha_result(hdmi_tx_dev_t *dev, sha_t *sha)
{
	if (sha->mCorrupted == TRUE)
		return FALSE;

	if (sha->mComputed == FALSE) {
		sha_pad_message(dev, sha);
		sha->mComputed = TRUE;
	}
	return TRUE;
}

static void sha_input(hdmi_tx_dev_t *dev, sha_t *sha,
				const u8 *data, size_t size)
{
	int i = 0;
	unsigned j = 0;
	int rc = TRUE;

	if (data == 0 || size == 0) {
		pr_err("invalid input data\n");
		return;
	}
	if (sha->mComputed == TRUE || sha->mCorrupted == TRUE) {
		sha->mCorrupted = TRUE;
		return;
	}
	while (size-- && sha->mCorrupted == FALSE) {
		sha->mBlock[sha->mIndex++] = *data;

		for (i = 0; i < 8; i++) {
			rc = TRUE;
			for (j = 0; j < sizeof(sha->mLength); j++) {
				sha->mLength[j]++;
				if (sha->mLength[j] != 0) {
					rc = FALSE;
					break;
				}
			}
			sha->mCorrupted = (sha->mCorrupted == TRUE
					   || rc == TRUE) ? TRUE : FALSE;
		}
		/* if corrupted then message is too long */
		if (sha->mIndex == 64)
			sha_process_block(dev, sha);

		data++;
	}
}

static void sha_process_block(hdmi_tx_dev_t *dev, sha_t *sha)
{
	#define shaCircularShift(bits, word) ((((word) << (bits)) & 0xFFFFFFFF) | ((word) >> (32-(bits))))
	const unsigned K[] = {	/* constants defined in SHA-1 */
		0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xCA62C1D6
	};
	unsigned W[80];		/* word sequence */
	unsigned A, B, C, D, E;	/* word buffers */
	unsigned temp = 0;
	int t = 0;

	/* Initialize the first 16 words in the array W */
	for (t = 0; t < 80; t++) {
		if (t < 16) {
			W[t] = ((unsigned)sha->mBlock[t * 4 + 0]) << 24;
			W[t] |= ((unsigned)sha->mBlock[t * 4 + 1]) << 16;
			W[t] |= ((unsigned)sha->mBlock[t * 4 + 2]) << 8;
			W[t] |= ((unsigned)sha->mBlock[t * 4 + 3]) << 0;
		} else {
			W[t] =
			    shaCircularShift(1,
					     W[t - 3] ^ W[t - 8] ^ W[t -
							14] ^ W[t - 16]);
		}
	}

	A = sha->mDigest[0];
	B = sha->mDigest[1];
	C = sha->mDigest[2];
	D = sha->mDigest[3];
	E = sha->mDigest[4];

	for (t = 0; t < 80; t++) {
		temp = shaCircularShift(5, A);
		if (t < 20)
			temp += ((B & C) | ((~B) & D)) + E + W[t] + K[0];
		else if (t < 40)
			temp += (B ^ C ^ D) + E + W[t] + K[1];
		else if (t < 60)
			temp += ((B & C) | (B & D) | (C & D)) + E + W[t] + K[2];
		else
			temp += (B ^ C ^ D) + E + W[t] + K[3];

		E = D;
		D = C;
		C = shaCircularShift(30, B);
		B = A;
		A = (temp & 0xFFFFFFFF);
	}

	sha->mDigest[0] = (sha->mDigest[0] + A) & 0xFFFFFFFF;
	sha->mDigest[1] = (sha->mDigest[1] + B) & 0xFFFFFFFF;
	sha->mDigest[2] = (sha->mDigest[2] + C) & 0xFFFFFFFF;
	sha->mDigest[3] = (sha->mDigest[3] + D) & 0xFFFFFFFF;
	sha->mDigest[4] = (sha->mDigest[4] + E) & 0xFFFFFFFF;

	sha->mIndex = 0;
}

static void sha_pad_message(hdmi_tx_dev_t *dev, sha_t *sha)
{
	/*
	 *  Check to see if the current message block is too small to hold
	 *  the initial padding bits and length.  If so, we will pad the
	 *  block, process it, and then continue padding into a second
	 *  block.
	 */
	if (sha->mIndex > 55) {
		sha->mBlock[sha->mIndex++] = 0x80;
		while (sha->mIndex < 64)
			sha->mBlock[sha->mIndex++] = 0;

		sha_process_block(dev, sha);
		while (sha->mIndex < 56)
			sha->mBlock[sha->mIndex++] = 0;

	} else {
		sha->mBlock[sha->mIndex++] = 0x80;
		while (sha->mIndex < 56)
			sha->mBlock[sha->mIndex++] = 0;
	}

	/* Store the message length as the last 8 octets */
	sha->mBlock[56] = sha->mLength[7];
	sha->mBlock[57] = sha->mLength[6];
	sha->mBlock[58] = sha->mLength[5];
	sha->mBlock[59] = sha->mLength[4];
	sha->mBlock[60] = sha->mLength[3];
	sha->mBlock[61] = sha->mLength[2];
	sha->mBlock[62] = sha->mLength[1];
	sha->mBlock[63] = sha->mLength[0];

	sha_process_block(dev, sha);
}

static int hdcp_verify_ksv(hdmi_tx_dev_t *dev, const u8 *data, size_t size)
{
	size_t i = 0;
	sha_t sha;

	LOG_TRACE1((int)size);

	if (data == 0 || size < (HEADER + SHAMAX)) {
		pr_err("invalid input data\n");
		return FALSE;
	}
	sha_reset(dev, &sha);
	sha_input(dev, &sha, data, size - SHAMAX);

	if (sha_result(dev, &sha) == FALSE) {
		pr_err("cannot process SHA digest\n");
		return FALSE;
	}

	for (i = 0; i < SHAMAX; i++) {
		if (data[size - SHAMAX + i] !=
				(u8) (sha.mDigest[i / 4] >> ((i % 4) * 8))) {
			pr_err("SHA digest does not match\n");
			return FALSE;
		}
	}
	return TRUE;
}


/* To judge if sink support hdcp2.2
* return: 1: sink support hdcp2.2
*         0: sink do not support hdcp2.2
*        -1: ddc reading fail
*/
static int hdcp22_is_supported(hdmi_tx_dev_t *dev)
{
	u8 data = 0;
	if (ddc_read_hdcp2Version(dev, &data) == 0) {
		if ((data & 0x4) == 0x4)
			return 1;
		else
			return 0;
	} else {
		pr_err("Error:ddc_read_hdcp2Version fail\n");
		return -1;
	}
	return -1;
}


/* read the hdcp type that sink support
* return: 1: sink support hdcp2.2 and hdcp 1.4
*         0: sink only support hdcp1.4
*        -1: ddc read failed
*/
static int read_sink_hdcp_type(hdmi_tx_dev_t *dev)
{
	int ret = 0;

	ret = hdcp22_is_supported(dev);

	return ret;
}

#ifdef CONFIG_AW_HDMI2_HDCP22_SUNXI
void hdcp22_data_enable(u8 enable)
{
	mc_hdcp_clock_enable(hdmi_dev, enable ? 0 : 1);
	hdcp22_avmute_ovr_enable(hdmi_dev, enable ? 0 : 1, 1);
}
#endif

static void hdcp_1p4_configure(hdmi_tx_dev_t *dev, hdcpParams_t *hdcp)
{
	LOG_TRACE();
	hdcp_status = HDCP_FAILED;
	hdcp14_authenticate_work(NULL);

	hdcp14_enable = true;
	hdcp22_enable = false;
}

static void hdcp14_disconfigure(hdmi_tx_dev_t *dev)
{
	_DisableEncryption(dev, 1);
	hdcp_rxdetect(dev, 0);
	mc_hdcp_clock_enable(dev, 1);
	hdcp14_enable = false;
	hdcp14_auth_enable = 0;
	hdcp14_auth_complete = 0;
	hdcp_status = HDCP_FAILED;
}

#ifdef CONFIG_AW_HDMI2_HDCP22_SUNXI
/* configure hdcp2.2 and enable hdcp2.2 encrypt */
static void hdcp22_configure(hdmi_tx_dev_t *dev, hdcpParams_t *hdcp, videoParams_t *video)
{
	video_mode_t mode = video->mHdmi;
	u8 hsPol = pvideo->mDtd.mHSyncPolarity;
	u8 vsPol = pvideo->mDtd.mVSyncPolarity;

	LOG_TRACE();
	/* 1 - set main controller hdcp clock disable */
	hdcp22_data_enable(0);

	/* 2 - set hdcp keepout */
	fc_video_hdcp_keepout(hdmi_dev, true);

	/* 3 - Select DVI or HDMI mode */
	dev_write_mask(hdmi_dev, A_HDCPCFG0, A_HDCPCFG0_HDMIDVI_MASK,
						(mode == HDMI) ? 1 : 0);

	/* 4 - Set the Data enable, Hsync, and VSync polarity */
	_HSyncPolarity(hdmi_dev, (hsPol > 0) ? 1 : 0);
	_VSyncPolarity(hdmi_dev, (vsPol > 0) ? 1 : 0);
	_DataEnablePolarity(hdmi_dev, 1);
	/* hdcp22_switch_lck(dev, 0); */
	dev_write_mask(dev, 0x1000C, 1 << 5, 0x1);
	hdcp22_ovr_val_2p2(dev, 1);
	hdcp22_hpd_ovr_enable(dev, 1, 1);
	hdcp22_avmute_ovr_enable(dev, 0, 0);
	dev_write_mask(dev, 0x1000C, 1 << 4, 0x1);
	/* mask the interrupt of hdcp22 event */
	dev_write_mask(dev, HDCP22REG_MASK, 0xff, 0);
	/* hdcp22_switch_lck(dev, 1); */

	if (esm_tx_open() < 0)
		return;
	set_hdcp22_authenticate();
	hdcp14_enable = false;
	hdcp22_enable = true;
}

static void hdcp22_disconfigure(hdmi_tx_dev_t *dev)
{
	mc_hdcp_clock_enable(dev, 1);/* 0:enable   1:disable */

	/* hdcp22_switch_lck(dev, 0); */
	dev_write_mask(dev, 0x1000C, 1 << 5, 0x0);
	hdcp22_ovr_val_2p2(dev, 0);
	hdcp22_hpd_ovr_enable(dev, 0, 1);
	dev_write_mask(dev, 0x1000C, 1 << 4, 0x0);
	/* hdcp22_switch_lck(dev, 1); */
	esm_tx_disable();

	hdcp22_enable = false;
}
#endif

void hdcp_configure_new(hdmi_tx_dev_t *dev, hdcpParams_t *hdcp, videoParams_t *video)
{
	int hdcp_type = 0;
	hdmi_dev = dev;
	phdcp = hdcp;
	pvideo = video;

	LOG_TRACE();
	if ((dev->snps_hdmi_ctrl.hdcp_on == 0) || (hdcp->use_hdcp == 0)) {
		pr_info("HDCP is not active\n");
		return;
	}

	if (dev->snps_hdmi_ctrl.hdcp_enable) {
		pr_info("hdcp has been enable:%d\n", dev->snps_hdmi_ctrl.hdcp_enable);
		return;
	}

	/* Before configure HDCP we should configure the internal parameters */
	hdcp->maxDevices = 128;
	hdcp->mI2cFastMode = 0;
	memcpy(&dev->hdcp, hdcp, sizeof(hdcpParams_t));

	if (!hdcp->use_hdcp22) {
		hdcp_1p4_configure(dev, hdcp);
		dev->snps_hdmi_ctrl.hdcp_type = HDCP14;
		pr_info("NOT use hdcp2.2, Configure hdcp1.4\n");
	} else {
		hdcp_type = read_sink_hdcp_type(dev);
		if (hdcp_type == 1) {
#ifdef CONFIG_AW_HDMI2_HDCP22_SUNXI
			pr_info("The HDMI RX support hdcp2.2\n");
			hdcp22_configure(dev, hdcp, video);
#endif
			dev->snps_hdmi_ctrl.hdcp_type = HDCP22;
		} else {
			pr_info("The HDMI RX Only support hdcp1.4\n");
			hdcp_1p4_configure(dev, hdcp);
			dev->snps_hdmi_ctrl.hdcp_type = HDCP14;
		}
	}

	dev->snps_hdmi_ctrl.hdcp_enable = 1;
}

void hdcp_disconfigure_new(hdmi_tx_dev_t *dev)
{
	if (dev->snps_hdmi_ctrl.hdcp_enable == 0) {
		pr_info("hdcp has been disable\n");
		return;
	}
	/* fc_video_hdcp_keepout(dev, false); */
#ifdef CONFIG_AW_HDMI2_HDCP22_SUNXI
	if (hdcp22_enable)
		hdcp22_disconfigure(dev);
#endif
	if (hdcp14_enable)
		hdcp14_disconfigure(dev);

	mc_hdcp_clock_enable(dev, 1);/* 0:enable   1:disable */
	dev->snps_hdmi_ctrl.hdcp_enable = 0;
}

static void hdcp_close(hdmi_tx_dev_t *dev)
{
	dev->snps_hdmi_ctrl.hdcp_enable = 0;
	dev->snps_hdmi_ctrl.hdcp_type = HDCP_UNDEFINED;

	hdcp14_disconfigure(dev);
#ifdef CONFIG_AW_HDMI2_HDCP22_SUNXI
	if (dev->snps_hdmi_ctrl.use_hdcp22) {
		hdcp22_disconfigure(dev);
		esm_tx_close();
	}
#endif
}

static u8 get_hdcp14_status(hdmi_tx_dev_t *dev)
{
	LOG_TRACE();
	return hdcp_interrupt_status(dev);
}

static int hdcp14_encrypt_status_check_and_handle(hdmi_tx_dev_t *dev)
{
	u8 hdcp14_status = 0;
	int param;
	u8 ret = 0;

	LOG_TRACE();
	if ((!hdcp14_auth_enable) || (!hdcp14_auth_complete))
		return 0;

	/* mutex_lock(&hdmi_dev->i2c_lock); */
	hdcp14_status = get_hdcp14_status(dev);
	hdcp_interrupt_clear(dev, hdcp14_status);

	ret = hdcp_event_handler(dev, &param, (u32)hdcp14_status);
	if ((ret != HDCP_ERR_KSV_LIST_NOT_VALID) && (ret != HDCP_FAILED))
			return 0;
	else
		return -1;
}

/* Check hdcp encrypt status and handle the status
 * return value: 1-indicate that hdcp is being in authentication;
 *               0-indicate that hdcp authenticate is sucessful
 *              -1-indicate that hdcp authenticate is failed
 * */
static int get_hdcp_status(hdmi_tx_dev_t *dev)
{

	LOG_TRACE();

	if (hdcp22_enable)
#ifdef CONFIG_AW_HDMI2_HDCP22_SUNXI
		return esm_encrypt_status_check_and_handle();
#else
		return 0;
#endif
	else if (hdcp14_enable)
		return hdcp14_encrypt_status_check_and_handle(dev);
	else
		return 0;
}

static void hdcp14_authenticate_work(struct work_struct *work)
{
	video_mode_t mode = pvideo->mHdmi;
	u8 hsPol = pvideo->mDtd.mHSyncPolarity;
	u8 vsPol = pvideo->mDtd.mVSyncPolarity;

	if (hdcp14_auth_complete)
		return;
	mutex_lock(&hdmi_dev->i2c_lock);
	msleep(20);
	hdcp14_auth_enable = 1;
	_DisableEncryption(hdmi_dev, TRUE);

	/* 1 - set main controller hdcp clock disable */
	mc_hdcp_clock_enable(hdmi_dev, 1);/* 0:enable   1:disable */

	/* 2 - set hdcp keepout */
	fc_video_hdcp_keepout(hdmi_dev, true);

	/* 3 - Select DVI or HDMI mode */
	dev_write_mask(hdmi_dev, A_HDCPCFG0, A_HDCPCFG0_HDMIDVI_MASK,
						(mode == HDMI) ? 1 : 0);

	/* 4 - Set the Data enable, Hsync, and VSync polarity */
	_HSyncPolarity(hdmi_dev, (hsPol > 0) ? 1 : 0);
	_VSyncPolarity(hdmi_dev, (vsPol > 0) ? 1 : 0);
	_DataEnablePolarity(hdmi_dev,
		(hdmi_dev->snps_hdmi_ctrl.data_enable_polarity > 0) ? 1 : 0);

	dev_write_mask(hdmi_dev, 0x1000C, 1 << 5, 0x0);/* bypass hdcp_block = 0 */
	hdcp22_ovr_val_1p4(hdmi_dev);

		/* HDCP only */
	_EnableFeature11(hdmi_dev, (phdcp->mEnable11Feature > 0) ? 1 : 0);
	_RiCheck(hdmi_dev, (phdcp->mRiCheck > 0) ? 1 : 0);
	_EnableI2cFastMode(hdmi_dev, (phdcp->mI2cFastMode > 0) ? 1 : 0);
	_EnhancedLinkVerification(hdmi_dev,
				(phdcp->mEnhancedLinkVerification > 0) ? 1 : 0);

		/* fixed */
	_EnableAvmute(hdmi_dev, FALSE);
	_UnencryptedVideoColor(hdmi_dev, 0x02);
	_EncodingPacketHeader(hdmi_dev, TRUE);

	/* 9 - Set encryption */
	_OessWindowSize(hdmi_dev, 64);
	_BypassEncryption(hdmi_dev, FALSE);
	/* _DisableEncryption(hdmi_dev, FALSE); */

	/* 10 - Reset the HDCP 1.4 engine */
	hdcp_sw_reset(hdmi_dev);
	/* 12 - Enable authenticate */
	hdcp_rxdetect(hdmi_dev, 1);

	_InterruptClear(hdmi_dev, A_APIINTCLR_KSVACCESSINT_MASK |
					A_APIINTCLR_KSVSHA1CALCINT_MASK |
					A_APIINTCLR_KEEPOUTERRORINT_MASK |
					A_APIINTCLR_LOSTARBITRATION_MASK |
					A_APIINTCLR_I2CNACK_MASK |
					A_APIINTCLR_HDCP_FAILED_MASK |
					A_APIINTCLR_HDCP_ENGAGED_MASK);

	_InterruptMask(hdmi_dev, (~(A_APIINTMSK_KSVACCESSINT_MASK |
					     A_APIINTMSK_KSVSHA1CALCINT_MASK |
					     A_APIINTMSK_KEEPOUTERRORINT_MASK |
					     A_APIINTMSK_LOSTARBITRATION_MASK |
					     A_APIINTMSK_I2CNACK_MASK |
					     A_APIINTMSK_SPARE_MASK |
					     A_APIINTMSK_HDCP_FAILED_MASK |
					     A_APIINTMSK_HDCP_ENGAGED_MASK)) &
					     _InterruptMaskStatus(hdmi_dev));

	mc_hdcp_clock_enable(hdmi_dev, 0);/* 0:enable   1:disable */
	hdcp14_auth_complete = 1;
	mutex_unlock(&hdmi_dev->i2c_lock);
}


static void hdcp14_initial(hdmi_tx_dev_t *dev, hdcpParams_t *hdcp)
{
	LOG_TRACE();
	hdcp14_auth_enable = 0;
	hdcp14_auth_complete = 0;
}

#ifdef CONFIG_AW_HDMI2_HDCP22_SUNXI
static void hdcp22_initial(hdmi_tx_dev_t *dev, hdcpParams_t *hdcp)
{
	if (esm_tx_initial(hdcp->esm_hpi_base,
			 (u32)hdcp->esm_firm_phy_addr,
			 hdcp->esm_firm_vir_addr,
			 hdcp->esm_firm_size,
			 (u32)hdcp->esm_data_phy_addr,
			 hdcp->esm_data_vir_addr,
			 hdcp->esm_data_size))
		pr_err("ERROR: esm_tx_initial failed\n");
}
#endif

static void hdcp14_exit(void)
{
	LOG_TRACE();
}

#ifdef CONFIG_AW_HDMI2_HDCP22_SUNXI
static void hdcp22_exit(void)
{
	esm_tx_exit();
}
#endif

static void hdcp_initial(hdmi_tx_dev_t *dev, hdcpParams_t *hdcp)
{
	i2cddc_fast_mode(dev, 0);
	i2cddc_clk_config(dev, I2C_SFR_CLK,
				I2C_MIN_SS_SCL_LOW_TIME,
				I2C_MIN_SS_SCL_HIGH_TIME,
				I2C_MIN_FS_SCL_LOW_TIME,
				I2C_MIN_FS_SCL_HIGH_TIME);
	_BypassEncryption(dev, TRUE);
	mc_hdcp_clock_enable(dev, 1);/* 0:enable   1:disable */
	_DisableEncryption(dev, TRUE);
	hdcp14_initial(dev, hdcp);
#ifdef CONFIG_AW_HDMI2_HDCP22_SUNXI
	if (hdcp->use_hdcp22)
		hdcp22_initial(dev, hdcp);
#endif
}

void hdcp_init(hdmi_tx_dev_t *dev)
{
	_OessWindowSize(dev, 64);
	fc_video_hdcp_keepout(dev, true);
	_BypassEncryption(dev, TRUE);
	mc_hdcp_clock_enable(dev, 1);/* 0:enable   1:disable */
	_DisableEncryption(dev, TRUE);
	_DataEnablePolarity(dev, 0);
}

void hdcp_exit(void)
{
	hdcp14_exit();
#ifdef CONFIG_AW_HDMI2_HDCP22_SUNXI
	hdcp22_exit();
#endif
}

/* SHA-1 calculation by Software */
static u8 _read_ksv_list(hdmi_tx_dev_t *dev, int *param)
{
	int timeout = 1000;
	u16 bstatus = 0;
	u16 deviceCount = 0;
	int valid = HDCP_IDLE;
	int size = 0;
	int i = 0;

	u8 *hdcp_ksv_list_buffer = dev->hdcp.mKsvListBuffer;

	/* 1 - Wait for an interrupt to be triggered
		(a_apiintstat.KSVSha1calcint) */
	/* This is called from the INT_KSV_SHA1 irq
		so nothing is required for this step */

	/* 2 - Request access to KSV memory through
		setting a_ksvmemctrl.KSVMEMrequest to 1'b1 and */
	/* pool a_ksvmemctrl.KSVMEMaccess until
		this value is 1'b1 (access granted). */
	_MemoryAccessRequest(dev, TRUE);
	while (_MemoryAccessGranted(dev) == 0 && timeout--)
		asm volatile ("nop");

	if (_MemoryAccessGranted(dev) == 0) {
		_MemoryAccessRequest(dev, FALSE);
		HDCP_INF("KSV List memory access denied");
		*param = 0;
		return HDCP_KSV_LIST_ERR_MEM_ACCESS;
	}

	/* 3 - Read VH', M0, Bstatus, and the KSV FIFO.
	The data is stored in the revocation memory, as */
	/* provided in the "Address Mapping for Maximum Memory Allocation"
	table in the databook. */
	bstatus = _BStatusRead(dev);
	deviceCount = bstatus & BSTATUS_DEVICE_COUNT_MASK;

	if (deviceCount > dev->hdcp.maxDevices) {
		*param = 0;
		HDCP_INF("depth exceeds KSV List memory");
		return HDCP_KSV_LIST_ERR_DEPTH_EXCEEDED;
	}

	size = deviceCount * KSV_LEN + HEADER + SHAMAX;

	for (i = 0; i < size; i++) {
		if (i < HEADER) { /* BSTATUS & M0 */
			hdcp_ksv_list_buffer[(deviceCount * KSV_LEN) + i] =
			(u8)dev_read(dev, HDCP_BSTATUS + (i * ADDR_JUMP));
		} else if (i < (HEADER + (deviceCount * KSV_LEN))) { /* KSV list */
			hdcp_ksv_list_buffer[i - HEADER] =
			(u8)dev_read(dev, HDCP_BSTATUS + (i * ADDR_JUMP));
		} else { /* SHA */
			hdcp_ksv_list_buffer[i] = (u8)dev_read(dev,
				HDCP_BSTATUS + (i * ADDR_JUMP));
		}
	}

	/* 4 - Calculate the SHA-1 checksum (VH) over M0,
		Bstatus, and the KSV FIFO. */
	if (hdcp_verify_ksv(dev, hdcp_ksv_list_buffer, size) == TRUE) {
		valid = HDCP_KSV_LIST_READY;
		HDCP_INF("HDCP_KSV_LIST_READY");
	} else {
		valid = HDCP_ERR_KSV_LIST_NOT_VALID;
		HDCP_INF("HDCP_ERR_KSV_LIST_NOT_VALID");
	}

	/* 5 - If the calculated VH equals the VH',
	set a_ksvmemctrl.SHA1fail to 0 and set */
	/* a_ksvmemctrl.KSVCTRLupd to 1.
	If the calculated VH is different from VH' then set */
	/* a_ksvmemctrl.SHA1fail to 1 and set a_ksvmemctrl.KSVCTRLupd to 1,
	forcing the controller */
	/* to re-authenticate from the beginning. */
	_MemoryAccessRequest(dev, 0);
	_UpdateKsvListState(dev, (valid == HDCP_KSV_LIST_READY) ? 0 : 1);

	return valid;
}

/* do nor encry until stabilizing successful authentication */
static void check_hdcp14_engaged(hdmi_tx_dev_t *dev)
{
	if ((hdcp_status == HDCP_ENGAGED)
		&& (hdcp_engaged_count >= 20)) {
		_DisableEncryption(dev, false);
		hdcp_engaged_count = 0;
		pr_info("HDCP Encryption\n");
	}
}

static u8 hdcp_event_handler(hdmi_tx_dev_t *dev, int *param, u32 irq_stat)
{
	u8 interrupt_status = 0;
	int valid = HDCP_IDLE;

	LOG_TRACE();
	interrupt_status = irq_stat;

	if (interrupt_status != 0)
		HDCP_INF("hdcp_interrupt_status 0x%x\n", interrupt_status);

	if (interrupt_status == 0) {
		if (hdcp_engaged_count && (hdcp_status == HDCP_ENGAGED)) {
			hdcp_engaged_count++;
			check_hdcp14_engaged(dev);
		}

		return hdcp_status;
	}

	if ((interrupt_status & A_APIINTSTAT_KEEPOUTERRORINT_MASK) != 0) {
		pr_info("keep out error interrupt\n");
		hdcp_status = HDCP_FAILED;

		hdcp_engaged_count = 0;
		return HDCP_FAILED;
	}

	if ((interrupt_status & A_APIINTSTAT_LOSTARBITRATION_MASK) != 0) {
		pr_info("lost arbitration error interrupt\n");
		hdcp_status = HDCP_FAILED;

		hdcp_engaged_count = 0;
		return HDCP_FAILED;
	}

	if ((interrupt_status & A_APIINTSTAT_I2CNACK_MASK) != 0) {
		pr_info("i2c nack error interrupt\n");
		hdcp_status = HDCP_FAILED;

		hdcp_engaged_count = 0;
		return HDCP_FAILED;
	}

	if (interrupt_status & INT_KSV_SHA1) {
		pr_info("INT_KSV_SHA1\n");
		return _read_ksv_list(dev, param);
	}

	if ((interrupt_status & INT_HDCP_FAIL) != 0) {
		*param = 0;
		pr_info("HDCP_FAILED\n");
		_DisableEncryption(dev, true);
		hdcp_status = HDCP_FAILED;

		hdcp_engaged_count = 0;
		return HDCP_FAILED;
	}

	if ((interrupt_status & INT_HDCP_ENGAGED) != 0) {
		*param = 1;
		pr_info("HDCP_ENGAGED\n");

		hdcp_status = HDCP_ENGAGED;
		hdcp_engaged_count = 1;

		return HDCP_ENGAGED;
	}

	return valid;
}

static u8 hdcp_interrupt_status(hdmi_tx_dev_t *dev)
{
	return _InterruptStatus(dev);
}

static int hdcp_interrupt_clear(hdmi_tx_dev_t *dev, u8 value)
{
	_InterruptClear(dev, value);
	return TRUE;
}

static void _EnableAvmute(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, A_HDCPCFG0, A_HDCPCFG0_AVMUTE_MASK, bit);
}


void hdcp_av_mute(hdmi_tx_dev_t *dev, int enable)
{
	LOG_TRACE1(enable);
	_EnableAvmute(dev,
			(enable == TRUE) ? 1 : 0);
}

static u8 hdcp_av_mute_state(hdmi_tx_dev_t *dev)
{
	LOG_TRACE();
	return dev_read_mask(dev, A_HDCPCFG0, A_HDCPCFG0_AVMUTE_MASK);

}

ssize_t hdcp_config_dump(hdmi_tx_dev_t *dev, char *buf)
{
	ssize_t n = 0;
	struct hdmi_tx_ctrl *ctrl = &dev->snps_hdmi_ctrl;

	n += sprintf(buf + n, "Lowlevel Part:\n");

	n += sprintf(buf + n, "%s\n",
		ctrl->use_hdcp ? "Tx use hdcp 1.4" : "Tx do NOT use hdcp");
	n += sprintf(buf + n, "%s\n",
		ctrl->use_hdcp22 ? "Tx use hdcp 2.2" : "Tx do NOT use hdcp 2.2");
	n += sprintf(buf + n, "%s\n",
		ctrl->hdcp_on ? "Enable HDCP" : "Disable HDCP");
	n += sprintf(buf + n, "%s\n",
		ctrl->hdcp_enable ? "HDCP  hardware has been Enable" :
			"HDCP  hardware has NOT been Enable");
	if (ctrl->hdcp_on)
		n += sprintf(buf + n, "HDMI MODE: %s\n",
			ctrl->hdmi_on == DVI ? "DVI" : "HDMI");

	if (hdcp14_enable)
		n += sprintf(buf + n, "hdcp1.4 enable\n");
#ifdef CONFIG_AW_HDMI2_HDCP22_SUNXI
	if (hdcp22_enable) {
		n += sprintf(buf + n, "hdcp2.2 enable\n");
		n += hdcp22_dump(buf);
	}
#endif
	return n;
}

void api_hdcp_close(void)
{
	hdcp_close(hdmi_dev);
}

static void api_hdcp_configure(hdcpParams_t *hdcp, videoParams_t *video)
{
	hdmi_dev->snps_hdmi_ctrl.hdcp_on = 1;
	hdcp_configure_new(hdmi_dev, hdcp, video);
}

static void api_hdcp_disconfigure(void)
{
	hdmi_dev->snps_hdmi_ctrl.hdcp_on = 0;
	hdcp_disconfigure_new(hdmi_dev);
}

static int get_hdcp_type(void)
{
	return (int)hdmi_dev->snps_hdmi_ctrl.hdcp_type;
}

static u8 api_hdcp_event_handler(int *param, u32 irq_stat)
{
	return hdcp_event_handler(hdmi_dev, param, irq_stat);
}

static u32 api_get_hdcp_avmute(void)
{
	return hdcp_av_mute_state(hdmi_dev);

}

static int    api_get_hdcp_status(void)
{
	return get_hdcp_status(hdmi_dev);
}

static ssize_t api_hdcp_config_dump(char *buf)
{
	return hdcp_config_dump(hdmi_dev, buf);
}

void hdcp_api_init(hdmi_tx_dev_t *dev, hdcpParams_t *hdcp,
					struct hdmi_dev_func *func)
{
	hdmi_dev = dev;

	hdmi_dev->snps_hdmi_ctrl.use_hdcp = hdcp->use_hdcp;
	hdmi_dev->snps_hdmi_ctrl.use_hdcp22 = hdcp->use_hdcp22;

	if (hdcp->use_hdcp)
		hdcp_initial(dev, hdcp);

	func->hdcp_close = api_hdcp_close;
	func->hdcp_configure = api_hdcp_configure;
	func->hdcp_disconfigure = api_hdcp_disconfigure;
	func->get_hdcp_type = get_hdcp_type;
	func->hdcp_event_handler = api_hdcp_event_handler;
	func->get_hdcp_status = api_get_hdcp_status;
	func->hdcp_config_dump = api_hdcp_config_dump;
	func->get_hdcp_avmute = api_get_hdcp_avmute;
}
