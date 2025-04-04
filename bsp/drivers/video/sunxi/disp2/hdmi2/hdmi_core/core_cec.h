/*
 * linux-3.10/drivers/video/sunxi/disp2/hdmi2/hdmi_core/core_cec.h
 *
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _CORE_CEC_H_
#define _CORE_CEC_H_

#define CEC_BUF_SIZE 100
#define CEC_MSG_SIZE 80


#define IMAGE_VIEW_ON			0x4

#define	ACTIVE_SOURCE			0x82
#define INACTIVE_SOURCE			0x9d
#define REQUEST_ACTIVE_SOURCE		0x85

#define ROUTING_CHANGE			0x80
#define ROUTING_INFORMATION		0x81
#define SET_STREAM_PATH			0x86


#define STANDBY				0x36

#define ABORT				0xff
#define FEATURE_ABORT			0x0

#define GIVE_PHYSICAL_ADDRESS		0x83
#define REPORT_PHYSICAL_ADDRESS		0x84

#define GIVE_DEVICE_VENDOR_ID		0x8c
#define DEVICE_VENDOR_ID			0x87

#define GIVE_DEVICE_POWER_STATUS	0x8f
#define REPORT_POWER_STATUS		0x90

/* cec_msg tx/rx_status field */
#define CEC_TX_STATUS_OK            (1 << 0)
#define CEC_TX_STATUS_NACK          (1 << 1)
#define CEC_TX_STATUS_ERROR         (1 << 2)
#define CEC_TX_STATUS_MAX_RETRIES   (1 << 3)

#define CEC_RX_STATUS_OK            (1 << 0)
#define CEC_RX_STATUS_TIMEOUT       (1 << 1)
#define CEC_RX_STATUS_FEATURE_ABORT (1 << 2)

/* CEC config ioctl */
#define CEC_S_PHYS_ADDR             _IOW('c', 1, __u16)
#define CEC_G_PHYS_ADDR             _IOR('c', 2, __u16)
#define CEC_S_LOG_ADDR              _IOW('c', 3, __u8)
#define CEC_G_LOG_ADDR              _IOR('c', 4, __u8)

/* CEC transmit/receive ioctl */
#define CEC_TRANSMIT                _IOWR('c', 5, struct cec_msg)
#define CEC_RECEIVE                 _IOWR('c', 6, struct cec_msg)


enum cec_power_status {
	CEC_POWER_ON = 0,
	CEC_STANDBY = 1,
	CEC_STANDBY_TO_ON = 2,
	CEC_ON_TO_STANDBY = 3,
};

enum cec_logic_addr {
	TV_DEV = 0,
	RD_DEV_1,
	RD_DEV_2,
	TN_DEV_1,
	PB_DEV_1 = 4,
	AUDIO_SYS,
	TN_DEV_2,
	TN_DEV_3,
	PB_DEV_2 = 8,
	RD_DEV_3,
	TN_DEV_4,
	PB_DEV_3 = 11,
	CEC_BROADCAST = 15,
};

enum cec_trans_type {
	NON_BLOCK_SEND = 0,
	BLOCK_SEND,
};

struct cec_opcode {
	u8 opcode;
	u8 name[30];
};

#ifdef CONFIG_AW_HDMI2_CEC_USER
struct cec_msg {
	uint32_t len;       /* Length in bytes of the message */
	uint32_t timeout;   /* The timeout (in ms) for waiting for a reply */
	uint32_t sequence;  /* Seq num for reply tracing */
	uint32_t tx_status;
	uint32_t rx_status;
	uint8_t  msg[32];
};

struct cec_msg_tx {
	struct cec_msg msg;
	unsigned char trans_type;
	struct list_head list;
};

struct cec_msg_rx {
	struct cec_msg msg;
	struct list_head list;
};


struct cec_private {
	u32 i;
};
#endif

int cec_thread_init(void *param);
void cec_thread_exit(void);
s32 hdmi_cec_enable(int enable);
void hdmi_cec_soft_disable(void);
void cec_set_local_standby(bool enable);
bool cec_get_local_standby(void);
ssize_t cec_dump_core(char *buf);

void hdmi_cec_init(void);
void hdmi_cec_exit(void);


#ifndef CONFIG_AW_HDMI2_CEC_USER
void hdmi_cec_wakup_request(void);
s32 hdmi_cec_standby_request(void);
s32 hdmi_cec_send_inactive_source(void);
void hdmi_cec_send_active_source(void);
int hdmi_cec_send_one_touch_play(void);
void hdmi_cec_set_phyaddr_to_cpus(void);

#endif

extern struct mutex ddc_analog_lock;

extern unsigned char hdmi_get_ddc_analog(void);

extern u32 hdmi_enable_mask;
extern u32 hdmi_suspend_mask;

extern void mc_cec_clock_enable(hdmi_tx_dev_t *dev, u8 bit);
#endif
