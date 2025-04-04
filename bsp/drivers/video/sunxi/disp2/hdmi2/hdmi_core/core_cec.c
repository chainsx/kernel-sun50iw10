/*
 * linux-3.10/drivers/video/sunxi/disp2/hdmi2/hdmi_core/corDEFINE_MUTEX( * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
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
#include "hdmi_core.h"
#include "api/api.h"
#include "api/cec.h"

#include <linux/clk.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/cdev.h>
#include <linux/list.h>
#include <linux/spinlock.h>

#define WAKEUP_TV_MSG			(1 << 0)
#define STANDBY_TV_MSG			(1 << 1)

DEFINE_MUTEX(thread_lock);
DEFINE_MUTEX(send_lock);

#ifdef CONFIG_AW_HDMI2_CEC_USER

#define RX_CEC_MAX_NUM 300

DEFINE_MUTEX(logaddr_lock);
DEFINE_MUTEX(file_ops_lock);
DEFINE_MUTEX(cectx_list_lock);
DEFINE_MUTEX(cecrx_list_lock);
#endif

static int cec_active;
static bool cec_local_standby;
static struct task_struct *cec_task;

#ifndef CONFIG_AW_HDMI2_CEC_USER
static char cec_msg_rx[16];
static int msg_count_rx;
static char cec_msg_tx[16];
static int msg_count_tx;
static bool active_src;
static char src_logic, dst_logic;

static char cec_buf[CEC_BUF_SIZE];
static char cec_buf_temp[CEC_MSG_SIZE];

#else
static dev_t cec_devid;
static struct cdev *cec_cdev;
static struct class *cec_class;
static struct device *cec_dev;

static char src_logic;

static wait_queue_head_t cec_poll_queue;
static wait_queue_head_t cec_send_queue;

struct cec_private cec_priv;
static LIST_HEAD(cec_tx_list);
static LIST_HEAD(cec_rx_list);
static unsigned int rx_list_count;
#endif


static char *logic_addr[] = {
	"TV",
	"Recording_1",
	"Recording_2",
	"Tuner_1",
	"Playback_1",
	"Audio",
	"Tuner_2",
	"Tuner_3",
	"Playback_2",
	"Recording_3",
	"Tuner_4",
	"Playback_3",
	"Reserve",
	"Reserve",
	"Specific Use",
	"All",
};

#ifndef CONFIG_AW_HDMI2_CEC_USER
static struct cec_opcode use_cec_opcode[] = {
	{IMAGE_VIEW_ON, "image view on"},
	{ACTIVE_SOURCE, "active source"},
	{REQUEST_ACTIVE_SOURCE, "request active source"},
	{INACTIVE_SOURCE, "inactive source"},
	{ROUTING_CHANGE, "routing change"},
	{ROUTING_INFORMATION, "routing information"},
	{SET_STREAM_PATH, "set stream path"},
	{STANDBY, "standby"},
	{ABORT, "abort"},
	{FEATURE_ABORT, "feature abort"},
	{GIVE_PHYSICAL_ADDRESS, "give physical address"},
	{REPORT_PHYSICAL_ADDRESS, "report physical address"},
	{GIVE_DEVICE_VENDOR_ID, "give device vendor id"},
	{DEVICE_VENDOR_ID, "vendor id"},
	{GIVE_DEVICE_POWER_STATUS, "give device power status"},
	{REPORT_POWER_STATUS, "report power status"},
};



static int get_cec_opcode_name(char opcode, char *buf)
{
	int i, index = -1;
	bool find = false;
	int count = sizeof(use_cec_opcode) / sizeof(struct cec_opcode);

	for (i = 0; i < count; i++) {
		if (use_cec_opcode[i].opcode == opcode) {
			strcpy(buf, use_cec_opcode[i].name);
			find = true;
			index = i;
		}
	}

	if (!find)
		CEC_INF("Not suopport\n");

	return index;
}
#endif

void cec_set_local_standby(bool enable)
{
	cec_local_standby = enable;
}

bool cec_get_local_standby(void)
{
	return cec_local_standby;
}

static s32 hdmi_cec_get_simple_msg(unsigned char *msg, unsigned size)
{
	struct hdmi_tx_core *core = get_platform();
	return cec_ctrlReceiveFrame(&core->hdmi_tx, msg, size);
}

u16 cec_core_get_phyaddr(void)
{
	struct hdmi_tx_core *core = get_platform();

	if (core->mode.sink_cap && core->mode.edid_done)
		return core->mode.sink_cap->edid_mHdmivsdb.mPhysicalAddress;
	return 0x1000;
}

static void cec_msleep(unsigned int ms)
{
	set_current_state(TASK_INTERRUPTIBLE);
	schedule_timeout(msecs_to_jiffies(ms));
}

#ifdef CONFIG_AW_HDMI2_CEC_USER
void cec_core_set_logaddr(unsigned char logaddr)
{
	struct hdmi_tx_core *core = get_platform();

	mutex_lock(&logaddr_lock);
	src_logic = logaddr;
	cec_CfgLogicAddr(&core->hdmi_tx, logaddr, 1);
	mutex_unlock(&logaddr_lock);
}

unsigned char cec_core_get_logaddr(void)
{
	unsigned char logical;
	struct hdmi_tx_core *core = get_platform();

	mutex_lock(&logaddr_lock);
	logical = (unsigned char)cec_get_log_addr(&core->hdmi_tx);
	mutex_unlock(&logaddr_lock);

	return logical;
}

int cec_core_msg_transmit(struct cec_msg *msg)
{
	int ret;
	struct hdmi_tx_core *core = get_platform();

	if ((!core) || (!msg))
		pr_err("Error: has NULL point\n");

	if (!cec_active) {
		pr_err("cec NOT enable now!\n");
		ret = -1;
		goto _cec_send_out;
	}

#ifdef TCON_PAN_SEL
	if (!hdmi_get_ddc_analog()) {
		pr_err("TCON pad is NOT set!\n");
		return 0;
	}
#endif

	mutex_lock(&ddc_analog_lock);
	ret = cec_ctrlSendFrame(&core->hdmi_tx, &msg->msg[1],
						msg->len - 1,
						(msg->msg[0] >> 4) & 0x0f,
						msg->msg[0] & 0x0f);
	mutex_unlock(&ddc_analog_lock);

	if (ret == 0) {
		msg->tx_status = CEC_TX_STATUS_OK;
	} else if (ret < 0) {
		CEC_INF("CEC send error, error code:%d\n", ret);
		msg->tx_status = CEC_TX_STATUS_ERROR;
	} else if (ret == 1) {
		msg->tx_status = CEC_TX_STATUS_NACK;
	}

	return ret;

_cec_send_out:
	msg->tx_status = CEC_TX_STATUS_ERROR;
	return ret;

}

int cec_core_msg_receive(struct cec_msg *msg)
{
	unsigned int ret = 0;
	struct cec_msg_rx *rx_msg = NULL, *n;

	mutex_lock(&cecrx_list_lock);
	if (!list_empty(&cec_rx_list)) {
		list_for_each_entry_safe(rx_msg, n, &cec_rx_list, list) {
			mutex_unlock(&cecrx_list_lock);
			memcpy(msg, &rx_msg->msg, sizeof(struct cec_msg));

			mutex_lock(&cecrx_list_lock);
			list_del(&rx_msg->list);
			rx_list_count--;
			mutex_unlock(&cecrx_list_lock);

			vfree(rx_msg);

			mutex_lock(&cecrx_list_lock);
		}
	} else {
		pr_err("Error:%s : Rx List is empty\n", __func__);
		ret = -1;
	}
	mutex_unlock(&cecrx_list_lock);

	return ret;
}

bool cec_core_msg_poll(struct file *filp)
{
	mutex_lock(&cecrx_list_lock);
	if (!list_empty(&cec_rx_list)) {
		mutex_unlock(&cecrx_list_lock);
		return true;
	} else {
		mutex_unlock(&cecrx_list_lock);
		return false;
	}
	mutex_unlock(&cecrx_list_lock);
	return false;
}
#endif

#ifndef CONFIG_AW_HDMI2_CEC_USER
static void hdmi_printf_cec_info(char *buf, int count, int type)
{
	int i, ret;
	char opcode_name[30];
	u32 init_addr = (buf[0] >> 4) & 0x0f;
	u32 follow_addr = buf[0] & 0x0f;

	if (!buf) {
		pr_err("Error:print buf is NULL\n");
		return;
	}

	if (count <= 0) {
		pr_err("Error: No valid print count\n");
		return;
	}

	if ((init_addr < 0) || (init_addr > 15)) {
		pr_err("Error: Invalid init_addr%d\n", init_addr);
		return;
	}

	if ((follow_addr < 0) || (follow_addr > 15)) {
		pr_err("Error: Invalid init_addr%d\n", follow_addr);
		return;
	}

	if (type)
		CEC_INF("%s--->%s\n",
			logic_addr[init_addr],
			logic_addr[follow_addr]);
	else
		CEC_INF("%s<---%s\n",
			logic_addr[follow_addr],
			logic_addr[init_addr]);

	if (count > 1)
		ret = get_cec_opcode_name(buf[1], opcode_name);

	if ((count > 1) && (ret >= 0))
		CEC_INF("%s\n", opcode_name);
	else if (count == 1)
		CEC_INF("CEC Ping\n");

	CEC_INF("CEC data:");
	for (i = 0; i < count; i++)
		CEC_INF("0x%02x ", buf[i]);
	CEC_INF("\n\n");
}

/* void hdmi_cec_set_phyaddr_to_cpus(void)
{
	unsigned int phyaddr = (unsigned int)cec_core_get_phyaddr();

	if (phyaddr)
		arisc_set_hdmi_cec_phyaddr(phyaddr);
	else
		pr_warn("[HDMI CEC]Warning: cec physical address is 0\n");
} */

