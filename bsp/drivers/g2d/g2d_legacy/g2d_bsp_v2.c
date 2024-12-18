/*
 * Allwinner SoCs g2d driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include "g2d_bsp.h"
#include "g2d_regs_v2.h"
#include <linux/types.h>
#include <linux/stddef.h>

static unsigned long base_addr;
/* byte input */
#define read_bvalue(offset)		get_bvalue(base_addr + offset)
/* byte output */
#define write_bvalue(offset, value)	put_bvalue(base_addr + offset, value)
/* half word input */
#define read_hvalue(offset)		get_hvalue(base_addr + offset)
/* half word output */
#define write_hvalue(offset, value)	put_hvalue(base_addr + offset, value)
/* word input */
#define read_wvalue(offset)		get_wvalue(base_addr + offset)
/* word output */
#define write_wvalue(offset, value)	put_wvalue(base_addr + offset, value)

__s32 g2d_fc_set(__u32 sel, __u32 color_value);
__s32 g2d_format_trans(__s32 data_fmt, __s32 pixel_seq);
__s32 rgb2Ycbcr_709[12] = {
	0x0bb, 0x0275, 0x03f, 0x4200, 0xFFFFFF99, 0xFFFFFEA6, 0x01c2, 0x20200,
	0x01c2, 0xFFFFFE67, 0xFFFFFFD7, 0x20200, };
__s32 Ycbcr2rgb_709[12] = {
	0x04a8, 0x0, 0x072c, 0xFFFC1F7D, 0x04a8, 0xFFFFFF26, 0xFFFFFDDD,
	0x133F8, 0x04a8, 0x0876, 0, 0xFFFB7AA0, };

__s32 rgb2Ycbcr_601[12] = {
	0x0107, 0x0204, 0x064, 0x4200,
	0xFFFFFF68, 0xFFFFFED6, 0x01c2, 0x20200,
	0x01c2, 0xFFFFFE87, 0xFFFFFFB7, 0x20200,};
__s32 Ycbcr2rgb_601[12] = {
	0x04a8, 0x0, 0x0662, 0xFFFC865A,
	0x04a8, 0xFFFFFE70, 0xFFFFFCBF, 0x21FF4,
	0x04a8, 0x0812, 0x0, 0xFFFBAE4A,};

__s32 lan2coefftab32_full[512] = {
	0x00004000, 0x000140ff, 0x00033ffe, 0x00043ffd, 0x00063efc, 0xff083dfc,
	0x000a3bfb, 0xff0d39fb, 0xff0f37fb, 0xff1136fa, 0xfe1433fb,
	0xfe1631fb, 0xfd192ffb, 0xfd1c2cfb, 0xfd1f29fb, 0xfc2127fc,
	0xfc2424fc, 0xfc2721fc, 0xfb291ffd, 0xfb2c1cfd, 0xfb2f19fd,
	0xfb3116fe, 0xfb3314fe, 0xfa3611ff, 0xfb370fff, 0xfb390dff,
	0xfb3b0a00, 0xfc3d08ff, 0xfc3e0600, 0xfd3f0400, 0xfe3f0300,
	0xff400100,
	/* counter = 1 */
	0x00004000, 0x000140ff, 0x00033ffe, 0x00043ffd, 0x00063efc,
	0xff083dfc, 0x000a3bfb, 0xff0d39fb, 0xff0f37fb, 0xff1136fa,
	0xfe1433fb, 0xfe1631fb, 0xfd192ffb, 0xfd1c2cfb, 0xfd1f29fb,
	0xfc2127fc, 0xfc2424fc, 0xfc2721fc, 0xfb291ffd, 0xfb2c1cfd,
	0xfb2f19fd, 0xfb3116fe, 0xfb3314fe, 0xfa3611ff, 0xfb370fff,
	0xfb390dff, 0xfb3b0a00, 0xfc3d08ff, 0xfc3e0600, 0xfd3f0400,
	0xfe3f0300, 0xff400100,
	/* counter = 2 */
	0xff053804, 0xff063803, 0xff083801, 0xff093701, 0xff0a3700,
	0xff0c3500, 0xff0e34ff, 0xff1033fe, 0xff1232fd, 0xfe1431fd,
	0xfe162ffd, 0xfe182dfd, 0xfd1b2cfc, 0xfd1d2afc, 0xfd1f28fc,
	0xfd2126fc, 0xfd2323fd, 0xfc2621fd, 0xfc281ffd, 0xfc2a1dfd,
	0xfc2c1bfd, 0xfd2d18fe, 0xfd2f16fe, 0xfd3114fe, 0xfd3212ff,
	0xfe3310ff, 0xff340eff, 0x00350cff, 0x00360a00, 0x01360900,
	0x02370700, 0x03370600,
	/* counter = 3 */
	0xff083207, 0xff093206, 0xff0a3205, 0xff0c3203, 0xff0d3103,
	0xff0e3102, 0xfe113001, 0xfe132f00, 0xfe142e00, 0xfe162dff,
	0xfe182bff, 0xfe192aff, 0xfe1b29fe, 0xfe1d27fe, 0xfe1f25fe,
	0xfd2124fe, 0xfe2222fe, 0xfe2421fd, 0xfe251ffe, 0xfe271dfe,
	0xfe291bfe, 0xff2a19fe, 0xff2b18fe, 0xff2d16fe, 0x002e14fe,
	0x002f12ff, 0x013010ff, 0x02300fff, 0x03310dff, 0x04310cff,
	0x05310a00, 0x06310900,
	/* counter = 4 */
	0xff0a2e09, 0xff0b2e08, 0xff0c2e07, 0xff0e2d06, 0xff0f2d05,
	0xff102d04, 0xff122c03, 0xfe142c02, 0xfe152b02, 0xfe172a01,
	0xfe182901, 0xfe1a2800, 0xfe1b2700, 0xfe1d2500, 0xff1e24ff,
	0xfe2023ff, 0xff2121ff, 0xff2320fe, 0xff241eff, 0x00251dfe,
	0x00261bff, 0x00281afe, 0x012818ff, 0x012a16ff, 0x022a15ff,
	0x032b13ff, 0x032c12ff, 0x052c10ff, 0x052d0fff, 0x062d0d00,
	0x072d0c00, 0x082d0b00,
	/* counter = 5 */
	0xff0c2a0b, 0xff0d2a0a, 0xff0e2a09, 0xff0f2a08, 0xff102a07,
	0xff112a06, 0xff132905, 0xff142904, 0xff162803, 0xff172703,
	0xff182702, 0xff1a2601, 0xff1b2501, 0xff1c2401, 0xff1e2300,
	0xff1f2200, 0x00202000, 0x00211f00, 0x01221d00, 0x01231c00,
	0x01251bff, 0x02251aff, 0x032618ff, 0x032717ff, 0x042815ff,
	0x052814ff, 0x052913ff, 0x06291100, 0x072a10ff, 0x082a0e00,
	0x092a0d00, 0x0a2a0c00,
	/* counter = 6 */
	0xff0d280c, 0xff0e280b, 0xff0f280a, 0xff102809, 0xff112808,
	0xff122708, 0xff142706, 0xff152705, 0xff162605, 0xff172604,
	0xff192503, 0xff1a2403, 0x001b2302, 0x001c2202, 0x001d2201,
	0x001e2101, 0x011f1f01, 0x01211e00, 0x01221d00, 0x02221c00,
	0x02231b00, 0x03241900, 0x04241800, 0x04251700, 0x052616ff,
	0x06261400, 0x072713ff, 0x08271100, 0x08271100, 0x09271000,
	0x0a280e00, 0x0b280d00,
	/* counter = 7 */
	0xff0e260d, 0xff0f260c, 0xff10260b, 0xff11260a, 0xff122609,
	0xff132608, 0xff142508, 0xff152507, 0x00152506, 0x00172405,
	0x00182305, 0x00192304, 0x001b2203, 0x001c2103, 0x011d2002,
	0x011d2002, 0x011f1f01, 0x021f1e01, 0x02201d01, 0x03211c00,
	0x03221b00, 0x04221a00, 0x04231801, 0x05241700, 0x06241600,
	0x07241500, 0x08251300, 0x09251200, 0x09261100, 0x0a261000,
	0x0b260f00, 0x0c260e00,
	/* counter = 8 */
	0xff0e250e, 0xff0f250d, 0xff10250c, 0xff11250b, 0x0011250a,
	0x00132409, 0x00142408, 0x00152407, 0x00162307, 0x00172306,
	0x00182206, 0x00192205, 0x011a2104, 0x011b2004, 0x011c2003,
	0x021c1f03, 0x021e1e02, 0x031e1d02, 0x03201c01, 0x04201b01,
	0x04211a01, 0x05221900, 0x05221801, 0x06231700, 0x07231600,
	0x07241500, 0x08241400, 0x09241300, 0x0a241200, 0x0b241100,
	0x0c241000, 0x0d240f00,
	/* counter = 9 */
	0x000e240e, 0x000f240d, 0x0010240c, 0x0011240b, 0x0013230a,
	0x0013230a, 0x00142309, 0x00152308, 0x00162208, 0x00172207,
	0x01182106, 0x01192105, 0x011a2005, 0x021b1f04, 0x021b1f04,
	0x021d1e03, 0x031d1d03, 0x031e1d02, 0x041e1c02, 0x041f1b02,
	0x05201a01, 0x05211901, 0x06211801, 0x07221700, 0x07221601,
	0x08231500, 0x09231400, 0x0a231300, 0x0a231300, 0x0b231200,
	0x0c231100, 0x0d231000,
	/* counter = 10 */
	0x000f220f, 0x0010220e, 0x0011220d, 0x0012220c, 0x0013220b,
	0x0013220b, 0x0015210a, 0x0015210a, 0x01162108, 0x01172008,
	0x01182007, 0x02191f06, 0x02191f06, 0x021a1e06, 0x031a1e05,
	0x031c1d04, 0x041c1c04, 0x041d1c03, 0x051d1b03, 0x051e1a03,
	0x061f1902, 0x061f1902, 0x07201801, 0x08201701, 0x08211601,
	0x09211501, 0x0a211500, 0x0b211400, 0x0b221300, 0x0c221200,
	0x0d221100, 0x0e221000,
	/* counter = 11 */
	0x0010210f, 0x0011210e, 0x0011210e, 0x0012210d, 0x0013210c,
	0x0014200c, 0x0114200b, 0x0115200a, 0x01161f0a, 0x01171f09,
	0x02171f08, 0x02181e08, 0x03181e07, 0x031a1d06, 0x031a1d06,
	0x041b1c05, 0x041c1c04, 0x051c1b04, 0x051d1a04, 0x061d1a03,
	0x071d1903, 0x071e1803, 0x081e1802, 0x081f1702, 0x091f1602,
	0x0a201501, 0x0b1f1501, 0x0b201401, 0x0c211300, 0x0d211200,
	0x0e201200, 0x0e211100,
	/* counter = 12 */
	0x00102010, 0x0011200f, 0x0012200e, 0x0013200d, 0x0013200d,
	0x01141f0c, 0x01151f0b, 0x01151f0b, 0x01161f0a, 0x02171e09,
	0x02171e09, 0x03181d08, 0x03191d07, 0x03191d07, 0x041a1c06,
	0x041b1c05, 0x051b1b05, 0x051c1b04, 0x061c1a04, 0x071d1903,
	0x071d1903, 0x081d1803, 0x081e1703, 0x091e1702, 0x0a1f1601,
	0x0a1f1502, 0x0b1f1501, 0x0c1f1401, 0x0d201300, 0x0d201300,
	0x0e201200, 0x0f201100,
	/* counter = 13 */
	0x00102010, 0x0011200f, 0x00121f0f, 0x00131f0e, 0x00141f0d,
	0x01141f0c, 0x01141f0c, 0x01151e0c, 0x02161e0a, 0x02171e09,
	0x03171d09, 0x03181d08, 0x03181d08, 0x04191c07, 0x041a1c06,
	0x051a1b06, 0x051b1b05, 0x061b1a05, 0x061c1a04, 0x071c1904,
	0x081c1903, 0x081d1803, 0x091d1703, 0x091e1702, 0x0a1e1602,
	0x0b1e1502, 0x0c1e1501, 0x0c1f1401, 0x0d1f1400, 0x0e1f1300,
	0x0e1f1201, 0x0f1f1200,
	/* counter = 14 */
	0x00111e11, 0x00121e10, 0x00131e0f, 0x00131e0f, 0x01131e0e,
	0x01141d0e, 0x02151d0c, 0x02151d0c, 0x02161d0b, 0x03161c0b,
	0x03171c0a, 0x04171c09, 0x04181b09, 0x05181b08, 0x05191b07,
	0x06191a07, 0x061a1a06, 0x071a1906, 0x071b1905, 0x081b1805,
	0x091b1804, 0x091c1704, 0x0a1c1703, 0x0a1c1604, 0x0b1d1602,
	0x0c1d1502, 0x0c1d1502, 0x0d1d1402, 0x0e1d1401, 0x0e1e1301,
	0x0f1e1300, 0x101e1200,
	/* counter = 15 */
	0x00111e11, 0x00121e10, 0x00131d10, 0x01131d0f, 0x01141d0e,
	0x01141d0e, 0x02151c0d, 0x02151c0d, 0x03161c0b, 0x03161c0b,
	0x04171b0a, 0x04171b0a, 0x05171b09, 0x05181a09, 0x06181a08,
	0x06191a07, 0x07191907, 0x071a1906, 0x081a1806, 0x081a1806,
	0x091a1805, 0x0a1b1704, 0x0a1b1704, 0x0b1c1603, 0x0b1c1603,
	0x0c1c1503, 0x0d1c1502, 0x0d1d1402, 0x0e1d1401, 0x0f1d1301,
	0x0f1d1301, 0x101e1200,
	/* counter = 16 */
};

