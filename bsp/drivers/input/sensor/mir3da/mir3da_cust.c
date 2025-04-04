/* For AllWinner android platform.
 *
 * mir3da.c - Linux kernel modules for 3-Axis Accelerometer
 *
 * Copyright (C) 2011-2013 MiraMEMS Sensing Technology Co., Ltd.
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

//#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/hrtimer.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/miscdevice.h>
#include <linux/stat.h>
#include <linux/syscalls.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/ktime.h>
#include "../../init-input.h"
#include "mir3da_core.h"
#include "mir3da_cust.h"

#define MIR3DA_DEVICE_NAME			  "mir3da"
#define MIR3DA_DRV_NAME				 	"mir3da"
#define MIR3DA_INPUT_DEV_NAME	 	MIR3DA_DRV_NAME
#define MIR3DA_MISC_NAME					MIR3DA_DRV_NAME

#define POLL_INTERVAL_MAX			   	500
#define POLL_INTERVAL				   	40
#define INPUT_FUZZ					  	0
#define INPUT_FLAT					  	0

static int mir3da_startup(void);

static const unsigned short normal_i2c[] = {0x27, 0x26, I2C_CLIENT_END};

static struct sensor_config_info gsensor_info = {
	.input_type = GSENSOR_TYPE,
	.np_name = MIR3DA_DEVICE_NAME,
};

static struct input_dev	  *mir3da_idev;
//static struct device 			*hwmon_dev;
static struct hrtimer 		hr_timer;
static ktime_t 				myktime;
static struct work_struct 	wq_hrtimer;
static MIR_HANDLE					  mir_handle;

static unsigned int					   delayMs = POLL_INTERVAL;
static unsigned char 			twi_id;
static int is_probe_success;

extern int							  	Log_level;

#define MI_DATA(format, ...) 	\
	do {\
		if (DEBUG_DATA & Log_level) {\
			printk(KERN_ERR MI_TAG format "\n", ## __VA_ARGS__);\
		} \
	} while (0)
#define MI_MSG(format, ...)		\
	do {\
		if (DEBUG_MSG & Log_level) {\
			printk(KERN_ERR MI_TAG format "\n", ## __VA_ARGS__);\
		} \
	} while (0)
#define MI_ERR(format, ...)		\
	do {\
		if (DEBUG_ERR & Log_level) {\
			printk(KERN_ERR MI_TAG format "\n", ## __VA_ARGS__);\
		} \
	} while (0)
#define MI_FUN					\
	do {\
		if (DEBUG_FUNC & Log_level) {\
			printk(KERN_ERR MI_TAG "%s is called, line: %d\n", __FUNCTION__, __LINE__);\
		} \
	} while (0)
#define MI_ASSERT(expr)				 \
	do {\
		if (!(expr)) {\
			printk(KERN_ERR "Assertion failed! %s,%d,%s,%s\n",\
				__FILE__, __LINE__, __func__, #expr);\
		} \
	} while (0)

/*----------------------------------------------------------------------------*/
#if MIR3DA_OFFSET_TEMP_SOLUTION
static char OffsetFileName[] = "/data/misc/miraGSensorOffset.txt";
static char OffsetFolerName[] = "/data/misc/";
#define OFFSET_STRING_LEN			   26
struct work_info {
	char		tst1[20];
	char		tst2[20];
	char		buffer[OFFSET_STRING_LEN];
	struct	  workqueue_struct *wq;
	struct	  delayed_work read_work;
	struct	  delayed_work write_work;
	struct	  completion completion;
	int		 len;
	int		 rst;
};

