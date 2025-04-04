/*
 * Allwinner SoCs hdmi driver.
 *
 * Copyright (C) 2017 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include "hdmi_bsp.h"
#include "hdmi_core.h"

static unsigned int hdmi_base_addr;
static unsigned int hdmi_version;
/* static struct audio_para glb_audio; */
static unsigned int tmp_rcal_100, tmp_rcal_200;
static unsigned int rcal_flag;

#define get_bvalue(addr) (*((volatile unsigned char *)(addr)))
#define put_bvalue(addr, v)                                                    \
	(*((volatile unsigned char *)(addr)) = (unsigned char)(v))
#define get_wvalue(addr) (*((volatile unsigned long *)(addr)))
#define put_wvalue(addr, v)                                                    \
	(*((volatile unsigned long *)(addr)) = (unsigned long)(v))

static int hdmi_phy_set(struct video_para *video);

struct para_tab {
	unsigned int para[19];
};

struct pcm_sf {
	unsigned int sf;
	unsigned char cs_sf;
};

static struct para_tab ptbl[] = {
	{ {6, 1, 1, 1, 5, 3, 0, 1, 4, 0, 0, 160, 20, 38, 124, 240, 22, 0, 0} },
	{ {21, 11, 1, 1, 5, 3, 1, 1, 2, 0, 0, 160, 32, 24, 126, 32, 24, 0, 0} },
	{ {2, 11, 0, 0, 2, 6, 1, 0, 9, 0, 0, 208, 138, 16, 62, 224, 45, 0, 0} },
	{ {17, 11, 0, 0, 2, 5, 2, 0, 5, 0, 0, 208, 144, 12, 64, 64, 49, 0, 0} },
	{ {19, 4, 0, 96, 5, 5, 2, 2, 5, 1, 0, 0, 188, 184, 40, 208, 30, 1, 1} },
	{ {4, 4, 0, 96, 5, 5, 2, 1, 5, 0, 0, 0, 114, 110, 40, 208, 30, 1, 1} },
	{ {20, 4, 0, 97, 7, 5, 4, 2, 2, 2, 0, 128, 208, 16, 44, 56, 22, 1, 1} },
	{ {5, 4, 0, 97, 7, 5, 4, 1, 2, 0, 0, 128, 24, 88, 44, 56, 22, 1, 1} },
	{ {31, 2, 0, 96, 7, 5, 4, 2, 4, 2, 0, 128, 208, 16, 44, 56, 45, 1, 1} },
	{ {16, 2, 0, 96, 7, 5, 4, 1, 4, 0, 0, 128, 24, 88, 44, 56, 45, 1, 1} },
	{ {32, 4, 0, 96, 7, 5, 4, 3, 4, 2, 0, 128, 62, 126, 44, 56, 45, 1, 1} },
	{ {33, 4, 0, 0, 7, 5, 4, 2, 4, 2, 0, 128, 208, 16, 44, 56, 45, 1, 1} },
	{ {34, 4, 0, 0, 7, 5, 4, 1, 4, 0, 0, 128, 24, 88, 44, 56, 45, 1, 1} },
	{ {160, 2, 0, 96, 7, 5, 8, 3, 4, 2, 0, 128, 62, 126, 44, 157, 45, 1, 1}
	},
	{ {147, 2, 0, 96, 5, 5, 5, 2, 5, 1, 0, 0, 188, 184, 40, 190, 30, 1, 1}
	},
	{ {132, 2, 0, 96, 5, 5, 5, 1, 5, 0, 0, 0, 114, 110, 40, 160, 30, 1, 1}
	},
	{ {257, 1, 0, 96, 15, 10, 8, 2, 8, 0, 0, 0, 48, 176, 88, 112, 90, 1, 1}
	},
	{ {258, 1, 0, 96, 15, 10, 8, 5, 8, 4, 0, 0, 160, 32, 88, 112, 90, 1, 1}
	},
	{ {259, 1, 0, 96, 15, 10, 8, 6, 8, 4, 0, 0, 124, 252, 88, 112, 90, 1, 1}
	},
	{ {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0} },
};

static unsigned char ca_table[64] = {
	0x00, 0x11, 0x01, 0x13, 0x02, 0x31, 0x03, 0x33, 0x04, 0x15, 0x05,
	0x17, 0x06, 0x35, 0x07, 0x37, 0x08, 0x55, 0x09, 0x57, 0x0a, 0x75,
	0x0b, 0x77, 0x0c, 0x5d, 0x0d, 0x5f, 0x0e, 0x7d, 0x0f, 0x7f, 0x10,
	0xdd, 0x11, 0xdf, 0x12, 0xfd, 0x13, 0xff, 0x14, 0x99, 0x15, 0x9b,
	0x16, 0xb9, 0x17, 0xbb, 0x18, 0x9d, 0x19, 0x9f, 0x1a, 0xbd, 0x1b,
	0xbf, 0x1c, 0xdd, 0x1d, 0xdf, 0x1e, 0xfd, 0x1f, 0xff,
};

static struct pcm_sf sf[10] = {
	{22050, 0x04}, {44100, 0x00},  {88200, 0x08}, {176400, 0x0c},
	{24000, 0x06}, {48000, 0x02},  {96000, 0x0a}, {192000, 0x0e},
	{32000, 0x03}, {768000, 0x09},
};

static unsigned int n_table[21] = {
	32000, 3072,  4096,   44100, 4704,   6272,  88200,
	9408,  12544, 176400, 18816, 25088,  48000, 5120,
	6144,  96000, 10240,  12288, 192000, 20480, 24576,
};

static void hdmi_write(unsigned int addr, unsigned char data)
{
	put_bvalue(hdmi_base_addr + addr, data);
}

static void hdmi_writel(unsigned int addr, unsigned int data)
{
	put_wvalue(hdmi_base_addr + addr, data);
}

static unsigned char hdmi_read(unsigned int addr)
{
	return get_bvalue(hdmi_base_addr + addr);
}

static unsigned int hdmi_readl(unsigned int addr)
{
	return get_wvalue(hdmi_base_addr + addr);
}

static void hdmi_udelay(unsigned long us)
{
	hdmi_delay_us(us);
}