static s32 hdmi_cec_send_with_allocated_srclogic(char *buf, unsigned size,
							 unsigned dst)
{
	unsigned src = src_logic;
	int retval;
	struct hdmi_tx_core *core = get_platform();
	if (!cec_active) {
		pr_err("cec NOT enable now!\n");
		retval = -1;
		goto _cec_send_out;
	}

	mutex_lock(&send_lock);
	retval = cec_ctrlSendFrame(&core->hdmi_tx, buf, size, src, dst);
	if (retval >= 0) {
		msg_count_tx = size + 1;
		memset(cec_msg_tx, 0, sizeof(cec_msg_tx));
		cec_msg_tx[0] = (src << 4) | dst;
		memcpy(&cec_msg_tx[1], buf, size);
		hdmi_printf_cec_info(cec_msg_tx, msg_count_tx, 1);
	} else {
		pr_err("CEC send error, error code:%d\n", retval);
	}
	mutex_unlock(&send_lock);

_cec_send_out:
	return retval;
}

static void hdmi_cec_image_view_on(void)
{
	int i;
	char image_view_on[2] = {IMAGE_VIEW_ON, 0};

	for (i = 0; i < 5; i++) {
		if (hdmi_cec_send_with_allocated_srclogic(image_view_on, 1,
								TV_DEV) == 1)
			continue;
		else
			break;
	}
}