__s32 linearcoefftab32[32] = {
	0x00004000, 0x00023e00, 0x00043c00, 0x00063a00, 0x00083800,
	0x000a3600, 0x000c3400, 0x000e3200, 0x00103000, 0x00122e00,
	0x00142c00, 0x00162a00, 0x00182800, 0x001a2600, 0x001c2400,
	0x001e2200, 0x00202000, 0x00221e00, 0x00241c00, 0x00261a00,
	0x00281800, 0x002a1600, 0x002c1400, 0x002e1200, 0x00301000,
	0x00320e00, 0x00340c00, 0x00360a00, 0x00380800, 0x003a0600,
	0x003c0400, 0x003e0200, };
__s32 g2d_bsp_open(void)
{
	write_wvalue(G2D_SCLK_GATE, 0x3);
	write_wvalue(G2D_HCLK_GATE, 0x3);
	write_wvalue(G2D_AHB_RESET, 0x3);
	return 0;
}

__s32 g2d_bsp_close(void)
{
	write_wvalue(G2D_AHB_RESET, 0x0);
	write_wvalue(G2D_HCLK_GATE, 0x0);
	write_wvalue(G2D_SCLK_GATE, 0x0);
	return 0;
}

__s32 g2d_bsp_reset(void)
{
	write_wvalue(G2D_AHB_RESET, 0x0);
	write_wvalue(G2D_AHB_RESET, 0x3);
	return 0;
}

__s32 g2d_mixer_reset(void)
{
	__u32 reg_val;

	reg_val = read_wvalue(G2D_AHB_RESET);
	write_wvalue(G2D_AHB_RESET, reg_val & 0xfffffffe);
	write_wvalue(G2D_AHB_RESET, reg_val & 0xffffffff);
	return 0;
}

__s32 g2d_rot_reset(void)
{
	__u32 reg_val;

	reg_val = read_wvalue(G2D_AHB_RESET);
	write_wvalue(G2D_AHB_RESET, reg_val & 0xfffffffd);
	write_wvalue(G2D_AHB_RESET, reg_val & 0xffffffff);
	return 0;
}

__s32 g2d_scan_order_fun(__u32 scan_order)
{
	__u32 tmp;

	tmp = read_wvalue(G2D_MIXER_CTL);
	tmp |= ((scan_order >> 24) & 0xf0);
	write_wvalue(G2D_MIXER_CTL, tmp);
	return 0;
}


/*
 * G2D IRQ query funct
 * if the mission finish IRQ flag was set to 1, then clear the flag
 * and return 1
 * if the IRQ was set to 0, then return 0
 */
__s32 mixer_irq_query(void)
{
	__u32 tmp;

	tmp = read_wvalue(G2D_MIXER_INT);
	if (tmp & 0x1) {
		write_wvalue(G2D_MIXER_INT, tmp);
		return 0;
	}
	return -1;
}


/*
 * G2D IRQ query funct
 * if the mission finish IRQ flag was set to 1, then clear the flag
 * and return 1
 * if the IRQ was set to 0, then return 0
 */
__s32 rot_irq_query(void)
{
	__u32 tmp;

	tmp = read_wvalue(ROT_INT);
	if (tmp & 0x1) {
		write_wvalue(ROT_INT, tmp);
		return 0;
	}
	return -1;
}

__s32 mixer_irq_enable(void)
{
	write_wvalue(G2D_MIXER_INT, 0x10);
	return 0;
}

__s32 rot_irq_enable(void)
{
	write_wvalue(ROT_INT, 0x10000);
	return 0;
}

__s32 g2d_irq_disable(void)
{
	write_wvalue(G2D_MIXER_INT, 0x0);
	return 0;
}

__s32 rot_irq_disable(void)
{
	write_wvalue(ROT_INT, 0x0);
	return 0;
}

__s32 g2d_sclk_div(__u32 div)
{
	__u32 reg_val;

	reg_val = read_wvalue(G2D_SCLK_DIV);
	reg_val &= 0xfffffff0;
	reg_val |= (div & 0xf);
	write_wvalue(G2D_SCLK_DIV, reg_val);
	return 0;
}

__s32 rot_sclk_div(__u32 div)
{
	__u32 reg_val;

	reg_val = read_wvalue(G2D_SCLK_DIV);
	reg_val &= 0xffffff0f;
	reg_val |= (div & 0xf) << 4;
	write_wvalue(G2D_SCLK_DIV, reg_val);
	return 0;
}

__s32 porter_duff(__u32 cmd)
{
	switch (cmd) {
	case G2D_BLD_CLEAR:
		write_wvalue(BLD_CTL, 0x00000000);
		break;
	case G2D_BLD_COPY:
		write_wvalue(BLD_CTL, 0x00010001);
		break;
	case G2D_BLD_DST:
		write_wvalue(BLD_CTL, 0x01000100);
		break;
	case G2D_BLD_SRCOVER:
		write_wvalue(BLD_CTL, 0x03010301);
		break;
	case G2D_BLD_DSTOVER:
		write_wvalue(BLD_CTL, 0x01030103);
		break;
	case G2D_BLD_SRCIN:
		write_wvalue(BLD_CTL, 0x00020002);
		break;
	case G2D_BLD_DSTIN:
		write_wvalue(BLD_CTL, 0x02000200);
		break;
	case G2D_BLD_SRCOUT:
		write_wvalue(BLD_CTL, 0x00030003);
		break;
	case G2D_BLD_DSTOUT:
		write_wvalue(BLD_CTL, 0x03000300);
		break;
	case G2D_BLD_SRCATOP:
		write_wvalue(BLD_CTL, 0x03020302);
		break;
	case G2D_BLD_DSTATOP:
		write_wvalue(BLD_CTL, 0x02030203);
		break;
	case G2D_BLD_XOR:
		write_wvalue(BLD_CTL, 0x03030303);
		break;
	default:
		write_wvalue(BLD_CTL, 0x03010301);
		}
	return 0;
}


/*
 * @csc_no: CSC ID, G2D support three CSC,
 * -1 will return to indicate inappropriate CSC number.
 * @csc_sel: CSC format, G2D support the ITU-R 601. ITU-R 709. standard trans-
 *  form between RGB and YUV colorspace.
 */
__s32 g2d_csc_reg_set(__u32 csc_no, g2d_csc_sel csc_sel)
{
	__u32 i;
	__u32 csc_base_addr;
	__u32 tmp;

	switch (csc_no) {
	case 0:
		csc_base_addr = G2D_BLD + 0x110;
		tmp = read_wvalue(BLD_CSC_CTL);
		tmp |= 0x1;
		write_wvalue(BLD_CSC_CTL, tmp);
		break;
	case 1:
		csc_base_addr = G2D_BLD + 0x140;
		tmp = read_wvalue(BLD_CSC_CTL);
		tmp |= 0x1 << 1;
		write_wvalue(BLD_CSC_CTL, tmp);
		break;
	case 2:
		csc_base_addr = G2D_BLD + 0x170;
		tmp = read_wvalue(BLD_CSC_CTL);
		tmp |= 0x1 << 2;
		write_wvalue(BLD_CSC_CTL, tmp);
		break;
	default:

/* __wrn("sel wrong csc no.\n"); */
		    return -1;
	}
	switch (csc_sel) {
	case G2D_RGB2YUV_709:
		for (i = 0; i < 12; i++)
			write_wvalue(csc_base_addr + (i << 2),
				      rgb2Ycbcr_709[i]);
		break;
	case G2D_YUV2RGB_709:
		for (i = 0; i < 12; i++)
			write_wvalue(csc_base_addr + (i << 2),
				      Ycbcr2rgb_709[i]);
		break;
	case G2D_RGB2YUV_601:
		for (i = 0; i < 12; i++)
			write_wvalue(csc_base_addr + (i << 2),
				      rgb2Ycbcr_601[i]);

/* write_wvalue(csc_base_addr + (i<<2), */
/* rgb2Ycbcr_601[i]); */
		    break;
	case G2D_YUV2RGB_601:
		for (i = 0; i < 12; i++)
			write_wvalue(csc_base_addr + (i << 2),
				      Ycbcr2rgb_601[i]);

/* write_wvalue(csc_base_addr + (i<<2), */
/* Ycbcr2rgb_601[i]); */
		    break;
	default:

/* __wrn("wrong csc standard\n"); */
		    return -2;
	}
	return 0;
}


/*
 * set colorkey para.
 */
__s32 ck_para_set(g2d_ck *para)
{
	__u32 tmp = 0x0;

	if (para->match_rule)
		tmp = 0x7;
	write_wvalue(BLD_KEY_CON, tmp);
	write_wvalue(BLD_KEY_MAX, para->max_color & 0x00ffffff);
	write_wvalue(BLD_KEY_MIN, para->min_color & 0x00ffffff);
	return 0;
}


/*
 */
__s32 g2d_byte_cal(__u32 format, __u32 *ycnt, __u32 *ucnt, __u32 *vcnt)
{
	*ycnt = 0;
	*ucnt = 0;
	*vcnt = 0;
	if (format <= G2D_FORMAT_BGRX8888)
		*ycnt = 4;

	else if (format <= G2D_FORMAT_BGR888)
		*ycnt = 3;

	else if (format <= G2D_FORMAT_BGRA5551)
		*ycnt = 2;

	else if (format <= G2D_FORMAT_BGRA1010102)
		*ycnt = 4;

	else if (format <= 0x23) {
		*ycnt = 2;
	}

	else if (format <= 0x25) {
		*ycnt = 1;
		*ucnt = 2;
	}

	else if (format == 0x26) {
		*ycnt = 1;
		*ucnt = 1;
		*vcnt = 1;
	}

	else if (format <= 0x29) {
		*ycnt = 1;
		*ucnt = 2;
	}

	else if (format == 0x2a) {
		*ycnt = 1;
		*ucnt = 1;
		*vcnt = 1;
	}

	else if (format <= 0x2d) {
		*ycnt = 1;
		*ucnt = 2;
	}

	else if (format == 0x2e) {
		*ycnt = 1;
		*ucnt = 1;
		*vcnt = 1;
	}

	else if (format == 0x30)
		*ycnt = 1;

	else if (format <= 0x36) {
		*ycnt = 2;
		*ucnt = 4;
	}

	else if (format <= 0x39)
		*ycnt = 6;
	return 0;
}


/*
 */
__u32 cal_align(__u32 width, __u32 align)
{
	switch (align) {
	case 0:
		return width;
	case 4:
		return (width + 3) >> 1 << 1;
	case 8:
		return (width + 7) >> 3 << 3;
	case 16:
		return (width + 15) >> 4 << 4;
	case 32:
		return (width + 31) >> 5 << 5;
	case 64:
		return (width + 63) >> 6 << 6;
	case 128:
		return (width + 127) >> 7 << 7;
	default:
		return (width + 31) >> 5 << 5;
	}
}


/*
 * @sel:layer no.
 */