static void hdmi_phy_init(struct video_para *video)
{
	unsigned int to_cnt;
	unsigned int tmp;

	hdmi_writel(0x10020, 0);
	hdmi_writel(0x10020, (1 << 0));
	hdmi_udelay(5);
	hdmi_writel(0x10020, hdmi_readl(0x10020) | (1 << 16));
	hdmi_writel(0x10020, hdmi_readl(0x10020) | (1 << 1));
	hdmi_udelay(10);
	hdmi_writel(0x10020, hdmi_readl(0x10020) | (1 << 2));
	hdmi_udelay(5);
	hdmi_writel(0x10020, hdmi_readl(0x10020) | (1 << 3));
	hdmi_udelay(40);
	hdmi_writel(0x10020, hdmi_readl(0x10020) | (1 << 19));
	hdmi_udelay(100);
	hdmi_writel(0x10020, hdmi_readl(0x10020) | (1 << 18));
	hdmi_writel(0x10020, hdmi_readl(0x10020) | (7 << 4));
	to_cnt = 10;
	while (1) {
		if ((hdmi_readl(0x10038) & 0x80) == 0x80)
			break;
		hdmi_udelay(200);

		to_cnt--;
		if (to_cnt == 0) {
			pr_warn("%s, timeout\n", __func__);
			break;
		}
	}
	hdmi_writel(0x10020, hdmi_readl(0x10020) | (0xf << 8));
	/* hdmi_writel(0x10020,hdmi_readl(0x10020)&(~(1<<19))); */
	hdmi_writel(0x10020, hdmi_readl(0x10020) | (1 << 7));
	/* hdmi_writel(0x10020,hdmi_readl(0x10020)|(0xf<<12)); */

	hdmi_writel(0x1002c, 0x39dc5040);
	hdmi_writel(0x10030, 0x80084343);
	hdmi_udelay(10000);
	hdmi_writel(0x10034, 0x00000001);
	hdmi_writel(0x1002c, hdmi_readl(0x1002c) | 0x02000000);
	hdmi_udelay(100000);
	tmp = hdmi_readl(0x10038);
	tmp_rcal_100 = (tmp & 0x3f) >> 1;
	tmp_rcal_200 = (tmp & 0x3f) >> 2;
	rcal_flag = 1;
	hdmi_writel(0x1002c, hdmi_readl(0x1002c) | 0xC0000000);
	hdmi_writel(0x1002c, hdmi_readl(0x1002c) | ((tmp & 0x1f800) >> 11));
	hdmi_writel(0x10020, 0x01FF0F7F);
	hdmi_writel(0x10024, 0x80639000);
	hdmi_writel(0x10028, 0x0F81C405);
}

static unsigned int get_vid(unsigned int id)
{
	unsigned int i, count;

	count = sizeof(ptbl) / sizeof(struct para_tab) - 1;
	for (i = 0; i < count; i++) {
		if (id == ptbl[i].para[0])
			return i;
	}
	ptbl[i].para[0] = id;
	return i;
}

static int hdmi_phy_set(struct video_para *video)
{
	unsigned int id;
	unsigned int count;
	unsigned int tmp;
	unsigned int div;

	count = sizeof(ptbl) / sizeof(struct para_tab);

	if (rcal_flag == 0)
		hdmi_phy_init(video);

	id = get_vid(video->vic);
	if (id == (count - 1))
		div = video->clk_div - 1;
	else
		div = ptbl[id].para[1] - 1;
	div &= 0xf;
	hdmi_writel(0x10020, hdmi_readl(0x10020) & (~0xf000));
	switch (ptbl[id].para[1]) {
	case 1:
		if (hdmi_version == 0)
			hdmi_writel(0x1002c, 0x31dc5fc0);
		else
			hdmi_writel(0x1002c, 0x30dc5fc0);
		hdmi_writel(0x10030, 0x800863C0 | div);
		hdmi_udelay(10000);
		hdmi_writel(0x10034, 0x00000001);
		hdmi_writel(0x1002c, hdmi_readl(0x1002c) | 0x02000000);
		hdmi_udelay(200000);
		tmp = hdmi_readl(0x10038);
		hdmi_writel(0x1002c, hdmi_readl(0x1002c) | 0xC0000000);
		if (((tmp & 0x1f800) >> 11) < 0x3d)
			hdmi_writel(0x1002c,
				    hdmi_readl(0x1002c) |
					(((tmp & 0x1f800) >> 11) + 2));
		else
			hdmi_writel(0x1002c, hdmi_readl(0x1002c) | 0x3f);
		hdmi_udelay(100000);
		hdmi_writel(0x10020, 0x01FFFF7F);
		hdmi_writel(0x10024, 0x8063b000);
		hdmi_writel(0x10028, 0x0F8246B5);
		break;
	case 2:
		hdmi_writel(0x1002c, 0x39dc5040);
		hdmi_writel(0x10030, 0x80084380 | div);
		hdmi_udelay(10000);
		hdmi_writel(0x10034, 0x00000001);
		hdmi_writel(0x1002c, hdmi_readl(0x1002c) | 0x02000000);
		hdmi_udelay(100000);
		tmp = hdmi_readl(0x10038);
		hdmi_writel(0x1002c, hdmi_readl(0x1002c) | 0xC0000000);
		hdmi_writel(0x1002c,
			    hdmi_readl(0x1002c) | ((tmp & 0x1f800) >> 11));
		hdmi_writel(0x10020, 0x01FFFF7F);
		hdmi_writel(0x10024, 0x8063a800);
		hdmi_writel(0x10028, 0x0F81C405);
		break;
	case 4:
		hdmi_writel(0x1002c, 0x39dc5040);
		hdmi_writel(0x10030, 0x80084340 | div);
		hdmi_udelay(10000);
		hdmi_writel(0x10034, 0x00000001);
		hdmi_writel(0x1002c, hdmi_readl(0x1002c) | 0x02000000);
		hdmi_udelay(100000);
		tmp = hdmi_readl(0x10038);
		hdmi_writel(0x1002c, hdmi_readl(0x1002c) | 0xC0000000);
		hdmi_writel(0x1002c,
			    hdmi_readl(0x1002c) | ((tmp & 0x1f800) >> 11));
		hdmi_writel(0x10020, 0x11FFFF7F);
		hdmi_writel(0x10024, 0x80623000 | tmp_rcal_200);
		hdmi_writel(0x10028, 0x0F814385);
		break;
	case 11:
		hdmi_writel(0x1002c, 0x39dc5040);
		hdmi_writel(0x10030, 0x80084300 | div);
		hdmi_udelay(10000);
		hdmi_writel(0x10034, 0x00000001);
		hdmi_writel(0x1002c, hdmi_readl(0x1002c) | 0x02000000);
		hdmi_udelay(100000);
		tmp = hdmi_readl(0x10038);
		hdmi_writel(0x1002c, hdmi_readl(0x1002c) | 0xC0000000);
		hdmi_writel(0x1002c,
			    hdmi_readl(0x1002c) | ((tmp & 0x1f800) >> 11));
		hdmi_writel(0x10020, 0x11FFFF7F);
		hdmi_writel(0x10024, 0x80623000 | tmp_rcal_200);
		hdmi_writel(0x10028, 0x0F80C285);
		break;
	default:
		return -1;
	}
	return 0;
}