static struct work_info m_work_info = {{0}};
/*----------------------------------------------------------------------------*/
static void sensor_write_work(struct work_struct *work)
{
	struct work_info *pWorkInfo;
	struct file		 *filep;
	mm_segment_t		orgfs;
	int				 ret;

	orgfs = get_fs();
	set_fs(KERNEL_DS);

	pWorkInfo = container_of((struct delayed_work *)work, struct work_info, write_work);
	if (pWorkInfo == NULL) {
		MI_ERR("get pWorkInfo failed!");
		return;
	}

	filep = filp_open(OffsetFileName, O_RDWR|O_CREAT, 0600);
	if (IS_ERR(filep)) {
		MI_ERR("write, sys_open %s error!!.\n", OffsetFileName);
		ret =  -1;
	} else {
		filep->f_op->write(filep, pWorkInfo->buffer, pWorkInfo->len, &filep->f_pos);
		filp_close(filep, NULL);
		ret = 0;
	}

	set_fs(orgfs);
	pWorkInfo->rst = ret;
	complete(&pWorkInfo->completion);
}
/*----------------------------------------------------------------------------*/
static void sensor_read_work(struct work_struct *work)
{
	mm_segment_t orgfs;
	struct file *filep;
	int ret;
	struct work_info *pWorkInfo;

	orgfs = get_fs();
	set_fs(KERNEL_DS);

	pWorkInfo = container_of((struct delayed_work *)work, struct work_info, read_work);
	if (pWorkInfo == NULL) {
		MI_ERR("get pWorkInfo failed!");
		return;
	}

	filep = filp_open(OffsetFileName, O_RDONLY, 0600);
	if (IS_ERR(filep)) {
		MI_ERR("read, sys_open %s error!!.\n", OffsetFileName);
		set_fs(orgfs);
		ret =  -1;
	} else {
		filep->f_op->read(filep, pWorkInfo->buffer,  sizeof(pWorkInfo->buffer), &filep->f_pos);
		filp_close(filep, NULL);
		set_fs(orgfs);
		ret = 0;
	}

	pWorkInfo->rst = ret;
	complete(&(pWorkInfo->completion));
}
/*----------------------------------------------------------------------------*/
static int sensor_sync_read(u8 *offset)
{
	int	 err;
	int	 off[MIR3DA_OFFSET_LEN] = {0};
	struct work_info *pWorkInfo = &m_work_info;

	init_completion(&pWorkInfo->completion);
	queue_delayed_work(pWorkInfo->wq, &pWorkInfo->read_work, msecs_to_jiffies(0));
	err = wait_for_completion_timeout(&pWorkInfo->completion, msecs_to_jiffies(2000));
	if (err == 0) {
		MI_ERR("wait_for_completion_timeout TIMEOUT");
		return -1;
	}

	if (pWorkInfo->rst != 0) {
		MI_ERR("work_info.rst  not equal 0");
		return pWorkInfo->rst;
	}

	sscanf(m_work_info.buffer, "%x,%x,%x,%x,%x,%x,%x,%x,%x", &off[0], &off[1], &off[2],
			&off[3], &off[4], &off[5], &off[6], &off[7], &off[8]);

	offset[0] = (u8)off[0];
	offset[1] = (u8)off[1];
	offset[2] = (u8)off[2];
	offset[3] = (u8)off[3];
	offset[4] = (u8)off[4];
	offset[5] = (u8)off[5];
	offset[6] = (u8)off[6];
	offset[7] = (u8)off[7];
	offset[8] = (u8)off[8];

	return 0;
}
/*----------------------------------------------------------------------------*/
static int sensor_sync_write(u8 *off)
{
	int err = 0;
	struct work_info *pWorkInfo = &m_work_info;

	init_completion(&pWorkInfo->completion);

	sprintf(m_work_info.buffer, "%x,%x,%x,%x,%x,%x,%x,%x,%x\n", off[0], off[1], off[2],
			off[3], off[4], off[5], off[6], off[7], off[8]);

	pWorkInfo->len = sizeof(m_work_info.buffer);

	queue_delayed_work(pWorkInfo->wq, &pWorkInfo->write_work, msecs_to_jiffies(0));
	err = wait_for_completion_timeout(&pWorkInfo->completion, msecs_to_jiffies(2000));
	if (err == 0) {
		MI_ERR("wait_for_completion_timeout TIMEOUT");
		return -1;
	}

	if (pWorkInfo->rst != 0) {
		MI_ERR("work_info.rst  not equal 0");
		return pWorkInfo->rst;
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
static int check_califolder_exist(void)
{
	mm_segment_t	 orgfs;
	struct  file *filep;

	orgfs = get_fs();
	set_fs(KERNEL_DS);

	filep = filp_open(OffsetFolerName, O_RDONLY, 0600);
	if (IS_ERR(filep)) {
		MI_ERR("%s read, sys_open %s error!!.\n", __func__, OffsetFolerName);
		set_fs(orgfs);
		return 0;
	}

	filp_close(filep, NULL);
	set_fs(orgfs);

	return 1;
}
/*----------------------------------------------------------------------------*/
static int support_fast_auto_cali(void)
{
#if MIR3DA_SUPPORT_FAST_AUTO_CALI
	return 1;
#else
	return 0;
#endif
}
#endif
/*----------------------------------------------------------------------------*/
static int get_address(PLAT_HANDLE handle)
{
	if (NULL == handle) {
		MI_ERR("chip init failed !\n");
		return -1;
	}

	return ((struct i2c_client *)handle)->addr;
}
/*----------------------------------------------------------------------------*/
static void report_abs(void)
{
	short x = 0, y = 0, z = 0;
	MIR_HANDLE handle = mir_handle;

	if (mir3da_read_data(handle, &x, &y, &z) != 0) {
		MI_ERR("MIR3DA data read failed!\n");
		return;
	}

	input_report_abs(mir3da_idev, ABS_X, x);
	input_report_abs(mir3da_idev, ABS_Y, y);
	input_report_abs(mir3da_idev, ABS_Z, z);
	input_sync(mir3da_idev);
}
/*----------------------------------------------------------------------------*/
#if 0
static void mir3da_dev_poll(struct input_polled_dev *dev)
{
	dev->poll_interval = delayMs;
	report_abs();
}
#endif
/*----------------------------------------------------------------------------*/
static long mir3da_misc_ioctl(struct file *file,
		unsigned int cmd, unsigned long arg)
{
	void __user	 *argp = (void __user *)arg;
	int			 err = 0;
	int			 interval = 0;
	char			bEnable = 0;
	short		   xyz[3] = {0};
	MIR_HANDLE	  handle = mir_handle;

	if (_IOC_DIR(cmd) & _IOC_READ) {
		err = !access_ok((void __user *)arg, _IOC_SIZE(cmd));
	} else if (_IOC_DIR(cmd) & _IOC_WRITE) {
		err = !access_ok((void __user *)arg, _IOC_SIZE(cmd));
	}

	if (err) {
		return -EFAULT;
	}

	switch (cmd) {
	case MIR3DA_ACC_IOCTL_GET_DELAY:
		interval = POLL_INTERVAL;
		if (copy_to_user(argp, &interval, sizeof(interval)))
			return -EFAULT;
		break;

	case MIR3DA_ACC_IOCTL_SET_DELAY:
		if (copy_from_user(&interval, argp, sizeof(interval)))
			return -EFAULT;
		if (interval < 0 || interval > 1000) {
			MI_ERR("mir3da_misc_ioctl interval 大于1000\n");
			return -EINVAL;
		}
		//if((interval <=30)&&(interval > 10))
		//{
		//	interval = 10;
		//}

		if ((interval < 20)) {
			//if(interval <= 10) {
			//	delayMs = 3;
			//} else {
				delayMs = 8;
			//}
		} else {
			delayMs = interval;
		}
		MI_ERR("mir3da_misc_ioctl interval = %d, delayMs = %d\n", interval, delayMs);
		//delayMs = interval;
		break;

	case MIR3DA_ACC_IOCTL_SET_ENABLE:
		if (copy_from_user(&bEnable, argp, sizeof(bEnable)))
			return -EFAULT;

		err = mir3da_set_enable(handle, bEnable);
		if (err < 0)
			return EINVAL;
		break;

	case MIR3DA_ACC_IOCTL_GET_ENABLE:
		err = mir3da_get_enable(handle, &bEnable);
		if (err < 0) {
			return -EINVAL;
		}

		if (copy_to_user(argp, &bEnable, sizeof(bEnable)))
				return -EINVAL;
		break;

#if MIR3DA_OFFSET_TEMP_SOLUTION
	case MIR3DA_ACC_IOCTL_CALIBRATION:
		int z_dir = 0;
		if (copy_from_user(&z_dir, argp, sizeof(z_dir)))
			return -EFAULT;

		if (mir3da_calibrate(handle, z_dir)) {
			return -EFAULT;
		}

		if (copy_to_user(argp, &z_dir, sizeof(z_dir)))
			return -EFAULT;
		break;

	case MIR3DA_ACC_IOCTL_UPDATE_OFFSET:
		manual_load_cali_file(handle);
		break;
#endif

	case MIR3DA_ACC_IOCTL_GET_COOR_XYZ:

		if (mir3da_read_data(handle, &xyz[0], &xyz[1], &xyz[2]))
			return -EFAULT;

		if (copy_to_user((void __user *)arg, xyz, sizeof(xyz)))
			return -EFAULT;
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

/*----------------------------------------------------------------------------*/
static const struct file_operations mir3da_misc_fops = {
		.owner = THIS_MODULE,
		.unlocked_ioctl = mir3da_misc_ioctl,
};

static struct miscdevice misc_mir3da = {
		.minor = MISC_DYNAMIC_MINOR,
		.name = MIR3DA_MISC_NAME,
		.fops = &mir3da_misc_fops,
};
/*----------------------------------------------------------------------------*/
static ssize_t mir3da_enable_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	int			 ret;
	char			bEnable;
	MIR_HANDLE	  handle = mir_handle;

	ret = mir3da_get_enable(handle, &bEnable);
	if (ret < 0) {
		ret = -EINVAL;
	} else {
		ret = sprintf(buf, "%d\n", bEnable);
	}

	return ret;
}
/*----------------------------------------------------------------------------*/
static ssize_t mir3da_enable_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	int			 ret;
	char			bEnable;
	unsigned long   enable;
	MIR_HANDLE	  handle = mir_handle;

	if (buf == NULL) {
		return -1;
	}

	enable = simple_strtoul(buf, NULL, 10);
	bEnable = (enable > 0) ? 1 : 0;

	ret = mir3da_set_enable(handle, bEnable);
	if (ret < 0) {
		ret = -EINVAL;
	} else {
		ret = count;
	}

	if (bEnable) {
		myktime = ns_to_ktime(delayMs * NSEC_PER_MSEC);
		hrtimer_start(&hr_timer, myktime, HRTIMER_MODE_REL);
	} else {
		hrtimer_cancel(&hr_timer);
	}

	return ret;
}
/*----------------------------------------------------------------------------*/
static ssize_t mir3da_delay_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", delayMs);
}
/*----------------------------------------------------------------------------*/
static ssize_t mir3da_delay_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int interval = 0;

	interval = simple_strtoul(buf, NULL, 10);

	if (interval < 0 || interval > 1000) {
		MI_ERR("mir3da_delay_store interval 大于1000\n");
		return -EINVAL;
	}

	//if ((interval <=30)&&(interval > 10)){
	//	interval = 10;
	//}

	if ((interval < 20)) {
		//if(interval <= 10) {
		//	delayMs = 3;
		//} else {
			delayMs = 8;
		//}
	} else {
		delayMs = interval;
	}

	//delayMs = interval;

	MI_ERR("mir3da_delay_store interval = %d, delayMs = %d\n", interval, delayMs);


	myktime = ns_to_ktime(delayMs * NSEC_PER_MSEC);


	//myktime = ktime_set(0, delayMs* NSEC_PER_MSEC);
	hrtimer_start(&hr_timer, myktime, HRTIMER_MODE_REL);

	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t mir3da_axis_data_show(struct device *dev,
		   struct device_attribute *attr, char *buf)
{
	int result;
	short x, y, z;
	int count = 0;
	MIR_HANDLE	  handle = mir_handle;

	result = mir3da_read_data(handle, &x, &y, &z);
	if (result == 0)
		count += sprintf(buf+count, "x= %d;y=%d;z=%d\n", x, y, z);
	else
		count += sprintf(buf+count, "reading failed!");

	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t mir3da_reg_data_store(struct device *dev,
		   struct device_attribute *attr, const char *buf, size_t count)
{
	int				 addr, data;
	int				 result;
	MIR_HANDLE		  handle = mir_handle;

	sscanf(buf, "0x%x, 0x%x\n", &addr, &data);

	result = mir3da_register_write(handle, addr, data);

	MI_ASSERT(result == 0);

	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t mir3da_reg_data_show(struct device *dev,
		   struct device_attribute *attr, char *buf)
{
	MIR_HANDLE		  handle = mir_handle;

	return mir3da_get_reg_data(handle, buf);
}
/*----------------------------------------------------------------------------*/
#if 0
static ssize_t mir3da_offset_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;

	//if(bLoad==FILE_EXIST)
	//	count += sprintf(buf,"%s",m_work_info.buffer);
	//else
		count += sprintf(buf, "%s", "Calibration file not exist!\n");

	return count;
}
#endif
/*----------------------------------------------------------------------------*/
#if FILTER_AVERAGE_ENHANCE
static ssize_t mir3da_average_enhance_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	int							 ret = 0;
	struct mir3da_filter_param_s	param = {0};

	ret = mir3da_get_filter_param(&param);
	ret |= sprintf(buf, "%d %d %d\n", param.filter_param_l, param.filter_param_h, param.filter_threhold);

	return ret;
}
/*----------------------------------------------------------------------------*/
static ssize_t mir3da_average_enhance_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int							 ret = 0;
	struct mir3da_filter_param_s	param = {0};

	sscanf(buf, "%d %d %d\n", &param.filter_param_l, &param.filter_param_h, &param.filter_threhold);

	ret = mir3da_set_filter_param(&param);

	return count;
}
#endif
/*----------------------------------------------------------------------------*/
#if MIR3DA_OFFSET_TEMP_SOLUTION
int bCaliResult = -1;
static ssize_t mir3da_calibrate_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret;

	ret = sprintf(buf, "%d\n", bCaliResult);
	return ret;
}
/*----------------------------------------------------------------------------*/
static ssize_t mir3da_calibrate_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	s8			  z_dir = 0;
	MIR_HANDLE	  handle = mir_handle;

	z_dir = simple_strtol(buf, NULL, 10);
	bCaliResult = mir3da_calibrate(handle, z_dir);

	return count;
}
#endif
/*----------------------------------------------------------------------------*/
static ssize_t mir3da_log_level_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	int ret;

	ret = sprintf(buf, "%d\n", Log_level);

	return ret;
}
/*----------------------------------------------------------------------------*/
static ssize_t mir3da_log_level_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	Log_level = simple_strtoul(buf, NULL, 10);

	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t mir3da_primary_offset_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	MIR_HANDLE   handle = mir_handle;
	int x = 0, y = 0, z = 0;

	mir3da_get_primary_offset(handle, &x, &y, &z);

	return sprintf(buf, "x=%d ,y=%d ,z=%d\n", x, y, z);

}
/*----------------------------------------------------------------------------*/
static ssize_t mir3da_version_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{

	return sprintf(buf, "%s_%s\n", DRI_VER, CORE_VER);
}
/*----------------------------------------------------------------------------*/
static ssize_t mir3da_vendor_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", "MiraMEMS");
}
/*----------------------------------------------------------------------------*/
static DEVICE_ATTR(enable,		  0644,  mir3da_enable_show,			 mir3da_enable_store);
static DEVICE_ATTR(delay,	  0644,  mir3da_delay_show,			  mir3da_delay_store);
static DEVICE_ATTR(axis_data,	   S_IRUGO |S_IWUSR | S_IWGRP,	mir3da_axis_data_show,		  NULL);
static DEVICE_ATTR(reg_data,		S_IWUSR | S_IWGRP | S_IRUGO,  mir3da_reg_data_show,		   mir3da_reg_data_store);
static DEVICE_ATTR(log_level,	   S_IWUSR | S_IWGRP | S_IRUGO,  mir3da_log_level_show,		  mir3da_log_level_store);
#if MIR3DA_OFFSET_TEMP_SOLUTION
static DEVICE_ATTR(offset,		  S_IWUSR | S_IWGRP | S_IRUGO,  mir3da_offset_show,			 NULL);
static DEVICE_ATTR(calibrate_miraGSensor,	   S_IWUSR | S_IWGRP | S_IRUGO,  mir3da_calibrate_show,		  mir3da_calibrate_store);
#endif
#if FILTER_AVERAGE_ENHANCE
static DEVICE_ATTR(average_enhance, S_IWUSR | S_IWGRP | S_IRUGO,  mir3da_average_enhance_show,	mir3da_average_enhance_store);
#endif
static DEVICE_ATTR(primary_offset,  S_IWUSR | S_IWGRP,			mir3da_primary_offset_show,			NULL);
static DEVICE_ATTR(version,		 S_IRUGO,			mir3da_version_show,			NULL);
static DEVICE_ATTR(vendor,		  S_IRUGO,			mir3da_vendor_show,			 NULL);
/*----------------------------------------------------------------------------*/
static struct attribute *mir3da_attributes[] = {
	&dev_attr_enable.attr,
	&dev_attr_delay.attr,
	&dev_attr_axis_data.attr,
	&dev_attr_reg_data.attr,
	&dev_attr_log_level.attr,
#if MIR3DA_OFFSET_TEMP_SOLUTION
	&dev_attr_offset.attr,
	&dev_attr_calibrate_miraGSensor.attr,
#endif
#if FILTER_AVERAGE_ENHANCE
	&dev_attr_average_enhance.attr,
#endif /* ! FILTER_AVERAGE_ENHANCE */
	&dev_attr_primary_offset.attr,
	&dev_attr_version.attr,
	&dev_attr_vendor.attr,
	NULL
};