__s32 g2d_vlayer_set(__u32 sel, g2d_image_enh *image)
{
	unsigned long long addr0, addr1, addr2;
	__u32 tmp;
	__u32 ycnt, ucnt, vcnt;
	__u32 pitch0, pitch1, pitch2;
	__u32 ch, cw, cy, cx;

	switch (sel) {
	case 0:

		    /* base_addr = G2D_V0; */
		    break;
	default:
		return -1;
	}
	tmp = ((image->alpha & 0xff) << 24);
	if (image->bpremul)
		tmp |= (0x1 << 17);
	tmp |= (image->format << 8);
	tmp |= (image->mode << 1);
	tmp |= 1;
	write_wvalue(V0_ATTCTL, tmp);
	tmp =
	    (((image->clip_rect.h ==
	       0 ? 0 : image->clip_rect.h -
	       1) & 0x1fff) << 16) | ((image->clip_rect.w ==
					0 ? 0 : image->clip_rect.w -
					1) & 0x1fff);
	write_wvalue(V0_MBSIZE, tmp);

	    /* offset is set to 0, ovl size is set to layer size */
	    write_wvalue(V0_SIZE, tmp);
	write_wvalue(V0_COOR, 0);
	if ((image->format >= G2D_FORMAT_YUV422UVC_V1U1V0U0)
	      && (image->format <= G2D_FORMAT_YUV422_PLANAR)) {
		cw = image->width >> 1;
		ch = image->height;
		cx = image->clip_rect.x >> 1;
		cy = image->clip_rect.y;
	}

	else if ((image->format >= G2D_FORMAT_YUV420UVC_V1U1V0U0)
		 && (image->format <= G2D_FORMAT_YUV420_PLANAR)) {
		cw = image->width >> 1;
		ch = image->height >> 1;
		cx = image->clip_rect.x >> 1;
		cy = image->clip_rect.y >> 1;
	}

	else if ((image->format >= G2D_FORMAT_YUV411UVC_V1U1V0U0)
		 && (image->format <= G2D_FORMAT_YUV411_PLANAR)) {
		cw = image->width >> 2;
		ch = image->height;
		cx = image->clip_rect.x >> 2;
		cy = image->clip_rect.y;
	}

	else {
		cw = 0;
		ch = 0;
		cx = 0;
		cy = 0;
	}
	g2d_byte_cal(image->format, &ycnt, &ucnt, &vcnt);
	pitch0 = cal_align(ycnt * image->width, image->align[0]);
	write_wvalue(V0_PITCH0, pitch0);
	pitch1 = cal_align(ucnt * cw, image->align[1]);
	write_wvalue(V0_PITCH1, pitch1);
	pitch2 = cal_align(vcnt * cw, image->align[2]);
	write_wvalue(V0_PITCH2, pitch2);
	G2D_INFO_MSG("VInPITCH: %d, %d, %d\n",
				pitch0, pitch1, pitch2);
	G2D_INFO_MSG("VInAddrB: 0x%x, 0x%x, 0x%x\n",
			image->laddr[0], image->laddr[1], image->laddr[2]);
	addr0 =
	    image->laddr[0] + ((__u64) image->haddr[0] << 32) +
	    pitch0 * image->clip_rect.y + ycnt * image->clip_rect.x;
	write_wvalue(V0_LADD0, addr0 & 0xffffffff);
	addr1 =
	    image->laddr[1] + ((__u64) image->haddr[1] << 32) + pitch1 * cy +
	    ucnt * cx;
	write_wvalue(V0_LADD1, addr1 & 0xffffffff);
	addr2 =
	    image->laddr[2] + ((__u64) image->haddr[2] << 32) + pitch2 * cy +
	    vcnt * cx;
	write_wvalue(V0_LADD2, addr2 & 0xffffffff);
	tmp = ((addr0 >> 32) & 0xff) | ((addr1 >> 32) & 0xff) << 8 |
	    ((addr2 >> 32) & 0xff) << 16;
	write_wvalue(V0_HADD, tmp);
	G2D_INFO_MSG("VInAddrA: 0x%llx, 0x%llx, 0x%llx\n",
							addr0, addr1, addr2);
	if (image->bbuff == 0)
		g2d_fc_set((sel + VI_LAYER_NUMBER), image->color);
	return 0;
}

__s32 g2d_uilayer_set(__u32 sel, g2d_image_enh *img)
{
	__u64 addr0;
	__u32 base_addr_u, tmp;
	__u32 ycnt, ucnt, vcnt;
	__u32 pitch0;

	switch (sel) {
	case 0:
		base_addr_u = G2D_UI0;
		break;
	case 1:
		base_addr_u = G2D_UI1;
		break;
	case 2:
		base_addr_u = G2D_UI2;
		break;
	default:
		return -1;
	}
	tmp = (img->alpha & 0xff) << 24;
	if (img->bpremul)
		tmp |= 0x1 << 17;
	tmp |= img->format << 8;
	tmp |= img->mode << 1;
	tmp |= 1;
	write_wvalue(base_addr_u, tmp);
	tmp =
	    (((img->clip_rect.h ==
	       0 ? 0 : img->clip_rect.h -
	       1) & 0x1fff) << 16) | ((img->clip_rect.w ==
					0 ? 0 : img->clip_rect.w - 1) & 0x1fff);
	write_wvalue(base_addr_u + 0x4, tmp);
	write_wvalue(base_addr_u + 0x1C, tmp);
	write_wvalue(base_addr_u + 0x8, 0);
	g2d_byte_cal(img->format, &ycnt, &ucnt, &vcnt);
	pitch0 = cal_align(ycnt * img->width, img->align[0]);
	write_wvalue(base_addr_u + 0xC, pitch0);
	addr0 =
	    img->laddr[0] + ((__u64) img->haddr[0] << 32) +
	    pitch0 * img->clip_rect.y + ycnt * img->clip_rect.x;
	write_wvalue(base_addr_u + 0x10, addr0 & 0xffffffff);
	write_wvalue(base_addr_u + 0x18, (addr0 >> 32) & 0xff);
	if (img->bbuff == 0)
		g2d_fc_set((sel + VI_LAYER_NUMBER), img->color);
	return 0;
}

__s32 g2d_wb_set(g2d_image_enh *image)
{
	__u64 addr0, addr1, addr2;
	__u32 tmp;
	__u32 ycnt, ucnt, vcnt;
	__u32 pitch0, pitch1, pitch2;
	__u32 ch, cw, cy, cx;

	write_wvalue(WB_ATT, image->format);
	tmp =
	    (((image->clip_rect.h ==
	       0 ? 0 : image->clip_rect.h -
	       1) & 0x1fff) << 16) | ((image->clip_rect.w ==
					0 ? 0 : image->clip_rect.w -
					1) & 0x1fff);
	write_wvalue(WB_SIZE, tmp);
	/* write to the bld out reg */
	G2D_INFO_MSG("BLD_CH_OSIZE W:  0x%x\n", image->clip_rect.w);
	G2D_INFO_MSG("BLD_CH_OSIZE H:  0x%x\n", image->clip_rect.h);
	write_wvalue(BLD_SIZE, tmp);
	/* set outdata premul */
	tmp = read_wvalue(BLD_OUT_COLOR);

	if (image->bpremul)
		write_wvalue(BLD_OUT_COLOR, tmp | 0x1);

	else
		write_wvalue(BLD_OUT_COLOR, tmp & 0x2);
	if ((image->format >= G2D_FORMAT_YUV422UVC_V1U1V0U0)
	      && (image->format <= G2D_FORMAT_YUV422_PLANAR)) {
		cw = image->width >> 1;
		ch = image->height;
		cx = image->clip_rect.x >> 1;
		cy = image->clip_rect.y;
	}

	else if ((image->format >= G2D_FORMAT_YUV420UVC_V1U1V0U0)
		 && (image->format <= G2D_FORMAT_YUV420_PLANAR)) {
		cw = image->width >> 1;
		ch = image->height >> 1;
		cx = image->clip_rect.x >> 1;
		cy = image->clip_rect.y >> 1;
	}

	else if ((image->format >= G2D_FORMAT_YUV411UVC_V1U1V0U0)
		 && (image->format <= G2D_FORMAT_YUV411_PLANAR)) {
		cw = image->width >> 2;
		ch = image->height;
		cx = image->clip_rect.x >> 2;
		cy = image->clip_rect.y;
	}

	else {
		cw = 0;
		ch = 0;
		cx = 0;
		cy = 0;
	}
	g2d_byte_cal(image->format, &ycnt, &ucnt, &vcnt);
	pitch0 = cal_align(ycnt * image->width, image->align[0]);
	write_wvalue(WB_PITCH0, pitch0);
	pitch1 = cal_align(ucnt * cw, image->align[1]);
	write_wvalue(WB_PITCH1, pitch1);
	pitch2 = cal_align(vcnt * cw, image->align[2]);
	write_wvalue(WB_PITCH2, pitch2);
	G2D_INFO_MSG("OutputPitch: %d, %d, %d\n", pitch0, pitch1, pitch2);

	addr0 =
	    image->laddr[0] + ((__u64) image->haddr[0] << 32) +
	    pitch0 * image->clip_rect.y + ycnt * image->clip_rect.x;
	write_wvalue(WB_LADD0, addr0 & 0xffffffff);
	write_wvalue(WB_HADD0, (addr0 >> 32) & 0xff);
	addr1 =
	    image->laddr[1] + ((__u64) image->haddr[1] << 32) + pitch1 * cy +
	    ucnt * cx;
	write_wvalue(WB_LADD1, addr1 & 0xffffffff);
	write_wvalue(WB_HADD1, (addr1 >> 32) & 0xff);
	addr2 =
	    image->laddr[2] + ((__u64) image->haddr[2] << 32) + pitch2 * cy +
	    vcnt * cx;
	write_wvalue(WB_LADD2, addr2 & 0xffffffff);
	write_wvalue(WB_HADD2, (addr2 >> 32) & 0xff);
	G2D_INFO_MSG("WbAddr: 0x%llx, 0x%llx, 0x%llx\n", addr0, addr1, addr2);
	return 0;
}


/*
 * fillcolor set
 * @sel:layer_no, 0--Layer Video,1--Layer UI0,2--Layer UI1,3--Layer UI2
 * @color_value:fill color value
 */
__s32 g2d_fc_set(__u32 sel, __u32 color_value)
{
	__u32 tmp;

	G2D_INFO_MSG("FILLCOLOR: sel: %d, color: 0x%x\n", sel, color_value);

	if (sel == 0) {
		/* Layer Video */
		tmp = read_wvalue(V0_ATTCTL);
		tmp |= (0x1 << 4);
		write_wvalue(V0_ATTCTL, tmp);
		write_wvalue(V0_FILLC, color_value);
	}
	if (sel == 1) {
		/* Layer UI0 */
		tmp = read_wvalue(UI0_ATTR);
		tmp |= (0x1 << 4);
		write_wvalue(UI0_ATTR, tmp);
		write_wvalue(UI0_FILLC, color_value);
	}
	if (sel == 2) {
		/* Layer UI1 */
		tmp = read_wvalue(UI1_ATTR);
		tmp |= (0x1 << 4);
		write_wvalue(UI1_ATTR, tmp);
		write_wvalue(UI1_FILLC, color_value);
	}
	if (sel == 3) {
		/* Layer UI2 */
		tmp = read_wvalue(UI2_ATTR);
		tmp |= (0x1 << 4);
		write_wvalue(UI2_ATTR, tmp);
		write_wvalue(UI2_FILLC, color_value);
	}
	return 0;
}


/*
 * ROP2 cmd register set
 * Index0 is selected
 * dst mapping ch0'
 * src mapping ch1'
 */
__s32 g2d_rop2_set(__u32 rop_cmd)
{
	if (rop_cmd == G2D_BLT_BLACKNESS) {
		/* blackness */
		/* tmpue = 0x1<<18; */
		write_wvalue(ROP_INDEX0, 0x40000);
	} else if (rop_cmd == G2D_BLT_NOTMERGEPEN) {
		/* ~(dst | src) */
		/* tmpue = (0x1<<6) | (0x1<<10) | (0x2<<11) | (0x1<<18); */
		write_wvalue(ROP_INDEX0, 0x41440);
	} else if (rop_cmd == G2D_BLT_MASKNOTPEN) {
		/* ~src&dst */
		/* tmpue = (0x1<<4) | (0x0<<10) | (0x2<<11) | (0x1<<18); */
		write_wvalue(ROP_INDEX0, 0x41010);
	} else if (rop_cmd == G2D_BLT_NOTCOPYPEN) {
		/* ~src */
		/* tmpue = (0x1<<4) | (0x2<<6) | (0x2<<11) |
		 * (0x1<<18) | (0x1<<17);
		 */
		write_wvalue(ROP_INDEX0, 0x61090);
	} else if (rop_cmd == G2D_BLT_MASKPENNOT) {
		/* src&~dst */
		/* tmpue = (0x1<<3) | (0x0<<10) | (0x2<<11) | (0x1<<18); */
		write_wvalue(ROP_INDEX0, 0x41008);
	} else if (rop_cmd == G2D_BLT_NOT) {
		/* ~dst */
		/* tmpue = (0x1<<3) | (0x2<<6) | (0x2<<11) |
		 * (0x1<<18) | (0x1<<16);
		 */
		write_wvalue(ROP_INDEX0, 0x51088);
	} else if (rop_cmd == G2D_BLT_XORPEN) {
		/* src xor dst */
		/* tmpue = (0x2<<6) | (0x2<<11) | (0x1<<18); */
		write_wvalue(ROP_INDEX0, 0x41080);
	} else if (rop_cmd == G2D_BLT_NOTMASKPEN) {
		/* ~(src & dst) */
		/* tmpue = (0x0<<6) | (0x1<<10) | (0x2<<11) | (0x1<<18); */
		write_wvalue(ROP_INDEX0, 0x41400);
	} else if (rop_cmd == G2D_BLT_MASKPEN) {
		/* src&dst */
		/* tmpue = (0x0<<6) | (0x2<<11) | (0x1<<18); */
		write_wvalue(ROP_INDEX0, 0x41000);
	} else if (rop_cmd == G2D_BLT_NOTXORPEN) {
		/* ~(src xor dst) */
		/* tmpue = (0x2<<6) | (0x1<<10) | (0x2<<11) | (0x1<<18); */
		write_wvalue(ROP_INDEX0, 0x41480);
	} else if (rop_cmd == G2D_BLT_NOP) {
		/* dst */
		/* tmpue = (0x2<<6) | (0x2<<11) | (0x1<<18) | (0x1<<16); */
		write_wvalue(ROP_INDEX0, 0x51080);
	} else if (rop_cmd == G2D_BLT_MERGENOTPEN) {
		/* ~dst or src */
		/* tmpue = (0x1<<3)| (0x1<<6) | (0x2<<11) | (0x1<<18) */
		/* write_wvalue(ROP_INDEX0, 0x40A20); */
		write_wvalue(ROP_INDEX0, 0x41048);
	} else if (rop_cmd == G2D_BLT_COPYPEN) {
		/* src */
		/* tmpue = (0x2<<6) | (0x2<<11) | (0x1<<18) | (0x1<<17); */
		write_wvalue(ROP_INDEX0, 0x61080);
	} else if (rop_cmd == G2D_BLT_MERGEPENNOT) {
		/* src or ~dst */
		/* tmpue =  (0x1<<3)| (0x1<<6) | (0x2<<11) | (0x1<<18) */
		write_wvalue(ROP_INDEX0, 0x41048);
	} else if (rop_cmd == G2D_BLT_MERGEPEN) {
		/* src or dst */
		/* tmpue = (0x1<<6) | (0x1<<18) | (0x2<<11); */
		write_wvalue(ROP_INDEX0, 0x41040);
	} else if (rop_cmd == G2D_BLT_WHITENESS) {
		/* whiteness */
		/* tmpue = (0x1<<18) | (0x1<<15); */
		write_wvalue(ROP_INDEX0, 0x48000);
	} else
		return -1;
	return 0;
}