void bsp_hdmi_set_version(unsigned int version)
{
	hdmi_version = version;
}

void bsp_hdmi_set_addr(uintptr_t base_addr)
{
	hdmi_base_addr = base_addr;
	rcal_flag = 0;
}

void bsp_hdmi_inner_init(void)
{
	hdmi_write(0x10010, 0x45);
	hdmi_write(0x10011, 0x45);
	hdmi_write(0x10012, 0x52);
	hdmi_write(0x10013, 0x54);
	hdmi_write(0x8080, 0x00);
	hdmi_udelay(1);
	hdmi_write(0xF01F, 0x00);
	hdmi_write(0x8403, 0xff);
	hdmi_write(0x904C, 0xff);
	hdmi_write(0x904E, 0xff);
	hdmi_write(0xD04C, 0xff);
	hdmi_write(0x8250, 0xff);
	hdmi_write(0x8A50, 0xff);
	hdmi_write(0x8272, 0xff);
	hdmi_write(0x40C0, 0xff);
	hdmi_write(0x86F0, 0xff);
	hdmi_write(0x0EE3, 0xff);
	hdmi_write(0x8EE2, 0xff);
	hdmi_write(0xA049, 0xf0);
	hdmi_write(0xB045, 0x1e);
	hdmi_write(0x00C1, 0x00);
	hdmi_write(0x00C1, 0x03);
	hdmi_write(0x00C0, 0x00);
	hdmi_write(0x40C1, 0x10);
	hdmi_write(0x0081, 0xfd);
	hdmi_write(0x0081, 0x00);
	hdmi_write(0x0081, 0xfd);
	hdmi_write(0x0010, 0xff);
	hdmi_write(0x0011, 0xff);
	hdmi_write(0x8010, 0xff);
	hdmi_write(0x8011, 0xff);
	hdmi_write(0x0013, 0xff);
	hdmi_write(0x8012, 0xff);
	hdmi_write(0x8013, 0xff);
}

void bsp_hdmi_init(void)
{
	struct video_para vpara;

	vpara.vic = 17;
	hdmi_phy_init(&vpara);
	bsp_hdmi_inner_init();
}

void bsp_hdmi_set_video_en(unsigned char enable)
{
	if (enable) {
		hdmi_writel(0x10020, hdmi_readl(0x10020) | (0xf << 12));
	} else {
		hdmi_write(0x10010, 0x45);
		hdmi_write(0x10011, 0x45);
		hdmi_write(0x10012, 0x52);
		hdmi_write(0x10013, 0x54);

		hdmi_write(0x4044, 0x02);/* set avmute */

		hdmi_write(0x00C1, hdmi_read(0x00C1) | 0x02);/* _DisableEncryption(true) */
		hdmi_write(0x00C0, hdmi_read(0x00C0) & 0xfb);/* hdcp_rxdetect(flase) */
		hdmi_write(0x00C0, hdmi_read(0x00C0) | 0x08);/* set hdcp avmute */

		hdmi_write(0x10010, 0x52);
		hdmi_write(0x10011, 0x54);
		hdmi_write(0x10012, 0x41);
		hdmi_write(0x10013, 0x57);
		hdmi_writel(0x10020, hdmi_readl(0x10020) & (~(0xf << 12)));
	}
}

int bsp_hdmi_video_get_div(unsigned int pixel_clk)
{
	int div = 1;

	if (pixel_clk > 148500000)
		div = 1;
	else if (pixel_clk > 74250000)
		div = 2;
	else if (pixel_clk > 27000000)
		div = 4;
	else
		div = 11;

	return div;
}

int bsp_hdcp_enable(u32 enable, struct video_para *video)
{
	unsigned int id = get_vid(video->vic);

	pr_info("[%s]: en [%d]\n", __func__, enable);
	hdmi_write(0x10010, 0x45);
	hdmi_write(0x10011, 0x45);
	hdmi_write(0x10012, 0x52);
	hdmi_write(0x10013, 0x54);
	if (enable) {
		hdmi_write(0x00C1, hdmi_read(0x00C1) | 0x02);/* _DisableEncryption */
		hdmi_write(0x0081, hdmi_read(0x0081) & 0xbf);/* mc_hdcp_clock_enable */
		hdmi_write(0xC0C0, 0x40);/* _OessWindowSize */
		hdmi_write(0x0040, hdmi_read(0x0040) | 0x80);/* fc_video_hdcp_keepout */
		hdmi_write(0x00C0, video->is_hdmi ? (hdmi_read(0x00C0) | 0x01) : (hdmi_read(0x00C0) & 0xfe));/* hdmi mode */
	/* 	hdmi_write(0x40C1, hdmi_read(0x40C1) | 0x02);_HSyncPolarity */
	/* 	hdmi_write(0x40C1, hdmi_read(0x40C1) | 0x08);_VSyncPolarity */
		hdmi_write(0x40C1, (ptbl[id].para[3] < 96) ? 0x10 : 0x1a);
	/* 	hdmi_write(0x40C1, hdmi_read(0x40C1) | 0x10);_DataEnablePolarity */
		hdmi_write(0x8081, hdmi_read(0x8081) & 0xdf);/* bypass hdcp_block */
		hdmi_write(0x00C0, hdmi_read(0x00C0) & 0xfd);/* _EnableFeature11 */
		hdmi_write(0x00C0, hdmi_read(0x00C0) & 0xef);/* _RiCheck */
		hdmi_write(0x00C0, hdmi_read(0x00C0) & 0xbf);/* _EnableI2cFastMode */
		hdmi_write(0x00C0, hdmi_read(0x00C0) & 0x7f);/* _EnhancedLinkVerification */
		hdmi_write(0x00C0, hdmi_read(0x00C0) & 0xf7);/* _EnableAvmute */
		hdmi_write(0x40C1, hdmi_read(0x40C1) | 0x40);/* _UnencryptedVideoColor */
		hdmi_write(0x00C1, hdmi_read(0x00C1) | 0x04);/* _EncodingPacketHeader */
		hdmi_write(0x00C0, hdmi_read(0x00C0) & 0xdf);/* _BypassEncryption */
		hdmi_write(0x00C1, hdmi_read(0x00C1) & 0xfe);/* hdcp_sw_reset */
		hdmi_write(0x00C0, hdmi_read(0x00C0) | 0x04);/* hdcp_rxdetect */
		hdmi_write(0x80C2, 0xff);/* _InterruptClear */
		hdmi_write(0x40C0, 0x00);/* _InterruptMask */
	} else {
		hdmi_write(0x00C1, hdmi_read(0x00C1) | 0x02);/* _DisableEncryption(true) */
#if 0
		hdmi_write(0x00C0, hdmi_read(0x00C0) & 0xfb);/* hdcp_rxdetect(flase) */
		hdmi_write(0x0081, hdmi_read(0x0081) | 0x40);/* mc_hdcp_clock_enable(disable) */
#endif
	}

	return 0;
}