void hdmi_cec_send_active_source(void)
{
	int i;
	char active[4];
	u16 addr = cec_core_get_phyaddr();

	active[0] = ACTIVE_SOURCE;
	active[1] = (addr >> 8) & 0xff;
	active[2] = addr & 0xff;

	for (i = 0; i < 5; i++) {
		if (hdmi_cec_send_with_allocated_srclogic(active, 3,
						CEC_BROADCAST) == 1)
			continue;
		else
			break;
	}
	active_src = true;
}

static void hdmi_cec_send_request_active_source(void)
{
	int i;
	char rq[1];

	rq[0] = REQUEST_ACTIVE_SOURCE;

	for (i = 0; i < 5; i++) {
		if (hdmi_cec_send_with_allocated_srclogic(rq, 1,
						CEC_BROADCAST) == 1)
			continue;
		else
			break;
	}
}

s32 hdmi_cec_send_inactive_source(void)
{
	int i;
	char active[4];
	u16 addr = cec_core_get_phyaddr();

	if (!active_src)
		return -1;

	active[0] = INACTIVE_SOURCE;
	active[1] = (addr >> 8) & 0xff;
	active[2] = addr & 0xff;

	for (i = 0; i < 5; i++) {
		if (hdmi_cec_send_with_allocated_srclogic(active, 3,
						TV_DEV) == 1)
			continue;
		else
			break;
	}

	return 0;
}

/* static void hdmi_cec_send_routing_info(void)
{
	char info[4];
	u16 addr = get_cec_phyaddr();

	info[0] = ROUTING_INFORMATION;
	info[1] = (addr >> 8) & 0xff;
	info[2] = addr & 0xff;

	hdmi_cec_send_with_allocated_srclogic(info, 3, CEC_BROADCAST);
} */

static void hdmi_cec_standby(void)
{
	int i;
	char standby[2] = {STANDBY, 0};

	for (i = 0; i < 5; i++) {
		if (hdmi_cec_send_with_allocated_srclogic(standby, 1,
						CEC_BROADCAST) == 1)
			continue;
		else
			break;
	}
}

static void hdmi_cec_send_feature_abort(char dst)
{
	int i;
	char fea_abort[3] = {FEATURE_ABORT, 0xff, 4};

	for (i = 0; i < 5; i++) {
		if (hdmi_cec_send_with_allocated_srclogic(fea_abort, 3,
								dst) == 1)
			continue;
		else
			break;
	}
}

static int hdmi_cec_send_phyaddr(void)
{
	int i;
	unsigned char phyaddr[4];
	u16 addr = cec_core_get_phyaddr();
	int ret = 0;

	if (!addr)
		addr = 1000;
	phyaddr[0] = REPORT_PHYSICAL_ADDRESS;
	phyaddr[1] = (addr >> 8) & 0xff;
	phyaddr[2] = addr & 0xff;
	phyaddr[3] = 4;

	for (i = 0; i < 5; i++) {
		if (hdmi_cec_send_with_allocated_srclogic(phyaddr, 4,
							CEC_BROADCAST) == 1)
			continue;
		else
			break;
	}

	return ret;
}

static int hdmi_cec_send_vendor_id(void)
{
	int i;
	unsigned char ven_id[4];

	ven_id[0] = DEVICE_VENDOR_ID;
	ven_id[1] = 0;
	ven_id[2] = 0x0c;
	ven_id[3] = 0x03;

	for (i = 0; i < 5; i++) {
		if (hdmi_cec_send_with_allocated_srclogic(ven_id, 4,
						CEC_BROADCAST) == 1)
			continue;
		else
			break;
	}
	return 0;
}

static int hdmi_cec_send_power_status(char st, char dst)
{
	int i;
	unsigned char status[2];
	int ret = 0;

	status[0] = REPORT_POWER_STATUS;
	status[1] = st;

	for (i = 0; i < 5; i++) {
		if (hdmi_cec_send_with_allocated_srclogic(status, 2, dst) == 1)
			continue;
		else
			break;
	}
	return ret;
}