/*
 * ROP3 cmd register set
 * dst mapping ch0'
 * src mapping ch1'
 * ptn mapping ch2'
 * -1 return meaning that the operate is not supported by now
 */
__s32 g2d_rop3_set(__u32 sel, __u32 rop3_cmd)
{
	__u32 addr;

	if (sel == 0)
		addr = ROP_INDEX0;
	else if (sel == 1)
		addr = ROP_INDEX1;

	else
		return -1;
	if (rop3_cmd == G2D_ROP3_BLACKNESS) {
		/* blackness */
		/* 0x1<<18; */
		write_wvalue(addr, 0x40000);
	} else if (rop3_cmd == G2D_ROP3_NOTSRCERASE) {
		/* (~src) AND (~dst) */
		/* (0x1<<3) | (0x1<<4) | (0x1<<18) | (0x2<<11); */
		write_wvalue(addr, 0x41018);
	} else if (rop3_cmd == G2D_ROP3_NOTSRCCOPY) {

		/* ~src */
		/* (0x1<<4) | (0x2<<6) | (0x2<<11) | (0x1<<18) | (0x1<<16); */
		write_wvalue(addr, 0x51090);
	} else if (rop3_cmd == G2D_ROP3_SRCERASE) {
		/* src AND ~dst */
		/* (0x1<<3) | (0x0<<6) | (0x2<<11) | (0x1<<18); */
		write_wvalue(addr, 0x41008);
	} else if (rop3_cmd == G2D_ROP3_DSTINVERT) {
		/* ~dst */
		/* (0x1<<3) | (0x2<<6) | (0x2<<11) | (0x1<<18) | (0x1<<17); */
		write_wvalue(addr, 0x61088);
	} else if (rop3_cmd == G2D_ROP3_PATINVERT) {
		/* ptn XOR dst */
		/* (0x2<<6) | (0x2<<11) | (0x1<<17) */
		write_wvalue(addr, 0x21080);
	} else if (rop3_cmd == G2D_ROP3_SRCINVERT) {
		/* src XOR dst */
		/* (0x2<<6) | (0x2<<11) | (0x1<<18); */
		write_wvalue(addr, 0x41080);
	} else if (rop3_cmd == G2D_ROP3_SRCAND) {
		/* src AND dst */
		/* (0x0<<6) | (0x2<<11) | (0x1<<18); */
		write_wvalue(addr, 0x41000);
	} else if (rop3_cmd == G2D_ROP3_MERGEPAINT) {
		/* ~src OR dst */
		/* (0x1<<4) | (0x1<<6) | (0x2<<11) | (0x1<<18); */
		write_wvalue(addr, 0x41050);
	} else if (rop3_cmd == G2D_ROP3_MERGECOPY) {
		/* src AND pattern */
		/* (0x2<<6) | (0x1<<16) */
		write_wvalue(addr, 0x10080);
	} else if (rop3_cmd == G2D_ROP3_SRCCOPY) {
		/* src */
		/* (0x2<<6) | (0x2<<11) | (0x1<<18) | (0x1<<16); */
		write_wvalue(addr, 0x51080);
	} else if (rop3_cmd == G2D_ROP3_SRCPAINT) {
		/* src OR dst */
		/* (0x1<<6) | (0x2<<11) | (0x1<<18); */
		write_wvalue(addr, 0x41040);
	} else if (rop3_cmd == G2D_ROP3_PATCOPY) {
		/* ptn */
		/* (0x1<<16) | (0x1<<17) | (0x2)<<11 */
		write_wvalue(addr, 0x31000);
	} else if (rop3_cmd == G2D_ROP3_PATPAINT) {
		/* DPSnoo */
		/* (0x1<<3) | (0x1<<6) | (0x1<<11) */
		write_wvalue(addr, 0x848);
	} else if (rop3_cmd == G2D_ROP3_WHITENESS) {
		/* whiteness */
		write_wvalue(addr, 0x48000);
	} else
		return -1;
	return 0;
}

/*
 * background color set
 */
__s32 g2d_bk_set(__u32 color)
{
	write_wvalue(BLD_BK_COLOR, color & 0xffffffff);
	return 0;
}

/*
 * function       : g2d_vsu_calc_fir_coef(unsigned int step)
 * description    : set fir coefficients
 * parameters     :
 *                  step		<horizontal scale ratio of vsu>
 * return         :
 *                  offset (in word) of coefficient table
 */
static __u32 g2d_vsu_calc_fir_coef(__u32 step)
{
	__u32 pt_coef;
	__u32 scale_ratio, int_part, float_part, fir_coef_ofst;

	scale_ratio = step >> (VSU_PHASE_FRAC_BITWIDTH - 3);
	int_part = scale_ratio >> 3;
	float_part = scale_ratio & 0x7;
	fir_coef_ofst = (int_part == 0) ? VSU_ZOOM0_SIZE :
	    (int_part == 1) ? VSU_ZOOM0_SIZE + float_part :
	    (int_part ==
	     2) ? VSU_ZOOM0_SIZE + VSU_ZOOM1_SIZE +
	    (float_part >> 1) : (int_part ==
				  3) ? VSU_ZOOM0_SIZE + VSU_ZOOM1_SIZE +
	    VSU_ZOOM2_SIZE : (int_part ==
			       4) ? VSU_ZOOM0_SIZE + VSU_ZOOM1_SIZE +
	    VSU_ZOOM2_SIZE + VSU_ZOOM3_SIZE : VSU_ZOOM0_SIZE +
	    VSU_ZOOM1_SIZE + VSU_ZOOM2_SIZE + VSU_ZOOM3_SIZE + VSU_ZOOM4_SIZE;
	pt_coef = fir_coef_ofst * VSU_PHASE_NUM;
	return pt_coef;
}

__s32 g2d_rop_by_pass(__u32 sel)
{
	if (sel == 0)
		write_wvalue(ROP_CTL, 0xF0);

	else if (sel == 1)
		write_wvalue(ROP_CTL, 0x55F0);

	else if (sel == 2)
		write_wvalue(ROP_CTL, 0xAAF0);

	else
		return -1;
	return 0;
}

__s32 g2d_vsu_para_set(__u32 fmt, __u32 in_w, __u32 in_h, __u32 out_w,
			   __u32 out_h, __u8 alpha)
{
	__u32 i;
	__u64 tmp, temp;
	__u32 yhstep, yvstep;
	__u32 incw, inch;
	__u32 yhcoef_offset, yvcoef_offset, chcoef_offset;
	__u32 format;

	if (fmt > G2D_FORMAT_IYUV422_Y1U0Y0V0)
		write_wvalue(VS_CTRL, 0x10101);

	else
		write_wvalue(VS_CTRL, 0x00000101);
	tmp = ((out_h - 1) << 16) | (out_w - 1);
	write_wvalue(VS_OUT_SIZE, tmp);
	write_wvalue(VS_GLB_ALPHA, alpha & 0xff);
	write_wvalue(VS_Y_SIZE, ((in_h - 1) << 16) | (in_w - 1));
	temp = in_w << VSU_PHASE_FRAC_BITWIDTH;
	if (out_w)
		do_div(temp, out_w);

	    /* temp = temp/out_w; */
	else
		temp = 0;
	yhstep = temp;
	write_wvalue(VS_Y_HSTEP, yhstep << 1);
	temp = in_h << VSU_PHASE_FRAC_BITWIDTH;
	if (out_h)
		do_div(temp, out_h);
	else
		temp = 0;
	yvstep = temp;
	write_wvalue(VS_Y_VSTEP, yvstep << 1);
	yhcoef_offset = g2d_vsu_calc_fir_coef(yhstep);
	for (i = 0; i < VSU_PHASE_NUM; i++) {
		write_wvalue(VS_Y_HCOEF0 + (i << 2),
			      lan2coefftab32_full[yhcoef_offset + i]);
	}
	yvcoef_offset = g2d_vsu_calc_fir_coef(yvstep);
	switch (fmt) {
	case G2D_FORMAT_IYUV422_V0Y1U0Y0:
	case G2D_FORMAT_IYUV422_Y1V0Y0U0:
	case G2D_FORMAT_IYUV422_U0Y1V0Y0:
	case G2D_FORMAT_IYUV422_Y1U0Y0V0:{
			incw = (in_w + 1) >> 1;
			inch = in_h;
			format = VSU_FORMAT_YUV422;
			write_wvalue(VS_C_SIZE,
				      ((inch - 1) << 16) | (incw - 1));

			    /* chstep = yhstep>>1 cvstep = yvstep */
			    write_wvalue(VS_C_HSTEP, yhstep);
			write_wvalue(VS_C_VSTEP, yvstep << 1);
			chcoef_offset = g2d_vsu_calc_fir_coef(yhstep >> 1);
			for (i = 0; i < VSU_PHASE_NUM; i++)
				write_wvalue(VS_C_HCOEF0 + (i << 2),
					      lan2coefftab32_full[chcoef_offset
								  + i]);
			for (i = 0; i < VSU_PHASE_NUM; i++)
				write_wvalue(VS_Y_VCOEF0 + (i << 2),
					      linearcoefftab32[i]);
			break;
		}
	case G2D_FORMAT_YUV422UVC_V1U1V0U0:
	case G2D_FORMAT_YUV422UVC_U1V1U0V0:
	case G2D_FORMAT_YUV422_PLANAR:{
			incw = (in_w + 1) >> 1;
			inch = in_h;
			format = VSU_FORMAT_YUV422;
			write_wvalue(VS_C_SIZE,
				      ((inch - 1) << 16) | (incw - 1));

			    /* chstep = yhstep>>1 cvstep = yvstep>>1 */
			    write_wvalue(VS_C_HSTEP, yhstep);
			write_wvalue(VS_C_VSTEP, yvstep << 1);
			chcoef_offset = g2d_vsu_calc_fir_coef(yhstep >> 1);
			for (i = 0; i < VSU_PHASE_NUM; i++)
				write_wvalue(VS_C_HCOEF0 + (i << 2),
					      lan2coefftab32_full[chcoef_offset
								  + i]);
			for (i = 0; i < VSU_PHASE_NUM; i++)
				write_wvalue(VS_Y_VCOEF0 + (i << 2),
					      lan2coefftab32_full[yvcoef_offset
								  + i]);
			break;
		}
	case G2D_FORMAT_YUV420_PLANAR:
	case G2D_FORMAT_YUV420UVC_V1U1V0U0:
	case G2D_FORMAT_YUV420UVC_U1V1U0V0:{
			incw = (in_w + 1) >> 1;
			inch = (in_h + 1) >> 1;
			format = VSU_FORMAT_YUV420;
			write_wvalue(VS_C_SIZE,
				      ((inch - 1) << 16) | (incw - 1));
			write_wvalue(VS_C_HSTEP, yhstep);
			write_wvalue(VS_C_VSTEP, yvstep);
			chcoef_offset = g2d_vsu_calc_fir_coef(yhstep >> 1);
			for (i = 0; i < VSU_PHASE_NUM; i++)
				write_wvalue(VS_C_HCOEF0 + (i << 2),
					      lan2coefftab32_full[chcoef_offset
								  + i]);
			for (i = 0; i < VSU_PHASE_NUM; i++)
				write_wvalue(VS_Y_VCOEF0 + (i << 2),
					      lan2coefftab32_full[yvcoef_offset
								  + i]);
			break;
		}
	case G2D_FORMAT_YUV411_PLANAR:
	case G2D_FORMAT_YUV411UVC_V1U1V0U0:
	case G2D_FORMAT_YUV411UVC_U1V1U0V0:{
			incw = (in_w + 3) >> 2;
			inch = in_h;
			format = VSU_FORMAT_YUV411;
			write_wvalue(VS_C_SIZE,
				      ((inch - 1) << 16) | (incw - 1));

			    /* chstep = yhstep>>2 cvstep = yvstep */
			    write_wvalue(VS_C_HSTEP, yhstep >> 1);
			write_wvalue(VS_C_VSTEP, yvstep << 1);
			chcoef_offset = g2d_vsu_calc_fir_coef(yhstep >> 2);
			for (i = 0; i < VSU_PHASE_NUM; i++)
				write_wvalue(VS_C_HCOEF0 + (i << 2),
					      lan2coefftab32_full[chcoef_offset
								  + i]);
			for (i = 0; i < VSU_PHASE_NUM; i++)
				write_wvalue(VS_Y_VCOEF0 + (i << 2),
					      lan2coefftab32_full[yvcoef_offset
								  + i]);
			break;
		}
	default:
		format = VSU_FORMAT_RGB;
		incw = in_w;
		inch = in_h;
		write_wvalue(VS_C_SIZE, ((inch - 1) << 16) | (incw - 1));

		    /* chstep = yhstep cvstep = yvstep */
		    write_wvalue(VS_C_HSTEP, yhstep << 1);
		write_wvalue(VS_C_VSTEP, yvstep << 1);
		chcoef_offset = g2d_vsu_calc_fir_coef(yhstep);
		for (i = 0; i < VSU_PHASE_NUM; i++)
			write_wvalue(VS_C_HCOEF0 + (i << 2),
				      lan2coefftab32_full[chcoef_offset + i]);
		for (i = 0; i < VSU_PHASE_NUM; i++)
			write_wvalue(VS_Y_VCOEF0 + (i << 2),
				      linearcoefftab32[i]);
		break;
	}
	if (format == VSU_FORMAT_YUV420) {

		/*
		 * yhphase = 0;
		 * yvphase = 0;
		 * chphase = 0xFFFE0000;
		 * cvphase = 0xFFFE0000;
		 */
		write_wvalue(VS_Y_HPHASE, 0);
		write_wvalue(VS_Y_VPHASE0, 0);
		write_wvalue(VS_C_HPHASE, 0xFFFc0000);
		write_wvalue(VS_C_VPHASE0, 0xFFFc0000);
	}

	else {
		write_wvalue(VS_Y_HPHASE, 0);
		write_wvalue(VS_Y_VPHASE0, 0);
		write_wvalue(VS_C_HPHASE, 0);
		write_wvalue(VS_C_VPHASE0, 0);
	}
	if (fmt >= G2D_FORMAT_IYUV422_Y1U0Y0V0)
		write_wvalue(VS_CTRL, 0x10001);

	else
		write_wvalue(VS_CTRL, 0x00001);
	return 0;
}


