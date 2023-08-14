/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * Allwinner mm Vendor Hooks
 *
 * Copyright (C) 2022 Allwinner.
 */

#define pr_fmt(fmt) "sunxi_mm: " fmt

#include <linux/module.h>
#include <linux/types.h>
#include <trace/hooks/vmscan.h>
#include <linux/swap.h>
#include <linux/proc_fs.h>

static int g_direct_swappiness = 60;
static int g_swappiness = 160;

#define PARA_BUF_LEN 128
static struct proc_dir_entry *para_entry;

static void sunxi_set_swappiness(void *data, int *swappiness)
{
	if (!current_is_kswapd())
		*swappiness = g_direct_swappiness;

	return;
}

static void sunxi_set_inactive_ratio(void *data, unsigned long *inactive_ratio, int file)
{
	if (file)
		*inactive_ratio = min(2UL, *inactive_ratio);
	else
		*inactive_ratio = 1;

	return;
}

static void sunxi_set_balance_anon_file_reclaim(void *data, bool *balance_anon_file_reclaim)
{
	*balance_anon_file_reclaim = true;

	return;
}

static int register_sunxi_mm_vendor_hooks(void)
{
	int ret = 0;

	ret = register_trace_android_vh_tune_swappiness(sunxi_set_swappiness, NULL);
	if (ret != 0) {
		pr_err("register_trace_android_vh_set_swappiness failed! ret=%d\n", ret);
		goto out;
	}

	ret = register_trace_android_vh_tune_inactive_ratio(sunxi_set_inactive_ratio, NULL);
	if (ret != 0) {
		pr_err("register_trace_android_vh_tune_inactive_ratio failed! ret=%d\n", ret);
		goto out;
	}

	ret = register_trace_android_rvh_set_balance_anon_file_reclaim(
						sunxi_set_balance_anon_file_reclaim, NULL);
	if (ret != 0) {
		pr_err("register_trace_android_rvh_set_balance_anon_file_reclaim failed! ret=%d\n", ret);
		goto out;
	}

out:
	return ret;
}

static void unregister_sunxi_mm_vendor_hooks(void)
{
	unregister_trace_android_vh_tune_swappiness(sunxi_set_swappiness, NULL);
	unregister_trace_android_vh_tune_inactive_ratio(sunxi_set_inactive_ratio, NULL);

	return;
}

static ssize_t direct_swappiness_write(struct file *file,
		const char __user *buff, size_t len, loff_t *ppos)
{
	char kbuf[PARA_BUF_LEN] = {'\0'};
	char *str;
	long val;
	int ret = 0;

	if (len > PARA_BUF_LEN - 1) {
		pr_err("len %ld is too long\n", len);
		return -EINVAL;
	}

	if (copy_from_user(&kbuf, buff, len))
		return -EFAULT;
	kbuf[len] = '\0';

	str = strstrip(kbuf);
	if (!str) {
		pr_err("buff %s is invalid\n", kbuf);
		return -EINVAL;
	}

	ret = kstrtoul(str, 0, &val);
	if (ret)
		return -EINVAL;
	else
		g_direct_swappiness = val;

	return len;
}

static ssize_t direct_swappiness_read(struct file *file,
		char __user *buffer, size_t count, loff_t *off)
{
	char kbuf[PARA_BUF_LEN] = {'\0'};
	int len;

	len = snprintf(kbuf, PARA_BUF_LEN, "%d\n", g_direct_swappiness);
	if (len == PARA_BUF_LEN)
		kbuf[len - 1] = '\0';

	if (len > *off)
		len -= *off;
	else
		len = 0;

	if (copy_to_user(buffer, kbuf + *off, (len < count ? len : count)))
		return -EFAULT;

	*off += (len < count ? len : count);
	return (len < count ? len : count);
}

static const struct proc_ops proc_direct_swappiness_para_ops = {
	.proc_write          = direct_swappiness_write,
	.proc_read		= direct_swappiness_read,
};

static int __init create_sunxi_vm_proc(void)
{
	struct proc_dir_entry *root_dir_entry = proc_mkdir("sunxi_vm", NULL);

	para_entry = proc_create((root_dir_entry ?
				"direct_swappiness" : "sunxi_vm/direct_swappiness"),
			S_IRUSR|S_IWUSR, root_dir_entry, &proc_direct_swappiness_para_ops);

	if (para_entry) {
		printk("Register swappiness_para interface passed.\n");
		return 0;
	}

	pr_err("Register swappiness_para interface failed.\n");
	return -ENOMEM;
}

static void destroy_sunxi_vm_proc(void)
{
	proc_remove(para_entry);
	para_entry = NULL;
}


static int __init sunxi_mm_init(void)
{
	int ret = 0;

	ret = create_sunxi_vm_proc();
	if (ret)
		return ret;

	ret = register_sunxi_mm_vendor_hooks();
	if (ret != 0) {
		destroy_sunxi_vm_proc();
		return ret;
	}

	return 0;
}

static void __exit sunxi_mm_exit(void)
{
	unregister_sunxi_mm_vendor_hooks();
	destroy_sunxi_vm_proc();

	pr_info("sunxi_mm_exit succeed!\n");

	return;
}

module_init(sunxi_mm_init);
module_exit(sunxi_mm_exit);

module_param_named(vm_swappiness, g_swappiness, int, S_IRUGO | S_IWUSR);
module_param_named(direct_vm_swappiness, g_direct_swappiness, int, S_IRUGO | S_IWUSR);
MODULE_LICENSE("GPL v2");