s32 hdmi_cec_send_one_touch_play(void)
{
	int temp = cec_active;

	mutex_lock(&thread_lock);

	if (!cec_active)
		cec_active = 1;

	hdmi_cec_image_view_on();
	msleep(500);
	hdmi_cec_send_active_source();

	cec_active = temp;

	mutex_unlock(&thread_lock);
	return 0;
}

/* intend to abtain the follower's and initiator's logical address */
static int hdmi_cec_send_poll(void)
{
	int i, poll_ret;
	unsigned char src = 0, buf;
	struct hdmi_tx_core *core = get_platform();

	for (i = 0; i < 3; i++) {
		if (i == 0) {
			if (src_logic != 0)
				src = src_logic;
			else
				src = PB_DEV_1;
		} else {
			if (src == PB_DEV_1)
				src = (i == 1) ? PB_DEV_2 : PB_DEV_3;
			else if (src == PB_DEV_2)
				src = (i == 1) ? PB_DEV_1 : PB_DEV_3;
			else if (src == PB_DEV_3)
				src = (i == 1) ? PB_DEV_1 : PB_DEV_2;
			else {
				pr_err("Error: error pre src logical address\n");
				src = (i == 1) ? PB_DEV_1 : PB_DEV_2;
			}
		}

		buf = (src << 4) | src;
		hdmi_printf_cec_info(&buf, 1, 1);
		mutex_lock(&send_lock);
		poll_ret = cec_send_poll(&core->hdmi_tx, src);
		mutex_unlock(&send_lock);
		if (!poll_ret) {
			CEC_INF("Get cec src logical address:%s\n", logic_addr[src]);
			cec_CfgLogicAddr(&core->hdmi_tx, src, 1);
			src_logic = src;
			return 0;
		}
		pr_warn("Warn:The polling logical address:0x%x do not get nack\n",
								src);
	}

	if (i == 3) {
		pr_err("err:sink do not respond polling\n");
		src_logic = PB_DEV_1;
		return -1;
	} else {
		return 0;
	}
}

/* static void hdmi_cec_active_src(void)
{
	int time_out = 20;
	int ret;
	char msg[16];

	mutex_lock(&thread_lock);
	hdmi_cec_send_request_active_source();

	while (time_out--) {
		ret = hdmi_cec_get_simple_msg(msg, sizeof(msg));
		if ((ret > 0) && (msg[1] == ACTIVE_SOURCE)) {
			active_src = false;
			pr_info("CEC WARN:There has been an Active Source\n");
			break;
		}
		msleep(20);
	}

	if (time_out <= 0) {
		hdmi_cec_send_active_source();
		active_src = true;
	}
	mutex_unlock(&thread_lock);
} */

void hdmi_cec_wakup_request(void)
{
	int time_out = 20;
	int ret;
	char msg[16];

	active_src = true;
	mutex_lock(&thread_lock);
	hdmi_cec_send_request_active_source();
	while (time_out--) {
		ret = hdmi_cec_get_simple_msg(msg, sizeof(msg));
		if ((ret > 0) && (msg[1] == ACTIVE_SOURCE)) {
			hdmi_printf_cec_info(msg, ret, 0);
			active_src = false;
			pr_info("CEC WARN:There has been an Active Source\n");
			break;
		}
		msleep(20);
	}

	if (time_out <= 0) {
		hdmi_cec_image_view_on();
		udelay(200);
		hdmi_cec_send_active_source();
	}
	mutex_unlock(&thread_lock);
}

s32 hdmi_cec_standby_request(void)
{
	struct hdmi_tx_core *core = get_platform();

	mutex_lock(&thread_lock);
	if ((!cec_get_local_standby()) && core->is_cts) {
		hdmi_cec_standby();
		CEC_INF("cec standby boardcast send\n");
	}
	mutex_unlock(&thread_lock);

	return 0;
}

static char *string_append(char *dest, const char *src)
{
	char *tmp = dest;

	while (*dest)
		dest++;
	while ((*dest++ = *src++) != '\0')
		;
	return tmp;
}

static void cec_msg_sent(struct device *dev, char *buf)
{
	char *envp[2];

	envp[0] = buf;
	envp[1] = NULL;
	kobject_uevent_env(&dev->kobj, KOBJ_CHANGE, envp);
}

