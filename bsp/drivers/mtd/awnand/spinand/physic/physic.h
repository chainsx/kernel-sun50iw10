/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __SPINAND_PHYSIC_H
#define __SPINAND_PHYSIC_H

#include <linux/mtd/aw-spinand.h>
#include <linux/bitops.h>
#include <linux/printk.h>

#define AW_SPINAND_PHY_VER_MAIN	0x01
#define AW_SPINAND_PHY_VER_SUB	0x11
#define AW_SPINAND_PHY_VER_DATE	0x20211125

#define SECTOR_SHIFT 9


/* spinand cmd num */
#define SPI_NAND_WREN		0x06
#define SPI_NAND_WRDI		0x04
#define SPI_NAND_GETSR		0x0f
#define SPI_NAND_SETSR		0x1f
#define SPI_NAND_PAGE_READ	0x13
#define SPI_NAND_FAST_READ_X1	0x0b
#define SPI_NAND_READ_X1	0x03
#define SPI_NAND_READ_X2	0x3b
#define SPI_NAND_READ_X4	0x6b
#define SPI_NAND_READ_DUAL_IO	0xbb
#define SPI_NAND_READ_QUAD_IO	0xeb
#define SPI_NAND_RDID		0x9f
#define SPI_NAND_PP		0x02
#define SPI_NAND_PP_X4		0x32
#define SPI_NAND_RANDOM_PP	0x84
#define SPI_NAND_RANDOM_PP_X4	0x34
#define SPI_NAND_PE		0x10
#define SPI_NAND_BE		0xd8
#define SPI_NAND_RESET		0xff
#define SPI_NAND_READ_INT_ECCSTATUS 0x7c

/* status register */
#define REG_STATUS		0xc0
#define GD_REG_EXT_ECC_STATUS	0xf0
#define STATUS_BUSY		BIT(0)
#define STATUS_ERASE_FAILED	BIT(2)
#define STATUS_PROG_FAILED	BIT(3)
#define STATUS_ECC_SHIFT	4

/* feature register */
#define REG_BLOCK_LOCK		0xa0

/* configuration register */
#define FORESEE_REG_ECC_CFG	0x90
#define REG_CFG			0xb0
#define CFG_OTP_ENABLE		BIT(6)
#define CFG_BUF_MODE		BIT(3)
#define CFG_ECC_ENABLE		BIT(4)
#define CFG_QUAD_ENABLE		BIT(0)

/* driver strength register */
#define REG_DRV			0xd0

/*differrent manufacture spinand's ecc status location maybe not the same*/
enum ecc_status_shift {
	ECC_STATUS_SHIFT_0 = 0,
	ECC_STATUS_SHIFT_1,
	ECC_STATUS_SHIFT_2,
	ECC_STATUS_SHIFT_3,
	ECC_STATUS_SHIFT_4,
	ECC_STATUS_SHIFT_5,
	ECC_STATUS_SHIFT_6,
	ECC_STATUS_SHIFT_7,
};

enum ecc_limit_err {
	ECC_TYPE_ERR = 0,
	BIT3_LIMIT2_TO_6_ERR7,
	BIT2_LIMIT1_ERR2,
	BIT2_LIMIT1_ERR2_LIMIT3,
	BIT2_ERR2_LIMIT3,
	BIT4_LIMIT3_TO_4_ERR15,
	BIT3_LIMIT3_TO_4_ERR7,
	BIT3_LIMIT5_ERR2,
	BIT4_LIMIT5_TO_7_ERR8_LIMIT_12,
	BIT4_LIMIT5_TO_8_ERR9_TO_15,
};

enum ecc_oob_protected {
	ECC_PROTECTED_TYPE = 0,
	/* all spare data are under ecc protection */
	SIZE16_OFF0_LEN16,
	SIZE16_OFF4_LEN12,
	SIZE16_OFF4_LEN4_OFF8,
	/*compatible with GD5F1GQ4UBYIG@R6*/
	SIZE16_OFF4_LEN8_OFF4,
	SIZE16_OFF32_LEN16,
	/*compatible with XTX*/
	SIZE16_OFF8_LEN16,
};

struct aw_spinand_ecc {
	int (*copy_to_oob)(enum ecc_oob_protected type, unsigned char *to,
			unsigned char *from, unsigned int len);
	int (*copy_from_oob)(enum ecc_oob_protected type, unsigned char *to,
			unsigned char *from, unsigned int len);
	int (*check_ecc)(enum ecc_limit_err type, u8 status);
};