int bsp_hdcp_disconfig(void)
{
	hdmi_write(0x10010, 0x45);
	hdmi_write(0x10011, 0x45);
	hdmi_write(0x10012, 0x52);
	hdmi_write(0x10013, 0x54);
	hdmi_write(0x00C1, hdmi_read(0x00C1) | 0x02);/* _DisableEncryption(true) */
#if 0
	hdmi_write(0x00C0, hdmi_read(0x00C0) & 0xfb);/* hdcp_rxdetect(flase) */
	hdmi_write(0x0081, hdmi_read(0x0081) | 0x40);/* mc_hdcp_clock_enable(disable) */
#endif
	hdmi_write(0x10010, 0x52);
	hdmi_write(0x10011, 0x54);
	hdmi_write(0x10012, 0x41);
	hdmi_write(0x10013, 0x57);
	return 0;
}

int bsp_hdmi_video(struct video_para *video)
{
	unsigned int count;
	unsigned int id = get_vid(video->vic);

	switch (video->vic) {
	case 2:
	case 6:
	case 17:
	case 21:
		video->csc = BT601;
		break;
	default:
		video->csc = BT709;
		break;
	}

	count = sizeof(ptbl) / sizeof(struct para_tab);
	if (id == count - 1) {
		ptbl[id].para[1] = bsp_hdmi_video_get_div(video->pixel_clk);
		ptbl[id].para[2] = video->pixel_repeat;
		ptbl[id].para[3] = ((video->hor_sync_polarity & 1) << 5) |
				   ((video->ver_sync_polarity & 1) << 6) |
				   (video->b_interlace & 1);
		ptbl[id].para[4] = video->x_res / 256;
		ptbl[id].para[5] = video->ver_sync_time;
		ptbl[id].para[6] = video->y_res / 256;
		ptbl[id].para[7] = (video->hor_total_time - video->x_res) / 256;
		ptbl[id].para[8] = video->ver_front_porch;
		ptbl[id].para[9] = video->hor_front_porch / 256;
		ptbl[id].para[10] = video->hor_sync_time / 256;
		ptbl[id].para[11] = video->x_res % 256;
		ptbl[id].para[12] =
		    (video->hor_total_time - video->x_res) % 256;
		ptbl[id].para[13] = video->hor_front_porch % 256;
		ptbl[id].para[14] = video->hor_sync_time % 256;
		ptbl[id].para[15] = video->y_res % 256;
		ptbl[id].para[16] = video->ver_total_time - video->y_res;
		ptbl[id].para[17] = 1;
		if (video->x_res <= 736 && video->y_res <= 576)
			video->csc = BT601;
		else
			video->csc = BT709;
	}

	bsp_hdmi_inner_init();

	hdmi_write(0x4044, 0x02);
	hdmi_write(0x0840, 0x01);
	hdmi_write(0x4845, 0x00);
	hdmi_write(0x0040, ptbl[id].para[3] | 0x10);
	hdmi_write(0x10001, ((ptbl[id].para[3] < 96) ? 0x03 : 0x00));
	hdmi_write(0x8040, ptbl[id].para[4]);
	hdmi_write(0x4043, ptbl[id].para[5]);
	hdmi_write(0x8042, ptbl[id].para[6]);
	hdmi_write(0x0042, ptbl[id].para[7]);
	hdmi_write(0x4042, ptbl[id].para[8]);
	hdmi_write(0x4041, ptbl[id].para[9]);
	hdmi_write(0xC041, ptbl[id].para[10]);
	hdmi_write(0x0041, ptbl[id].para[11]);
	hdmi_write(0x8041, ptbl[id].para[12]);
	hdmi_write(0x4040, ptbl[id].para[13]);
	hdmi_write(0xC040, ptbl[id].para[14]);
	hdmi_write(0x0043, ptbl[id].para[15]);
	hdmi_write(0x8043, ptbl[id].para[16]);
	hdmi_write(0x0045, 0x0c);
	hdmi_write(0x8044, 0x20);
	hdmi_write(0x8045, 0x01);
	hdmi_write(0x0046, 0x0b);
	hdmi_write(0x0047, 0x16);
	hdmi_write(0x8046, 0x21);
	hdmi_write(0x3048, ptbl[id].para[2] ? 0x21 : 0x10);
	hdmi_write(0x0401, ptbl[id].para[2] ? 0x01 : 0x00);
	hdmi_write(0x8400, 0x07);
	hdmi_write(0x8401, 0x00);
	hdmi_write(0x0402, 0x47);
	hdmi_write(0x0800, 0x01);
	hdmi_write(0x0801, 0x07);
	hdmi_write(0x8800, 0x00);
	hdmi_write(0x8801, 0x00);
	hdmi_write(0x0802, 0x00);
	hdmi_write(0x0803, 0x00);
	hdmi_write(0x8802, 0x00);
	hdmi_write(0x8803, 0x00);

	if (video->is_hdmi) {
		hdmi_write(0xB045, 0x08);
		hdmi_write(0x2045, 0x00);
		hdmi_write(0x2044, 0x0c);
		hdmi_write(0x6041, 0x03);
		hdmi_write(
		    0xA044,
		    ((ptbl[id].para[0] & 0x100) == 0x100)
			? 0x20
			: (((ptbl[id].para[0] & 0x80) == 0x80) ? 0x40 : 0x00));
		hdmi_write(0xA045,
			   ((ptbl[id].para[0] & 0x100) == 0x100)
			       ? (ptbl[id].para[0] & 0x7f)
			       : 0x00);
		hdmi_write(0x2046, 0x00);
		hdmi_write(0x3046, 0x01);
		hdmi_write(0x3047, 0x11);
		/* hdmi_write(0x4044, 0x00); */
		hdmi_write(0x0052, 0x00);
		hdmi_write(0x8051, 0x11);
		hdmi_write(0x10010, 0x45);
		hdmi_write(0x10011, 0x45);
		hdmi_write(0x10012, 0x52);
		hdmi_write(0x10013, 0x54);
		hdmi_write(0x0040, hdmi_read(0x0040) | 0x08);
		hdmi_write(0x10010, 0x52);
		hdmi_write(0x10011, 0x54);
		hdmi_write(0x10012, 0x41);
		hdmi_write(0x10013, 0x57);
		hdmi_write(0x4045, video->is_yuv ? 0x02 : 0x00);
		if (ptbl[id].para[17] == 0)
			hdmi_write(0xC044, (video->csc << 6) | 0x18);
		else if (ptbl[id].para[17] == 1)
			hdmi_write(0xC044, (video->csc << 6) | 0x28);
		else
			hdmi_write(0xC044, (video->csc << 6) | 0x08);

		hdmi_write(0xC045, video->is_yuv ? 0x00 : 0x04);
		hdmi_write(0x4046,
			   ((ptbl[id].para[0] & 0x100) == 0x100)
			       ? 0x00
			       : (ptbl[id].para[0] & 0x7f));
	}

	if (hdmi_phy_set(video) != 0)
		return -1;

	hdmi_write(0x0082, 0x00);
	hdmi_write(0x0081, 0x00);

	hdmi_write(0x0840, 0x00);
	hdmi_write(0x4044, 0x01);

	if (video->is_hcts) {
		bsp_hdcp_enable(1, video);
	} else {
		bsp_hdcp_enable(0, video);
	}

	return 0;
}

