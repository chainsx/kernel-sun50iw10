/* SPDX-License-Identifier: GPL-2.0 */
/*
 * allwinner socs boot information.
 *
 * copyright (c) 2019 allwinner.
 *
 * this file is licensed under the terms of the gnu general public
 * license version 2.  this program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#define to_delete 0
#ifndef __SUNXI_BOOT_H__
#define __SUNXI_BOOT_H__

#include <linux/types.h>

enum size_type {
	BYTE,
	SECTOR,
	PAGE,
	BLOCK
};


#define SBROM_TOC0_HEAD_SPACE  (0x80)
#define NDFC_PAGE_TAB_MAGIC "BT0.NTAB"
#define NDFC_PAGE_TAB_HEAD_SIZE (64)
#define STAMP_VALUE 0x5F0A6C39
#define NDFC_RR_TAB_MAGIC "BT0.RRTB"
#define NDFC_DMY_TAB_MAGIC "BT0.DMTB" /* dummy table */


typedef struct {
	__u8        ChipCnt;                 /* the count of the total nand flash chips are currently connecting on the CE pin */
	__u8        ConnectMode;             /* the rb connect  mode */
	__u8        BankCntPerChip;          /* the count of the banks in one nand chip, multiple banks can support Inter-Leave */
	__u8        DieCntPerChip;           /* the count of the dies in one nand chip, block management is based on Die */
	__u8        PlaneCntPerDie;          /* the count of planes in one die, multiple planes can support multi-plane operation */
	__u8        SectorCntPerPage;        /* the count of sectors in one single physic page, one sector is 0.5k */
	__u16       ChipConnectInfo;         /* chip connect information, bit == 1 means there is a chip connecting on the CE pin */
	__u32       PageCntPerPhyBlk;        /* the count of physic pages in one physic block */
	__u32       BlkCntPerDie;            /* the count of the physic blocks in one die, include valid block and invalid block */
	__u32       OperationOpt;            /* the mask of the operation types which current nand flash can support support */
	__u32       FrequencePar;            /* the parameter of the hardware access clock, based on 'MHz' */
	__u32       SpiMode;                 /* spi nand mode, 0:mode 0, 3:mode 3 */
	__u8        NandChipId[8];           /* the nand chip id of current connecting nand chip */
	__u32       pagewithbadflag;         /* bad block flag was written at the first byte of spare area of this page */
	__u32       MultiPlaneBlockOffset;   /* the value of the block number offset between the two plane block */
	__u32       MaxEraseTimes;           /* the max erase times of a physic block */
	__u32       MaxEccBits;              /* the max ecc bits that nand support */
	__u32       EccLimitBits;            /* the ecc limit flag for tne nand */
	__u32       uboot_start_block;
	__u32       uboot_next_block;
	__u32       logic_start_block;
	__u32       nand_specialinfo_page;
	__u32       nand_specialinfo_offset;
	__u32       physic_block_reserved;
	__u32       Reserved[4];
} boot_spinand_para_t;

typedef struct {
	unsigned int ChannelCnt;
	/* count of total nand chips are currently connecting on the CE pin */
	unsigned int ChipCnt;
	/* chip connect info, bit=1 means one chip connecting on the CE pin */
	unsigned int ChipConnectInfo;
	unsigned int RbCnt;
	/* connect info of all rb  chips are connected */
	unsigned int RbConnectInfo;
	unsigned int RbConnectMode;	/* rb connect mode */
	/* count of banks in one nand chip, multi banks can support Inter-Leave */
	unsigned int BankCntPerChip;
	/* count of dies in one nand chip, block management is based on Die */
	unsigned int DieCntPerChip;
	/* count of planes in one die, >1 can support multi-plane operation */
	unsigned int PlaneCntPerDie;
	/* count of sectors in one single physic page, one sector is 0.5k */
	unsigned int SectorCntPerPage;
	/* count of physic pages in one physic block */
	unsigned int PageCntPerPhyBlk;
	/* count of physic blocks in one die, include valid and invalid blocks */
	unsigned int BlkCntPerDie;
	/* mask of operation types which current nand flash can support support */
	unsigned int OperationOpt;
	/* parameter of hardware access clock, based on 'MHz' */
	unsigned int FrequencePar;
	/* Ecc Mode for nand chip, 0: bch-16, 1:bch-28, 2:bch_32 */
	unsigned int EccMode;
	/* nand chip id of current connecting nand chip */
	unsigned char NandChipId[8];
	/* ratio of valid physical blocks, based on 1024 */
	unsigned int ValidBlkRatio;
	unsigned int good_block_ratio; /* good block ratio get from hwscan */
	unsigned int ReadRetryType; /* read retry type */
	unsigned int DDRType;
	unsigned int uboot_start_block;
	unsigned int uboot_next_block;
	unsigned int logic_start_block;
	unsigned int nand_specialinfo_page;
	unsigned int nand_specialinfo_offset;
	unsigned int physic_block_reserved;
	/* special nand cmd for some nand in batch cmd, only for write */
	unsigned int random_cmd2_send_flag;
	/* random col addr num in batch cmd */
	unsigned int random_addr_num;
	/* real physic page size */
	unsigned int nand_real_page_size;
	unsigned int Reserved[23];
} boot_nand_para_t;

