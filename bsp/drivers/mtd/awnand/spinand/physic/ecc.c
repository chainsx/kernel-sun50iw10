// SPDX-License-Identifier: GPL-2.0

#define pr_fmt(fmt) "sunxi-spinand-phy: " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mtd/aw-spinand.h>

#include "physic.h"


static int __copy_to_oob(unsigned char *to, unsigned char *from,
		unsigned int cnt, unsigned int spare_size,
		unsigned int offset, unsigned int datalen)
{
	unsigned int i;

	/* bad block mark byte */
	/*to[0] = from[0];*/

	for (i = 0; cnt > 0; i++) {
		/*
		 * only the last time, num maybe not datalen, that's why
		 * memcpy offset add datalen rather than num
		 */
		unsigned int num = min(cnt, datalen);

		memcpy(to + offset + spare_size * i, from + datalen * i, num);
		cnt -= num;
	}
	return 0;
}

static int aw_spinand_ecc_copy_to_oob(enum ecc_oob_protected type,
		unsigned char *to, unsigned char *from, unsigned int len)
{
	switch (type) {
	case SIZE16_OFF0_LEN16:
		return __copy_to_oob(to, from, len, 16, 0, 16);
	case SIZE16_OFF4_LEN12:
		return __copy_to_oob(to, from, len, 16, 4, 12);
	case SIZE16_OFF4_LEN4_OFF8:
		return __copy_to_oob(to, from, len, 16, 4, 4);
	case SIZE16_OFF4_LEN8_OFF4:
		return __copy_to_oob(to, from, len, 16, 4, 8);
	case SIZE16_OFF32_LEN16:
		return __copy_to_oob(to, from, len, 16, 32, 16);
	case SIZE16_OFF8_LEN16:
		return __copy_to_oob(to, from, len, 16, 8, 16);
	default:
		return -EINVAL;
	}
}

static int __copy_from_oob(unsigned char *to, unsigned char *from,
		unsigned int cnt, unsigned int spare_size,
		unsigned int offset, unsigned int datalen)
{
	unsigned int i;

	for (i = 0; cnt > 0; i++) {
		/*
		 * only the last time, num maybe not datalen, that's why
		 * memcpy offset add datalen rather than num
		 */
		unsigned int num = min(cnt, datalen);

		memcpy(to + datalen * i, from + offset + spare_size * i, num);
		cnt -= num;
	}
	return 0;
}

static int aw_spinand_ecc_copy_from_oob(enum ecc_oob_protected type,
		unsigned char *to, unsigned char *from, unsigned int len)
{
	switch (type) {
	case SIZE16_OFF0_LEN16:
		return __copy_from_oob(to, from, len, 16, 0, 16);
	case SIZE16_OFF4_LEN12:
		return __copy_from_oob(to, from, len, 16, 4, 12);
	case SIZE16_OFF4_LEN4_OFF8:
		return __copy_from_oob(to, from, len, 16, 4, 4);
	case SIZE16_OFF4_LEN8_OFF4:
		return __copy_from_oob(to, from, len, 16, 4, 8);
	case SIZE16_OFF32_LEN16:
		return __copy_from_oob(to, from, len, 16, 32, 16);
	case SIZE16_OFF8_LEN16:
		return __copy_from_oob(to, from, len, 16, 8, 16);
	default:
		return -EINVAL;
	}
}

static inline int general_check_ecc(unsigned char ecc,
		unsigned char limit_from, unsigned char limit_to,
		unsigned char err_from, unsigned char err_to)
{
	if (ecc < limit_from) {
		return ECC_GOOD;
	} else if (ecc >= limit_from && ecc <= limit_to) {
		pr_debug("ecc limit 0x%x\n", ecc);
		return ECC_LIMIT;
	} else if (ecc >= err_from && ecc <= err_to) {
		pr_err("ecc error 0x%x\n", ecc);
		return ECC_ERR;
	}

	pr_err("unknown ecc value 0x%x\n", ecc);
	return ECC_ERR;
}