int bsp_hdmi_audio(struct audio_para *audio)
{
	unsigned int i;
	unsigned int n;
	unsigned int count;
	unsigned id = get_vid(audio->vic);

	count = sizeof(ptbl) / sizeof(struct para_tab);

	hdmi_write(0xA049, (audio->ch_num > 2) ? 0xf1 : 0xf0);

	for (i = 0; i < 64; i += 2) {
		if (audio->ca == ca_table[i]) {
			hdmi_write(0x204B, ~ca_table[i + 1]);
			break;
		}
	}
	hdmi_write(0xA04A, 0x00);
	hdmi_write(0xA04B, 0x30);
	hdmi_write(0x6048, 0x00);
	hdmi_write(0x6049, 0x01);
	hdmi_write(0xE048, 0x42);
	hdmi_write(0xE049, 0x86);
	hdmi_write(0x604A, 0x31);
	hdmi_write(0x604B, 0x75);
	hdmi_write(0xE04A, 0x00 | 0x01);
	for (i = 0; i < 10; i += 1) {
		if (audio->sample_rate == sf[i].sf) {
			hdmi_write(0xE04A, 0x00 | sf[i].cs_sf);
			break;
		}
	}
	hdmi_write(0xE04B,
		   0x00 | (audio->sample_bit == 16)
		       ? 0x02
		       : ((audio->sample_bit == 24) ? 0xb : 0x0));

	hdmi_write(0x0251, audio->sample_bit);

	n = 6272;
	/* cts = 0; */
	for (i = 0; i < 21; i += 3) {
		if (audio->sample_rate == n_table[i]) {
			if ((id != count - 1) && (ptbl[id].para[1] == 1))
				n = n_table[i + 1];
			else
				n = n_table[i + 2];

			/* cts = (n / 128) * (glb_video.tmds_clk / 100) / */
			/* (audio->sample_rate / 100); */
			break;
		}
	}

	hdmi_write(0x0A40, n);
	hdmi_write(0x0A41, n >> 8);
	hdmi_write(0x8A40, n >> 16);
	hdmi_write(0x0A43, 0x00);
	hdmi_write(0x8A42, 0x04);
	hdmi_write(0xA049, (audio->ch_num > 2) ? 0x01 : 0x00);
	if (audio->type == PCM)
		hdmi_write(0x2043, (audio->ch_num - 1) * 16);
	else
		hdmi_write(0x2043, 0x00);
	hdmi_write(0xA042, 0x00);
	hdmi_write(0xA043, audio->ca);
	hdmi_write(0x6040, 0x00);

	if (audio->type == PCM) {
		hdmi_write(0x8251, 0x00);
	} else if ((audio->type == DTS_HD) || (audio->type == MAT)) {
		hdmi_write(0x8251, 0x03);
		hdmi_write(0x0251, 0x15);
		hdmi_write(0xA043, 0);
	} else {
		hdmi_write(0x8251, 0x02);
		hdmi_write(0x0251, 0x15);
		hdmi_write(0xA043, 0);
	}

	hdmi_write(0x0250, 0x00);
	hdmi_write(0x0081, 0x08);
	hdmi_write(0x8080, 0xf7);
	hdmi_udelay(100);
	hdmi_write(0x0250, 0xaf);
	hdmi_udelay(100);
	hdmi_write(0x0081, 0x00);

	return 0;
}