static const struct attribute_group mir3da_attr_group = {
	.attrs  = mir3da_attributes,
};
/*----------------------------------------------------------------------------*/
int i2c_smbus_read(PLAT_HANDLE handle, u8 addr, u8 *data)
{
	int				 res = 0;
	struct i2c_client   *client = (struct i2c_client *)handle;

	*data = i2c_smbus_read_byte_data(client, addr);

	return res;
}
/*----------------------------------------------------------------------------*/
int i2c_smbus_read_block(PLAT_HANDLE handle, u8 addr, u8 count, u8 *data)
{
	int				 res = 0;
	struct i2c_client   *client = (struct i2c_client *)handle;

	res = i2c_smbus_read_i2c_block_data(client, addr, count, data);

	return res;
}
/*----------------------------------------------------------------------------*/
int i2c_smbus_write(PLAT_HANDLE handle, u8 addr, u8 data)
{
	int				 res = 0;
	struct i2c_client   *client = (struct i2c_client *)handle;

	res = i2c_smbus_write_byte_data(client, addr, data);

	return res;
}
/*----------------------------------------------------------------------------*/
void msdelay(int ms)
{
	mdelay(ms);
}

static void wq_func_hrtimer(struct work_struct *work)
{
	report_abs();
}

static enum hrtimer_restart my_hrtimer_callback(struct hrtimer *timer)
{
	schedule_work(&wq_hrtimer);
	hrtimer_forward_now(&hr_timer, myktime);
	return HRTIMER_RESTART;
}