__s32 g2d_calc_coarse(__u32 format, __u32 inw, __u32 inh, __u32 outw,
			  __u32 outh, __u32 *midw, __u32 *midh)
{
	__u32 tmp;

	switch (format) {
	case G2D_FORMAT_IYUV422_V0Y1U0Y0:
	case G2D_FORMAT_IYUV422_Y1V0Y0U0:
	case G2D_FORMAT_IYUV422_U0Y1V0Y0:
	case G2D_FORMAT_IYUV422_Y1U0Y0V0:{
		/* interleaved YUV422 format */
		*midw = inw;
		*midh = inh;
		break;
	}
	case G2D_FORMAT_YUV422UVC_V1U1V0U0:
	case G2D_FORMAT_YUV422UVC_U1V1U0V0:
	case G2D_FORMAT_YUV422_PLANAR:{
		if (inw >= (outw << 3)) {
			*midw = outw << 3;
			tmp = (*midw << 16) | inw;
			write_wvalue(V0_HDS_CTL0, tmp);
			tmp = (*midw << 15) | ((inw + 1) >> 1);
			write_wvalue(V0_HDS_CTL1, tmp);
		} else
			*midw = inw;
		if (inh >= (outh << 2)) {
			*midh = (outh << 2);
			tmp = (*midh << 16) | inh;
			write_wvalue(V0_VDS_CTL0, tmp);
			write_wvalue(V0_VDS_CTL1, tmp);
		} else
			*midh = inh;
		break;
	}
	case G2D_FORMAT_YUV420_PLANAR:
	case G2D_FORMAT_YUV420UVC_V1U1V0U0:
	case G2D_FORMAT_YUV420UVC_U1V1U0V0:{
		if (inw >= (outw << 3)) {
			*midw = outw << 3;
			tmp = (*midw << 16) | inw;
			write_wvalue(V0_HDS_CTL0, tmp);
			tmp = (*midw << 15) | ((inw + 1) >> 1);
			write_wvalue(V0_HDS_CTL0, tmp);
		} else
			*midw = inw;
		if (inh >= (outh << 2)) {
			*midh = (outh << 2);
			tmp = (*midh << 16) | inh;
			write_wvalue(V0_VDS_CTL0, tmp);
			tmp = (*midh << 15) | ((inh + 1) >> 1);
			write_wvalue(V0_VDS_CTL1, tmp);
		} else
			*midh = inh;
		break;
	}
	case G2D_FORMAT_YUV411_PLANAR:
	case G2D_FORMAT_YUV411UVC_V1U1V0U0:
	case G2D_FORMAT_YUV411UVC_U1V1U0V0:{
		if (inw >= (outw << 3)) {
			*midw = outw << 3;
			tmp = ((*midw) << 16) | inw;
			write_wvalue(V0_HDS_CTL0, tmp);
			tmp = ((*midw) << 14) | ((inw + 3) >> 2);
			write_wvalue(V0_HDS_CTL1, tmp);
		} else
			*midw = inw;
		if (inh >= (outh << 2)) {
			*midh = (outh << 2);
			tmp = ((*midh) << 16) | inh;
			write_wvalue(V0_VDS_CTL0, tmp);
			write_wvalue(V0_VDS_CTL1, tmp);
		} else
			*midh = inh;
		break;
	}
	default:
		if (inw >= (outw << 3)) {
			*midw = outw << 3;
			tmp = ((*midw) << 16) | inw;
			write_wvalue(V0_HDS_CTL0, tmp);
			write_wvalue(V0_HDS_CTL1, tmp);
		} else
			*midw = inw;
		if (inh >= (outh << 2)) {
			*midh = (outh << 2);
			tmp = ((*midh) << 16) | inh;
			write_wvalue(V0_VDS_CTL0, tmp);
			write_wvalue(V0_VDS_CTL1, tmp);
		} else
			*midh = inh;
		break;
	}
	return 0;
}


/*
 * sel: 0-->pipe0 1-->pipe1 other:error
 */
__s32 g2d_bldin_set(__u32 sel, g2d_rect rect, int premul)
{
	__u32 tmp;
	__u32 offset;

	if (sel == 0) {
		offset = 0;
		tmp = read_wvalue(BLD_EN_CTL);
		tmp |= 0x1 << 8;
		write_wvalue(BLD_EN_CTL, tmp);
		if (premul) {
			tmp = read_wvalue(BLD_PREMUL_CTL);
			tmp |= 0x1;
			write_wvalue(BLD_PREMUL_CTL, tmp);
		}
	} else if (sel == 1) {
		offset = 0x4;
		tmp = read_wvalue(BLD_EN_CTL);
		tmp |= 0x1 << 9;
		write_wvalue(BLD_EN_CTL, tmp);
		if (premul) {
			tmp = read_wvalue(BLD_PREMUL_CTL);
			tmp |= 0x1 << 1;
			write_wvalue(BLD_PREMUL_CTL, tmp);
		}
	} else
		return -1;
	tmp = ((rect.h - 1) << 16) | (rect.w - 1);

	G2D_INFO_MSG("BLD_CH_ISIZE W:  0x%x\n", rect.w);
	G2D_INFO_MSG("BLD_CH_ISIZE H:  0x%x\n", rect.h);

	write_wvalue(BLD_CH_ISIZE0 + offset, tmp);
	tmp =
	    ((rect.y <= 0 ? 0 : rect.y - 1) << 16) | (rect.x <=
						      0 ? 0 : rect.x - 1);

	G2D_INFO_MSG("BLD_CH_ISIZE X:  0x%x\n", rect.x);
	G2D_INFO_MSG("BLD_CH_ISIZE Y:  0x%x\n", rect.y);

	write_wvalue(BLD_CH_OFFSET0 + offset, tmp);
	return 0;
}

/*
 * set the bld color space based on the format
 * if the format is UI, then set the bld in RGB color space
 * if the format is Video, then set the bld in YUV color space
 */
__s32 g2d_bld_cs_set(__u32 format)
{
	__u32 tmp;

	if (format <= G2D_FORMAT_BGRA1010102) {
		tmp = read_wvalue(BLD_OUT_COLOR);
		tmp &= 0xFFFFFFFD;
		write_wvalue(BLD_OUT_COLOR, tmp);
	} else if (format <= G2D_FORMAT_YUV411_PLANAR) {
		tmp = read_wvalue(BLD_OUT_COLOR);
		tmp |= 0x1 << 1;
		write_wvalue(BLD_OUT_COLOR, tmp);
	} else
		return -1;
	return 0;
}

__s32 mixer_fillrectangle(g2d_fillrect *para)
{
	g2d_image_enh src_tmp, dst_tmp;
	g2d_image_enh *src = &src_tmp;
	g2d_image_enh *dst = &dst_tmp;
	__s32 result;

	g2d_mixer_reset();
	if (para->flag == G2D_FIL_NONE) {
		pr_info("fc only!\n");
		dst->bbuff = 1;
		dst->color = para->color;
		dst->format =
			g2d_format_trans(para->dst_image.format,
					para->dst_image.pixel_seq);
		dst->laddr[0] = para->dst_image.addr[0];
		dst->laddr[1] = para->dst_image.addr[1];
		dst->laddr[2] = para->dst_image.addr[2];
		dst->width = para->dst_image.w;
		dst->height = para->dst_image.h;
		dst->clip_rect.x = para->dst_rect.x;
		dst->clip_rect.y = para->dst_rect.y;
		dst->clip_rect.w = para->dst_rect.w;
		dst->clip_rect.h = para->dst_rect.h;
		dst->gamut = G2D_BT709;
		dst->alpha = para->alpha;
		dst->mode = 0;
		result = g2d_fillrectangle(dst, dst->color);
		return result;
	}
	dst->bbuff = 1;
	dst->color = para->color;
	dst->format =
		g2d_format_trans(para->dst_image.format,
				para->dst_image.pixel_seq);
	dst->laddr[0] = para->dst_image.addr[0];
	dst->laddr[1] = para->dst_image.addr[1];
	dst->laddr[2] = para->dst_image.addr[2];
	dst->width = para->dst_image.w;
	dst->height = para->dst_image.h;
	dst->clip_rect.x = para->dst_rect.x;
	dst->clip_rect.y = para->dst_rect.y;
	dst->clip_rect.w = para->dst_rect.w;
	dst->clip_rect.h = para->dst_rect.h;
	dst->gamut = G2D_BT709;
	dst->alpha = para->alpha;
	if (para->flag & G2D_FIL_PIXEL_ALPHA)
		dst->mode = G2D_PIXEL_ALPHA;
	if (para->flag & G2D_FIL_PLANE_ALPHA)
		dst->mode = G2D_GLOBAL_ALPHA;
	if (para->flag & G2D_FIL_MULTI_ALPHA)
		dst->mode = G2D_MIXER_ALPHA;
	src->bbuff = 0;
	src->color = para->color;
	src->format = dst->format;
	src->gamut = G2D_BT709;
	src->format = 0;
	dst->laddr[0] = para->dst_image.addr[0];
	src->width = para->dst_image.w;
	src->height = para->dst_image.h;
	src->clip_rect.x = para->dst_rect.x;
	src->clip_rect.y = para->dst_rect.y;
	src->clip_rect.w = para->dst_rect.w;
	src->clip_rect.h = para->dst_rect.h;
	result = g2d_bsp_bld(src, dst, G2D_BLD_DSTOVER, NULL);
	return result;
}

