/*
 * SUNXI MTD partitioning
 *
 * Copyright © 2016 WimHuang <huangwei@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#define pr_fmt(fmt)	"sunxipart: " fmt

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include "efi.h"

#if IS_ENABLED(CONFIG_ARCH_SUN8IW16) || IS_ENABLED(CONFIG_ARCH_SUN8IW18) || \
		IS_ENABLED(CONFIG_ARCH_SUN8IW19) || IS_ENABLED(CONFIG_UBOOT_DISP_ENABLE)
#define MBR_OFFSET		((1024 - 16) * 1024)
#else
#define MBR_OFFSET		((512 - 16) * 1024)
#endif
#define MBR_SIZE		(16 * 1024)
#define DL_SIZE			(16 * 1024)
#define MBR_MAGIC		"softw411"
#define MBR_MAX_PART_COUNT	120
#define MBR_RESERVED		(MBR_SIZE - 32 - (MBR_MAX_PART_COUNT * sizeof(struct sunxi_partition)))
#define NOR_BLK_SIZE		512

/* partition information */
struct sunxi_partition {
	unsigned  int		addrhi;
	unsigned  int		addrlo;
	unsigned  int		lenhi;
	unsigned  int		lenlo;
	unsigned  char		classname[16];
	unsigned  char		name[16];
	unsigned  int		user_type;
	unsigned  int		keydata;
	unsigned  int		ro;
	unsigned  char		reserved[68];
} __packed;

/* mbr information */
struct sunxi_mbr {
	unsigned  int		crc32;
	unsigned  int		version;
	unsigned  char		magic[8];
	unsigned  int		copy;
	unsigned  int		index;
	unsigned  int		PartCount;
	unsigned  int		stamp[1];
	struct sunxi_partition	array[MBR_MAX_PART_COUNT];
	unsigned  char		res[MBR_RESERVED];
} __packed;

/* save partition's name */
static char partition_name[MBR_MAX_PART_COUNT][16];

/* to get mbr_offset from cmdline */
static int cmdline_mbr_offset = -1;

static int __init get_mbr_offset_from_cmdline(char *p)
{

	pr_debug("%s(%s)\n", __func__, p);
	if (p == NULL) {
		cmdline_mbr_offset = -1;
	} else {
		cmdline_mbr_offset = memparse(p, &p);

		if (cmdline_mbr_offset < 0)
			cmdline_mbr_offset = -1;
	}
	return 0;
}

early_param("mbr_offset", get_mbr_offset_from_cmdline);

static void sunxipart_add_part(struct mtd_partition *part, char *name,
				uint64_t size, uint64_t offset)
{
	part->name = name;
	part->size = size;
	part->offset = offset;
}

static int sunxipart_parse(struct mtd_info *master,
				const struct mtd_partition **pparts,
				struct mtd_part_parser_data *data)
{
	int i, ret, nrparts, parts_size;
	size_t bytes_read;
	struct sunxi_mbr *sunxi_mbr;
	struct mtd_partition *parts;
	int mbr_offset;

	printk("%s ... %d \n", __func__, __LINE__);
	sunxi_mbr = kzalloc(MBR_SIZE, GFP_KERNEL);
	if (sunxi_mbr == NULL) {
		pr_err("failed to alloc sunxi_mbr\n");
		return -ENOMEM;
	}

	if (cmdline_mbr_offset > 0)
		mbr_offset =  cmdline_mbr_offset;
	else
		mbr_offset = MBR_OFFSET;

	ret = mtd_read(master, mbr_offset, MBR_SIZE,
		       &bytes_read, (uint8_t *)sunxi_mbr);
	if ((ret < 0)) {
		pr_err("failed to read sunxi_mbr!\n");
		kfree(sunxi_mbr);
		return -EIO;
	}

	if (memcmp(sunxi_mbr->magic, MBR_MAGIC, strlen(MBR_MAGIC)) == 0) {
		if ((sunxi_mbr->PartCount == 0)
		     || (sunxi_mbr->PartCount > MBR_MAX_PART_COUNT)) {
			pr_err("failed to parse sunxi_mbr)!\n");
			kfree(sunxi_mbr);
			return -EINVAL;
		}

		nrparts = sunxi_mbr->PartCount + 1;
		parts_size = nrparts * sizeof(*parts);
		parts = kzalloc(parts_size, GFP_KERNEL);
		if (parts == NULL) {
			pr_err("failed to alloc %d patitions\n", nrparts);
			kfree(sunxi_mbr);
			return -ENOMEM;
		}

		strncpy(partition_name[0], "uboot", 16);
		sunxipart_add_part(&parts[0], partition_name[0],
						mbr_offset + MBR_SIZE, 0);
		for (i = 1; i < nrparts; i++) {
			strncpy(partition_name[i], sunxi_mbr->array[i - 1].name,
				16);

			sunxipart_add_part(
			    &parts[i], partition_name[i],
			    sunxi_mbr->array[i - 1].lenlo * NOR_BLK_SIZE,
			    sunxi_mbr->array[i - 1].addrlo * NOR_BLK_SIZE +
				mbr_offset);
		}
	} else {
		struct _gpt_header *gpt_head =
		    (struct _gpt_header *)((char *)sunxi_mbr + 512);
		struct _gpt_entry *entry =
		    (struct _gpt_entry *)((char *)sunxi_mbr + 1024);
		unsigned int j = 0;

		if (gpt_head->signature != GPT_HEADER_SIGNATURE) {
			pr_err("failed to parse sunxi_gpt!\n");
			kfree(sunxi_mbr);
			return -EINVAL;
		}
		nrparts = gpt_head->num_partition_entries + 1;
		parts_size = nrparts * sizeof(*parts);
		parts = kzalloc(parts_size, GFP_KERNEL);
		if (parts == NULL) {
			pr_err("failed to alloc %d patitions\n", nrparts);
			kfree(sunxi_mbr);
			return -ENOMEM;
		}

		strncpy(partition_name[0], "uboot", 16);
		sunxipart_add_part(&parts[0], partition_name[0],
				   mbr_offset + MBR_SIZE, 0);
		for (i = 1; i < nrparts; i++) {
			for (j = 0; j < 16; j++)
				partition_name[i][j] =
				    (char)(entry[i - 1].partition_name[j]);

			pr_debug("GPT:%-12s: %-12llx  %-12llx\n",
				 partition_name[i], entry[i - 1].starting_lba,
				 entry[i - 1].ending_lba);

			sunxipart_add_part(&parts[i], partition_name[i],
					   (entry[i - 1].ending_lba -
					    entry[i - 1].starting_lba + 1) *
					       NOR_BLK_SIZE,
					   entry[i - 1].starting_lba *
						   NOR_BLK_SIZE +
					       mbr_offset);
		}
	}

	kfree(sunxi_mbr);
	*pparts = parts;
	return nrparts;
}


static struct mtd_part_parser sunxipart_mtd_parser = {
	.owner = THIS_MODULE,
	.parse_fn = sunxipart_parse,
	.name = "sunxipart",
};

static int __init sunxipart_init(void)
{
	register_mtd_parser(&sunxipart_mtd_parser);

	return 0;
}

static void __exit sunxipart_exit(void)
{
	deregister_mtd_parser(&sunxipart_mtd_parser);
}

fs_initcall_sync(sunxipart_init);
module_exit(sunxipart_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MTD partitioning for SUNXI flash memories");