#if MIR3DA_OFFSET_TEMP_SOLUTION
MIR_GENERAL_OPS_DECLARE(ops_handle, i2c_smbus_read, i2c_smbus_read_block, i2c_smbus_write,
		sensor_sync_write, sensor_sync_read, check_califolder_exist, get_address,
		support_fast_auto_cali, msdelay, sprintf);
#else
MIR_GENERAL_OPS_DECLARE(ops_handle, i2c_smbus_read, i2c_smbus_read_block, i2c_smbus_write,
		NULL, NULL, NULL, get_address, NULL, msdelay, sprintf);
#endif
/*----------------------------------------------------------------------------*/
static int mir3da_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int	result = -1;
	struct input_dev	*idev;

	if (gsensor_info.dev == NULL)
		gsensor_info.dev = &client->dev;

	if (mir3da_install_general_ops(&ops_handle)) {
		MI_ERR("Install ops failed !\n");
		goto err_detach_client;
	}

#if MIR3DA_OFFSET_TEMP_SOLUTION
	m_work_info.wq = create_singlethread_workqueue("oo");
	if (NULL == m_work_info.wq) {
		MI_ERR("Failed to create workqueue !");
		goto err_detach_client;
	}

	INIT_DELAYED_WORK(&m_work_info.read_work, sensor_read_work);
	INIT_DELAYED_WORK(&m_work_info.write_work, sensor_write_work);