__s32 g2d_fillrectangle(g2d_image_enh *dst, __u32 color_value)
{
	g2d_rect rect0;
	__u32 tmp;
	__s32 result;

	g2d_bsp_reset();
	/* set the input layer */
	g2d_vlayer_set(0, dst);
	/* set the fill color value */
	g2d_fc_set(0, color_value);
	if (dst->format >= G2D_FORMAT_IYUV422_V0Y1U0Y0) {
		g2d_vsu_para_set(dst->format, dst->clip_rect.w,
				  dst->clip_rect.h, dst->clip_rect.w,
				  dst->clip_rect.h, 0xff);
		g2d_csc_reg_set(1, G2D_RGB2YUV_709);
	}

	/* for interleaved test */
	if ((dst->format >= G2D_FORMAT_IYUV422_V0Y1U0Y0)
			&& (dst->format <= G2D_FORMAT_IYUV422_Y1U0Y0V0)) {
		g2d_csc_reg_set(0, G2D_RGB2YUV_709);
		g2d_csc_reg_set(2, G2D_RGB2YUV_709);
		write_wvalue(BLD_CSC_CTL, 0x2);
		g2d_bk_set(0xff123456);
		porter_duff(G2D_BLD_SRCOVER);
		write_wvalue(BLD_FILLC0, 0x00108080);
		write_wvalue(BLD_FILLC1, 0x00108080);
		write_wvalue(UI0_FILLC, 0xffffffff);
		write_wvalue(UI1_FILLC, 0xffffffff);
	}
	rect0.x = 0;
	rect0.y = 0;
	rect0.w = dst->clip_rect.w;
	rect0.h = dst->clip_rect.h;
	g2d_bldin_set(0, rect0, dst->bpremul);
	g2d_bld_cs_set(dst->format);

	/* ROP sel ch0 pass */
	write_wvalue(ROP_CTL, 0xf0);
	g2d_wb_set(dst);

	/* start the module */
	mixer_irq_enable();
	tmp = read_wvalue(G2D_MIXER_CTL);
	tmp |= 0x80000000;
	G2D_INFO_MSG("INIT_MODULE: 0x%x\n", tmp);
	write_wvalue(G2D_MIXER_CTL, tmp);

	result = g2d_wait_cmd_finish();
	return result;
}


/*
 * src:source
 * ptn:pattern
 * dst:destination
 * mask:mask
 * if mask is set to NULL, do ROP3 among src, ptn, and dst using the
 * fore_flag
 */
__s32 g2d_bsp_maskblt(g2d_image_enh *src, g2d_image_enh *ptn,
			  g2d_image_enh *mask, g2d_image_enh *dst,
			  __u32 back_flag, __u32 fore_flag)
{
	__u32 tmp;
	g2d_rect rect0;
	bool b_pre;
	__s32 result;

	/* int b_pre; */
	g2d_bsp_reset();
	if (dst == NULL)
		return -1;
	if (dst->format > G2D_FORMAT_BGRA1010102)
		return -2;
	g2d_vlayer_set(0, dst);
	if (src != NULL) {
		src->clip_rect.w = dst->clip_rect.w;
		src->clip_rect.h = dst->clip_rect.h;
		g2d_uilayer_set(0, src);
	}
	if (ptn != NULL) {
		ptn->clip_rect.w = dst->clip_rect.w;
		ptn->clip_rect.h = dst->clip_rect.h;
		g2d_uilayer_set(1, ptn);
	}
	if (mask != NULL) {
		mask->clip_rect.w = dst->clip_rect.w;
		mask->clip_rect.h = dst->clip_rect.h;
		g2d_uilayer_set(2, mask);

		/* set the ROP4 */
		write_wvalue(ROP_CTL, 0x1);
		g2d_rop3_set(0, back_flag & 0xff);
		g2d_rop3_set(1, fore_flag & 0xff);
	} else {
		write_wvalue(ROP_CTL, 0x0);
		g2d_rop3_set(0, back_flag);
	}
	b_pre = dst->bpremul;
	if (src)
		b_pre |= src->bpremul;
	if (ptn)
		b_pre |= ptn->bpremul;
	if (b_pre) {
		/* some layer is not premul */
		if (!src->bpremul) {
			tmp = read_wvalue(UI0_ATTR);
			tmp |= 0x1 << 16;
			write_wvalue(UI0_ATTR, tmp);
		}
		if (!dst->bpremul) {
			tmp = read_wvalue(V0_ATTCTL);
			tmp |= 0x1 << 16;
			write_wvalue(V0_ATTCTL, tmp);
		}
		if (!ptn->bpremul) {
			tmp = read_wvalue(UI1_ATTR);
			tmp |= 0x1 << 16;
			write_wvalue(UI1_ATTR, tmp);
		}

		    /* set bld in premul data */
		    write_wvalue(BLD_PREMUL_CTL, 0x1);
	}

	/* set bld para */
	rect0.x = 0;
	rect0.y = 0;
	rect0.w = dst->clip_rect.w;
	rect0.h = dst->clip_rect.h;
	g2d_bldin_set(0, rect0, dst->bpremul);
	g2d_wb_set(dst);

	/* start the module */
	mixer_irq_enable();
	tmp = read_wvalue(G2D_MIXER_CTL);
	tmp |= 0x80000000;
	G2D_INFO_MSG("INIT_MODULE: 0x%x\n", tmp);
	write_wvalue(G2D_MIXER_CTL, tmp);

	result = g2d_wait_cmd_finish();
	return result;
}

__s32 g2d_format_trans(__s32 data_fmt, __s32 pixel_seq)
{
	/* transform the g2d format 2 enhance format */
	switch (data_fmt) {
	case G2D_FMT_ARGB_AYUV8888:
		return G2D_FORMAT_ARGB8888;
	case G2D_FMT_BGRA_VUYA8888:
		return G2D_FORMAT_BGRA8888;
	case G2D_FMT_ABGR_AVUY8888:
		return G2D_FORMAT_ABGR8888;
	case G2D_FMT_RGBA_YUVA8888:
		return G2D_FORMAT_RGBA8888;
	case G2D_FMT_XRGB8888:
		return G2D_FORMAT_XRGB8888;
	case G2D_FMT_BGRX8888:
		return G2D_FORMAT_BGRX8888;
	case G2D_FMT_XBGR8888:
		return G2D_FORMAT_XBGR8888;
	case G2D_FMT_RGBX8888:
		return G2D_FORMAT_RGBX8888;
	case G2D_FMT_ARGB4444:
		return G2D_FORMAT_ARGB4444;
	case G2D_FMT_ABGR4444:
		return G2D_FORMAT_ABGR4444;
	case G2D_FMT_RGBA4444:
		return G2D_FORMAT_RGBA4444;
	case G2D_FMT_BGRA4444:
		return G2D_FORMAT_BGRA4444;
	case G2D_FMT_ARGB1555:
		return G2D_FORMAT_ARGB1555;
	case G2D_FMT_ABGR1555:
		return G2D_FORMAT_ABGR1555;
	case G2D_FMT_RGBA5551:
		return G2D_FORMAT_RGBA5551;
	case G2D_FMT_BGRA5551:
		return G2D_FORMAT_BGRA5551;
	case G2D_FMT_RGB565:
		return G2D_FORMAT_RGB565;
	case G2D_FMT_BGR565:
		return G2D_FORMAT_BGR565;
	case G2D_FMT_IYUV422:
		if (pixel_seq == G2D_SEQ_VYUY)
			return G2D_FORMAT_IYUV422_V0Y1U0Y0;
		if (pixel_seq == G2D_SEQ_YVYU)
			return G2D_FORMAT_IYUV422_Y1V0Y0U0;
		return -1;
	case G2D_FMT_PYUV422UVC:
		if (pixel_seq == G2D_SEQ_VUVU)
			return G2D_FORMAT_YUV422UVC_V1U1V0U0;
		return G2D_FORMAT_YUV422UVC_U1V1U0V0;
	case G2D_FMT_PYUV420UVC:
		if (pixel_seq == G2D_SEQ_VUVU)
			return G2D_FORMAT_YUV420UVC_V1U1V0U0;
		return G2D_FORMAT_YUV420UVC_U1V1U0V0;
	case G2D_FMT_PYUV411UVC:
		if (pixel_seq == G2D_SEQ_VUVU)
			return G2D_FORMAT_YUV411UVC_V1U1V0U0;
		return G2D_FORMAT_YUV411UVC_U1V1U0V0;
	case G2D_FMT_PYUV422:
		return G2D_FORMAT_YUV422_PLANAR;
	case G2D_FMT_PYUV420:
		return G2D_FORMAT_YUV420_PLANAR;
	case G2D_FMT_PYUV411:
		return G2D_FORMAT_YUV411_PLANAR;
	default:
		return -1;
	}
}

__s32 mixer_stretchblt(g2d_stretchblt *para,
			   enum g2d_scan_order scan_order)
{
	g2d_image_enh src_tmp, dst_tmp;
	g2d_image_enh *src = &src_tmp, *dst = &dst_tmp;
	g2d_ck ck_para_tmp;
	g2d_ck *ck_para = &ck_para_tmp;
	__s32 result;

	memset(src, 0, sizeof(g2d_image_enh));
	memset(dst, 0, sizeof(g2d_image_enh));
	memset(ck_para, 0, sizeof(g2d_ck));

	src->bbuff = 1;
	src->color = para->color;
	src->format =
		g2d_format_trans(para->src_image.format,
				para->src_image.pixel_seq);
	src->laddr[0] = para->src_image.addr[0];
	src->laddr[1] = para->src_image.addr[1];
	src->laddr[2] = para->src_image.addr[2];
	src->width = para->src_image.w;
	src->height = para->src_image.h;
	src->clip_rect.x = para->src_rect.x;
	src->clip_rect.y = para->src_rect.y;
	src->clip_rect.w = para->src_rect.w;
	src->clip_rect.h = para->src_rect.h;
	src->gamut = G2D_BT709;
	src->alpha = para->alpha;
	dst->bbuff = 1;
	dst->color = para->color;
	dst->format =
		g2d_format_trans(para->dst_image.format,
				para->dst_image.pixel_seq);
	dst->laddr[0] = para->dst_image.addr[0];
	dst->laddr[1] = para->dst_image.addr[1];
	dst->laddr[2] = para->dst_image.addr[2];
	dst->width = para->dst_image.w;
	dst->height = para->dst_image.h;
	dst->clip_rect.x = para->dst_rect.x;
	dst->clip_rect.y = para->dst_rect.y;
	dst->clip_rect.w = para->dst_rect.w;
	dst->clip_rect.h = para->dst_rect.h;
	dst->gamut = G2D_BT709;
	dst->alpha = para->alpha;

	if ((para->flag == G2D_BLT_NONE) ||
	      (para->flag == G2D_BLT_FLIP_HORIZONTAL) ||
	      (para->flag == G2D_BLT_FLIP_VERTICAL) ||
	      (para->flag == G2D_BLT_ROTATE90) ||
	      (para->flag == G2D_BLT_ROTATE180) ||
	      (para->flag == G2D_BLT_ROTATE270) ||
	      (para->flag == G2D_BLT_MIRROR45) ||
	      (para->flag == G2D_BLT_MIRROR135)) {
		/* ROT or scal case */
		switch (para->flag) {
		case G2D_BLT_NONE:
			if ((dst->width == src->width) &&
					(dst->height == src->height)) {
				result = g2d_bsp_bitblt(src, dst, G2D_ROT_0);
				return result;
			}
			if (scan_order == G2D_SM_DTLR)
				result = g2d_bsp_bitblt(src, dst,
						G2D_BLT_NONE | G2D_SM_DTLR_1);
			else
				result = g2d_bsp_bitblt(src, dst, G2D_BLT_NONE);
			return result;
		case G2D_BLT_FLIP_HORIZONTAL:
			result = g2d_bsp_bitblt(src, dst, G2D_ROT_H);
			return result;
		case G2D_BLT_FLIP_VERTICAL:
			result = g2d_bsp_bitblt(src, dst, G2D_ROT_V);
			return result;
		case G2D_BLT_ROTATE90:
			result = g2d_bsp_bitblt(src, dst, G2D_ROT_90);
			return result;
		case G2D_BLT_ROTATE180:
			result = g2d_bsp_bitblt(src, dst, G2D_ROT_180);
			return result;
		case G2D_BLT_ROTATE270:
			result = g2d_bsp_bitblt(src, dst, G2D_ROT_270);
			return result;
		case G2D_BLT_MIRROR45:
			result = g2d_bsp_bitblt(src, dst,
						G2D_ROT_90 | G2D_ROT_H);
			return result;
		case G2D_BLT_MIRROR135:
			result = g2d_bsp_bitblt(src, dst,
						G2D_ROT_90 | G2D_ROT_V);
			return result;
		default:
			return -1;
		}
	} else {
		if (para->flag & 0xfe0) {
			pr_err("Wrong! mixer and rot cant use at same time!\n");
			return -1;
		}
		if (para->flag & G2D_BLT_SRC_PREMULTIPLY)
			src->bpremul = 1;
		if (para->flag & G2D_BLT_DST_PREMULTIPLY)
			dst->bpremul = 1;
		if (para->flag & G2D_BLT_PIXEL_ALPHA) {
			src->mode = G2D_PIXEL_ALPHA;
			dst->mode = G2D_PIXEL_ALPHA;
		}
		if (para->flag & G2D_BLT_PLANE_ALPHA) {
			src->mode = G2D_GLOBAL_ALPHA;
			dst->mode = G2D_GLOBAL_ALPHA;
		}
		if (para->flag & G2D_BLT_MULTI_ALPHA) {
			src->mode = G2D_MIXER_ALPHA;
			dst->mode = G2D_MIXER_ALPHA;
		}
		ck_para->match_rule = 0;
		ck_para->max_color = para->color;
		ck_para->min_color = para->color;

		result = g2d_bsp_bitblt(src, dst, G2D_BLT_NONE);

		return result;
	}
}