int bsp_hdmi_ddc_read(char cmd, char pointer, char offset, int nbyte,
		      char *pbuf)
{
	unsigned char off = offset;
	unsigned int to_cnt;
	int ret = 0;

	hdmi_write(0x10010, 0x45);
	hdmi_write(0x10011, 0x45);
	hdmi_write(0x10012, 0x52);
	hdmi_write(0x10013, 0x54);
	hdmi_write(0x4EE1, 0x00);

	to_cnt = 50;
	while ((hdmi_read(0x4EE1) & 0x01) != 0x01) {
		hdmi_udelay(10);
		to_cnt--; /* wait for 500us for timeout */
		if (to_cnt == 0) {
			pr_warn("ddc rst timeout\n");
			break;
		}
	}

	hdmi_write(0x8EE3, 0x05);
	hdmi_write(0x0EE3, 0x08);
	hdmi_write(0x4EE2, 0xd8);
	hdmi_write(0xCEE2, 0xfe);

	to_cnt = 10;
	while (nbyte > 0) {
		to_cnt = 10;
		hdmi_write(0x0EE0, 0xa0 >> 1);
		hdmi_write(0x0EE1, off);
		hdmi_write(0x4EE0, 0x60 >> 1);
		hdmi_write(0xCEE0, pointer);
		hdmi_write(0x0EE2, 0x02);

		while (1) {
			to_cnt--; /* wait for 10ms for timeout */
			if (to_cnt == 0) {
				pr_warn("ddc read timeout, byte cnt = %d\n",
					nbyte);
				break;
			}
			if ((hdmi_read(0x0013) & 0x02) == 0x02) {
				hdmi_write(0x0013, hdmi_read(0x0013) & 0x02);
				*pbuf++ = hdmi_read(0x8EE1);
				break;
			} else if ((hdmi_read(0x0013) & 0x01) == 0x01) {
				hdmi_write(0x0013, hdmi_read(0x0013) & 0x01);
				ret = -1;
				break;
			}
			hdmi_udelay(1000);
		}
		nbyte--;
		off++;
	}
	hdmi_write(0x10010, 0x52);
	hdmi_write(0x10011, 0x54);
	hdmi_write(0x10012, 0x41);
	hdmi_write(0x10013, 0x57);

	return ret;
}

unsigned int bsp_hdmi_get_hpd(void)
{
	unsigned int ret = 0;

	hdmi_write(0x10010, 0x45);
	hdmi_write(0x10011, 0x45);
	hdmi_write(0x10012, 0x52);
	hdmi_write(0x10013, 0x54);

	if (hdmi_readl(0x10038) & 0x80000)
		ret = 1;
	else
		ret = 0;

	hdmi_write(0x10010, 0x52);
	hdmi_write(0x10011, 0x54);
	hdmi_write(0x10012, 0x41);
	hdmi_write(0x10013, 0x57);

	return ret;
}

void bsp_hdmi_standby(void)
{
	hdmi_write(0x10020, 0x07);
	hdmi_write(0x1002c, 0x00);
}

void bsp_hdmi_hrst(void)
{
	hdmi_write(0x10010, 0x45);
	hdmi_write(0x10011, 0x45);
	hdmi_write(0x10012, 0x52);
	hdmi_write(0x10013, 0x54);
	hdmi_write(0x00C1, 0x04);/* 5001 */
	hdmi_write(0x0081, 0x40);/* 4001 */
	hdmi_write(0x10010, 0x52);
	hdmi_write(0x10011, 0x54);
	hdmi_write(0x10012, 0x41);
	hdmi_write(0x10013, 0x57);
}

void bsp_hdmi_hdl(void)
{
}

static void _MemoryAccessRequest(u8 en)
{
	hdmi_write(0x10010, 0x45);
	hdmi_write(0x10011, 0x45);
	hdmi_write(0x10012, 0x52);
	hdmi_write(0x10013, 0x54);

	if (en)
		hdmi_write(0x80c6, hdmi_read(0x80c6) | 0x01);/* 5016 */
	else
		hdmi_write(0x80c6, hdmi_read(0x80c6) & 0xfe);

	hdmi_write(0x10010, 0x52);
	hdmi_write(0x10011, 0x54);
	hdmi_write(0x10012, 0x41);
	hdmi_write(0x10013, 0x57);
	return;
}

static u8 _MemoryAccessGranted(void)
{
	hdmi_write(0x10010, 0x45);
	hdmi_write(0x10011, 0x45);
	hdmi_write(0x10012, 0x52);
	hdmi_write(0x10013, 0x54);
	return (u8)((hdmi_read(0x80c6) & 0x02) >> 1);
}

static u16 _BStatusRead(void)
{
	u16 bstatus = 0;

	hdmi_write(0x10010, 0x45);
	hdmi_write(0x10011, 0x45);
	hdmi_write(0x10012, 0x52);
	hdmi_write(0x10013, 0x54);
	bstatus	= hdmi_read(0x20c0);/* 20c0 5020 */
	bstatus	|= hdmi_read(0x20c1) << 8;/* 5021 */
	return bstatus;
}

static u32 bsp_reg_mapping(u32 reg)
{
	int i = 0;
	u32 reg_map = 0;
	unsigned int offset[16] = {1, 3, 5, 7, 9, 11, 13, 15, 14, 12, 10, 8, 6, 4, 2, 0};

	for (i = 0; i < 16; i++)
		reg_map |= ((((reg >> offset[i]) & 0x1)) << (15 - i));

	return reg_map;
}

static void sha_reset(sha_t *sha)
{
	size_t i = 0;

	sha->mIndex = 0;
	sha->mComputed = FALSE;
	sha->mCorrupted = FALSE;
	for (i = 0; i < sizeof(sha->mLength); i++)
		sha->mLength[i] = 0;

	sha->mDigest[0] = 0x67452301;
	sha->mDigest[1] = 0xEFCDAB89;
	sha->mDigest[2] = 0x98BADCFE;
	sha->mDigest[3] = 0x10325476;
	sha->mDigest[4] = 0xC3D2E1F0;
}