#endif

	/* Initialize the MIR3DA chip */
	mir_handle = mir3da_core_init((PLAT_HANDLE)client);
	if (NULL == mir_handle) {
		MI_ERR("chip init failed !\n");
		//input_set_power_enable(&(gsensor_info.input_type), 0);
		result = -ENODEV;
		goto err_detach_client;
	}

	//hwmon_dev = hwmon_device_register(&client->dev);
	//MI_ASSERT(!(IS_ERR(hwmon_dev)));

	/* input poll device register */
	//mir3da_idev = input_allocate_polled_device();
	mir3da_idev = input_allocate_device();
	if (!mir3da_idev) {
		MI_ERR("alloc poll device failed!\n");
		result = -ENOMEM;
		goto err_hwmon_device_unregister;
	}
	//mir3da_idev->poll = mir3da_dev_poll;
	//mir3da_idev->poll_interval = POLL_INTERVAL;
	delayMs = POLL_INTERVAL;
	//mir3da_idev->poll_interval_max = POLL_INTERVAL_MAX;
	idev = mir3da_idev;

	idev->name = MIR3DA_INPUT_DEV_NAME;
	idev->id.bustype = BUS_I2C;
	idev->evbit[0] = BIT_MASK(EV_ABS);

	input_set_abs_params(idev, ABS_X, -16384, 16383, INPUT_FUZZ, INPUT_FLAT);
	input_set_abs_params(idev, ABS_Y, -16384, 16383, INPUT_FUZZ, INPUT_FLAT);
	input_set_abs_params(idev, ABS_Z, -16384, 16383, INPUT_FUZZ, INPUT_FLAT);

	//result = input_register_polled_device(mir3da_idev);
	result = input_register_device(mir3da_idev);
	if (result) {
		MI_ERR("register poll device failed!\n");
		goto err_free_polled_device;
	}

	/* Sys Attribute Register */
	result = sysfs_create_group(&idev->dev.kobj, &mir3da_attr_group);
	if (result) {
		MI_ERR("create device file failed!\n");
		result = -EINVAL;
		goto err_unregister_polled_device;
	}

	kobject_uevent(&idev->dev.kobj, KOBJ_CHANGE);

	/* Misc device interface Register */
	result = misc_register(&misc_mir3da);
	if (result) {
		MI_ERR("%s: mir3da_dev register failed", __func__);
		goto err_remove_sysfs_group;
	}


	//start timer
	hrtimer_init(&hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hr_timer.function = my_hrtimer_callback;
	myktime = ns_to_ktime(POLL_INTERVAL * NSEC_PER_MSEC);
	INIT_WORK(&wq_hrtimer, wq_func_hrtimer);

	return result;

err_remove_sysfs_group:
	sysfs_remove_group(&idev->dev.kobj, &mir3da_attr_group);
err_unregister_polled_device:
	//input_unregister_polled_device(mir3da_idev);
	input_unregister_device(mir3da_idev);
err_free_polled_device:
	//input_free_polled_device(mir3da_idev);
	input_free_device(mir3da_idev);
err_hwmon_device_unregister:
	//hwmon_device_unregister(&client->dev);
err_detach_client:
	is_probe_success = result;
	return result;
}
/*----------------------------------------------------------------------------*/
static int mir3da_remove(struct i2c_client *client)
{
	MIR_HANDLE	  handle = mir_handle;

	mir3da_set_enable(handle, 0);

	misc_deregister(&misc_mir3da);

	sysfs_remove_group(&mir3da_idev->dev.kobj, &mir3da_attr_group);

	//input_unregister_polled_device(mir3da_idev);
	input_unregister_device(mir3da_idev);

	//input_free_polled_device(mir3da_idev);
#if MIR3DA_OFFSET_TEMP_SOLUTION
	flush_workqueue(m_work_info.wq);

	destroy_workqueue(m_work_info.wq);
#endif
	//hwmon_device_unregister(hwmon_dev);

	return 0;
}
/*----------------------------------------------------------------------------*/
static int mir3da_suspend(struct device *dev)
{
	int result = 0;
	MIR_HANDLE	  handle = mir_handle;

	MI_FUN;

	result = mir3da_set_enable(handle, 0);
	if (result) {
		MI_ERR("%s:set disable fail!!\n", __func__);
		return result;
	}
	//mir3da_idev->close(mir3da_idev);

	input_set_power_enable(&(gsensor_info.input_type), 0);
	return result;
}
/*----------------------------------------------------------------------------*/
static int mir3da_resume(struct device *dev)
{
	int result = 0;
	MIR_HANDLE	  handle = mir_handle;

	MI_FUN;
	input_set_power_enable(&(gsensor_info.input_type), 1);
	result = mir3da_chip_resume(handle);
	if (result) {
		MI_ERR("%s:chip resume fail!!\n", __func__);
		return result;
	}

	result = mir3da_set_enable(handle, 1);
	if (result) {
		MI_ERR("%s:set enable fail!!\n", __func__);
		return result;
	}

	//mir3da_idev->open(mir3da_idev);

	return result;
}