static void rx_msg_process(struct device *pdev, unsigned char *msg, int count)
{
	int send_up = 1, i;

	if (((msg[0] & 0x0f) == src_logic) && (count == 1)) {
		hdmi_cec_send_phyaddr();
		udelay(200);
		hdmi_cec_send_vendor_id();
		send_up = 0;
	} else if (count > 1) {
		switch (msg[1]) {
		case ABORT:
			dst_logic = (msg[0] >> 4) & 0x0f;
			if ((msg[0] & 0x0f) == src_logic)
				hdmi_cec_send_feature_abort(dst_logic);

			send_up = 1;
			break;

		case REQUEST_ACTIVE_SOURCE:
			if (((msg[0] & 0x0f) == 0x0f)
					&& active_src)
				hdmi_cec_send_active_source();

			send_up = 1;
			break;

		case GIVE_PHYSICAL_ADDRESS:
			if ((msg[0] & 0x0f) == src_logic)
				hdmi_cec_send_phyaddr();
			send_up = 1;
			break;
		case GIVE_DEVICE_VENDOR_ID:
			hdmi_cec_send_vendor_id();
			send_up = 1;
			break;
		case SET_STREAM_PATH:
			if ((((msg[2] << 8) | msg[3]) == cec_core_get_phyaddr())
						&& (count == 4)) {
				if ((msg[0] & 0x0f) == 0x0f) {
					hdmi_cec_send_active_source();
					active_src = true;
				}
			} else if ((msg[0] & 0x0f) == 0x0f) {
				active_src = false;
			}

			send_up = 1;
			break;
		/* case ROUTING_CHANGE:
			if ((((msg[4] << 8) | msg[5]) == get_cec_phyaddr())
				&& (count >= 6))
				hdmi_cec_send_routing_info();

				send_up = 1;
				break;
		case ROUTING_INFORMATION:
			hdmi_cec_send_routing_info();
			send_up = 1;
			break; */
		case GIVE_DEVICE_POWER_STATUS:
			if ((!hdmi_suspend_mask) && (!hdmi_enable_mask)) {
				dst_logic = (msg[0] >> 4) & 0x0f;
				if ((msg[0] & 0x0f) == src_logic)
					hdmi_cec_send_power_status(CEC_STANDBY_TO_ON, dst_logic);
			} else if ((!hdmi_suspend_mask) && hdmi_enable_mask) {
				dst_logic = (msg[0] >> 4) & 0x0f;
				if ((msg[0] & 0x0f) == src_logic)
					hdmi_cec_send_power_status(CEC_POWER_ON, dst_logic);
			} else if (hdmi_suspend_mask && hdmi_enable_mask) {
				dst_logic = (msg[0] >> 4) & 0x0f;
				if ((msg[0] & 0x0f) == src_logic)
					hdmi_cec_send_power_status(CEC_ON_TO_STANDBY, dst_logic);
			} else {
				pr_err("Error: unknow current power status\n");
			}

			send_up = 1;
			break;
		case STANDBY:
			if (((msg[0] & 0x0f) != 0xf) && ((msg[0] & 0x0f) != src_logic))
				send_up = 0;
			else
				send_up = 1;
			break;

		case INACTIVE_SOURCE:
			if (((msg[0] & 0x0f) == src_logic)
					&& (count == 4)
					&& (((msg[2] << 8) | msg[3])
					!= cec_core_get_phyaddr()))
				hdmi_cec_send_active_source();

			send_up = 1;
			break;

		case ACTIVE_SOURCE:
			if ((msg[0] & 0x0f) == 0xf
					&& (count == 4)
					&& (((msg[2] << 8) | msg[3])
					!= cec_core_get_phyaddr()))
				active_src = false;
			send_up = 1;
			break;

		default:
			send_up = 1;
			break;
		}
	}

	if (send_up) {
		memset(cec_buf, 0, CEC_BUF_SIZE);
		memset(cec_buf_temp, 0, CEC_MSG_SIZE);
		string_append(cec_buf, "CEC_MSG=");
		for (i = 1; i < count; i++) {
			if (i < count - 1)
				sprintf(cec_buf_temp, "0x%02x ", msg[i]);
			else
				sprintf(cec_buf_temp, "0x%02x", msg[i]);
			string_append(cec_buf, cec_buf_temp);
		}

		cec_msg_sent(pdev, cec_buf);
	}
}

#else
s32 hdmi_cec_standby_request(void){return 0; }
s32 hdmi_cec_send_one_touch_paly(void){return 0; }

static void rx_msg_process(struct cec_msg *msg)
{
	if (msg->len > 1) {
		switch (msg->msg[1]) {
		case FEATURE_ABORT:
			msg->rx_status = CEC_RX_STATUS_FEATURE_ABORT;
			break;
		}
	}
}
#endif