typedef struct _normal_gpio_cfg {
	unsigned char port;
	unsigned char port_num;
	char mul_sel;
	char pull;
	char drv_level;
	char data;
	unsigned char reserved[2];
} normal_gpio_cfg;

/******************************************************************************/
/*                              head of Boot0                                 */
/******************************************************************************/
typedef struct _boot0_private_head_t {
	unsigned int prvt_head_size;
	char prvt_head_vsn[4];        /* the version of boot0_private_head_t */
	unsigned int dram_para[32];   /* Original values is arbitrary */
	int uart_port;
	normal_gpio_cfg uart_ctrl[2];
	int enable_jtag;              /* 1 : enable,  0 : disable */
	normal_gpio_cfg jtag_gpio[5];
	normal_gpio_cfg storage_gpio[32];
	char storage_data[512 - sizeof(normal_gpio_cfg) * 32];
}
boot0_private_head_t;

typedef struct standard_Boot_file_head {
	unsigned int jump_instruction;  /* one intruction jumping to real code */
	unsigned char magic[8];         /* ="eGON.BT0" or "eGON.BT1",  not C-style string */
	unsigned int check_sum;         /* generated by PC */
	unsigned int length;            /* generated by PC */
	unsigned int pub_head_size;     /* size of boot_file_head_t */
	unsigned char pub_head_vsn[4];  /* version of boot_file_head_t */
	unsigned char file_head_vsn[4]; /* version of boot0_file_head_t or boot1_file_head_t */
	unsigned char Boot_vsn[4];      /* Boot version */
	unsigned char eGON_vsn[4];      /* eGON version */
	unsigned char platform[8];      /* platform information */
} standard_boot_file_head_t;

typedef struct _boot0_file_head_t {
	standard_boot_file_head_t boot_head;
	boot0_private_head_t prvt_head;
} boot0_file_head_t;

typedef struct _boot_core_para_t {
	unsigned int user_set_clock;
	unsigned int user_set_core_vol;
	unsigned int vol_threshold;
} boot_core_para_t;

typedef struct {
	u8 name[8];
	u32 magic;
	u32 check_sum;

	u32 serial_num;
	u32 status;

	u32 items_nr;
	u32 length;
	u8 platform[4];
	u32 reserved[2];
	u32 end;

} toc0_private_head_t;

typedef struct sbrom_toc0_config {
	unsigned char config_vsn[4];
	unsigned int dram_para[32];
	int uart_port;
	normal_gpio_cfg uart_ctrl[2];
	int enable_jtag;
	normal_gpio_cfg jtag_gpio[5];
	normal_gpio_cfg storage_gpio[50];
	char storage_data[384];
	unsigned int secure_dram_mbytes;
	unsigned int drm_start_mbytes;
	unsigned int drm_size_mbytes;
	unsigned int res[8];
} sbrom_toc0_config_t;

/******************************************************************************/
/*                                   head of Boot1                            */
/******************************************************************************/
typedef struct _boot1_private_head_t {
	unsigned int dram_para[32];
	int run_clock;		/* Mhz */
	int run_core_vol;	/* mV */
	int uart_port;
	normal_gpio_cfg uart_gpio[2];
	int twi_port;
	normal_gpio_cfg twi_gpio[2];
	int work_mode;
	int storage_type;	/* 0nand   1sdcard    2: spinor */
	normal_gpio_cfg nand_gpio[32];
	char nand_spare_data[256];
	normal_gpio_cfg sdcard_gpio[32];
	char sdcard_spare_data[256];
	int reserved[6];
} boot1_private_head_t;
#if to_delete
typedef struct _Boot_file_head {
	unsigned int jump_instruction; /* one intruction jumping to real code */
	unsigned char magic[8];        /* ="u-boot" */
	unsigned int check_sum;        /* generated by PC */
	unsigned int align_size;       /* align size in byte */
	unsigned int length;           /* the size of all file */
	unsigned int uboot_length;     /* the size of uboot */
	unsigned char version[8];      /* uboot version */
	unsigned char platform[8];     /* platform information */
	int reserved[1];               /* stamp space, 16bytes align */
} boot_file_head_t;

typedef struct _boot1_file_head_t {
	boot_file_head_t boot_head;
	boot1_private_head_t prvt_head;
} boot1_file_head_t;
#endif

typedef struct _Boot_file_head {
	__u32  jump_instruction;   /* one intruction jumping to real code */
	__u8   magic[8];  /* ="eGON.BT0" */
	__u32  check_sum;          /* generated by PC */
	__u32  length;             /* generated by PC */
	__u32  pub_head_size;      /* the size of boot_file_head_t */
	__u8   pub_head_vsn[4];    /* the version of boot_file_head_t */
	__u32  ret_addr;           /* the return value */
	__u32  run_addr;           /* run addr */
	__u32  boot_cpu;           /* eGON version */
	__u8   platform[8];        /* platform information */
} boot_file_head_t;



struct boot_ndfc_cfg {
	u8 page_size_kb;
	u8 ecc_mode;
	u8 sequence_mode;
	u8 res[5];
};
#endif /* SUNXI_BOOT_H */
