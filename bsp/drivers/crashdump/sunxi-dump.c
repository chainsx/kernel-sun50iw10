// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * drivers/crashdump/sunxi-dump.c
 *
 * Copyright(c) 2019-2020 Allwinnertech Co., Ltd.
 *         http://www.allwinnertech.com
 *
 * Allwinner sunxi crash dump debug
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/kmemleak.h>
#include <asm/cacheflush.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
#include <linux/panic_notifier.h>
#endif
#include "sunxi-sip.h"
#include "sunxi-smc.h"
#include "sunxi-dump.h"

#define SUNXI_DUMP_COMPATIBLE "sunxi-dump"
static LIST_HEAD(dump_group_list);
static int sunxi_dump = 1;

/*
 * Sometimes panic can't stop several CPUs unexpectedly, we'd better wait
 * for them offline in a period of time rather than forever.
 */
#define SUNXI_DUMP_CPU_ONLINE_TIMEOUT (1000 * 30)

struct sunxi_dump_group {
	char name[10];
	u32 *reg_buf;
	void __iomem *vir_base;
	phys_addr_t phy_base;
	u32 len;
	struct list_head list;
};

int sunxi_dump_group_reg(struct sunxi_dump_group *group)
{
	u32 *buf = group->reg_buf;
	void __iomem *membase = group->vir_base;
	u32 len = ALIGN(group->len, 4);
	int i;

	for (i = 0; i < len; i += 4)
		*(buf++) = readl(membase + i);

	return 0;
}

int sunxi_dump_group_dump(void)
{
	struct sunxi_dump_group *dump_group;

	list_for_each_entry(dump_group, &dump_group_list, list) {
		sunxi_dump_group_reg(dump_group);
	}
	return 0;
}
EXPORT_SYMBOL(sunxi_dump_group_dump);

int sunxi_dump_group_register(const char *name, phys_addr_t start, u32 len)
{
	struct sunxi_dump_group *dump_group = NULL;

	dump_group = kmalloc(sizeof(struct sunxi_dump_group), GFP_KERNEL);
	if (!dump_group)
		return -ENOMEM;

	memcpy(dump_group->name, name, sizeof(dump_group->name));
	dump_group->phy_base = start;
	dump_group->len = len;
	dump_group->vir_base = ioremap(dump_group->phy_base, dump_group->len);
	if (!dump_group->vir_base) {
		pr_err("%s can't iomap\n", dump_group->name);
		return -EINVAL;
	}
	dump_group->reg_buf = kmalloc(dump_group->len, GFP_KERNEL);
	if (!dump_group->reg_buf)
		return -ENOMEM;

	list_add_tail(&dump_group->list, &dump_group_list);
	return 0;
}

static int sunxi_dump_panic_event(struct notifier_block *self, unsigned long val, void *reason)
{
	unsigned int i, online_time;
	unsigned long __maybe_unused offset;

	if (!sunxi_dump) {
		pr_emerg("crashdump disabled\n");
		return NOTIFY_DONE;
	}

	flush_cache_all();
	mdelay(1000);

	sunxi_dump_group_dump();

	for (i = 0; i < num_possible_cpus(); i++) {
		if (i == smp_processor_id())
			continue;

		/*
		 * Notice: record the online time of per cpu,
		 * ignore those more than 30 seconds.
		 */
		online_time = 0;
		while (1) {
			if (!cpu_online(i))
				break;
			mdelay(10);
			online_time += 10;
			if (online_time >= SUNXI_DUMP_CPU_ONLINE_TIMEOUT)
				break;
		}
	}

	/* Notice: make sure to print the full stack trace */
	mdelay(5000);

	pr_emerg("crashdump enter\n");
	/* Support to provide debug information for arm64 */
#if IS_ENABLED(CONFIG_ARM64)
	offset = kaslr_offset();
	pr_emerg("kimage_voffset: 0x%llx, kaslr: 0x%lx\n", kimage_voffset, offset);
#endif
	sunxi_set_crashdump_mode();

	return NOTIFY_DONE;
}

static struct notifier_block sunxi_dump_panic_event_nb = {
	.notifier_call = sunxi_dump_panic_event,
	// .priority = INT_MAX,
};

static struct ctl_table sunxi_dump_sysctl_table[] = {
	{
		.procname = "sunxi_dump",
		.data = &sunxi_dump,
		.maxlen = sizeof(sunxi_dump),
		.mode = 0644,
		.proc_handler = proc_dointvec,
	},
	{ }
};

static struct ctl_table sunxi_dump_sysctl_root[] = {
	{
		.procname = "kernel",
		.mode = 0555,
		.child = sunxi_dump_sysctl_table,
	},
	{ }
};

static int __init sunxi_dump_init(void)
{
	struct device_node *node;
	int i = 0;
	const char *name = NULL;
	struct resource res;
	struct ctl_table_header *hdr;

	node = of_find_compatible_node(NULL, NULL, SUNXI_DUMP_COMPATIBLE);

	for (i = 0; ; i++) {
		if (of_address_to_resource(node, i, &res))
			break;
		if (of_property_read_string_index(node, "group-names", i, &name))
			break;
		sunxi_dump_group_register(name, res.start, resource_size(&res));
	}

	/* register sunxi dump sysctl */
	hdr = register_sysctl_table(sunxi_dump_sysctl_root);
	kmemleak_not_leak(hdr);

	/* register sunxi dump panic notifier */
	atomic_notifier_chain_register(&panic_notifier_list, &sunxi_dump_panic_event_nb);

	return 0;
}

int sunxi_set_crashdump_mode(void)
{
#if IS_ENABLED(CONFIG_ARM64)
	invoke_scp_fn_smc(ARM_SVC_SUNXI_CRASHDUMP_START, 0, 0, 0);
#endif
#if IS_ENABLED(CONFIG_ARM)
	sunxi_optee_call_crashdump();
#endif
	while (1)
		cpu_relax();
}
late_initcall(sunxi_dump_init);

MODULE_AUTHOR("kanghoupeng<kanghoupeng@allwinnertech.com>");
MODULE_DESCRIPTION("sunxi crash dump debug");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.0");