static int cec_thread(void *param)
{
	int ret = 0;
	unsigned char msg[16];
#ifndef CONFIG_AW_HDMI2_CEC_USER
	struct device *pdev = (struct device *)param;
#else
	struct cec_msg_tx *tx_msg = NULL, *n;
	struct cec_msg_rx *rx_msg = NULL;
#endif

	while (1) {
		if (kthread_should_stop()) {
			break;
		}

		if (!hdmi_enable_mask) {
			cec_msleep(10);
			continue;
		}

#ifdef CONFIG_AW_HDMI2_CEC_USER
	/* Send CEC msg */
	mutex_lock(&cectx_list_lock);
	if (!list_empty(&cec_tx_list)) {
		list_for_each_entry_safe(tx_msg, n, &cec_tx_list, list) {
			/* int i;
			for (i = 0; i < 6; i++) { */
				ret = cec_core_msg_transmit(&tx_msg->msg);
				/* if ((ret == 1) && (tx_msg->msg.len >= 2))
					continue;
				else
					break; */
			/* } */

			list_del(&tx_msg->list);

			if (tx_msg->trans_type == BLOCK_SEND) {
				wake_up(&cec_send_queue);
			} else if (tx_msg->trans_type == NON_BLOCK_SEND) {
				/* add the successfully sending msg to cec_rx_list */
				mutex_lock(&cecrx_list_lock);
				INIT_LIST_HEAD(&tx_msg->list);
				list_add_tail(&tx_msg->list, &cec_rx_list);
				rx_list_count++;
				mutex_unlock(&cecrx_list_lock);
			}

		}
	}
	mutex_unlock(&cectx_list_lock);
#endif

	/* Get cec MSG */
	mutex_lock(&thread_lock);
	if (cec_active) {
		ret = hdmi_cec_get_simple_msg(msg, sizeof(msg));
		if (ret > 0) {
#ifndef CONFIG_AW_HDMI2_CEC_USER
			dst_logic = 0;

			msg_count_rx = ret;
			memset(cec_msg_rx, 0, sizeof(cec_msg_rx));
			memcpy(cec_msg_rx, msg, sizeof(msg));
			hdmi_printf_cec_info(cec_msg_rx, msg_count_rx, 0);
			rx_msg_process(pdev, msg, ret);
#else
			if (rx_list_count >= RX_CEC_MAX_NUM)
				goto next_loop;
			rx_msg = vmalloc(sizeof(struct cec_msg_rx));
			if (!rx_msg) {
				pr_err("Error: vmalloc for rx_msg failed\n");
				goto next_loop;
			}
			memset(rx_msg, 0, sizeof(struct cec_msg_rx));
			rx_msg->msg.len = ret;
			memcpy(rx_msg->msg.msg, msg, sizeof(msg));
			rx_msg->msg.rx_status = CEC_RX_STATUS_OK;
			rx_msg_process(&rx_msg->msg);

			mutex_lock(&cecrx_list_lock);
			INIT_LIST_HEAD(&rx_msg->list);
			list_add_tail(&rx_msg->list, &cec_rx_list);
			rx_list_count++;
			mutex_unlock(&cecrx_list_lock);
#endif
			}
		}

#ifdef CONFIG_AW_HDMI2_CEC_USER
next_loop:
#endif
		mutex_unlock(&thread_lock);
		cec_msleep(10);
	}

	return 0;
}

int cec_thread_init(void *param)
{
	cec_task = kthread_create(cec_thread, (void *)param, "cec thread");
	if (IS_ERR(cec_task)) {
		printk("Unable to start kernel thread %s.\n\n", "cec thread");
		cec_task = NULL;
		return PTR_ERR(cec_task);
	}
	wake_up_process(cec_task);
	return 0;
}

void cec_thread_exit(void)
{
	if (cec_task) {
		kthread_stop(cec_task);
		cec_task = NULL;
	}
}

#ifdef CONFIG_AW_HDMI2_CEC_USER
static int cec_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &cec_priv;
	return 0;
}

static int cec_release(struct inode *inode, struct file *filp)
{
	return 0;
}

unsigned int cec_poll(struct file *filp, struct poll_table_struct *wait)
{
	unsigned int mask = 0;

	mutex_lock(&file_ops_lock);
	poll_wait(filp, &cec_poll_queue, wait);
	if (cec_core_msg_poll(filp))
		mask |= POLLIN | POLLRDNORM;
	mutex_unlock(&file_ops_lock);

	return mask;
}