static void sha_process_block(sha_t *sha)
{
	#define shaCircularShift(bits, word) ((((word) << (bits)) & 0xFFFFFFFF) | ((word) >> (32-(bits))))
	const unsigned K[] = {	/* constants defined in SHA-1 */
		0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xCA62C1D6
	};
	unsigned W[80];		/* word sequence */
	unsigned A, B, C, D, E;	/* word buffers */
	unsigned temp = 0;
	int t = 0;

	/* Initialize the first 16 words in the array W */
	for (t = 0; t < 80; t++) {
		if (t < 16) {
			W[t] = ((unsigned)sha->mBlock[t * 4 + 0]) << 24;
			W[t] |= ((unsigned)sha->mBlock[t * 4 + 1]) << 16;
			W[t] |= ((unsigned)sha->mBlock[t * 4 + 2]) << 8;
			W[t] |= ((unsigned)sha->mBlock[t * 4 + 3]) << 0;
		} else {
			W[t] =
			    shaCircularShift(1,
					     W[t - 3] ^ W[t - 8] ^ W[t -
							14] ^ W[t - 16]);
		}
	}

	A = sha->mDigest[0];
	B = sha->mDigest[1];
	C = sha->mDigest[2];
	D = sha->mDigest[3];
	E = sha->mDigest[4];

	for (t = 0; t < 80; t++) {
		temp = shaCircularShift(5, A);
		if (t < 20)
			temp += ((B & C) | ((~B) & D)) + E + W[t] + K[0];
		else if (t < 40)
			temp += (B ^ C ^ D) + E + W[t] + K[1];
		else if (t < 60)
			temp += ((B & C) | (B & D) | (C & D)) + E + W[t] + K[2];
		else
			temp += (B ^ C ^ D) + E + W[t] + K[3];

		E = D;
		D = C;
		C = shaCircularShift(30, B);
		B = A;
		A = (temp & 0xFFFFFFFF);
	}

	sha->mDigest[0] = (sha->mDigest[0] + A) & 0xFFFFFFFF;
	sha->mDigest[1] = (sha->mDigest[1] + B) & 0xFFFFFFFF;
	sha->mDigest[2] = (sha->mDigest[2] + C) & 0xFFFFFFFF;
	sha->mDigest[3] = (sha->mDigest[3] + D) & 0xFFFFFFFF;
	sha->mDigest[4] = (sha->mDigest[4] + E) & 0xFFFFFFFF;

	sha->mIndex = 0;
}

static void sha_input(sha_t *sha, const u8 *data, size_t size)
{
	int i = 0;
	unsigned j = 0;
	int rc = TRUE;

	if (data == 0 || size == 0) {
		pr_err("invalid input data\n");
		return;
	}
	if (sha->mComputed == TRUE || sha->mCorrupted == TRUE) {
		sha->mCorrupted = TRUE;
		return;
	}
	while (size-- && sha->mCorrupted == FALSE) {
		sha->mBlock[sha->mIndex++] = *data;

		for (i = 0; i < 8; i++) {
			rc = TRUE;
			for (j = 0; j < sizeof(sha->mLength); j++) {
				sha->mLength[j]++;
				if (sha->mLength[j] != 0) {
					rc = FALSE;
					break;
				}
			}
			sha->mCorrupted = (sha->mCorrupted == TRUE
					   || rc == TRUE) ? TRUE : FALSE;
		}
		/* if corrupted then message is too long */
		if (sha->mIndex == 64)
			sha_process_block(sha);

		data++;
	}
}

static void sha_pad_message(sha_t *sha)
{
	/*
	 *  Check to see if the current message block is too small to hold
	 *  the initial padding bits and length.  If so, we will pad the
	 *  block, process it, and then continue padding into a second
	 *  block.
	 */
	if (sha->mIndex > 55) {
		sha->mBlock[sha->mIndex++] = 0x80;
		while (sha->mIndex < 64)
			sha->mBlock[sha->mIndex++] = 0;

		sha_process_block(sha);
		while (sha->mIndex < 56)
			sha->mBlock[sha->mIndex++] = 0;

	} else {
		sha->mBlock[sha->mIndex++] = 0x80;
		while (sha->mIndex < 56)
			sha->mBlock[sha->mIndex++] = 0;
	}

	/* Store the message length as the last 8 octets */
	sha->mBlock[56] = sha->mLength[7];
	sha->mBlock[57] = sha->mLength[6];
	sha->mBlock[58] = sha->mLength[5];
	sha->mBlock[59] = sha->mLength[4];
	sha->mBlock[60] = sha->mLength[3];
	sha->mBlock[61] = sha->mLength[2];
	sha->mBlock[62] = sha->mLength[1];
	sha->mBlock[63] = sha->mLength[0];

	sha_process_block(sha);
}

static int sha_result(sha_t *sha)
{
	if (sha->mCorrupted == TRUE)
		return FALSE;

	if (sha->mComputed == FALSE) {
		sha_pad_message(sha);
		sha->mComputed = TRUE;
	}
	return TRUE;
}

static int bsp_hdcp_verify_ksv(const u8 *data, size_t size)
{
	size_t i = 0;
	sha_t sha;

	if (data == 0 || size < (HEADER + SHAMAX)) {
		pr_err("invalid input data\n");
		return FALSE;
	}
	sha_reset(&sha);
	sha_input(&sha, data, size - SHAMAX);

	if (sha_result(&sha) == FALSE) {
		pr_err("cannot process SHA digest\n");
		return FALSE;
	}

	for (i = 0; i < SHAMAX; i++) {
		if (data[size - SHAMAX + i] !=
				(u8) (sha.mDigest[i / 4] >> ((i % 4) * 8))) {
			pr_err("SHA digest does not match\n");
			return FALSE;
		}
	}
	return TRUE;
}

static void _UpdateKsvListState(u8 en)
{
	hdmi_write(0x10010, 0x45);
	hdmi_write(0x10011, 0x45);
	hdmi_write(0x10012, 0x52);
	hdmi_write(0x10013, 0x54);
	if (en)
		hdmi_write(0x80c6, hdmi_read(0x80c6) | 0x08);/* 5016 */
	else
		hdmi_write(0x80c6, hdmi_read(0x80c6) & 0xf7);

	hdmi_write(0x80c6, hdmi_read(0x80c6) | 0x04);
	hdmi_write(0x80c6, hdmi_read(0x80c6) & 0xfb);
}