struct aw_spinand_phy_info {
	const char *Model;
	unsigned char NandID[MAX_ID_LEN];
	unsigned int DieCntPerChip;
	unsigned int BlkCntPerDie;
	unsigned int PageCntPerBlk;
	unsigned int SectCntPerPage;
	unsigned int OobSizePerPage;
#define BAD_BLK_FLAG_MARK			0x03
#define BAD_BLK_FLAG_FRIST_1_PAGE		0x00
#define BAD_BLK_FLAG_FIRST_2_PAGE		0x01
#define BAD_BLK_FLAG_LAST_1_PAGE		0x02
#define BAD_BLK_FLAG_LAST_2_PAGE		0x03
	int BadBlockFlag;
#define SPINAND_DUAL_READ			BIT(0)
#define SPINAND_QUAD_READ			BIT(1)
#define SPINAND_QUAD_PROGRAM			BIT(2)
#define SPINAND_QUAD_NO_NEED_ENABLE		BIT(3)
#define SPINAND_ONEDUMMY_AFTER_RANDOMREAD	BIT(8)
	int OperationOpt;
	int MaxEraseTimes;
#define HAS_EXT_ECC_SE01			BIT(0)
#define HAS_EXT_ECC_STATUS			BIT(1)
	enum ecc_status_shift ecc_status_shift;
	int EccFlag;
	enum ecc_limit_err EccType;
	enum ecc_oob_protected EccProtectedType;
};

/* bbt: bad block table */
struct aw_spinand_bbt {
	unsigned long *bitmap;
	unsigned long *en_bitmap;

	int (*mark_badblock)(struct aw_spinand_chip *chip,
			unsigned int blknum, bool badblk);
#define BADBLOCK	(1)
#define NON_BADBLOCK	(0)
#define NOT_MARKED	(-1)
	int (*is_badblock)(struct aw_spinand_chip *chip, unsigned int blknum);
};

struct aw_spinand_cache {
	unsigned char *databuf;
	unsigned char *oobbuf;
	unsigned int data_maxlen;
	unsigned int oob_maxlen;
	unsigned int block;
	unsigned int page;
#define INVALID_CACHE_ALL_AREA	0
#define VALID_CACHE_OOB		BIT(1)
#define VALID_CACHE_DATA	BIT(2)
	unsigned int area;

	/*
	 * If the structure cache already has the data before, just copy
	 * these data to req.
	 * @match_cache is helper to check whether the structure cache is
	 *   what req needed.
	 * @copy_to_cache is helper to copy req to structure cache.
	 * @copy_from_cache is helper to copy structure cache to req.
	 */
	bool (*match_cache)(struct aw_spinand_chip *chip,
			struct aw_spinand_chip_request *req);
	int (*copy_to_cache)(struct aw_spinand_chip *chip,
			struct aw_spinand_chip_request *req);
	int (*copy_from_cache)(struct aw_spinand_chip *chip,
			struct aw_spinand_chip_request *req);
	/*
	 * 3 step:
	 *  a) copy data from req to cache->databuf/oobbuf
	 *  b) update cache->block/page
	 *  c) send write cache command to spinand
	 */
	int (*write_to_cache)(struct aw_spinand_chip *chip,
			struct aw_spinand_chip_request *req);
	/*
	 * 3 step:
	 *  a) send read cache command to spinand
	 *  b) update cache->block/page
	 *  c) copy data from cache->databuf/oobbuf to req
	 */
	int (*read_from_cache)(struct aw_spinand_chip *chip,
			struct aw_spinand_chip_request *req);
};

extern int aw_spinand_chip_ecc_init(struct aw_spinand_chip *chip);
extern int aw_spinand_chip_ops_init(struct aw_spinand_chip *chip);
extern int aw_spinand_chip_detect(struct aw_spinand_chip *chip);
extern int aw_spinand_chip_bbt_init(struct aw_spinand_chip *chip);
extern void aw_spinand_chip_bbt_exit(struct aw_spinand_chip *chip);
extern int aw_spinand_chip_cache_init(struct aw_spinand_chip *chip);
extern void aw_spinand_chip_cache_exit(struct aw_spinand_chip *chip);

#endif