#if IS_ENABLED(CONFIG_PM)
static UNIVERSAL_DEV_PM_OPS(mir3da_pm_ops, mir3da_suspend,
		mir3da_resume, NULL);
#endif
/*----------------------------------------------------------------------------*/
static int mir3da_detect(struct i2c_client *new_client,
			   struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = new_client->adapter;

	MI_MSG("%s:bus[%d] addr[0x%x]\n", __func__, adapter->nr, new_client->addr);

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		//input_set_power_enable(&(gsensor_info.input_type), 0);
		return -ENODEV;
	}

	if (twi_id == adapter->nr) {
		if (mir3da_install_general_ops(&ops_handle)) {
			MI_ERR("Install ops failed !\n");
			return -ENODEV;
		}

		if (mir3da_module_detect((PLAT_HANDLE)new_client)) {
			MI_ERR("%s: Can't find Mir3da gsensor!!", __func__);
			input_set_power_enable(&(gsensor_info.input_type), 0);
		} else {
			MI_ERR("'Find Mir3da gsensor!!");
			strlcpy(info->type, MIR3DA_DRV_NAME, I2C_NAME_SIZE);
			return 0;
		}
	}
	return  -ENODEV;
}
/*----------------------------------------------------------------------------*/


static int mir3da_startup(void)
{
	int ret = -1;

	printk("function=%s=========LINE=%d. \n", __func__, __LINE__);
#ifdef MIR3DA_DEVICE_NAME
	printk("mir3da use sepical device node name = %s\n", MIR3DA_DEVICE_NAME);
	gsensor_info.node = of_find_node_by_name(NULL, MIR3DA_DEVICE_NAME);
#endif

	if (input_sensor_startup(&(gsensor_info.input_type))) {
		printk("%s: err.\n", __func__);
		return -1;
	} else {
		ret = input_sensor_init(&(gsensor_info.input_type));
	}
	if (0 != ret) {
		printk("%s:gsensor.init_platform_resource err. \n", __func__);
	}

	twi_id = gsensor_info.twi_id;
	input_set_power_enable(&(gsensor_info.input_type), 1);
	return 0;
}