static long cec_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	unsigned long p_arg[3];
	struct cec_msg msg;
	struct cec_msg_tx *tx_msg;
	unsigned int phy_addr;
	unsigned char log_addr;
	int ret = 0, i;

	if (copy_from_user((void *)p_arg, (void __user *)arg,
					3 * sizeof(unsigned long))) {
		pr_warn("copy_from_user fail\n");
		goto err_exit;
	}

	mutex_lock(&file_ops_lock);
	switch (cmd) {
	case CEC_S_PHYS_ADDR:
		break;
	case CEC_G_PHYS_ADDR:
		phy_addr = (unsigned int)cec_core_get_phyaddr();
		if (phy_addr > 0) {
			if (copy_to_user((void __user *)p_arg[0],
				&phy_addr, sizeof(unsigned int))) {
				pr_info("copy_to_user fail\n");
				goto err_exit;
			}
		}
	mutex_unlock(&file_ops_lock);
	return (long)phy_addr;

	case CEC_S_LOG_ADDR:
		if (copy_from_user((void *)&log_addr, (void __user *)p_arg[0],
						sizeof(unsigned char))) {
			pr_warn("copy_from_user fail\n");
			goto err_exit;
		}
		if ((log_addr >= 0) && (log_addr <= 0x0f))
			cec_core_set_logaddr(log_addr);
		else {
			pr_err("ERROR: invalid cec logcal address:0x%x\n", log_addr);
			goto err_exit;
		}
		break;
	case CEC_G_LOG_ADDR:
		log_addr = cec_core_get_logaddr();
		if (copy_to_user((void __user *)p_arg[0],
			&log_addr, sizeof(unsigned char))) {
			pr_info("copy_to_user fail\n");
			goto err_exit;
		}

		mutex_unlock(&file_ops_lock);
		return (long)log_addr;

	case CEC_TRANSMIT:
#ifdef TCON_PAN_SEL
		CEC_INF("T:HDMI PAD:%d\n", hdmi_get_ddc_analog());
#endif
		if (!cec_active || !hdmi_enable_mask) {
			CEC_INF("cec_active:%d hdmi_enable:%d\n",
				cec_active, hdmi_enable_mask);
			break;
		}

		tx_msg = vmalloc(sizeof(struct cec_msg_tx));
		if (!tx_msg)
			goto err_exit;
		if (copy_from_user((void *)&tx_msg->msg,
					(void __user *)p_arg[0],
					sizeof(struct cec_msg))) {
			pr_warn("copy_from_user fail\n");
			goto err_exit;
		}

		tx_msg->msg.tx_status = 0;
		if (!tx_msg->msg.timeout)
			tx_msg->msg.timeout = 1000;
		tx_msg->trans_type = (filp->f_flags & O_NONBLOCK) ?
					NON_BLOCK_SEND : BLOCK_SEND;

		CEC_INF("CEC start transmitting, para: ""len:%d  timeout:%d  sequence:%d "
			"tx_status:%d rx_status:%d\n",
						tx_msg->msg.len,
						tx_msg->msg.timeout,
						tx_msg->msg.sequence,
						tx_msg->msg.tx_status,
						tx_msg->msg.rx_status);
		for (i = 0; i < tx_msg->msg.len; i++)
			CEC_INF("msg[%d]:0x%x ", i, tx_msg->msg.msg[i]);

		CEC_INF("\n");

		mutex_lock(&cectx_list_lock);
		INIT_LIST_HEAD(&tx_msg->list);
		list_add_tail(&tx_msg->list, &cec_tx_list);
		mutex_unlock(&cectx_list_lock);

		if (tx_msg->trans_type == BLOCK_SEND) {
			ret = wait_event_interruptible_timeout(cec_send_queue,
						tx_msg->msg.tx_status > 0,
					msecs_to_jiffies(tx_msg->msg.timeout));
			CEC_INF("CEC sending finish, tx_status:%d\n",
						tx_msg->msg.tx_status);
			mutex_lock(&cectx_list_lock);
			if (ret < 0) {
				pr_err("wait_event_interruptible_timeout failed\n");
				if (!list_empty(&cec_tx_list))
					list_del(&tx_msg->list);
				vfree(tx_msg);
				mutex_unlock(&cectx_list_lock);
				goto err_exit;
			} else if (ret == 0) { /* Send msg timeout */
				pr_err("cec send wait timeout\n");
				if (!list_empty(&cec_tx_list))
					list_del(&tx_msg->list);
				tx_msg->msg.tx_status = CEC_TX_STATUS_ERROR;
			}

			if (copy_to_user((void __user *)p_arg[0],
						&tx_msg->msg,
					sizeof(struct cec_msg))) {
				pr_info("copy_to_user fail\n");
				if (!list_empty(&cec_tx_list))
					list_del(&tx_msg->list);
				vfree(tx_msg);
				mutex_unlock(&cectx_list_lock);
				goto err_exit;
			}
			vfree(tx_msg);
			mutex_unlock(&cectx_list_lock);
		}

		break;
	case CEC_RECEIVE:
#ifdef TCON_PAN_SEL
		CEC_INF("R:HDMI PAD:%d\n", hdmi_get_ddc_analog());
#endif
		ret = cec_core_msg_receive(&msg);
		if (ret >= 0) {
			if (copy_to_user((void __user *)p_arg[0], &msg,
						sizeof(struct cec_msg))) {
				pr_info("copy_to_user fail\n");
				goto err_exit;
			}

			CEC_INF("Receive para: ""len:%d  timeout:%d  sequence:%d "
			"tx_status:%d rx_status:%d\n",
						msg.len,
						msg.timeout,
						msg.sequence,
						msg.tx_status,
						msg.rx_status);
		for (i = 0; i < msg.len; i++)
			CEC_INF("msg[%d]:0x%x ", i, msg.msg[i]);

		CEC_INF("\n");
			break;
		} else {
			pr_info("Error:There is NOT a CEC MSG received!\n");
			goto err_exit;
		}

	default:
		break;
	}

	mutex_unlock(&file_ops_lock);
	return 0;

err_exit:
	mutex_unlock(&file_ops_lock);
	return -1;
}

#ifdef CONFIG_COMPAT
static long cec_compat_ioctl(struct file *file, unsigned int cmd,
					unsigned long arg)
{
	compat_uptr_t karg[3];
	unsigned long __user *ubuffer;

	if (copy_from_user((void *)karg, (void __user *)arg,
			3 * sizeof(compat_uptr_t))) {
		pr_warn("copy_from_user fail\n");
		return -EFAULT;
	}

	ubuffer = compat_alloc_user_space(3 * sizeof(unsigned long));
	if (!access_ok(/* VERIFY_WRITE, */ ubuffer, 3 * sizeof(unsigned long)))
		return -EFAULT;

	if (put_user(karg[0], &ubuffer[0]) ||
		put_user(karg[1], &ubuffer[1]) ||
		put_user(karg[2], &ubuffer[2])) {
		pr_warn("put_user fail\n");
		return -EFAULT;
	}

	return cec_ioctl(file, cmd, (unsigned long)ubuffer);
}
#endif

