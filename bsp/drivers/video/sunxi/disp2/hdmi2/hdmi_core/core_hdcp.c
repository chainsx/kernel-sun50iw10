/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include "core_hdcp.h"

int get_hdcp_type_core(struct hdmi_tx_core *core)
{
	if ((!core->mode.pHdcp.use_hdcp) ||
		(!core->mode.pHdcp.hdcp_on))
		return -1;

	return  core->dev_func.get_hdcp_type();
}

/* return 0: hdcp encrypting status is good
* return -1: hdcp encrypt failed
*/
int get_hdcp_status_core(void)
{
	struct hdmi_tx_core *core = get_platform();

	return core->dev_func.get_hdcp_status();
}

void core_init_hdcp(struct hdmi_mode *cfg, hdcpParams_t *hdcp)
{
	hdcpParams_t *pHdcp = &(cfg->pHdcp);

	pHdcp->hdcp_on = 0;
	pHdcp->mEnable11Feature = -1;
	pHdcp->mRiCheck = -1;
	pHdcp->mI2cFastMode = -1;
	pHdcp->mEnhancedLinkVerification = -1;
	pHdcp->maxDevices = 0;
	pHdcp->mKsvListBuffer = NULL;
	pHdcp->mAksv = NULL;
	pHdcp->mKeys = NULL;
	pHdcp->mSwEncKey = NULL;

	pHdcp->use_hdcp = hdcp->use_hdcp;
#ifdef CONFIG_AW_HDMI2_HDCP_SUNXI
	pHdcp->use_hdcp22 = hdcp->use_hdcp22;
	pHdcp->esm_hpi_base = hdcp->esm_hpi_base;
	pHdcp->esm_firm_phy_addr = hdcp->esm_firm_phy_addr;
	pHdcp->esm_firm_vir_addr = hdcp->esm_firm_vir_addr;
	pHdcp->esm_firm_size = hdcp->esm_firm_size;
	pHdcp->esm_data_phy_addr = hdcp->esm_data_phy_addr;
	pHdcp->esm_data_vir_addr = hdcp->esm_data_vir_addr;
	pHdcp->esm_data_size = hdcp->esm_data_size;
#endif
}

u8 get_hdcp22_status_core(void)
{
	return 0;
}

void hdcp_enable_core(struct hdmi_tx_core *core, u8 enable)
{
	if (enable) {
		if (core->mode.pHdcp.hdcp_on) {
			pr_info("hdcp has been on\n");
			return;
		}
		core->mode.pHdcp.hdcp_on = 1;
		if (get_drv_hpd_state())
			core->dev_func.hdcp_configure(&core->mode.pHdcp,
					&core->mode.pVideo);
	} else {
		if (!core->mode.pHdcp.hdcp_on) {
			pr_info("hdcp has been close\n");
			return;
		}

		core->mode.pHdcp.hdcp_on = 0;
		core->dev_func.hdcp_disconfigure();
	}
}

ssize_t hdcp_dump_core(char *buf)
{
	ssize_t n = 0;
	struct hdmi_tx_core *core = get_platform();
	hdcpParams_t *hdcp = &core->mode.pHdcp;
	videoParams_t *video = &core->mode.pVideo;

	LOG_TRACE();
	sprintf(buf + n, "Core Level Part:\n");
	n += sprintf(buf + n, "%s\n",
		hdcp->use_hdcp ? "Tx use hdcp 1.4" : "Tx do NOT use hdcp");
	n += sprintf(buf + n, "%s\n",
		hdcp->use_hdcp22 ? "Tx use hdcp 2.2" : "Tx do NOT use hdcp 2.2");
	n += sprintf(buf + n, "%s\n",
		hdcp->hdcp_on ? "Enable HDCP" : "Disable HDCP");
	if (hdcp->hdcp_on)
		n += sprintf(buf + n, "HDMI MODE: %s\n",
			video->mHdmi == DVI ? "DVI" : "HDMI");
#ifdef CONFIG_AW_HDMI2_HDCP22_SUNXI
	if (hdcp->use_hdcp22) {
		n += sprintf(buf + n, "esm firmware addr:0x%x  size:0x%x\n",
					(unsigned int)hdcp->esm_firm_phy_addr,
					hdcp->esm_firm_size);
		n += sprintf(buf + n, "esm data addr:0x%x  size:0x%x\n",
					(unsigned int)hdcp->esm_data_phy_addr,
					hdcp->esm_data_size);
	}
#endif
	return n;
}