static int check_ecc_bit2_limit1_err2_limit3(unsigned char ecc)
{
	if (ecc == 0) {
		return ECC_GOOD;
	} else if (ecc == 1 || ecc == 3) {
		pr_debug("ecc limit 0x%x\n", ecc);
		return ECC_LIMIT;
	}

	pr_err("ecc error 0x%x\n", ecc);
	return ECC_ERR;
}

static int check_ecc_bit3_limit5_err2(unsigned char ecc)
{
	if (ecc <= 1) {
		return ECC_GOOD;
	} else if (ecc == 3 || ecc == 5) {
		pr_debug("ecc limit 0x%x\n", ecc);
		return ECC_LIMIT;
	}

	pr_err("ecc error 0x%x\n", ecc);
	return ECC_ERR;
}

static int check_ecc_bit4_limit5_7_err8_limit12(unsigned char ecc)
{
	if (ecc <= 4) {
		return ECC_GOOD;
	} else if ((ecc >= 5 && ecc <= 7) || (ecc >= 12)) {
		pr_debug("ecc limit 0x%x\n", ecc);
		return ECC_LIMIT;
	}

	pr_err("ecc err 0x%x\n", ecc);
	return ECC_ERR;
}

static int check_ecc_bit2_err2_limit3(unsigned char ecc)
{
	if (ecc == 0 || ecc == 1) {
		return ECC_GOOD;
	} else if (ecc == 3) {
		pr_debug("ecc limit 0x%x\n", ecc);
		return ECC_LIMIT;
	}

	pr_err("ecc error 0x%x\n", ecc);
	return ECC_ERR;
}

static int aw_spinand_ecc_check_ecc(enum ecc_limit_err type, u8 status)
{
	unsigned char ecc;

	switch (type) {
	case BIT3_LIMIT2_TO_6_ERR7:
		ecc = status & 0x07;
		return general_check_ecc(ecc, 2, 6, 7, 7);
	case BIT2_LIMIT1_ERR2:
		ecc = status & 0x03;
		return general_check_ecc(ecc, 1, 1, 2, 2);
	case BIT2_LIMIT1_ERR2_LIMIT3:
		ecc = status & 0x03;
		return check_ecc_bit2_limit1_err2_limit3(ecc);
	case BIT2_ERR2_LIMIT3:
		ecc = status & 0x03;
		return check_ecc_bit2_err2_limit3(ecc);
	case BIT4_LIMIT3_TO_4_ERR15:
		ecc = status & 0x0f;
		return general_check_ecc(ecc, 3, 4, 15, 15);
	case BIT3_LIMIT3_TO_4_ERR7:
		ecc = status & 0x07;
		return general_check_ecc(ecc, 3, 4, 7, 7);
	case BIT3_LIMIT5_ERR2:
		ecc = status & 0x07;
		return check_ecc_bit3_limit5_err2(ecc);
	case BIT4_LIMIT5_TO_7_ERR8_LIMIT_12:
		ecc = status & 0x0f;
		return check_ecc_bit4_limit5_7_err8_limit12(ecc);
	case BIT4_LIMIT5_TO_8_ERR9_TO_15:
		ecc = status & 0x0f;
		return general_check_ecc(ecc, 5, 8, 9, 15);
	default:
		return -EINVAL;
	}
}

static struct aw_spinand_ecc aw_spinand_ecc = {
	.copy_to_oob = aw_spinand_ecc_copy_to_oob,
	.copy_from_oob = aw_spinand_ecc_copy_from_oob,
	.check_ecc = aw_spinand_ecc_check_ecc,
};

int aw_spinand_chip_ecc_init(struct aw_spinand_chip *chip)
{
	chip->ecc = &aw_spinand_ecc;
	return 0;
}

MODULE_AUTHOR("liaoweixiong <liaoweixiong@allwinnertech.com>");
MODULE_DESCRIPTION("Commond physic layer for Allwinner's spinand driver");