static u8 bsp_read_ksv_list(void)
{
	int timeout = 1000;
	u16 bstatus = 0;
	u16 deviceCount = 0;
	int valid = HDCP_DISABLE;
	int size = 0;
	int i = 0;
	unsigned int map_reg = 0, tmp_reg = 0;

	u8 *hdcp_ksv_list_buffer = NULL;/* dev->hdcp.mKsvListBuffer; */

	hdcp_ksv_list_buffer = kzalloc(sizeof(u8) * 670, GFP_KERNEL);

	/* 1 - Wait for an interrupt to be triggered
		(a_apiintstat.KSVSha1calcint) */
	/* This is called from the INT_KSV_SHA1 irq
		so nothing is required for this step */

	/* 2 - Request access to KSV memory through
		setting a_ksvmemctrl.KSVMEMrequest to 1'b1 and */
	/* pool a_ksvmemctrl.KSVMEMaccess until
		this value is 1'b1 (access granted). */
	_MemoryAccessRequest(1);
	while (_MemoryAccessGranted() == 0 && timeout--)
		asm volatile ("nop");

	if (_MemoryAccessGranted() == 0) {
		_MemoryAccessRequest(0);
		pr_err("KSV List memory access denied");
		kfree(hdcp_ksv_list_buffer);
		return -1;
	}

	/* 3 - Read VH', M0, Bstatus, and the KSV FIFO.
	The data is stored in the revocation memory, as */
	/* provided in the "Address Mapping for Maximum Memory Allocation"
	table in the databook. */
	bstatus = _BStatusRead();
	deviceCount = bstatus & BSTATUS_DEVICE_COUNT_MASK;

	if (deviceCount > 128) {
		pr_err("[%s]:depth exceeds KSV List memory", __func__);
		kfree(hdcp_ksv_list_buffer);
		return -1;
	}

	size = deviceCount * KSV_LEN + HEADER + SHAMAX;

	hdmi_write(0x10010, 0x45);
	hdmi_write(0x10011, 0x45);
	hdmi_write(0x10012, 0x52);
	hdmi_write(0x10013, 0x54);

	for (i = 0; i < size; i++) {
		tmp_reg = 0x5020 + i;
		map_reg = bsp_reg_mapping(tmp_reg);

		if (i < HEADER) { /* BSTATUS & M0 */
			hdcp_ksv_list_buffer[(deviceCount * KSV_LEN) + i] =
			(u8)hdmi_read(map_reg);
		} else if (i < (HEADER + (deviceCount * KSV_LEN))) { /* KSV list */
			hdcp_ksv_list_buffer[i - HEADER] =
			(u8)hdmi_read(map_reg);
		} else { /* SHA */
			hdcp_ksv_list_buffer[i] = (u8)hdmi_read(map_reg);
		}
	}

	/* 4 - Calculate the SHA-1 checksum (VH) over M0,
		Bstatus, and the KSV FIFO. */
	if (bsp_hdcp_verify_ksv(hdcp_ksv_list_buffer, size) == TRUE) {
		valid = HDCP_KSV_LIST_READY;
		pr_info("HDCP_KSV_LIST_READY");
	} else {
		valid = HDCP_ERR_KSV_LIST_NOT_VALID;
		pr_info("HDCP_ERR_KSV_LIST_NOT_VALID");
	}

	/* 5 - If the calculated VH equals the VH',
	set a_ksvmemctrl.SHA1fail to 0 and set */
	/* a_ksvmemctrl.KSVCTRLupd to 1.
	If the calculated VH is different from VH' then set */
	/* a_ksvmemctrl.SHA1fail to 1 and set a_ksvmemctrl.KSVCTRLupd to 1,
	forcing the controller */
	/* to re-authenticate from the beginning. */
	_MemoryAccessRequest(0);
	_UpdateKsvListState((valid == HDCP_KSV_LIST_READY) ? 0 : 1);

	return valid;
}

int bsp_hdmi_hdcp_err_check(void)
{
	int ret = 0;
	u8 hdcp_status = 0;

	hdmi_write(0x10010, 0x45);
	hdmi_write(0x10011, 0x45);
	hdmi_write(0x10012, 0x52);
	hdmi_write(0x10013, 0x54);
	hdcp_status = hdmi_read(0x80c3); /* 5007 get hdcp interrupt status */

#if 0
	if (hdcp_status) {
		pr_info("[%s]:hdcp_status = 0x%x\n", __func__, hdcp_status);
	}
#endif
	hdmi_write(0x80c2, hdcp_status); /* clear flag */
	if (hdcp_status & 0x10) {
		pr_err("[%s]:i2c nack error interrupt\n", __func__);
		ret = -1;
		goto err_status;
	}

	if (hdcp_status & 0x02) {
		pr_err("[%s]:KSV_SHA1 interrupt\n", __func__);
		ret = bsp_read_ksv_list();
		goto err_status;
	}

	if (hdcp_status & 0x40) {
		pr_err("[%s]:HDCP_FAILED interrupt\n", __func__);
		hdmi_write(0x00C1, hdmi_read(0x00C1) | 0x02); /* _DisableEncryption(dev, true); */
		ret = -1;
		goto err_status;
	}

	if (hdcp_status & 0x80) {
		pr_info("[%s]:HDCP_ENGAGED interrupt\n", __func__);
		hdmi_write(0x00C1, hdmi_read(0x00C1) & 0xfd); /* _DisableEncryption(dev, false); */
		ret = 0;
	}

err_status:
	hdmi_write(0x10010, 0x52);
	hdmi_write(0x10011, 0x54);
	hdmi_write(0x10012, 0x41);
	hdmi_write(0x10013, 0x57);
	return ret;
}


void bsp_hdmi_write(unsigned int addr, unsigned char data)
{
	hdmi_write(0x10010, 0x45);
	hdmi_write(0x10011, 0x45);
	hdmi_write(0x10012, 0x52);
	hdmi_write(0x10013, 0x54);

	hdmi_write(addr, data);

	hdmi_write(0x10010, 0x52);
	hdmi_write(0x10011, 0x54);
	hdmi_write(0x10012, 0x41);
	hdmi_write(0x10013, 0x57);

}

int bsp_hdmi_read(unsigned int addr, unsigned char *buf)
{
	int ret = -1;

	if (buf == NULL)
		goto OUT;

	hdmi_write(0x10010, 0x45);
	hdmi_write(0x10011, 0x45);
	hdmi_write(0x10012, 0x52);
	hdmi_write(0x10013, 0x54);
	*buf = hdmi_read(addr);
	/* read here */
	hdmi_write(0x10010, 0x52);
	hdmi_write(0x10011, 0x54);
	hdmi_write(0x10012, 0x41);
	hdmi_write(0x10013, 0x57);
	ret = 0;
OUT:
	return ret;
}

int bsp_hdmi_cec_get_simple_msg(unsigned char *msg)
{
	return 0;
}

int bsp_hdmi_cec_send(char *buf, unsigned char bytes)
{
	return 0;
}

void bsp_hdmi_cec_free_time_set(unsigned char value)
{
}

int bsp_hdmi_set_func(hdmi_bsp_func *func)
{
	return 0;
}