__s32 mixer_blt(g2d_blt *para, enum g2d_scan_order scan_order)
{
	g2d_image_enh src_tmp, dst_tmp;
	g2d_image_enh *src = &src_tmp;
	g2d_image_enh *dst = &dst_tmp;
	g2d_ck ck_para_tmp;
	g2d_ck *ck_para = &ck_para_tmp;
	__s32 result;

	memset(src, 0, sizeof(g2d_image_enh));
	memset(dst, 0, sizeof(g2d_image_enh));
	memset(ck_para, 0, sizeof(g2d_ck));

	G2D_INFO_MSG("Input_G2D_Format:  0x%x\n", para->src_image.format);
	G2D_INFO_MSG("BITBLT_flag:  0x%x\n", para->flag);
	G2D_INFO_MSG("inPICWidth:  %d\n", para->src_image.w);
	G2D_INFO_MSG("inPICHeight: %d\n", para->src_image.h);
	G2D_INFO_MSG("inRectX:  %d\n", para->src_rect.x);
	G2D_INFO_MSG("inRectY: %d\n", para->src_rect.y);
	G2D_INFO_MSG("inRectW:  %d\n", para->src_rect.w);
	G2D_INFO_MSG("inRectH: %d\n", para->src_rect.h);
	G2D_INFO_MSG("Output_G2D_Format:  0x%x\n", para->dst_image.format);
	G2D_INFO_MSG("outPICWidth:  %d\n", para->dst_image.w);
	G2D_INFO_MSG("outPICHeight: %d\n", para->dst_image.h);
	G2D_INFO_MSG("outRectX:  %d\n", para->dst_x);
	G2D_INFO_MSG("outRectY: %d\n", para->dst_y);
	src->bbuff = 1;
	src->color = para->color;
	src->format =
			g2d_format_trans(para->src_image.format,
					para->src_image.pixel_seq);
	src->laddr[0] = para->src_image.addr[0];
	src->laddr[1] = para->src_image.addr[1];
	src->laddr[2] = para->src_image.addr[2];
	src->width = para->src_image.w;
	src->height = para->src_image.h;
	src->clip_rect.x = para->src_rect.x;
	src->clip_rect.y = para->src_rect.y;
	src->clip_rect.w = para->src_rect.w;
	src->clip_rect.h = para->src_rect.h;
	src->gamut = G2D_BT709;
	src->alpha = para->alpha;
	dst->bbuff = 1;
	dst->format =
		g2d_format_trans(para->dst_image.format,
						para->dst_image.pixel_seq);
	dst->laddr[0] = para->dst_image.addr[0];
	dst->laddr[1] = para->dst_image.addr[1];
	dst->laddr[2] = para->dst_image.addr[2];
	dst->width = para->dst_image.w;
	dst->height = para->dst_image.h;
	dst->clip_rect.x = para->dst_x;
	dst->clip_rect.y = para->dst_y;
	dst->clip_rect.w = src->clip_rect.w;
	dst->clip_rect.h = src->clip_rect.h;
	dst->gamut = G2D_BT709;
	dst->alpha = para->alpha;

	G2D_INFO_MSG("inPICaddr0: 0x%x\n", src->laddr[0]);
	G2D_INFO_MSG("inPICaddr1: 0x%x\n", src->laddr[1]);
	G2D_INFO_MSG("inPICaddr2: 0x%x\n", src->laddr[2]);
	G2D_INFO_MSG("outPICaddr0: 0x%x\n", dst->laddr[0]);
	G2D_INFO_MSG("outPICaddr1: 0x%x\n", dst->laddr[1]);
	G2D_INFO_MSG("outPICaddr2: 0x%x\n", dst->laddr[2]);

	if ((para->flag == G2D_BLT_NONE) ||
	      (para->flag == G2D_BLT_FLIP_HORIZONTAL) ||
	      (para->flag == G2D_BLT_FLIP_VERTICAL) ||
	      (para->flag == G2D_BLT_ROTATE90) ||
	      (para->flag == G2D_BLT_ROTATE180) ||
	      (para->flag == G2D_BLT_ROTATE270) ||
	      (para->flag == G2D_BLT_MIRROR45) ||
	      (para->flag == G2D_BLT_MIRROR135)) {
		/* ROT case */
		switch (para->flag) {
		case G2D_BLT_NONE:
			if ((dst->width == src->width) &&
					(dst->height == src->height)) {
				result = g2d_bsp_bitblt(src, dst, G2D_ROT_0);
				return result;
			}
			if (scan_order == G2D_SM_DTLR)
				result = g2d_bsp_bitblt(src, dst,
						G2D_BLT_NONE | G2D_SM_DTLR_1);
			else
				result = g2d_bsp_bitblt(src, dst, G2D_BLT_NONE);
			return result;
		case G2D_BLT_FLIP_HORIZONTAL:
			result = g2d_bsp_bitblt(src, dst, G2D_ROT_H);
			return result;
		case G2D_BLT_FLIP_VERTICAL:
			result = g2d_bsp_bitblt(src, dst, G2D_ROT_V);
			return result;
		case G2D_BLT_ROTATE90:
			result = g2d_bsp_bitblt(src, dst, G2D_ROT_90);
			return result;
		case G2D_BLT_ROTATE180:
			result = g2d_bsp_bitblt(src, dst, G2D_ROT_180);
			return result;
		case G2D_BLT_ROTATE270:
			result = g2d_bsp_bitblt(src, dst, G2D_ROT_270);
			return result;
		case G2D_BLT_MIRROR45:
			result = g2d_bsp_bitblt(src, dst,
						G2D_ROT_90 | G2D_ROT_H);
			return result;
		case G2D_BLT_MIRROR135:
			result = g2d_bsp_bitblt(src, dst,
						G2D_ROT_90 | G2D_ROT_V);
			return result;
		default:
			return -1;
		}
	} else {
		if (para->flag & 0xfe0) {
			pr_err("Wrong! mixer and rot cant use at same time!\n");
			return -1;
		}
		if (para->flag & G2D_BLT_SRC_PREMULTIPLY)
			src->bpremul = 1;
		if (para->flag & G2D_BLT_DST_PREMULTIPLY)
			dst->bpremul = 1;
		if (para->flag & G2D_BLT_PIXEL_ALPHA) {
			src->mode = G2D_PIXEL_ALPHA;
			dst->mode = G2D_PIXEL_ALPHA;
		}
		if (para->flag & G2D_BLT_PLANE_ALPHA) {
			src->mode = G2D_GLOBAL_ALPHA;
			dst->mode = G2D_GLOBAL_ALPHA;
		}
		if (para->flag & G2D_BLT_MULTI_ALPHA) {
			src->mode = G2D_MIXER_ALPHA;
			dst->mode = G2D_MIXER_ALPHA;
		}
		ck_para->match_rule = 0;
		ck_para->max_color = para->color;
		ck_para->min_color = para->color;

		result = g2d_bsp_bld(src, dst, G2D_BLD_DSTOVER, NULL);

		return result;
	}
}