/*----------------------------------------------------------------------------*/
static const struct of_device_id mir3da_of_match[] = {
	{.compatible = "allwinner,mir3da"},
	{},
};

static const struct i2c_device_id mir3da_id[] = {
	{MIR3DA_DRV_NAME, 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, mir3da_id);
/*----------------------------------------------------------------------------*/
static struct i2c_driver mir3da_driver = {
	.class		= I2C_CLASS_HWMON,
	.driver = {
		.name	= MIR3DA_DRV_NAME,
		.owner	= THIS_MODULE,
		.pm = &mir3da_pm_ops,
		.of_match_table = mir3da_of_match,
	},

	.probe		= mir3da_probe,
	.remove		= mir3da_remove,
	.id_table 	= mir3da_id,
	.address_list	= normal_i2c,
};
/*----------------------------------------------------------------------------*/
static int __init mir3da_init(void)
{
	int res;

	MI_FUN;

	if (mir3da_startup() != 0)
		return -1;

	if (!gsensor_info.isI2CClient)
		mir3da_driver.detect = mir3da_detect;
	res = i2c_add_driver(&mir3da_driver);
	if (res < 0 || is_probe_success < 0) {
		MI_ERR("add mir3da i2c driver failed\n");
		i2c_del_driver(&mir3da_driver);
	//	input_set_power_enable(&(gsensor_info.input_type), 0);
		input_sensor_free(&(gsensor_info.input_type));
		return -ENODEV;
	}

	return (res);
}
/*----------------------------------------------------------------------------*/
static void __exit mir3da_exit(void)
{
	MI_FUN;

	i2c_del_driver(&mir3da_driver);
	//input_set_power_enable(&(gsensor_info.input_type), 0);
	input_sensor_free(&(gsensor_info.input_type));
}
/*----------------------------------------------------------------------------*/
MODULE_AUTHOR("MiraMEMS <lschen@miramems.com>");
MODULE_DESCRIPTION("MIR3DA 3-Axis Accelerometer driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");

module_init(mir3da_init);
module_exit(mir3da_exit);