static const struct file_operations cec_fops = {
	.owner		= THIS_MODULE,
	.open		= cec_open,
	.release	= cec_release,
	.poll		= cec_poll,
	.unlocked_ioctl	= cec_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= cec_compat_ioctl,
#endif
};

void hdmi_cec_init(void)
{
	int err;

	pr_info("%s\n", __func__);
	/* Create and add a character device */
	alloc_chrdev_region(&cec_devid, 0, 1, "cec");/* corely for device number */
	cec_cdev = cdev_alloc();
	cdev_init(cec_cdev, &cec_fops);
	cec_cdev->owner = THIS_MODULE;

	err = cdev_add(cec_cdev, cec_devid, 1);
	if (err)
		pr_err("Error: CEC cdev_add fail.\n");

	/* Create a path: sys/class/hdmi */
	cec_class = class_create(THIS_MODULE, "cec");
	if (IS_ERR(cec_class))
		pr_err("Error:cec class_create fail\n");
	cec_dev = device_create(cec_class, NULL,
			     cec_devid, NULL, "cec");

	init_waitqueue_head(&cec_poll_queue);
	init_waitqueue_head(&cec_send_queue);
	rx_list_count = 0;
}

void hdmi_cec_exit(void)
{
	class_destroy(cec_class);
	cdev_del(cec_cdev);
}
#else
void hdmi_cec_init(void){; }
void hdmi_cec_exit(void){; }
#endif

/* Just for CEC Super Standby
* In  CEC Super Standby Mode, CEC can not be disable when system suspend,
* So we create a cec soft disable operation which just disable some value
*/
void hdmi_cec_soft_disable(void)
{
	cec_active = 0;
}

s32 hdmi_cec_enable(int enable)
{
	struct hdmi_tx_core *core = get_platform();

	LOG_TRACE1(enable);

	mutex_lock(&thread_lock);
	if (enable) {
		if (cec_active == 1) {
			mutex_unlock(&thread_lock);
			return 0;
		}

		/*
		 * Enable cec module and set logic address to playback device 1.
		 * TODO: ping other devices and allocate a idle logic address.
		 */
		core->hdmi_tx.snps_hdmi_ctrl.cec_on = true;
		mc_cec_clock_enable(&core->hdmi_tx, 0);
		udelay(200);
		cec_Init(&core->hdmi_tx);
		cec_active = 1;

#ifndef CONFIG_AW_HDMI2_CEC_USER
		hdmi_cec_send_poll();
		udelay(200);
		hdmi_cec_send_phyaddr();
#else
		mutex_lock(&logaddr_lock);
		if (src_logic)
			cec_CfgLogicAddr(&core->hdmi_tx, src_logic, 1);
		mutex_unlock(&logaddr_lock);
#endif
	} else {
		if (cec_active == 0) {
			mutex_unlock(&thread_lock);
			return 0;
		}
		cec_active = 0;
		cec_Disable(&core->hdmi_tx, 0);
		mc_cec_clock_enable(&core->hdmi_tx, 1);
		core->hdmi_tx.snps_hdmi_ctrl.cec_on = false;
	}
	mutex_unlock(&thread_lock);
	return 0;
}

ssize_t cec_dump_core(char *buf)
{
	ssize_t n = 0;

#ifdef CONFIG_AW_HDMI2_CEC_USER
	struct hdmi_tx_core *core = get_platform();
#endif

	n += sprintf(buf + n, "\n");

	if (cec_task)
		n += sprintf(buf + n, "cec thread is running\n");
	else
		n += sprintf(buf + n, "cec thread has been stopped\n");

	if (cec_active) {
		n += sprintf(buf + n, "cec is active\n");
	} else {
		n += sprintf(buf + n, "cec is NOT active\n\n");
		return n;
	}

	if (cec_local_standby)
		n += sprintf(buf + n, "cec is in local standby model\n");
	else
		n += sprintf(buf + n, "cec is NOT in local standby model\n");

	n += sprintf(buf + n, "Tx Current cec physical address:0x%x\n",
							cec_core_get_phyaddr());

#ifndef CONFIG_AW_HDMI2_CEC_USER
	n += sprintf(buf + n, "Tx Current logical address:%s\n",
							logic_addr[(unsigned char)src_logic]);
	n += sprintf(buf + n, "Rx Current logical address:%s\n",
							logic_addr[(unsigned char)dst_logic]);
#else
	mutex_lock(&logaddr_lock);
	n += sprintf(buf + n, "Tx Current logical address:%s\n",
							logic_addr[cec_get_log_addr(&core->hdmi_tx)]);
	mutex_unlock(&logaddr_lock);
#endif
	n += sprintf(buf + n, "\n\n");

	return n;
}