__s32 g2d_bsp_bitblt(g2d_image_enh *src, g2d_image_enh *dst, __u32 flag)
{
	g2d_rect rect0, rect1;
	bool bpre;
	__u32 ycnt, ucnt, vcnt;
	__u32 pitch0, pitch1, pitch2;
	__u64 addr0, addr1, addr2;
	__u32 midw, midh;
	__u32 tmp;
	__u32 ch, cw, cy, cx;
	__s32 result;

	if (dst == NULL) {
		pr_err("[G2D]dst image is NULL!\n");
		return -1;
	}
	if (src == NULL) {
		pr_err("[G2D]src image is NULL!\n");
		return -2;
	}
	G2D_INFO_MSG("BITBLT_flag:  0x%x\n", flag);
	if (G2D_BLT_NONE == (flag & 0x0fffffff)) {
		G2D_INFO_MSG("Input info:---------------------------------\n");
		G2D_INFO_MSG("Src_fd:  %d\n", src->fd);
		G2D_INFO_MSG("Format:  0x%x\n", src->format);
		G2D_INFO_MSG("BITBLT_alpha_mode:  0x%x\n", src->mode);
		G2D_INFO_MSG("BITBLT_alpha_val:  0x%x\n", src->alpha);
		G2D_INFO_MSG("inClipRectX:  %d\n", src->clip_rect.x);
		G2D_INFO_MSG("inClipRectY: %d\n", src->clip_rect.y);
		G2D_INFO_MSG("inClipRectW:  %d\n", src->clip_rect.w);
		G2D_INFO_MSG("inClipRectH: %d\n", src->clip_rect.h);

		G2D_INFO_MSG("Output info:--------------------------------\n");
		G2D_INFO_MSG("Dst_fd:  %d\n", dst->fd);
		G2D_INFO_MSG("Format:  0x%x\n", dst->format);
		G2D_INFO_MSG("outClipRectX:  %d\n", dst->clip_rect.x);
		G2D_INFO_MSG("outClipRectY: %d\n", dst->clip_rect.y);
		G2D_INFO_MSG("outClipRectW:  %d\n", dst->clip_rect.w);
		G2D_INFO_MSG("outClipRectH: %d\n", dst->clip_rect.h);
		/* single src opt */
		g2d_vlayer_set(0, src);
		if (src->mode) {
			/* need abp process */
			g2d_uilayer_set(2, dst);
		}
		if ((src->format >= G2D_FORMAT_IYUV422_V0Y1U0Y0) ||
				(src->clip_rect.w != dst->clip_rect.w) ||
				(src->clip_rect.h != dst->clip_rect.h)) {
			g2d_calc_coarse(src->format, src->clip_rect.w,
					src->clip_rect.h, dst->clip_rect.w,
					dst->clip_rect.h, &midw, &midh);
			g2d_vsu_para_set(src->format, midw, midh,
					dst->clip_rect.w, dst->clip_rect.h,
					0xff);
		}
		write_wvalue(ROP_CTL, 0xf0);
		/* set bld para */
		rect0.x = 0;
		rect0.y = 0;
		rect0.w = dst->clip_rect.w;
		rect0.h = dst->clip_rect.h;
		g2d_bldin_set(0, rect0, dst->bpremul);
		g2d_bld_cs_set(src->format);
		if (src->mode) {
			/* need abp process */
			rect1.x = 0;
			rect1.y = 0;
			rect1.w = dst->clip_rect.w;
			rect1.h = dst->clip_rect.h;
			g2d_bldin_set(1, rect1, dst->bpremul);
		}
		if ((src->format <= G2D_FORMAT_BGRA1010102) &&
		      (dst->format > G2D_FORMAT_BGRA1010102)) {
			if (dst->clip_rect.w <= 1280 && dst->clip_rect.h <= 720)
				g2d_csc_reg_set(2, G2D_RGB2YUV_601);
			else
				g2d_csc_reg_set(2, G2D_RGB2YUV_709);
		}
		if ((src->format > G2D_FORMAT_BGRA1010102) &&
		      (dst->format <= G2D_FORMAT_BGRA1010102)) {
			if (dst->clip_rect.w <= 1280 && dst->clip_rect.h <= 720)
				g2d_csc_reg_set(2, G2D_YUV2RGB_601);
			else
				g2d_csc_reg_set(2, G2D_YUV2RGB_709);
		}
		g2d_wb_set(dst);
	}

	else if (flag & 0xff) {
		/* ROP2 operate */
		if ((src->format > G2D_FORMAT_BGRA1010102) | (dst->format >
					G2D_FORMAT_BGRA1010102))
			return -3;
		g2d_uilayer_set(0, dst);
		g2d_vlayer_set(0, src);

		/* bpre = 0; */
		bpre = false;
		if (src->bpremul || dst->bpremul) {
			bpre = true;
			/* bpre = 1; */
			/* some layer is premul */
			if (!src->bpremul) {
				tmp = read_wvalue(V0_ATTCTL);
				tmp |= 0x1 << 16;
				write_wvalue(V0_ATTCTL, tmp);
			}
			if (!dst->bpremul) {
				tmp = read_wvalue(UI0_ATTR);
				tmp |= 0x1 << 16;
				write_wvalue(UI0_ATTR, tmp);
			}
		}
		if ((src->clip_rect.w != dst->clip_rect.w)
		      || (src->clip_rect.h != dst->clip_rect.h)) {
			g2d_calc_coarse(src->format, src->clip_rect.w,
					 src->clip_rect.h, dst->clip_rect.w,
					 dst->clip_rect.h, &midw, &midh);
			g2d_vsu_para_set(src->format, midw, midh,
					   dst->clip_rect.w, dst->clip_rect.h,
					   0xff);
		}
		write_wvalue(ROP_CTL, 0x0);
		g2d_rop2_set(flag & 0xff);
		tmp = read_wvalue(ROP_INDEX0);
		tmp |= 0x2;
		write_wvalue(ROP_INDEX0, tmp);

		/* set bld para */
		rect0.x = 0;
		rect0.y = 0;
		rect0.w = dst->clip_rect.w;
		rect0.h = dst->clip_rect.h;
		g2d_bldin_set(0, rect0, bpre);
		g2d_wb_set(dst);
	}

	else if (flag & 0xff00) {
		/* ROT operate */
		tmp = 1;
		if (flag & G2D_ROT_H)
			tmp |= 0x1 << 7;
		if (flag & G2D_ROT_V)
			tmp |= 0x1 << 6;
		if ((flag & 0xf00) == G2D_ROT_90)
			tmp |= 0x1 << 4;
		if ((flag & 0xf00) == G2D_ROT_180)
			tmp |= 0x2 << 4;
		if ((flag & 0xf00) == G2D_ROT_270)
			tmp |= 0x3 << 4;
		if ((flag & 0xf00) == G2D_ROT_0)
			tmp |= 0x0 << 4;

		G2D_INFO_MSG("ROT input info: ----------------------------\n");
		G2D_INFO_MSG("Src_fd:  %d\n", src->fd);
		G2D_INFO_MSG("Format:  0x%x\n", src->format);
		G2D_INFO_MSG("Flag:  0x%x\n", flag);
		G2D_INFO_MSG("inClipRectX:  %d\n", src->clip_rect.x);
		G2D_INFO_MSG("inClipRectY: %d\n", src->clip_rect.y);
		G2D_INFO_MSG("inClipRectW:  %d\n", src->clip_rect.w);
		G2D_INFO_MSG("inClipRectH: %d\n", src->clip_rect.h);

		write_wvalue(ROT_CTL, tmp);
		write_wvalue(ROT_IFMT, src->format & 0x3F);
		write_wvalue(ROT_ISIZE,
			      ((((src->clip_rect.h -
				  1) & 0x1fff) << 16)) | ((src->clip_rect.w -
							    1) & 0x1fff));

		G2D_INFO_MSG("ROT_IFMT: 0x%x\n", read_wvalue(ROT_IFMT));
		G2D_INFO_MSG("ROT_ISIZE: 0x%x\n", read_wvalue(ROT_ISIZE));
		G2D_INFO_MSG("SRC_align: %d, %d, %d\n",
				src->align[0], src->align[1], src->align[2]);
		G2D_INFO_MSG("DST_align: %d, %d, %d\n",
				dst->align[0], dst->align[1], dst->align[2]);

		if ((src->format >= G2D_FORMAT_YUV422UVC_V1U1V0U0)
			&& (src->format <= G2D_FORMAT_YUV422_PLANAR)) {
			cw = src->width >> 1;
			ch = src->height;
			cx = src->clip_rect.x >> 1;
			cy = src->clip_rect.y;
		}

		else if ((src->format >= G2D_FORMAT_YUV420UVC_V1U1V0U0)
			&& (src->format <= G2D_FORMAT_YUV420_PLANAR)) {
			cw = src->width >> 1;
			ch = src->height >> 1;
			cx = src->clip_rect.x >> 1;
			cy = src->clip_rect.y >> 1;
		}

		else if ((src->format >= G2D_FORMAT_YUV411UVC_V1U1V0U0)
			&& (src->format <= G2D_FORMAT_YUV411_PLANAR)) {
			cw = src->width >> 2;
			ch = src->height;
			cx = src->clip_rect.x >> 2;
			cy = src->clip_rect.y;
		}

		else {
			cw = 0;
			ch = 0;
			cx = 0;
			cy = 0;
		}

		g2d_byte_cal(src->format, &ycnt, &ucnt, &vcnt);
		pitch0 = cal_align(ycnt * src->width, src->align[0]);
		write_wvalue(ROT_IPITCH0, pitch0);
		pitch1 = cal_align(ucnt * cw, src->align[1]);
		write_wvalue(ROT_IPITCH1, pitch1);
		pitch2 = cal_align(vcnt * cw, src->align[2]);
		write_wvalue(ROT_IPITCH2, pitch2);

		G2D_INFO_MSG("ROT_InPITCH: %d, %d, %d\n",
				pitch0, pitch1, pitch2);
		G2D_INFO_MSG("SRC_ADDR0: 0x%x\n", src->laddr[0]);
		G2D_INFO_MSG("SRC_ADDR1: 0x%x\n", src->laddr[1]);
		G2D_INFO_MSG("SRC_ADDR2: 0x%x\n", src->laddr[2]);

		addr0 =
		    src->laddr[0] + ((__u64) src->haddr[0] << 32) +
		    pitch0 * src->clip_rect.y + ycnt * src->clip_rect.x;
		write_wvalue(ROT_ILADD0, addr0 & 0xffffffff);
		write_wvalue(ROT_IHADD0, (addr0 >> 32) & 0xff);
		addr1 =
		    src->laddr[1] + ((__u64) src->haddr[1] << 32) +
		    pitch1 * cy + ucnt * cx;
		write_wvalue(ROT_ILADD1, addr1 & 0xffffffff);
		write_wvalue(ROT_IHADD1, (addr1 >> 32) & 0xff);
		addr2 =
		    src->laddr[2] + ((__u64) src->haddr[2] << 32) +
		    pitch2 * cy + vcnt * cx;
		write_wvalue(ROT_ILADD2, addr2 & 0xffffffff);
		write_wvalue(ROT_IHADD2, (addr2 >> 32) & 0xff);

		if (((flag & 0xf00) == G2D_ROT_90) | ((flag & 0xf00) ==
							G2D_ROT_270)) {
			dst->clip_rect.w = src->clip_rect.h;
			dst->clip_rect.h = src->clip_rect.w;
		}

		else {
			dst->clip_rect.w = src->clip_rect.w;
			dst->clip_rect.h = src->clip_rect.h;
		}
		write_wvalue(ROT_OSIZE,
			       ((((dst->clip_rect.h -
				   1) & 0x1fff) << 16)) | ((dst->clip_rect.w -
							     1) & 0x1fff));
		/* YUV output fmt only support 420 */
		if (src->format == G2D_FORMAT_YUV422UVC_V1U1V0U0)
			dst->format = G2D_FORMAT_YUV420UVC_V1U1V0U0;
		else if (src->format == G2D_FORMAT_YUV422UVC_U1V1U0V0)
			dst->format = G2D_FORMAT_YUV420UVC_U1V1U0V0;
		else if (src->format == G2D_FORMAT_YUV422_PLANAR)
			dst->format = G2D_FORMAT_YUV420_PLANAR;
		else
			dst->format = src->format;

		if ((dst->format >= G2D_FORMAT_YUV422UVC_V1U1V0U0)
			&& (dst->format <= G2D_FORMAT_YUV422_PLANAR)) {
			cw = dst->width >> 1;
			ch = dst->height;
			cx = dst->clip_rect.x >> 1;
			cy = dst->clip_rect.y;
		}

		else if ((dst->format >= G2D_FORMAT_YUV420UVC_V1U1V0U0)
			&& (dst->format <= G2D_FORMAT_YUV420_PLANAR)) {
			cw = dst->width >> 1;
			ch = dst->height >> 1;
			cx = dst->clip_rect.x >> 1;
			cy = dst->clip_rect.y >> 1;
		}

		else if ((dst->format >= G2D_FORMAT_YUV411UVC_V1U1V0U0)
			&& (dst->format <= G2D_FORMAT_YUV411_PLANAR)) {
			cw = dst->width >> 2;
			ch = dst->height;
			cx = dst->clip_rect.x >> 2;
			cy = dst->clip_rect.y;
		}

		else {
			cw = 0;
			ch = 0;
			cx = 0;
			cy = 0;
		}

		g2d_byte_cal(dst->format, &ycnt, &ucnt, &vcnt);
		G2D_INFO_MSG("ROT output info: ----------------------------\n");
		G2D_INFO_MSG("Dst_fd:  %d\n", dst->fd);
		G2D_INFO_MSG("Format:  0x%x\n", dst->format);

		pitch0 = cal_align(ycnt * dst->width, dst->align[0]);
		write_wvalue(ROT_OPITCH0, pitch0);
		pitch1 = cal_align(ucnt * cw, dst->align[1]);
		write_wvalue(ROT_OPITCH1, pitch1);
		pitch2 = cal_align(vcnt * cw, dst->align[2]);
		write_wvalue(ROT_OPITCH2, pitch2);

		G2D_INFO_MSG("ROT_OutPITCH: %d, %d, %d\n",
				pitch0, pitch1, pitch2);
		G2D_INFO_MSG("outClipRectX:  %d\n", dst->clip_rect.x);
		G2D_INFO_MSG("outClipRectY: %d\n", dst->clip_rect.y);
		G2D_INFO_MSG("outClipRectW:  %d\n", dst->clip_rect.w);
		G2D_INFO_MSG("outClipRectH: %d\n", dst->clip_rect.h);
		addr0 =
		    dst->laddr[0] + ((__u64) dst->haddr[0] << 32) +
		    pitch0 * dst->clip_rect.y + ycnt * dst->clip_rect.x;
		write_wvalue(ROT_OLADD0, addr0 & 0xffffffff);
		write_wvalue(ROT_OHADD0, (addr0 >> 32) & 0xff);
		addr1 =
		    dst->laddr[1] + ((__u64) dst->haddr[1] << 32) +
		    pitch1 * cy + ucnt * cx;
		write_wvalue(ROT_OLADD1, addr1 & 0xffffffff);
		write_wvalue(ROT_OHADD1, (addr1 >> 32) & 0xff);
		addr2 =
		    dst->laddr[2] + ((__u64) dst->haddr[2] << 32) +
		    pitch2 * cy + vcnt * cx;
		write_wvalue(ROT_OLADD2, addr2 & 0xffffffff);
		write_wvalue(ROT_OHADD2, (addr2 >> 32) & 0xff);

		G2D_INFO_MSG("DST_ADDR0: 0x%x\n", dst->laddr[0]);
		G2D_INFO_MSG("DST_ADDR1: 0x%x\n", dst->laddr[1]);
		G2D_INFO_MSG("DST_ADDR2: 0x%x\n", dst->laddr[2]);

		/* start the module */
		rot_irq_enable();
		tmp = read_wvalue(ROT_CTL);
		tmp |= (0x1 << 31);
		G2D_INFO_MSG("init_module: 0x%x\n", tmp);
		write_wvalue(ROT_CTL, tmp);

		result = g2d_wait_cmd_finish();

		return result;
	}
	g2d_scan_order_fun(flag & 0xf0000000);

	/* start the module */
	mixer_irq_enable();
	tmp = read_wvalue(G2D_MIXER_CTL);
	tmp |= 0x80000000;
	G2D_INFO_MSG("INIT_MODULE: 0x%x\n", tmp);
	write_wvalue(G2D_MIXER_CTL, tmp);

	result = g2d_wait_cmd_finish();
	return result;
}

__s32 g2d_bsp_bld(g2d_image_enh *src, g2d_image_enh *dst, __u32 flag,
		    g2d_ck *ck_para)
{
	g2d_rect rect0, rect1;
	__u32 tmp;
	__s32 result;

	if (dst == NULL)
		return -1;
	g2d_mixer_reset();
	g2d_vlayer_set(0, dst);
	g2d_uilayer_set(2, src);
	if ((dst->format > G2D_FORMAT_BGRA1010102) &&
			(src->format <= G2D_FORMAT_BGRA1010102))
		g2d_csc_reg_set(1, G2D_RGB2YUV_709);
	write_wvalue(ROP_CTL, 0xF0);

	rect0.x = 0;
	rect0.y = 0;
	rect0.w = dst->clip_rect.w;
	rect0.h = dst->clip_rect.h;

	rect1.x = 0;
	rect1.y = 0;
	rect1.w = src->clip_rect.w;
	rect1.h = src->clip_rect.h;
	g2d_bldin_set(0, rect0, dst->bpremul);
	g2d_bldin_set(1, rect1, src->bpremul);

	G2D_INFO_MSG("BLD_FLAG:  0x%x\n", flag);

	porter_duff(flag & 0xFFF);
	if (flag & G2D_CK_SRC)
		write_wvalue(BLD_KEY_CTL, 0x3);

	else if (flag & G2D_CK_DST)
		write_wvalue(BLD_KEY_CTL, 0x1);
	if (ck_para != NULL)
		ck_para_set(ck_para);
	g2d_wb_set(dst);
	g2d_bld_cs_set(dst->format);

	/* start the modult */
	mixer_irq_enable();
	tmp = read_wvalue(G2D_MIXER_CTL);
	tmp |= 0x80000000;
	G2D_INFO_MSG("INIT_MODULE: 0x%x\n", tmp);
	write_wvalue(G2D_MIXER_CTL, tmp);

	result = g2d_wait_cmd_finish();
	return result;
}

__s32 g2d_get_clk_cnt(__u32 *clk)
{
	__s32 ret;

	ret = read_wvalue(G2D_MIXER_CLK);
	if (ret != 0)
		return -1;

	    /* clear clk cnt */
	    write_wvalue(G2D_MIXER_CLK, 0);
	return 0;
}

__u32 mixer_set_reg_base(unsigned long addr)
{
	base_addr = addr;
	return 0;
}

__u32 g2d_ip_version(void)
{
	__u32 reg_val;
	reg_val = read_wvalue(G2D_IP_VERSION);
	return reg_val;
}
