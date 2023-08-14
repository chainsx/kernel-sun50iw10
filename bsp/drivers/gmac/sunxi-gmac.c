/*
* Allwinner GMAC driver.
*
* Copyright(c) 2022-2027 Allwinnertech Co., Ltd.
*
* This file is licensed under the terms of the GNU General Public
* License version 2.  This program is licensed "as is" without any
* warranty of any kind, whether express or implied.
*/

/* #define DEBUG */
#include <linux/kernel.h>
#include <linux/ctype.h>
#include <linux/printk.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/reset.h>
#include <linux/mii.h>
#include <linux/crc32.h>
#include <linux/skbuff.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/etherdevice.h>
#include <linux/netdevice.h>
#include <linux/phy.h>
#include <linux/init.h>
#include <linux/crypto.h>
#include <crypto/algapi.h>
#include <crypto/hash.h>
#include <linux/err.h>
#include <linux/scatterlist.h>
#include <linux/regulator/consumer.h>
#include <linux/of_net.h>
#include <linux/of_mdio.h>
#ifdef CONFIG_AW_EPHY_AC300
#include <linux/pwm.h>
#endif /* CONFIG_AW_EPHY_AC300 */

#define SUNXI_GMAC_MODULE_VERSION	"2.0.1"
#define SUNXI_GMAC_POWER_CHAN_NUM	3
#define	SUNXI_GMAC_POWER_CHAR_LENGTH	20

#define SUNXI_GMAC_DMA_DESC_RX		256
#define SUNXI_GMAC_DMA_DESC_TX		256
#define SUNXI_GMAC_BUDGET		(sunxi_gmac_dma_desc_rx / 4)
#define SUNXI_GMAC_TX_THRESH		(sunxi_gmac_dma_desc_tx / 4)

#define SUNXI_GMAC_HASH_TABLE_SIZE	64
#define SUNXI_GMAC_MAX_BUF_SZ		(SZ_2K - 1)

/* SUNXI_GMAC_FRAME_FILTER  register value */
#define SUNXI_GMAC_FRAME_FILTER_PR	0x00000001	/* Promiscuous Mode */
#define SUNXI_GMAC_FRAME_FILTER_HUC	0x00000002	/* Hash Unicast */
#define SUNXI_GMAC_FRAME_FILTER_HMC	0x00000004	/* Hash Multicast */
#define SUNXI_GMAC_FRAME_FILTER_DAIF	0x00000008	/* DA Inverse Filtering */
#define SUNXI_GMAC_FRAME_FILTER_PM	0x00000010	/* Pass all multicast */
#define SUNXI_GMAC_FRAME_FILTER_DBF	0x00000020	/* Disable Broadcast frames */
#define SUNXI_GMAC_FRAME_FILTER_SAIF	0x00000100	/* Inverse Filtering */
#define SUNXI_GMAC_FRAME_FILTER_SAF	0x00000200	/* Source Address Filter */
#define SUNXI_GMAC_FRAME_FILTER_HPF	0x00000400	/* Hash or perfect Filter */
#define SUNXI_GMAC_FRAME_FILTER_RA	0x80000000	/* Receive all mode */

/* Default tx descriptor */
#define SUNXI_GMAC_TX_SINGLE_DESC0	0x80000000
#define SUNXI_GMAC_TX_SINGLE_DESC1	0x63000000

/* Default rx descriptor */
#define SUNXI_GMAC_RX_SINGLE_DESC0	0x80000000
#define SUNXI_GMAC_RX_SINGLE_DESC1	0x83000000

/******************************************************************************
 *	sunxi gmac reg offset
 *****************************************************************************/
#define SUNXI_GMAC_BASIC_CTL0		0x00
#define SUNXI_GMAC_BASIC_CTL1		0x04
#define SUNXI_GMAC_INT_STA		0x08
#define SUNXI_GMAC_INT_EN		0x0C
#define SUNXI_GMAC_TX_CTL0		0x10
#define SUNXI_GMAC_TX_CTL1		0x14
#define SUNXI_GMAC_TX_FLOW_CTL		0x1C
#define SUNXI_GMAC_TX_DESC_LIST		0x20
#define SUNXI_GMAC_RX_CTL0		0x24
#define SUNXI_GMAC_RX_CTL1		0x28
#define SUNXI_GMAC_RX_DESC_LIST		0x34
#define SUNXI_GMAC_RX_FRM_FLT		0x38
#define SUNXI_GMAC_RX_HASH0		0x40
#define SUNXI_GMAC_RX_HASH1		0x44
#define SUNXI_GMAC_MDIO_ADDR		0x48
#define SUNXI_GMAC_MDIO_DATA		0x4C
#define SUNXI_GMAC_ADDR_HI(reg)		(0x50 + ((reg) << 3))
#define SUNXI_GMAC_ADDR_LO(reg)		(0x54 + ((reg) << 3))
#define SUNXI_GMAC_TX_DMA_STA		0xB0
#define SUNXI_GMAC_TX_CUR_DESC		0xB4
#define SUNXI_GMAC_TX_CUR_BUF		0xB8
#define SUNXI_GMAC_RX_DMA_STA		0xC0
#define SUNXI_GMAC_RX_CUR_DESC		0xC4
#define SUNXI_GMAC_RX_CUR_BUF		0xC8
#define SUNXI_GMAC_RGMII_STA		0xD0

#define SUNXI_GMAC_RGMII_IRQ		0x00000001

#define SUNXI_GMAC_CTL0_LM		0x02
#define SUNXI_GMAC_CTL0_DM		0x01
#define SUNXI_GMAC_CTL0_SPEED		0x04

#define SUNXI_GMAC_BURST_LEN		0x3F000000
#define SUNXI_GMAC_RX_TX_PRI		0x02
#define SUNXI_GMAC_SOFT_RST		0x01

#define SUNXI_GMAC_TX_FLUSH		0x01
#define SUNXI_GMAC_TX_MD		0x02
#define SUNXI_GMAC_TX_NEXT_FRM		0x04
#define SUNXI_GMAC_TX_TH		0x0700
#define SUNXI_GMAC_TX_FLOW_CTL_BIT	0x01

#define SUNXI_GMAC_RX_FLUSH		0x01
#define SUNXI_GMAC_RX_MD		0x02
#define SUNXI_GMAC_RX_RUNT_FRM		0x04
#define SUNXI_GMAC_RX_ERR_FRM		0x08
#define SUNXI_GMAC_RX_TH		0x0030
#define SUNXI_GMAC_RX_FLOW_CTL		0x1000000

#define SUNXI_GMAC_TX_INT		0x00001
#define SUNXI_GMAC_TX_STOP_INT		0x00002
#define SUNXI_GMAC_TX_UA_INT		0x00004
#define SUNXI_GMAC_TX_TOUT_INT		0x00008
#define SUNXI_GMAC_TX_UNF_INT		0x00010
#define SUNXI_GMAC_TX_EARLY_INT		0x00020
#define SUNXI_GMAC_RX_INT		0x00100
#define SUNXI_GMAC_RX_UA_INT		0x00200
#define SUNXI_GMAC_RX_STOP_INT		0x00400
#define SUNXI_GMAC_RX_TOUT_INT		0x00800
#define SUNXI_GMAC_RX_OVF_INT		0x01000
#define SUNXI_GMAC_RX_EARLY_INT		0x02000
#define SUNXI_GMAC_LINK_STA_INT		0x10000

#define SUNXI_GMAC_CHAIN_MODE_OFFSET	24
#define SUNXI_GMAC_LOOPBACK_OFFSET	2
#define SUNXI_GMAC_LOOPBACK		0x00000002
#define SUNXI_GMAC_CLEAR_SPEED		0x03
#define SUNXI_GMAC_1000M_SPEED		~0x0c
#define SUNXI_GMAC_100M_SPEED		0x0c
#define SUNXI_GMAC_10M_SPEED		0x08
#define SUNXI_GMAC_RX_FLOW_EN		0x10000
#define SUNXI_GMAC_TX_FLOW_EN		0x00001
#define SUNXI_GMAC_PAUSE_OFFSET		4
#define SUNXI_GMAC_INT_OFFSET		0x3fff
#define SUNXI_GMAC_RX_DMA_EN		0x40000000
#define SUNXI_GMAC_TX_DMA_EN		0x40000000
#define SUNXI_GMAC_BURST_VALUE		8
#define SUNXI_GMAC_BURST_OFFSET		24
#define SUNXI_GMAC_SF_DMA_MODE		1
#define SUNXI_GMAC_TX_FRM_LEN_OFFSET	30
#define SUNXI_GMAC_CRC_OFFSET		27
#define SUNXI_GMAC_STRIP_FCS_OFFSET	28
#define SUNXI_GMAC_JUMBO_EN_OFFSET	29
#define SUNXI_GMAC_MDC_DIV_RATIO_M	0x03
#define SUNXI_GMAC_MDC_DIV_OFFSET	20
#define SUNXI_GMAC_TX_DMA_TH64		64
#define SUNXI_GMAC_TX_DMA_TH128		128
#define SUNXI_GMAC_TX_DMA_TH192		192
#define SUNXI_GMAC_TX_DMA_TH256		256
#define SUNXI_GMAC_TX_DMA_TH64_VAL	0x00000000
#define SUNXI_GMAC_TX_DMA_TH128_VAL	0X00000100
#define SUNXI_GMAC_TX_DMA_TH192_VAL	0x00000200
#define SUNXI_GMAC_TX_DMA_TH256_VAL	0x00000300
#define SUNXI_GMAC_RX_DMA_TH32		32
#define SUNXI_GMAC_RX_DMA_TH64		64
#define SUNXI_GMAC_RX_DMA_TH96		96
#define SUNXI_GMAC_RX_DMA_TH128		128
#define SUNXI_GMAC_RX_DMA_TH32_VAL	0x10
#define SUNXI_GMAC_RX_DMA_TH64_VAL	0x00
#define SUNXI_GMAC_RX_DMA_TH96_VAL	0x20
#define SUNXI_GMAC_RX_DMA_TH128_VAL	0x30
#define SUNXI_GMAC_TX_DMA_START		31
#define SUNXI_GMAC_RX_DMA_START		31
#define SUNXI_GMAC_DMA_DESC_BUFSIZE	11
#define SUNXI_GMAC_LOOPBACK_OFF		0
#define SUNXI_GMAC_MAC_LOOPBACK_ON	1
#define SUNXI_GMAC_PHY_LOOPBACK_ON	2
#define SUNXI_GMAC_OWN_DMA		0x80000000
#define SUNXI_GMAC_GPHY_TEST_OFFSET	13
#define SUNXI_GMAC_GPHY_TEST_MASK	0x07
#define SUNXI_GMAC_PHY_RGMII_MASK	0x00000004
#define SUNXI_GMAC_ETCS_RMII_MASK	0x00002003
#define SUNXI_GMAC_RGMII_INTCLK_MASK	0x00000002
#define SUNXI_GMAC_RMII_MASK		0x00002000
#define SUNXI_GMAC_TX_DELAY_MASK	0x07
#define SUNXI_GMAC_TX_DELAY_OFFSET	10
#define SUNXI_GMAC_RX_DELAY_MASK	0x1F
#define SUNXI_GMAC_RX_DELAY_OFFSET	5
/* Flow Control defines */
#define SUNXI_GMAC_FLOW_OFF		0
#define SUNXI_GMAC_FLOW_RX		1
#define SUNXI_GMAC_FLOW_TX		2
#define SUNXI_GMAC_FLOW_AUTO		(SUNXI_GMAC_FLOW_TX | SUNXI_GMAC_FLOW_RX)

/* Ring buffer caculate method */
#define circ_cnt(head, tail, size) (((head) > (tail)) ? \
					((head) - (tail)) : \
					((head) - (tail)) & ((size) - 1))

#define circ_space(head, tail, size) circ_cnt((tail), ((head) + 1), (size))

#define circ_inc(n, s) (((n) + 1) % (s))

#define MAC_ADDR_LEN			18
#define SUNXI_GMAC_MAC_ADDRESS		"00:00:00:00:00:00"
static char mac_str[MAC_ADDR_LEN] = SUNXI_GMAC_MAC_ADDRESS;
module_param_string(mac_str, mac_str, MAC_ADDR_LEN, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(mac_str, "MAC Address String.(xx:xx:xx:xx:xx:xx)");

static int rxmode = 1;
module_param(rxmode, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(rxmode, "DMA threshold control value");

static int txmode = 1;
module_param(txmode, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(txmode, "DMA threshold control value");

static int pause = 0x400;
module_param(pause, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(pause, "Flow Control Pause Time");

#define TX_TIMEO	5000
static int watchdog = TX_TIMEO;
module_param(watchdog, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(watchdog, "Transmit timeout in milliseconds");

static int sunxi_gmac_dma_desc_rx = SUNXI_GMAC_DMA_DESC_RX;
module_param(sunxi_gmac_dma_desc_rx, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(sunxi_gmac_dma_desc_rx, "The number of receive's descriptors");

static int sunxi_gmac_dma_desc_tx = SUNXI_GMAC_DMA_DESC_TX;
module_param(sunxi_gmac_dma_desc_tx, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(sunxi_gmac_dma_desc_tx, "The number of transmit's descriptors");

/* - 0: Flow Off
 * - 1: Rx Flow
 * - 2: Tx Flow
 * - 3: Rx & Tx Flow
 */
static int flow_ctrl;
module_param(flow_ctrl, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(flow_ctrl, "Flow control [0: off, 1: rx, 2: tx, 3: both]");

typedef union {
	struct {
		/* TDES0 */
		unsigned int deferred:1;	/* Deferred bit (only half-duplex) */
		unsigned int under_err:1;	/* Underflow error */
		unsigned int ex_deferral:1;	/* Excessive deferral */
		unsigned int coll_cnt:4;	/* Collision count */
		unsigned int vlan_tag:1;	/* VLAN Frame */
		unsigned int ex_coll:1;		/* Excessive collision */
		unsigned int late_coll:1;	/* Late collision */
		unsigned int no_carr:1;		/* No carrier */
		unsigned int loss_carr:1;	/* Loss of collision */
		unsigned int ipdat_err:1;	/* IP payload error */
		unsigned int frm_flu:1;		/* Frame flushed */
		unsigned int jab_timeout:1;	/* Jabber timeout */
		unsigned int err_sum:1;		/* Error summary */
		unsigned int iphead_err:1;	/* IP header error */
		unsigned int ttss:1;		/* Transmit time stamp status */
		unsigned int reserved0:13;
		unsigned int own:1;		/* Own bit. CPU:0, DMA:1 */
	} tx;

	/* bits 5 7 0 | Frame status
	 * ----------------------------------------------------------
	 *      0 0 0 | IEEE 802.3 Type frame (length < 1536 octects)
	 *      1 0 0 | IPv4/6 No CSUM errorS.
	 *      1 0 1 | IPv4/6 CSUM PAYLOAD error
	 *      1 1 0 | IPv4/6 CSUM IP HR error
	 *      1 1 1 | IPv4/6 IP PAYLOAD AND HEADER errorS
	 *      0 0 1 | IPv4/6 unsupported IP PAYLOAD
	 *      0 1 1 | COE bypassed.. no IPv4/6 frame
	 *      0 1 0 | Reserved.
	 */
	struct {
		/* RDES0 */
		unsigned int chsum_err:1;	/* Payload checksum error */
		unsigned int crc_err:1;		/* CRC error */
		unsigned int dribbling:1;	/* Dribble bit error */
		unsigned int mii_err:1;		/* Received error (bit3) */
		unsigned int recv_wt:1;		/* Received watchdog timeout */
		unsigned int frm_type:1;	/* Frame type */
		unsigned int late_coll:1;	/* Late Collision */
		unsigned int ipch_err:1;	/* IPv header checksum error (bit7) */
		unsigned int last_desc:1;	/* Laset descriptor */
		unsigned int first_desc:1;	/* First descriptor */
		unsigned int vlan_tag:1;	/* VLAN Tag */
		unsigned int over_err:1;	/* Overflow error (bit11) */
		unsigned int len_err:1;		/* Length error */
		unsigned int sou_filter:1;	/* Source address filter fail */
		unsigned int desc_err:1;	/* Descriptor error */
		unsigned int err_sum:1;		/* Error summary (bit15) */
		unsigned int frm_len:14;	/* Frame length */
		unsigned int des_filter:1;	/* Destination address filter fail */
		unsigned int own:1;		/* Own bit. CPU:0, DMA:1 */
		#define RX_PKT_OK		0x7FFFB77C
		#define RX_LEN			0x3FFF0000
	} rx;

	unsigned int all;
} sunxi_gmac_desc0_u;

typedef union {
	struct {
		/* TDES1 */
		unsigned int buf1_size:11;	/* Transmit buffer1 size */
		unsigned int buf2_size:11;	/* Transmit buffer2 size */
		unsigned int ttse:1;		/* Transmit time stamp enable */
		unsigned int dis_pad:1;		/* Disable pad (bit23) */
		unsigned int adr_chain:1;	/* Second address chained */
		unsigned int end_ring:1;	/* Transmit end of ring */
		unsigned int crc_dis:1;		/* Disable CRC */
		unsigned int cic:2;		/* Checksum insertion control (bit27:28) */
		unsigned int first_sg:1;	/* First Segment */
		unsigned int last_seg:1;	/* Last Segment */
		unsigned int interrupt:1;	/* Interrupt on completion */
	} tx;

	struct {
		/* RDES1 */
		unsigned int buf1_size:11;	/* Received buffer1 size */
		unsigned int buf2_size:11;	/* Received buffer2 size */
		unsigned int reserved1:2;
		unsigned int adr_chain:1;	/* Second address chained */
		unsigned int end_ring:1;	/* Received end of ring */
		unsigned int reserved2:5;
		unsigned int dis_ic:1;		/* Disable interrupt on completion */
	} rx;

	unsigned int all;
} sunxi_gmac_desc1_u;

typedef struct sunxi_gmac_dma_desc {
	sunxi_gmac_desc0_u desc0;
	sunxi_gmac_desc1_u desc1;
	/* The address of buffers */
	unsigned int	desc2;
	/* Next desc's address */
	unsigned int	desc3;
} __attribute__((packed)) sunxi_gmac_dma_desc_t;

enum rx_frame_status { /* IPC status */
	good_frame = 0,
	discard_frame = 1,
	csum_none = 2,
	llc_snap = 4,
};

enum tx_dma_irq_status {
	tx_hard_error = 1,
	tx_hard_error_bump_tc = 2,
	handle_tx_rx = 3,
};

struct sunxi_gmac_extra_stats {
	/* Transmit errors */
	unsigned long tx_underflow;
	unsigned long tx_carrier;
	unsigned long tx_losscarrier;
	unsigned long vlan_tag;
	unsigned long tx_deferred;
	unsigned long tx_vlan;
	unsigned long tx_jabber;
	unsigned long tx_frame_flushed;
	unsigned long tx_payload_error;
	unsigned long tx_ip_header_error;

	/* Receive errors */
	unsigned long rx_desc;
	unsigned long sa_filter_fail;
	unsigned long overflow_error;
	unsigned long ipc_csum_error;
	unsigned long rx_collision;
	unsigned long rx_crc;
	unsigned long dribbling_bit;
	unsigned long rx_length;
	unsigned long rx_mii;
	unsigned long rx_multicast;
	unsigned long rx_gmac_overflow;
	unsigned long rx_watchdog;
	unsigned long da_rx_filter_fail;
	unsigned long sa_rx_filter_fail;
	unsigned long rx_missed_cntr;
	unsigned long rx_overflow_cntr;
	unsigned long rx_vlan;

	/* Tx/Rx IRQ errors */
	unsigned long tx_undeflow_irq;
	unsigned long tx_process_stopped_irq;
	unsigned long tx_jabber_irq;
	unsigned long rx_overflow_irq;
	unsigned long rx_buf_unav_irq;
	unsigned long rx_process_stopped_irq;
	unsigned long rx_watchdog_irq;
	unsigned long tx_early_irq;
	unsigned long fatal_bus_error_irq;

	/* Extra info */
	unsigned long threshold;
	unsigned long tx_pkt_n;
	unsigned long rx_pkt_n;
	unsigned long poll_n;
	unsigned long sched_timer_n;
	unsigned long normal_irq_n;
};

struct sunxi_gmac {
	struct sunxi_gmac_dma_desc *dma_tx;	/* Tx dma descriptor */
	struct sk_buff **tx_skb;		/* Tx socket buffer array */
	unsigned int tx_clean;			/* Tx ring buffer data consumer */
	unsigned int tx_dirty;			/* Tx ring buffer data provider */
	dma_addr_t dma_tx_phy;			/* Tx dma physical address */

	unsigned long buf_sz;			/* Size of buffer specified by current descriptor */

	struct sunxi_gmac_dma_desc *dma_rx;	/* Rx dma descriptor */
	struct sk_buff **rx_skb;			/* Rx socket buffer array */
	unsigned int rx_clean;			/* Rx ring buffer data consumer */
	unsigned int rx_dirty;			/* Rx ring buffer data provider */
	dma_addr_t dma_rx_phy;			/* Rx dma physical address */

	struct net_device *ndev;
	struct device *dev;
	struct napi_struct napi;

	struct sunxi_gmac_extra_stats xstats;	/* Additional network statistics */

	bool link;				/* Phy link status */
	int speed;				/* NIC network speed */
	int duplex;				/* NIC network duplex capability */

#define SUNXI_EXTERNAL_PHY		1
#define SUNXI_INTERNAL_PHY		0
	u32 phy_type;				/* 1: External phy, 0: Internal phy */

#define SUNXI_PHY_USE_CLK25M		0	/* External phy use phy25m clk provided by Soc */
#define SUNXI_PHY_USE_EXT_OSC		1	/* External phy use extern osc 25m */
	u32 phy_clk_type;

	phy_interface_t phy_interface;
	void __iomem *base;
	void __iomem *syscfg_base;
	struct clk *gmac_clk;
	struct clk *phy25m_clk;
	struct reset_control *reset;
	struct pinctrl *pinctrl;

	struct regulator *gmac_supply[SUNXI_GMAC_POWER_CHAN_NUM];
	u32 gmac_supply_vol[SUNXI_GMAC_POWER_CHAN_NUM];

	/* definition spinlock */
	spinlock_t universal_lock;		/* universal spinlock */
	spinlock_t tx_lock;			/* tx tramsmit spinlock */

	/* adjust transmit clock delay, value: 0~7 */
	/* adjust receive clock delay, value: 0~31 */
	u32 tx_delay;
	u32 rx_delay;

	struct device_node *phy_node;

#ifdef CONFIG_AW_EPHY_AC300
	struct device_node *ac300_np;
	struct phy_device *ac300_dev;
	struct pwm_device *ac300_pwm;
	u32 pwm_channel;
#define PWM_DUTY_NS		205
#define PWM_PERIOD_NS		410
#endif /* CONFIG_AW_EPHY_AC300 */
};

/**
 * sunxi_gmac_desc_init_chain - GMAC dma descriptor chain table initialization
 *
 * @desc:	Dma descriptor
 * @addr:	Dma descriptor physical address
 * @size:	Dma descriptor numsa
 *
 * Called when the NIC is up. We init Tx/Rx dma descriptor table.
 */
static void sunxi_gmac_desc_init_chain(struct sunxi_gmac_dma_desc *desc, unsigned long addr, unsigned int size)
{
	/* In chained mode the desc3 points to the next element in the ring.
	 * The latest element has to point to the head.
	 */
	int i;
	struct sunxi_gmac_dma_desc *p = desc;
	unsigned long dma_phy = addr;

	for (i = 0; i < (size - 1); i++) {
		dma_phy += sizeof(struct sunxi_gmac_dma_desc);
		p->desc3 = (unsigned int)dma_phy;
		/* Chain mode */
		p->desc1.all |= (1 << SUNXI_GMAC_CHAIN_MODE_OFFSET);
		p++;
	}
	p->desc1.all |= (1 << SUNXI_GMAC_CHAIN_MODE_OFFSET);
	p->desc3 = (unsigned int)addr;
}

/**
 * sunxi_gmac_set_link_mode - GMAC speed/duplex set func
 *
 * @iobase:	Gmac membase
 * @duplex:	Duplex capability:half/full
 * @speed:	Speed:10M/100M/1000M
 *
 * Updates phy status and takes action for network queue if required
 * based upon link status.
 */
static void sunxi_gmac_set_link_mode(void *iobase, int duplex, int speed)
{
	unsigned int ctrl = readl(iobase + SUNXI_GMAC_BASIC_CTL0);

	if (!duplex)
		ctrl &= ~SUNXI_GMAC_CTL0_DM;
	else
		ctrl |= SUNXI_GMAC_CTL0_DM;

	/* clear ctrl speed */
	ctrl &= SUNXI_GMAC_CLEAR_SPEED;

	switch (speed) {
	case 1000:
		ctrl &= SUNXI_GMAC_1000M_SPEED;
		break;
	case 100:
		ctrl |= SUNXI_GMAC_100M_SPEED;
		break;
	case 10:
		ctrl |= SUNXI_GMAC_10M_SPEED;
		break;
	default:
		break;
	}

	writel(ctrl, iobase + SUNXI_GMAC_BASIC_CTL0);
}

/**
 * sunxi_gmac_loop - GMAC loopback mode set func
 *
 * @iobase:		Gmac membase
 * @loopback_enable:	Loopback status
 */
static void sunxi_gmac_loopback(void *iobase, int loopback_enable)
{
	int reg;

	reg = readl(iobase + SUNXI_GMAC_BASIC_CTL0);
	if (loopback_enable)
		reg |= SUNXI_GMAC_LOOPBACK_OFFSET;
	else
		reg &= ~SUNXI_GMAC_LOOPBACK_OFFSET;
	writel(reg, iobase + SUNXI_GMAC_BASIC_CTL0);
}

/**
 * sunxi_gmac_flow_ctrl - GMAC flow ctrl set func
 *
 * @iobase:	Gmac membase
 * @duolex:	Duplex capability
 * @fc:		Flow control option
 * @pause:	Flow control pause time
 */
static void sunxi_gmac_flow_ctrl(void *iobase, int duplex, int fc, int pause)
{
	unsigned int flow;

	if (fc & SUNXI_GMAC_FLOW_RX) {
		flow = readl(iobase + SUNXI_GMAC_RX_CTL0);
		flow |= SUNXI_GMAC_RX_FLOW_EN;
		writel(flow, iobase + SUNXI_GMAC_RX_CTL0);
	}

	if (fc & SUNXI_GMAC_FLOW_TX) {
		flow = readl(iobase + SUNXI_GMAC_TX_FLOW_CTL);
		flow |= SUNXI_GMAC_TX_FLOW_EN;
		writel(flow, iobase + SUNXI_GMAC_TX_FLOW_CTL);
	}

	if (duplex) {
		flow = readl(iobase + SUNXI_GMAC_TX_FLOW_CTL);
		flow |= (pause << SUNXI_GMAC_PAUSE_OFFSET);
		writel(flow, iobase + SUNXI_GMAC_TX_FLOW_CTL);
	}
}

/**
 * sunxi_gmac_int_status - GMAC get int status func
 *
 * @iobase:	Gmac membase
 * @x:		Extra statistics
 */
static int sunxi_gmac_int_status(void *iobase, struct sunxi_gmac_extra_stats *x)
{
	int ret;
	/* read the status register (CSR5) */
	unsigned int intr_status;

	intr_status = readl(iobase + SUNXI_GMAC_RGMII_STA);
	if (intr_status & SUNXI_GMAC_RGMII_IRQ)
		readl(iobase + SUNXI_GMAC_RGMII_STA);

	intr_status = readl(iobase + SUNXI_GMAC_INT_STA);

	/* ABNORMAL interrupts */
	if (intr_status & SUNXI_GMAC_TX_UNF_INT) {
		ret = tx_hard_error_bump_tc;
		x->tx_undeflow_irq++;
	}
	if (intr_status & SUNXI_GMAC_TX_TOUT_INT)
		x->tx_jabber_irq++;

	if (intr_status & SUNXI_GMAC_RX_OVF_INT)
		x->rx_overflow_irq++;

	if (intr_status & SUNXI_GMAC_RX_UA_INT)
		x->rx_buf_unav_irq++;

	if (intr_status & SUNXI_GMAC_RX_STOP_INT)
		x->rx_process_stopped_irq++;

	if (intr_status & SUNXI_GMAC_RX_TOUT_INT)
		x->rx_watchdog_irq++;

	if (intr_status & SUNXI_GMAC_TX_EARLY_INT)
		x->tx_early_irq++;

	if (intr_status & SUNXI_GMAC_TX_STOP_INT) {
		x->tx_process_stopped_irq++;
		ret = tx_hard_error;
	}

	/* TX/RX NORMAL interrupts */
	if (intr_status & (SUNXI_GMAC_TX_INT | SUNXI_GMAC_RX_INT | SUNXI_GMAC_RX_EARLY_INT | SUNXI_GMAC_TX_UA_INT)) {
		x->normal_irq_n++;
		if (intr_status & (SUNXI_GMAC_TX_INT | SUNXI_GMAC_RX_INT))
			ret = handle_tx_rx;
	}
	/* Clear the interrupt by writing a logic 1 to the CSR5[15-0] */
	writel(intr_status & SUNXI_GMAC_INT_OFFSET, iobase + SUNXI_GMAC_INT_STA);

	return ret;
}

/**
 * sunxi_gmac_enable_rx - enable gmac rx dma
 *
 * @iobase:	Gmac membase
 * @rxbase:	Base address of Rx descriptor
 */
static void sunxi_gmac_enable_rx(void *iobase, unsigned long rxbase)
{
	unsigned int value;

	/* Write the base address of Rx descriptor lists into registers */
	writel(rxbase, iobase + SUNXI_GMAC_RX_DESC_LIST);

	value = readl(iobase + SUNXI_GMAC_RX_CTL1);
	value |= SUNXI_GMAC_RX_DMA_EN;
	writel(value, iobase + SUNXI_GMAC_RX_CTL1);
}

static int sunxi_gmac_read_rx_flowctl(void *iobase)
{
	unsigned int value;

	value = readl(iobase + SUNXI_GMAC_RX_CTL1);

	return value & SUNXI_GMAC_RX_FLOW_CTL;
}

static int sunxi_gmac_read_tx_flowctl(void *iobase)
{
	unsigned int value;

	value = readl(iobase + SUNXI_GMAC_TX_FLOW_CTL);

	return value & SUNXI_GMAC_TX_FLOW_CTL_BIT;
}

static void sunxi_gmac_write_rx_flowctl(void *iobase, bool flag)
{
	unsigned int value;

	value = readl(iobase + SUNXI_GMAC_RX_CTL1);

	if (flag)
		value |= SUNXI_GMAC_RX_FLOW_CTL;
	else
		value &= ~SUNXI_GMAC_RX_FLOW_CTL;

	writel(value, iobase + SUNXI_GMAC_RX_CTL1);
}

static void sunxi_gmac_write_tx_flowctl(void *iobase, bool flag)
{
	unsigned int value;

	value = readl(iobase + SUNXI_GMAC_TX_FLOW_CTL);

	if (flag)
		value |= SUNXI_GMAC_TX_FLOW_CTL_BIT;
	else
		value &= ~SUNXI_GMAC_TX_FLOW_CTL_BIT;

	writel(value, iobase + SUNXI_GMAC_TX_FLOW_CTL);
}

/**
 * sunxi_gmac_enable_tx - enable gmac tx dma
 *
 * @iobase:	Gmac membase
 * @rxbase:	Base address of Tx descriptor
 */
static void sunxi_gmac_enable_tx(void *iobase, unsigned long txbase)
{
	unsigned int value;

	/* Write the base address of Tx descriptor lists into registers */
	writel(txbase, iobase + SUNXI_GMAC_TX_DESC_LIST);

	value = readl(iobase + SUNXI_GMAC_TX_CTL1);
	value |= SUNXI_GMAC_TX_DMA_EN;
	writel(value, iobase + SUNXI_GMAC_TX_CTL1);
}

/**
 * sunxi_gmac_disable_tx - disable gmac tx dma
 *
 * @iobase:	Gmac membase
 * @rxbase:	Base address of Tx descriptor
 */
static void sunxi_gmac_disable_tx(void *iobase)
{
	unsigned int value = readl(iobase + SUNXI_GMAC_TX_CTL1);

	value &= ~SUNXI_GMAC_TX_DMA_EN;
	writel(value, iobase + SUNXI_GMAC_TX_CTL1);
}

static int sunxi_gmac_dma_init(void *iobase)
{
	unsigned int value;

	/* Burst should be 8 */
	value = (SUNXI_GMAC_BURST_VALUE << SUNXI_GMAC_BURST_OFFSET);

#ifdef CONFIG_SUNXI_GMAC_DA
	value |= SUNXI_GMAC_RX_TX_PRI;	/* Rx has priority over tx */
#endif
	writel(value, iobase + SUNXI_GMAC_BASIC_CTL1);

	/* Mask interrupts by writing to CSR7 */
	writel(SUNXI_GMAC_RX_INT | SUNXI_GMAC_TX_UNF_INT, iobase + SUNXI_GMAC_INT_EN);

	return 0;
}

/**
 * sunxi_gmac_init - init gmac config
 *
 * @iobase:	Gmac membase
 * @txmode:	tx flow control mode
 * @rxmode:	rx flow control mode
 */
static int sunxi_gmac_init(void *iobase, int txmode, int rxmode)
{
	unsigned int value;

	sunxi_gmac_dma_init(iobase);

	/* Initialize the core component */
	value = readl(iobase + SUNXI_GMAC_TX_CTL0);
	value |= (1 << SUNXI_GMAC_TX_FRM_LEN_OFFSET);
	writel(value, iobase + SUNXI_GMAC_TX_CTL0);

	value = readl(iobase + SUNXI_GMAC_RX_CTL0);
	value |= (1 << SUNXI_GMAC_CRC_OFFSET);			/* Enable CRC & IPv4 Header Checksum */
	value |= (1 << SUNXI_GMAC_STRIP_FCS_OFFSET);		/* Automatic Pad/CRC Stripping */
	value |= (1 << SUNXI_GMAC_JUMBO_EN_OFFSET);		/* Jumbo Frame Enable */
	writel(value, iobase + SUNXI_GMAC_RX_CTL0);

	writel((SUNXI_GMAC_MDC_DIV_RATIO_M << SUNXI_GMAC_MDC_DIV_OFFSET),
			iobase + SUNXI_GMAC_MDIO_ADDR);		/* MDC_DIV_RATIO */

	/* Set the Rx&Tx mode */
	value = readl(iobase + SUNXI_GMAC_TX_CTL1);
	if (txmode == SUNXI_GMAC_SF_DMA_MODE) {
		/* Transmit COE type 2 cannot be done in cut-through mode. */
		value |= SUNXI_GMAC_TX_MD;
		/* Operating on second frame increase the performance
		 * especially when transmit store-and-forward is used.
		 */
		value |= SUNXI_GMAC_TX_NEXT_FRM;
	} else {
		value &= ~SUNXI_GMAC_TX_MD;
		value &= ~SUNXI_GMAC_TX_TH;
		/* Set the transmit threshold */
		if (txmode <= SUNXI_GMAC_TX_DMA_TH64)
			value |= SUNXI_GMAC_TX_DMA_TH64_VAL;
		else if (txmode <= SUNXI_GMAC_TX_DMA_TH128)
			value |= SUNXI_GMAC_TX_DMA_TH128_VAL;
		else if (txmode <= SUNXI_GMAC_TX_DMA_TH192)
			value |= SUNXI_GMAC_TX_DMA_TH192_VAL;
		else
			value |= SUNXI_GMAC_TX_DMA_TH256_VAL;
	}
	writel(value, iobase + SUNXI_GMAC_TX_CTL1);

	value = readl(iobase + SUNXI_GMAC_RX_CTL1);
	if (rxmode == SUNXI_GMAC_SF_DMA_MODE) {
		value |= SUNXI_GMAC_RX_MD;
	} else {
		value &= ~SUNXI_GMAC_RX_MD;
		value &= ~SUNXI_GMAC_RX_TH;
		if (rxmode <= SUNXI_GMAC_RX_DMA_TH32)
			value |= SUNXI_GMAC_RX_DMA_TH32_VAL;
		else if (rxmode <= SUNXI_GMAC_RX_DMA_TH64)
			value |= SUNXI_GMAC_RX_DMA_TH64_VAL;
		else if (rxmode <= SUNXI_GMAC_RX_DMA_TH96)
			value |= SUNXI_GMAC_RX_DMA_TH96_VAL;
		else
			value |= SUNXI_GMAC_RX_DMA_TH128_VAL;
	}

	/* Forward frames with error and undersized good frame. */
	value |= (SUNXI_GMAC_RX_ERR_FRM | SUNXI_GMAC_RX_RUNT_FRM);

	writel(value, iobase + SUNXI_GMAC_RX_CTL1);

	return 0;
}

static void sunxi_gmac_hash_filter(void *iobase, unsigned long low, unsigned long high)
{
	writel(high, iobase + SUNXI_GMAC_RX_HASH0);
	writel(low, iobase + SUNXI_GMAC_RX_HASH1);
}

static void sunxi_gmac_set_filter(void *iobase, unsigned int flags)
{
	int tmp_flags = 0;

	/* TODO: replace numbers with marcos */
	tmp_flags |= (((flags >> 9) & 0x00000002) |
			((flags << 1) & 0x00000010) |
			((flags >> 3) & 0x00000060) |
			((flags << 7) & 0x00000300) |
			((flags << 6) & 0x00003000) |
			((flags << 12) & 0x00030000));

	writel(tmp_flags, iobase + SUNXI_GMAC_RX_FRM_FLT);
}

/* write macaddr into MAC register */
static void sunxi_gmac_set_mac_addr_to_reg(void *iobase, unsigned char *addr, int index)
{
	unsigned long data;

	/* one char is 8bit, so splice mac address in steps of 8 */
	data = (addr[5] << 8) | addr[4];
	writel(data, iobase + SUNXI_GMAC_ADDR_HI(index));
	data = (addr[3] << 24) | (addr[2] << 16) | (addr[1] << 8) | addr[0];
	writel(data, iobase + SUNXI_GMAC_ADDR_LO(index));
}

static void sunxi_gmac_dma_start(void *iobase)
{
	unsigned long value;

	value = readl(iobase + SUNXI_GMAC_TX_CTL0);
	value |= (1 << SUNXI_GMAC_TX_DMA_START);
	writel(value, iobase + SUNXI_GMAC_TX_CTL0);

	value = readl(iobase + SUNXI_GMAC_RX_CTL0);
	value |= (1 << SUNXI_GMAC_RX_DMA_START);
	writel(value, iobase + SUNXI_GMAC_RX_CTL0);
}

static void sunxi_gmac_dma_stop(void *iobase)
{
	unsigned long value;

	value = readl(iobase + SUNXI_GMAC_TX_CTL0);
	value &= ~(1 << SUNXI_GMAC_TX_DMA_START);
	writel(value, iobase + SUNXI_GMAC_TX_CTL0);

	value = readl(iobase + SUNXI_GMAC_RX_CTL0);
	value &= ~(1 << SUNXI_GMAC_RX_DMA_START);
	writel(value, iobase + SUNXI_GMAC_RX_CTL0);
}

static void sunxi_gmac_tx_poll(void *iobase)
{
	unsigned int value;

	value = readl(iobase + SUNXI_GMAC_TX_CTL1);
	writel(value | (1 << SUNXI_GMAC_TX_DMA_START), iobase + SUNXI_GMAC_TX_CTL1);
}

static void sunxi_gmac_irq_enable(void *iobase)
{
	writel(SUNXI_GMAC_RX_INT | SUNXI_GMAC_TX_UNF_INT, iobase + SUNXI_GMAC_INT_EN);
}

static void sunxi_gmac_irq_disable(void *iobase)
{
	writel(0, iobase + SUNXI_GMAC_INT_EN);
}

static void sunxi_gmac_desc_buf_set(struct sunxi_gmac_dma_desc *desc, unsigned long paddr, int size)
{
	desc->desc1.all &= (~((1 << SUNXI_GMAC_DMA_DESC_BUFSIZE) - 1));
	desc->desc1.all |= (size & ((1 << SUNXI_GMAC_DMA_DESC_BUFSIZE) - 1));
	desc->desc2 = paddr;
}

static void sunxi_gmac_desc_set_own(struct sunxi_gmac_dma_desc *desc)
{
	desc->desc0.all |= SUNXI_GMAC_OWN_DMA;
}

static void sunxi_gmac_desc_tx_close(struct sunxi_gmac_dma_desc *first, struct sunxi_gmac_dma_desc *end, int csum_insert)
{
	struct sunxi_gmac_dma_desc *desc = first;

	first->desc1.tx.first_sg = 1;
	end->desc1.tx.last_seg = 1;
	end->desc1.tx.interrupt = 1;

	if (csum_insert)
		do {
			desc->desc1.tx.cic = 3;
			desc++;
		} while (desc <= end);
}

static void sunxi_gmac_desc_init(struct sunxi_gmac_dma_desc *desc)
{
	desc->desc1.all = 0;
	desc->desc2  = 0;
	desc->desc1.all |= (1 << SUNXI_GMAC_CHAIN_MODE_OFFSET);
}

static int sunxi_gmac_desc_get_tx_status(struct sunxi_gmac_dma_desc *desc, struct sunxi_gmac_extra_stats *x)
{
	int ret = 0;

	if (desc->desc0.tx.under_err) {
		x->tx_underflow++;
		ret = -EIO;
	}

	if (desc->desc0.tx.no_carr) {
		x->tx_carrier++;
		ret = -EIO;
	}

	if (desc->desc0.tx.loss_carr) {
		x->tx_losscarrier++;
		ret = -EIO;
	}

	if (desc->desc0.tx.deferred) {
		x->tx_deferred++;
		ret = -EIO;
	}

	return ret;
}

static int sunxi_gmac_desc_buf_get_len(struct sunxi_gmac_dma_desc *desc)
{
	return (desc->desc1.all & ((1 << SUNXI_GMAC_DMA_DESC_BUFSIZE) - 1));
}

static int sunxi_gmac_desc_buf_get_addr(struct sunxi_gmac_dma_desc *desc)
{
	return desc->desc2;
}

static int sunxi_gmac_desc_rx_frame_len(struct sunxi_gmac_dma_desc *desc)
{
	return desc->desc0.rx.frm_len;
}

static int sunxi_gmac_desc_llc_snap(struct sunxi_gmac_dma_desc *desc)
{
	/* Splice flags as follow:
	 * bits 5 7 0 | Frame status
	 * ----------------------------------------------------------
	 *      0 0 0 | IEEE 802.3 Type frame (length < 1536 octects)
	 *      1 0 0 | IPv4/6 No CSUM errorS.
	 *      1 0 1 | IPv4/6 CSUM PAYLOAD error
	 *      1 1 0 | IPv4/6 CSUM IP HR error
	 *      1 1 1 | IPv4/6 IP PAYLOAD AND HEADER errorS
	 *      0 0 1 | IPv4/6 unsupported IP PAYLOAD
	 *      0 1 1 | COE bypassed.. no IPv4/6 frame
	 *      0 1 0 | Reserved.
	 */
	return ((desc->desc0.rx.frm_type << 2 |
			desc->desc0.rx.ipch_err << 1 |
			desc->desc0.rx.chsum_err) & 0x7);
}

static int sunxi_gmac_desc_get_rx_status(struct sunxi_gmac_dma_desc *desc, struct sunxi_gmac_extra_stats *x)
{
	int ret = good_frame;

	if (desc->desc0.rx.last_desc == 0) {
		return discard_frame;
	}

	if (desc->desc0.rx.err_sum) {
		if (desc->desc0.rx.desc_err)
			x->rx_desc++;

		if (desc->desc0.rx.sou_filter)
			x->sa_filter_fail++;

		if (desc->desc0.rx.over_err)
			x->overflow_error++;

		if (desc->desc0.rx.ipch_err)
			x->ipc_csum_error++;

		if (desc->desc0.rx.late_coll)
			x->rx_collision++;

		if (desc->desc0.rx.crc_err)
			x->rx_crc++;

		ret = discard_frame;
	}

	if (desc->desc0.rx.len_err) {
		ret = discard_frame;
	}
	if (desc->desc0.rx.mii_err) {
		ret = discard_frame;
	}

	if (ret == good_frame) {
		if (sunxi_gmac_desc_llc_snap(desc) == 0)
			ret = llc_snap;
	}

	return ret;
}

static int sunxi_gmac_desc_get_own(struct sunxi_gmac_dma_desc *desc)
{
	return desc->desc0.all & SUNXI_GMAC_OWN_DMA;
}

static int sunxi_gmac_desc_get_tx_last_seg(struct sunxi_gmac_dma_desc *desc)
{
	return desc->desc1.tx.last_seg;
}

static int sunxi_gmac_reset(void *iobase, int n)
{
	unsigned int value;

	/* gmac software reset */
	value = readl(iobase + SUNXI_GMAC_BASIC_CTL1);
	value |= SUNXI_GMAC_SOFT_RST;
	writel(value, iobase + SUNXI_GMAC_BASIC_CTL1);

	udelay(n);

	return !!(readl(iobase + SUNXI_GMAC_BASIC_CTL1) & SUNXI_GMAC_SOFT_RST);
}

static int sunxi_gmac_stop(struct net_device *ndev);

static void sunxi_gmac_dump_dma_desc(struct sunxi_gmac_dma_desc *desc, int size)
{
#ifdef DEBUG
	int i;

	for (i = 0; i < size; i++) {
		u32 *x = (u32 *)(desc + i);

		pr_info("\t%d [0x%08lx]: %08x %08x %08x %08x\n",
			i, (unsigned long)(&desc[i]),
			x[0], x[1], x[2], x[3]);
	}
	pr_info("\n");
#endif
}

static ssize_t sunxi_gmac_extra_tx_stats_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct net_device *ndev = dev_get_drvdata(dev);
	struct sunxi_gmac *chip = netdev_priv(ndev);

	return sprintf(buf, "tx_underflow: %lu\ntx_carrier: %lu\n"
			"tx_losscarrier: %lu\nvlan_tag: %lu\n"
			"tx_deferred: %lu\ntx_vlan: %lu\n"
			"tx_jabber: %lu\ntx_frame_flushed: %lu\n"
			"tx_payload_error: %lu\ntx_ip_header_error: %lu\n\n",
			chip->xstats.tx_underflow, chip->xstats.tx_carrier,
			chip->xstats.tx_losscarrier, chip->xstats.vlan_tag,
			chip->xstats.tx_deferred, chip->xstats.tx_vlan,
			chip->xstats.tx_jabber, chip->xstats.tx_frame_flushed,
			chip->xstats.tx_payload_error, chip->xstats.tx_ip_header_error);
}
/* eg: cat extra_tx_stats */
static DEVICE_ATTR(extra_tx_stats, 0444, sunxi_gmac_extra_tx_stats_show, NULL);

static ssize_t sunxi_gmac_extra_rx_stats_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct net_device *ndev = dev_get_drvdata(dev);
	struct sunxi_gmac *chip = netdev_priv(ndev);

	return sprintf(buf, "rx_desc: %lu\nsa_filter_fail: %lu\n"
			"overflow_error: %lu\nipc_csum_error: %lu\n"
			"rx_collision: %lu\nrx_crc: %lu\n"
			"dribbling_bit: %lu\nrx_length: %lu\n"
			"rx_mii: %lu\nrx_multicast: %lu\n"
			"rx_gmac_overflow: %lu\nrx_watchdog: %lu\n"
			"da_rx_filter_fail: %lu\nsa_rx_filter_fail: %lu\n"
			"rx_missed_cntr: %lu\nrx_overflow_cntr: %lu\n"
			"rx_vlan: %lu\n\n",
			chip->xstats.rx_desc, chip->xstats.sa_filter_fail,
			chip->xstats.overflow_error, chip->xstats.ipc_csum_error,
			chip->xstats.rx_collision, chip->xstats.rx_crc,
			chip->xstats.dribbling_bit, chip->xstats.rx_length,
			chip->xstats.rx_mii, chip->xstats.rx_multicast,
			chip->xstats.rx_gmac_overflow, chip->xstats.rx_length,
			chip->xstats.da_rx_filter_fail, chip->xstats.sa_rx_filter_fail,
			chip->xstats.rx_missed_cntr, chip->xstats.rx_overflow_cntr,
			chip->xstats.rx_vlan);
}
/* eg: cat extra_rx_stats */
static DEVICE_ATTR(extra_rx_stats, 0444, sunxi_gmac_extra_rx_stats_show, NULL);

static ssize_t sunxi_gmac_gphy_test_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "Usage:\necho [0/1/2/3/4] > gphy_test\n"
			"0 - Normal Mode\n"
			"1 - Transmit Jitter Test\n"
			"2 - Transmit Jitter Test(MASTER mode)\n"
			"3 - Transmit Jitter Test(SLAVE mode)\n"
			"4 - Transmit Distortion Test\n\n");
}

static ssize_t sunxi_gmac_gphy_test_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct net_device *ndev = dev_get_drvdata(dev);
	u16 value, phyreg_val;
	int ret;

	phyreg_val = phy_read(ndev->phydev, MII_CTRL1000);

	ret = kstrtou16(buf, 0, &value);
	if (ret)
		return ret;

	if (value >= 0 && value <= 4) {
		phyreg_val &= ~(SUNXI_GMAC_GPHY_TEST_MASK <<
				SUNXI_GMAC_GPHY_TEST_OFFSET);
		phyreg_val |= value << SUNXI_GMAC_GPHY_TEST_OFFSET;
		phy_write(ndev->phydev, MII_CTRL1000, phyreg_val);
		netdev_info(ndev, "Set MII_CTRL1000(0x09) Reg: 0x%x\n", phyreg_val);
	} else {
		netdev_err(ndev, "Error: Unknown value (%d)\n", value);
	}

	return count;
}
/* eg: echo 0 > gphy_test */
static DEVICE_ATTR(gphy_test, 0664, sunxi_gmac_gphy_test_show, sunxi_gmac_gphy_test_store);

static ssize_t sunxi_gmac_mii_read_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "Usage:\necho PHYREG > mii_read\n");
}

static ssize_t sunxi_gmac_mii_read_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct net_device *ndev = dev_get_drvdata(dev);
	u16 phyreg, phyreg_val;
	int ret;

	if (!netif_running(ndev)) {
		netdev_err(ndev, "Error: Nic is down\n");
		return count;
	}

	ret = kstrtou16(buf, 0, &phyreg);
	if (ret)
		return ret;

	phyreg_val = phy_read(ndev->phydev, phyreg);
	netdev_info(ndev, "PHYREG[0x%02x] = 0x%04x\n", phyreg, phyreg_val);
	return count;
}
/* eg: echo 0x00 > mii_read; cat mii_read */
static DEVICE_ATTR(mii_read, 0664, sunxi_gmac_mii_read_show, sunxi_gmac_mii_read_store);

static ssize_t sunxi_gmac_mii_write_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "Usage:\necho PHYREG PHYVAL > mii_write\n");
}

static ssize_t sunxi_gmac_mii_write_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct net_device *ndev = dev_get_drvdata(dev);
	u16 phyreg_val_before, phyreg_val_after;
	int i, ret;
	/* userspace_cmd[0]: phyreg
	 * userspace_cmd[1]: phyval
	 */
	u16 userspace_cmd[2] = {0};
	char *ptr1 = (char *)buf;
	char *ptr2;

	if (!netif_running(ndev)) {
		netdev_err(ndev, "Error: Nic is down\n");
		return count;
	}

	for (i = 0; i < ARRAY_SIZE(userspace_cmd); i++) {
		ptr1 = skip_spaces(ptr1);
		ptr2 = strchr(ptr1, ' ');
		if (ptr2)
			*ptr2 = '\0';

		ret = kstrtou16(ptr1, 16, &userspace_cmd[i]);
		if (!ptr2 || ret)
			break;

		ptr1 = ptr2 + 1;
	}

	phyreg_val_before = phy_read(ndev->phydev, userspace_cmd[0]);
	phy_write(ndev->phydev, userspace_cmd[0], userspace_cmd[1]);
	phyreg_val_after = phy_read(ndev->phydev, userspace_cmd[0]);
	netdev_info(ndev, "before PHYREG[0x%02x] = 0x%04x, after PHYREG[0x%02x] = 0x%04x\n",
			userspace_cmd[0], phyreg_val_before, userspace_cmd[0], phyreg_val_after);

	return count;
}
/* eg: echo 0x00 0x1234 > mii_write; cat mii_write */
static DEVICE_ATTR(mii_write, 0664, sunxi_gmac_mii_write_show, sunxi_gmac_mii_write_store);

static ssize_t sunxi_gmac_loopback_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct net_device *ndev = dev_get_drvdata(dev);
	struct sunxi_gmac *chip = netdev_priv(ndev);
	int macreg_val;
	u16 phyreg_val;

	phyreg_val = phy_read(ndev->phydev, MII_BMCR);
	if (phyreg_val & BMCR_LOOPBACK)
		netdev_dbg(ndev, "Phy loopback enabled\n");
	else
		netdev_dbg(ndev, "Phy loopback disabled\n");

	macreg_val = readl(chip->base);
	if (macreg_val & SUNXI_GMAC_LOOPBACK)
		netdev_dbg(ndev, "Mac loopback enabled\n");
	else
		netdev_dbg(ndev, "Mac loopback disabled\n");

	return sprintf(buf, "Usage:\necho [0/1/2] > loopback\n"
			"0 - Loopback off\n"
			"1 - Mac loopback mode\n"
			"2 - Phy loopback mode\n");
}

static ssize_t sunxi_gmac_loopback_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct net_device *ndev = dev_get_drvdata(dev);
	struct sunxi_gmac *chip = netdev_priv(ndev);
	int phyreg_val, ret;
	u16 mode;

	if (!netif_running(ndev)) {
		netdev_err(ndev, "Error: eth is down\n");
		return count;
	}

	ret = kstrtou16(buf, 0, &mode);
	if (ret)
		return ret;

	switch (mode) {
	case SUNXI_GMAC_LOOPBACK_OFF:
		sunxi_gmac_loopback(chip->base, 0);
		phyreg_val = phy_read(ndev->phydev, MII_BMCR);
		phy_write(ndev->phydev, MII_BMCR, phyreg_val & ~BMCR_LOOPBACK);
		break;
	case SUNXI_GMAC_MAC_LOOPBACK_ON:
		phyreg_val = phy_read(ndev->phydev, MII_BMCR);
		phy_write(ndev->phydev, MII_BMCR, phyreg_val & ~BMCR_LOOPBACK);
		sunxi_gmac_loopback(chip->base, 1);
		break;
	case SUNXI_GMAC_PHY_LOOPBACK_ON:
		sunxi_gmac_loopback(chip->base, 0);
		phyreg_val = phy_read(ndev->phydev, MII_BMCR);
		phy_write(ndev->phydev, MII_BMCR, phyreg_val | BMCR_LOOPBACK);
		break;
	default:
		netdev_err(ndev, "Error: Please echo right value\n");
		break;
	}

	return count;
}
/* eg: echo 1 > loopback */
static DEVICE_ATTR(loopback, 0664, sunxi_gmac_loopback_show, sunxi_gmac_loopback_store);

/* In phy state machine, we use this func to change link status */
static void sunxi_gmac_adjust_link(struct net_device *ndev)
{
	struct sunxi_gmac *chip = netdev_priv(ndev);
	struct phy_device *phydev = ndev->phydev;
	unsigned long flags;
	int new_state = 0;

	if (!phydev)
		return;

	spin_lock_irqsave(&chip->universal_lock, flags);
	if (phydev->link) {
		/* Now we make sure that we can be in full duplex mode.
		 * If not, we operate in half-duplex mode.
		 */
		if (phydev->duplex != chip->duplex) {
			new_state = 1;
			chip->duplex = phydev->duplex;
		}
		/* Flow Control operation */
		if (phydev->pause)
			sunxi_gmac_flow_ctrl(chip->base, phydev->duplex,
					flow_ctrl, pause);

		if (phydev->speed != chip->speed) {
			new_state = 1;
			chip->speed = phydev->speed;
		}

		if (chip->link == 0) {
			new_state = 1;
			chip->link = phydev->link;
		}

		if (new_state)
			sunxi_gmac_set_link_mode(chip->base, chip->duplex, chip->speed);

	} else if (chip->link != phydev->link) {
		new_state = 1;
		chip->link = 0;
		chip->speed = 0;
		chip->duplex = -1;
	}
	spin_unlock_irqrestore(&chip->universal_lock, flags);

	if (new_state)
		phy_print_status(phydev);
}

static int sunxi_gmac_phy_release(struct net_device *ndev)
{
	struct sunxi_gmac *chip = netdev_priv(ndev);
	struct phy_device *phydev = ndev->phydev;
	int value;

	/* Stop and disconnect the PHY */
	if (phydev)
		phy_stop(phydev);

	chip->link = PHY_DOWN;
	chip->speed = 0;
	chip->duplex = -1;

	if (phydev) {
		value = phy_read(phydev, MII_BMCR);
		phy_write(phydev, MII_BMCR, (value | BMCR_PDOWN));
	}

	if (phydev) {
		phy_disconnect(phydev);
		ndev->phydev = NULL;
	}

	return 0;
}

/* Refill rx dma descriptor after using */
static void sunxi_gmac_rx_refill(struct net_device *ndev)
{
	struct sunxi_gmac *chip = netdev_priv(ndev);
	struct sunxi_gmac_dma_desc *desc;
	struct sk_buff *skb = NULL;
	dma_addr_t dma_addr;

	while (circ_space(chip->rx_clean, chip->rx_dirty, sunxi_gmac_dma_desc_rx) > 0) {
		int entry = chip->rx_clean;

		/* Find the dirty's desc and clean it */
		desc = chip->dma_rx + entry;

		if (chip->rx_skb[entry] == NULL) {
			skb = netdev_alloc_skb_ip_align(ndev, chip->buf_sz);

			if (unlikely(skb == NULL))
				break;

			chip->rx_skb[entry] = skb;
			dma_addr = dma_map_single(chip->dev, skb->data,
					       chip->buf_sz, DMA_FROM_DEVICE);
			sunxi_gmac_desc_buf_set(desc, dma_addr, chip->buf_sz);
		}

		/* sync memery */
		wmb();
		sunxi_gmac_desc_set_own(desc);
		chip->rx_clean = circ_inc(chip->rx_clean, sunxi_gmac_dma_desc_rx);
	}
}

/*
 * sunxi_gmac_dma_desc_init - initialize the RX/TX descriptor list
 *
 * @ndev: net device structure
 * Description: initialize the list for dma.
 */
static int sunxi_gmac_dma_desc_init(struct net_device *ndev)
{
	struct sunxi_gmac *chip = netdev_priv(ndev);
	struct device *dev = &ndev->dev;

	chip->rx_skb = devm_kzalloc(dev, sizeof(chip->rx_skb[0]) * sunxi_gmac_dma_desc_rx,
				GFP_KERNEL);
	if (!chip->rx_skb) {
		netdev_err(ndev, "Error: Alloc rx_skb failed\n");
		goto rx_skb_err;
	}
	chip->tx_skb = devm_kzalloc(dev, sizeof(chip->tx_skb[0]) * sunxi_gmac_dma_desc_tx,
				GFP_KERNEL);
	if (!chip->tx_skb) {
		netdev_err(ndev, "Error: Alloc tx_skb failed\n");
		goto tx_skb_err;
	}

	chip->dma_tx = dma_alloc_coherent(chip->dev,
					sunxi_gmac_dma_desc_tx *
					sizeof(struct sunxi_gmac_dma_desc),
					&chip->dma_tx_phy,
					GFP_KERNEL);
	if (!chip->dma_tx) {
		netdev_err(ndev, "Error: Alloc dma_tx failed\n");
		goto dma_tx_err;
	}

	chip->dma_rx = dma_alloc_coherent(chip->dev,
					sunxi_gmac_dma_desc_rx *
					sizeof(struct sunxi_gmac_dma_desc),
					&chip->dma_rx_phy,
					GFP_KERNEL);
	if (!chip->dma_rx) {
		netdev_err(ndev, "Error: Alloc dma_rx failed\n");
		goto dma_rx_err;
	}

	/* Set the size of buffer depend on the MTU & max buf size */
	chip->buf_sz = SUNXI_GMAC_MAX_BUF_SZ;
	return 0;

dma_rx_err:
	dma_free_coherent(chip->dev, sunxi_gmac_dma_desc_rx * sizeof(struct sunxi_gmac_dma_desc),
			  chip->dma_tx, chip->dma_tx_phy);
dma_tx_err:
	kfree(chip->tx_skb);
tx_skb_err:
	kfree(chip->rx_skb);
rx_skb_err:
	return -ENOMEM;
}

static void sunxi_gmac_free_rx_skb(struct sunxi_gmac *chip)
{
	int i;

	for (i = 0; i < sunxi_gmac_dma_desc_rx; i++) {
		if (chip->rx_skb[i] != NULL) {
			struct sunxi_gmac_dma_desc *desc = chip->dma_rx + i;

			dma_unmap_single(chip->dev, (u32)sunxi_gmac_desc_buf_get_addr(desc),
					 sunxi_gmac_desc_buf_get_len(desc),
					 DMA_FROM_DEVICE);
			dev_kfree_skb_any(chip->rx_skb[i]);
			chip->rx_skb[i] = NULL;
		}
	}
}

static void sunxi_gmac_free_tx_skb(struct sunxi_gmac *chip)
{
	int i;

	for (i = 0; i < sunxi_gmac_dma_desc_tx; i++) {
		if (chip->tx_skb[i] != NULL) {
			struct sunxi_gmac_dma_desc *desc = chip->dma_tx + i;

			if (sunxi_gmac_desc_buf_get_addr(desc))
				dma_unmap_single(chip->dev, (u32)sunxi_gmac_desc_buf_get_addr(desc),
						 sunxi_gmac_desc_buf_get_len(desc),
						 DMA_TO_DEVICE);
			dev_kfree_skb_any(chip->tx_skb[i]);
			chip->tx_skb[i] = NULL;
		}
	}
}

static void sunxi_gmac_dma_desc_deinit(struct sunxi_gmac *chip)
{
	/* Free the region of consistent memory previously allocated for the DMA */
	dma_free_coherent(chip->dev, sunxi_gmac_dma_desc_tx * sizeof(struct sunxi_gmac_dma_desc),
			  chip->dma_tx, chip->dma_tx_phy);
	dma_free_coherent(chip->dev, sunxi_gmac_dma_desc_rx * sizeof(struct sunxi_gmac_dma_desc),
			  chip->dma_rx, chip->dma_rx_phy);

	kfree(chip->rx_skb);
	kfree(chip->tx_skb);
}

static int sunxi_gmac_select_gpio_state(struct pinctrl *pctrl, char *name)
{
	int ret;
	struct pinctrl_state *pctrl_state;

	pctrl_state = pinctrl_lookup_state(pctrl, name);
	if (IS_ERR(pctrl_state)) {
		pr_err("gmac pinctrl_lookup_state(%s) failed! return %p\n",
						name, pctrl_state);
		return -EINVAL;
	}

	ret = pinctrl_select_state(pctrl, pctrl_state);
	if (ret < 0)
		pr_err("gmac pinctrl_select_state(%s) failed! return %d\n",
						name, ret);

	return ret;
}

static int sunxi_gmac_stop(struct net_device *ndev)
{
	struct sunxi_gmac *chip = netdev_priv(ndev);

	netif_stop_queue(ndev);
	napi_disable(&chip->napi);

	netif_carrier_off(ndev);

	sunxi_gmac_phy_release(ndev);

	sunxi_gmac_dma_stop(chip->base);

	netif_tx_lock_bh(ndev);
	/* Release the DMA TX/RX socket buffers */
	sunxi_gmac_free_rx_skb(chip);
	sunxi_gmac_free_tx_skb(chip);
	netif_tx_unlock_bh(ndev);

	return 0;
}

static int sunxi_gmac_power_on(struct sunxi_gmac *chip)
{
	int i, value;

	value = readl(chip->syscfg_base);

	/* syscfg phy reg high 16 bit is unuse */
	if (chip->phy_type == SUNXI_EXTERNAL_PHY)
		value &= ~(1 << 15);
	else
		value |= (1 << 15);

	for (i = 0; i < SUNXI_GMAC_POWER_CHAN_NUM; i++) {
		if (IS_ERR_OR_NULL(chip->gmac_supply[i]))
			continue;

		if (regulator_set_voltage(chip->gmac_supply[i],
				chip->gmac_supply_vol[i],
				chip->gmac_supply_vol[i])) {
			pr_err("gmac-power%d set voltage error\n", i);
			return -EINVAL;
		}

		if (regulator_enable(chip->gmac_supply[i])) {
			pr_err("gmac-power%d enable error\n", i);
			return -EINVAL;
		}
	}

	writel(value, chip->syscfg_base);

	return 0;
}

static void sunxi_gmac_power_off(struct sunxi_gmac *chip)
{
	int i;

	for (i = 0; i < SUNXI_GMAC_POWER_CHAN_NUM; i++) {
		regulator_disable(chip->gmac_supply[i]);
		regulator_put(chip->gmac_supply[i]);
	}
}

/**
 * sunxi_gmac_open - GMAC device open
 * @ndev: The Allwinner GMAC network adapter
 *
 * Called when system wants to start the interface. We init TX/RX channels
 * and enable the hardware for packet reception/transmission and start the
 * network queue.
 *
 * Returns 0 for a successful open, or appropriate error code
 */
static int sunxi_gmac_open(struct net_device *ndev)
{
	struct sunxi_gmac *chip = netdev_priv(ndev);
	int ret;

	/*
	 * When changing the configuration of GMAC and PHY,
	 * it is necessary to turn off the carrier on the link.
	 */
	netif_carrier_off(ndev);

#ifdef CONFIG_AW_EPHY_AC300
	if (chip->ac300_np) {
		chip->ac300_dev = of_phy_find_device(chip->ac300_np);
		if (!chip->ac300_dev) {
			netdev_err(ndev, "Error: Could not find ac300 %s\n",
				chip->ac300_np->full_name);
			return -ENODEV;
		}
		phy_init_hw(chip->ac300_dev);
	}
#endif	/* CONFIG_AW_EPHY_AC300 */

	if (chip->phy_node) {
		ndev->phydev = of_phy_connect(ndev, chip->phy_node,
					&sunxi_gmac_adjust_link, 0, chip->phy_interface);
		if (!ndev->phydev) {
			netdev_err(ndev, "Error: Could not connect to phy %s\n",
				chip->phy_node->full_name);
			return -ENODEV;
		}
		netdev_info(ndev, "%s: Type(%d) PHY ID %08x at %d IRQ %s (%s)\n",
			ndev->name, ndev->phydev->interface, ndev->phydev->phy_id,
			ndev->phydev->mdio.addr, "poll", dev_name(&ndev->phydev->mdio.dev));
	}

	ret = sunxi_gmac_reset((void *)chip->base, 1000);
	if (ret) {
		netdev_err(ndev, "Error: Mac reset failed, please check phy and mac clk\n");
		goto mac_reset_err;
	}
	sunxi_gmac_init(chip->base, txmode, rxmode);
	sunxi_gmac_set_mac_addr_to_reg(chip->base, ndev->dev_addr, 0);

	memset(chip->dma_tx, 0, sunxi_gmac_dma_desc_tx * sizeof(struct sunxi_gmac_dma_desc));
	memset(chip->dma_rx, 0, sunxi_gmac_dma_desc_rx * sizeof(struct sunxi_gmac_dma_desc));

	sunxi_gmac_desc_init_chain(chip->dma_rx, (unsigned long)chip->dma_rx_phy, sunxi_gmac_dma_desc_rx);
	sunxi_gmac_desc_init_chain(chip->dma_tx, (unsigned long)chip->dma_tx_phy, sunxi_gmac_dma_desc_tx);

	chip->rx_clean = 0;
	chip->rx_dirty = 0;
	chip->tx_clean = 0;
	chip->tx_dirty = 0;
	sunxi_gmac_rx_refill(ndev);

	/* Extra statistics */
	memset(&chip->xstats, 0, sizeof(struct sunxi_gmac_extra_stats));

	if (ndev->phydev)
		phy_start(ndev->phydev);

	sunxi_gmac_enable_rx(chip->base, (unsigned long)((struct sunxi_gmac_dma_desc *)
		       chip->dma_rx_phy + chip->rx_dirty));
	sunxi_gmac_enable_tx(chip->base, (unsigned long)((struct sunxi_gmac_dma_desc *)
		       chip->dma_tx_phy + chip->tx_clean));

	napi_enable(&chip->napi);
	netif_start_queue(ndev);

	/* Start the Rx/Tx */
	sunxi_gmac_dma_start(chip->base);

	return 0;

mac_reset_err:
	phy_disconnect(ndev->phydev);
	return ret;
}

#if IS_ENABLED(CONFIG_PM)
static int sunxi_gmac_resume(struct device *dev)
{
	struct net_device *ndev = dev_get_drvdata(dev);
	struct sunxi_gmac *chip = netdev_priv(ndev);

	if (!netif_running(ndev))
		return 0;

	sunxi_gmac_select_gpio_state(chip->pinctrl, PINCTRL_STATE_DEFAULT);

	netif_device_attach(ndev);

	sunxi_gmac_open(ndev);

	return 0;
}

static int sunxi_gmac_suspend(struct device *dev)
{
	struct net_device *ndev = dev_get_drvdata(dev);
	struct sunxi_gmac *chip = netdev_priv(ndev);

	if (!ndev || !netif_running(ndev))
		return 0;

	netif_device_detach(ndev);

	sunxi_gmac_stop(ndev);

	sunxi_gmac_select_gpio_state(chip->pinctrl, PINCTRL_STATE_SLEEP);

	return 0;
}

static const struct dev_pm_ops sunxi_gmac_pm_ops = {
	.suspend = sunxi_gmac_suspend,
	.resume = sunxi_gmac_resume,
};
#else
static const struct dev_pm_ops sunxi_gmac_pm_ops;
#endif /* CONFIG_PM */

#define sunxi_get_soc_chipid(x) {}
static void sunxi_gmac_chip_hwaddr(struct net_device *ndev)
{
#define MD5_SIZE	16
#define CHIP_SIZE	16

	struct crypto_ahash *tfm;
	struct ahash_request *req;
	struct scatterlist sg;
	u8 *addr = ndev->dev_addr;
	u8 result[MD5_SIZE];
	u8 chipid[CHIP_SIZE];
	int i, ret;

	memset(chipid, 0, sizeof(chipid));
	memset(result, 0, sizeof(result));

	sunxi_get_soc_chipid((u8 *)chipid);

	tfm = crypto_alloc_ahash("md5", 0, CRYPTO_ALG_ASYNC);
	if (IS_ERR(tfm)) {
		netdev_err(ndev, "Error: Alloc md5 failed\n");
		return;
	}

	req = ahash_request_alloc(tfm, GFP_KERNEL);
	if (!req)
		goto out;

	ahash_request_set_callback(req, 0, NULL, NULL);

	ret = crypto_ahash_init(req);
	if (ret) {
		netdev_err(ndev, "Error: Crypto_ahash_init failed\n");
		goto out;
	}

	sg_init_one(&sg, chipid, sizeof(chipid));
	ahash_request_set_crypt(req, &sg, result, sizeof(chipid));
	ret = crypto_ahash_update(req);
	if (ret) {
		netdev_err(ndev, "Error: Crypto_ahash_update failed\n");
		goto out;
	}

	ret = crypto_ahash_final(req);
	if (ret) {
		netdev_err(ndev, "Error: Crypto_ahash_final failed\n");
		goto out;
	}

	ahash_request_free(req);

	/* Choose md5 result's [0][2][4][6][8][10] byte as mac address */
	for (i = 0; i < ETH_ALEN; i++)
		addr[i] = result[2 * i];
	addr[0] &= 0xfe; /* clear multicast bit */
	addr[0] |= 0x02; /* set local assignment bit (IEEE802) */

out:
	crypto_free_ahash(tfm);
}

static void sunxi_gmac_check_addr(struct net_device *ndev, unsigned char *mac)
{
	int i;
	char *p = mac;

	if (!is_valid_ether_addr(ndev->dev_addr)) {
		for (i = 0; i < ETH_ALEN; i++, p++)
			ndev->dev_addr[i] = simple_strtoul(p, &p, 16);

		if (!is_valid_ether_addr(ndev->dev_addr))
			sunxi_gmac_chip_hwaddr(ndev);

		if (!is_valid_ether_addr(ndev->dev_addr)) {
			random_ether_addr(ndev->dev_addr);
			netdev_dbg(ndev, "Error: Use random mac address\n");
		}
	}
}

static int sunxi_gmac_clk_enable(struct sunxi_gmac *chip)
{
	struct net_device *ndev = chip->ndev;
	int ret;
	u32 clk_value;

	ret = reset_control_deassert(chip->reset);
	if (ret) {
		netdev_err(ndev, "Error: Try to de-assert gmac rst failed\n");
		goto gmac_reset_err;
	}

	ret = clk_prepare_enable(chip->gmac_clk);
	if (ret) {
		netdev_err(ndev, "Error: Try to enable gmac_clk failed\n");
		goto gmac_clk_err;
	}

	if (chip->phy_clk_type == SUNXI_PHY_USE_CLK25M) {
		ret = clk_prepare_enable(chip->phy25m_clk);
		if (ret) {
			netdev_err(ndev, "Error: Try to enable phy25m_clk failed\n");
			goto phy25m_clk_err;
		}
	}

	clk_value = readl(chip->syscfg_base);
	/* Only support RGMII/RMII */
	if (chip->phy_interface == PHY_INTERFACE_MODE_RGMII)
		clk_value |= SUNXI_GMAC_PHY_RGMII_MASK;
	else
		clk_value &= (~SUNXI_GMAC_PHY_RGMII_MASK);

	clk_value &= (~SUNXI_GMAC_ETCS_RMII_MASK);
	if (chip->phy_interface == PHY_INTERFACE_MODE_RGMII
			|| chip->phy_interface == PHY_INTERFACE_MODE_GMII)
		clk_value |= SUNXI_GMAC_RGMII_INTCLK_MASK;
	else if (chip->phy_interface == PHY_INTERFACE_MODE_RMII)
		clk_value |= SUNXI_GMAC_RMII_MASK;

	/*
	 * Adjust Tx/Rx clock delay
	 * Tx clock delay: 0~7
	 * Rx clock delay: 0~31
	 */
	clk_value &= ~(SUNXI_GMAC_TX_DELAY_MASK << SUNXI_GMAC_TX_DELAY_OFFSET);
	clk_value |= ((chip->tx_delay & SUNXI_GMAC_TX_DELAY_MASK) << SUNXI_GMAC_TX_DELAY_OFFSET);
	clk_value &= ~(SUNXI_GMAC_RX_DELAY_MASK << SUNXI_GMAC_RX_DELAY_OFFSET);
	clk_value |= ((chip->rx_delay & SUNXI_GMAC_RX_DELAY_MASK) << SUNXI_GMAC_RX_DELAY_OFFSET);

	writel(clk_value, chip->syscfg_base);

	return 0;

phy25m_clk_err:
	clk_disable(chip->gmac_clk);
gmac_clk_err:
	reset_control_assert(chip->reset);
gmac_reset_err:
	return ret;
}

static void sunxi_gmac_clk_disable(struct sunxi_gmac *chip)
{
	writel(0, chip->syscfg_base);

	if (chip->phy25m_clk)
		clk_disable_unprepare(chip->phy25m_clk);

	if (chip->gmac_clk)
		clk_disable_unprepare(chip->gmac_clk);

	if (chip->reset)
		reset_control_assert(chip->reset);
}

static void sunxi_gmac_tx_err(struct sunxi_gmac *chip)
{
	netif_stop_queue(chip->ndev);

	sunxi_gmac_disable_tx(chip->base);

	sunxi_gmac_free_tx_skb(chip);
	memset(chip->dma_tx, 0, sunxi_gmac_dma_desc_tx * sizeof(struct sunxi_gmac_dma_desc));
	sunxi_gmac_desc_init_chain(chip->dma_tx, (unsigned long)chip->dma_tx_phy, sunxi_gmac_dma_desc_tx);
	chip->tx_dirty = 0;
	chip->tx_clean = 0;
	sunxi_gmac_enable_tx(chip->base, chip->dma_tx_phy);

	chip->ndev->stats.tx_errors++;
	netif_wake_queue(chip->ndev);
}

static inline void sunxi_gmac_schedule(struct sunxi_gmac *chip)
{
	if (likely(napi_schedule_prep(&chip->napi))) {
		sunxi_gmac_irq_disable(chip->base);
		__napi_schedule(&chip->napi);
	}
}

static irqreturn_t sunxi_gmac_interrupt(int irq, void *dev_id)
{
	struct net_device *ndev = (struct net_device *)dev_id;
	struct sunxi_gmac *chip = netdev_priv(ndev);
	int status;

	status = sunxi_gmac_int_status(chip->base, (void *)(&chip->xstats));

	if (likely(status == handle_tx_rx))
		sunxi_gmac_schedule(chip);
	else if (unlikely(status == tx_hard_error_bump_tc))
		netdev_info(ndev, "Do nothing for bump tc\n");
	else if (unlikely(status == tx_hard_error))
		sunxi_gmac_tx_err(chip);
	else
		netdev_info(ndev, "Do nothing.....\n");

	return IRQ_HANDLED;
}

static void sunxi_gmac_tx_complete(struct sunxi_gmac *chip)
{
	unsigned int entry = 0;
	struct sk_buff *skb = NULL;
	struct sunxi_gmac_dma_desc *desc = NULL;
	int tx_stat;

	spin_lock_bh(&chip->tx_lock);
	while (circ_cnt(chip->tx_dirty, chip->tx_clean, sunxi_gmac_dma_desc_tx) > 0) {
		entry = chip->tx_clean;
		desc = chip->dma_tx + entry;

		/* Check if the descriptor is owned by the DMA. */
		if (sunxi_gmac_desc_get_own(desc))
			break;

		/* Verify tx error by looking at the last segment */
		if (sunxi_gmac_desc_get_tx_last_seg(desc)) {
			tx_stat = sunxi_gmac_desc_get_tx_status(desc, (void *)(&chip->xstats));

			/*
			 * These stats will be parsed by net framework layer
			 * use ifconfig -a in linux cmdline to view
			 */
			if (likely(!tx_stat))
				chip->ndev->stats.tx_packets++;
			else
				chip->ndev->stats.tx_errors++;
		}

		dma_unmap_single(chip->dev, (u32)sunxi_gmac_desc_buf_get_addr(desc),
				 sunxi_gmac_desc_buf_get_len(desc), DMA_TO_DEVICE);

		skb = chip->tx_skb[entry];
		chip->tx_skb[entry] = NULL;
		sunxi_gmac_desc_init(desc);

		/* Find next dirty desc */
		chip->tx_clean = circ_inc(entry, sunxi_gmac_dma_desc_tx);

		if (unlikely(skb == NULL))
			continue;

		dev_kfree_skb(skb);
	}

	if (unlikely(netif_queue_stopped(chip->ndev)) &&
	    circ_space(chip->tx_dirty, chip->tx_clean, sunxi_gmac_dma_desc_tx) >
	    SUNXI_GMAC_TX_THRESH) {
		netif_wake_queue(chip->ndev);
	}
	spin_unlock_bh(&chip->tx_lock);
}

static netdev_tx_t sunxi_gmac_xmit(struct sk_buff *skb, struct net_device *ndev)
{
	struct sunxi_gmac *chip = netdev_priv(ndev);
	struct sunxi_gmac_dma_desc *desc, *first;
	unsigned int entry, len, tmp_len = 0;
	int i, csum_insert;
	int nfrags = skb_shinfo(skb)->nr_frags;
	dma_addr_t dma_addr;

	spin_lock_bh(&chip->tx_lock);
	if (unlikely(circ_space(chip->tx_dirty, chip->tx_clean,
	    sunxi_gmac_dma_desc_tx) < (nfrags + 1))) {
		if (!netif_queue_stopped(ndev)) {
			netdev_err(ndev, "Error: Tx Ring full when queue awake\n");
			netif_stop_queue(ndev);
		}
		spin_unlock_bh(&chip->tx_lock);

		return NETDEV_TX_BUSY;
	}

	csum_insert = (skb->ip_summed == CHECKSUM_PARTIAL);
	entry = chip->tx_dirty;
	first = chip->dma_tx + entry;
	desc = chip->dma_tx + entry;

	len = skb_headlen(skb);
	chip->tx_skb[entry] = skb;

	/* dump the packet */
	netdev_dbg(ndev, "TX packet:\n");
	print_hex_dump_debug("skb->data: ", DUMP_PREFIX_NONE,
		       16, 1, skb->data, 64, true);

	/* Every desc max size is 2K */
	while (len != 0) {
		desc = chip->dma_tx + entry;
		tmp_len = ((len > SUNXI_GMAC_MAX_BUF_SZ) ?  SUNXI_GMAC_MAX_BUF_SZ : len);

		dma_addr = dma_map_single(chip->dev, skb->data, tmp_len, DMA_TO_DEVICE);
		if (dma_mapping_error(chip->dev, dma_addr)) {
			dev_kfree_skb(skb);
			return -ENOMEM;
		}
		sunxi_gmac_desc_buf_set(desc, dma_addr, tmp_len);
		/* Don't set the first's own bit, here */
		if (first != desc) {
			chip->tx_skb[entry] = NULL;
			sunxi_gmac_desc_set_own(desc);
		}

		entry = circ_inc(entry, sunxi_gmac_dma_desc_tx);
		len -= tmp_len;
	}

	for (i = 0; i < nfrags; i++) {
		const skb_frag_t *frag = &skb_shinfo(skb)->frags[i];

		len = skb_frag_size(frag);
		desc = chip->dma_tx + entry;
		dma_addr = skb_frag_dma_map(chip->dev, frag, 0, len, DMA_TO_DEVICE);
		if (dma_mapping_error(chip->dev, dma_addr)) {
			dev_kfree_skb(skb);
			return -ENOMEM;
		}

		sunxi_gmac_desc_buf_set(desc, dma_addr, len);
		sunxi_gmac_desc_set_own(desc);
		chip->tx_skb[entry] = NULL;
		entry = circ_inc(entry, sunxi_gmac_dma_desc_tx);
	}

	ndev->stats.tx_bytes += skb->len;
	chip->tx_dirty = entry;
	sunxi_gmac_desc_tx_close(first, desc, csum_insert);

	sunxi_gmac_desc_set_own(first);
	spin_unlock_bh(&chip->tx_lock);

	if (circ_space(chip->tx_dirty, chip->tx_clean, sunxi_gmac_dma_desc_tx) <=
			(MAX_SKB_FRAGS + 1)) {
		netif_stop_queue(ndev);
		if (circ_space(chip->tx_dirty, chip->tx_clean, sunxi_gmac_dma_desc_tx) >
				SUNXI_GMAC_TX_THRESH)
			netif_wake_queue(ndev);
	}

	netdev_dbg(ndev, "TX descripotor DMA: 0x%08x, dirty: %d, clean: %d\n",
			(unsigned int)chip->dma_tx_phy, chip->tx_dirty, chip->tx_clean);
	sunxi_gmac_dump_dma_desc(chip->dma_tx, sunxi_gmac_dma_desc_tx);

	sunxi_gmac_tx_poll(chip->base);
	sunxi_gmac_tx_complete(chip);

	return NETDEV_TX_OK;
}

static int sunxi_gmac_rx(struct sunxi_gmac *chip, int limit)
{
	unsigned int rxcount = 0;
	unsigned int entry;
	struct sunxi_gmac_dma_desc *desc;
	struct sk_buff *skb;
	int status;
	int frame_len;

	while (rxcount < limit) {
		entry = chip->rx_dirty;
		desc = chip->dma_rx + entry;

		if (sunxi_gmac_desc_get_own(desc))
			break;

		rxcount++;
		chip->rx_dirty = circ_inc(chip->rx_dirty, sunxi_gmac_dma_desc_rx);

		/* Get length & status from hardware */
		frame_len = sunxi_gmac_desc_rx_frame_len(desc);
		status = sunxi_gmac_desc_get_rx_status(desc, (void *)(&chip->xstats));

		netdev_dbg(chip->ndev, "Rx frame size %d, status: %d\n",
			   frame_len, status);

		skb = chip->rx_skb[entry];
		if (unlikely(!skb)) {
			netdev_err(chip->ndev, "Skb is null\n");
			chip->ndev->stats.rx_dropped++;
			break;
		}

		netdev_dbg(chip->ndev, "RX packet:\n");
		/* dump the packet */
		print_hex_dump_debug("skb->data: ", DUMP_PREFIX_NONE,
				16, 1, skb->data, 64, true);

		if (status == discard_frame) {
			netdev_dbg(chip->ndev, "Get error pkt\n");
			chip->ndev->stats.rx_errors++;
			continue;
		}

		if (unlikely(status != llc_snap))
			frame_len -= ETH_FCS_LEN;

		chip->rx_skb[entry] = NULL;

		skb_put(skb, frame_len);
		dma_unmap_single(chip->dev, (u32)sunxi_gmac_desc_buf_get_addr(desc),
				 sunxi_gmac_desc_buf_get_len(desc), DMA_FROM_DEVICE);

		skb->protocol = eth_type_trans(skb, chip->ndev);

		skb->ip_summed = CHECKSUM_UNNECESSARY;
		napi_gro_receive(&chip->napi, skb);

		chip->ndev->stats.rx_packets++;
		chip->ndev->stats.rx_bytes += frame_len;
	}

	if (rxcount > 0) {
		netdev_dbg(chip->ndev, "RX descriptor DMA: 0x%08x, dirty: %d, clean: %d\n",
				(unsigned int)chip->dma_rx_phy, chip->rx_dirty, chip->rx_clean);
		sunxi_gmac_dump_dma_desc(chip->dma_rx, sunxi_gmac_dma_desc_rx);
	}

	sunxi_gmac_rx_refill(chip->ndev);

	return rxcount;
}

static int sunxi_gmac_poll(struct napi_struct *napi, int budget)
{
	struct sunxi_gmac *chip = container_of(napi, struct sunxi_gmac, napi);
	int work_done = 0;

	sunxi_gmac_tx_complete(chip);
	work_done = sunxi_gmac_rx(chip, budget);

	if (work_done < budget) {
		napi_complete(napi);
		sunxi_gmac_irq_enable(chip->base);
	}

	return work_done;
}

static int sunxi_gmac_change_mtu(struct net_device *ndev, int new_mtu)
{
	int max_mtu;

	if (netif_running(ndev)) {
		netdev_err(ndev, "Error: Nic must be stopped to change its MTU\n");
		return -EBUSY;
	}

	max_mtu = SKB_MAX_HEAD(NET_SKB_PAD + NET_IP_ALIGN);

	if ((new_mtu < 46) || (new_mtu > max_mtu)) {
		netdev_err(ndev, "Error: Invalid MTU, max MTU is: %d\n", max_mtu);
		return -EINVAL;
	}

	ndev->mtu = new_mtu;
	netdev_update_features(ndev);

	return 0;
}

static netdev_features_t sunxi_gmac_fix_features(struct net_device *ndev,
					   netdev_features_t features)
{
	return features;
}

static void sunxi_gmac_set_rx_mode(struct net_device *ndev)
{
	struct sunxi_gmac *chip = netdev_priv(ndev);
	unsigned int value = 0;

	netdev_dbg(ndev, "%s: # mcasts %d, # unicast %d\n",
		 __func__, netdev_mc_count(ndev), netdev_uc_count(ndev));

	spin_lock_bh(&chip->universal_lock);
	if (ndev->flags & IFF_PROMISC) {
		value = SUNXI_GMAC_FRAME_FILTER_PR;
	} else if ((netdev_mc_count(ndev) > SUNXI_GMAC_HASH_TABLE_SIZE) ||
		   (ndev->flags & IFF_ALLMULTI)) {
		value = SUNXI_GMAC_FRAME_FILTER_PM;	/* pass all multi */
		sunxi_gmac_hash_filter(chip->base, ~0UL, ~0UL);
	} else if (!netdev_mc_empty(ndev)) {
		u32 mc_filter[2];
		struct netdev_hw_addr *ha;

		/* Hash filter for multicast */
		value = SUNXI_GMAC_FRAME_FILTER_HMC;

		memset(mc_filter, 0, sizeof(mc_filter));
		netdev_for_each_mc_addr(ha, ndev) {
			/* The upper 6 bits of the calculated CRC are used to
			 *  index the contens of the hash table
			 */
			int bit_nr = bitrev32(~crc32_le(~0, ha->addr, 6)) >> 26;
			/* The most significant bit determines the register to
			 * use (H/L) while the other 5 bits determine the bit
			 * within the register.
			 */
			mc_filter[bit_nr >> 5] |= 1 << (bit_nr & 31);
		}
		sunxi_gmac_hash_filter(chip->base, mc_filter[0], mc_filter[1]);
	}

	/* Handle multiple unicast addresses (perfect filtering)*/
	if (netdev_uc_count(ndev) > 16) {
		/* Switch to promiscuous mode is more than 8 addrs are required */
		value |= SUNXI_GMAC_FRAME_FILTER_PR;
	} else {
		int reg = 1;
		struct netdev_hw_addr *ha;

		netdev_for_each_uc_addr(ha, ndev) {
			sunxi_gmac_set_mac_addr_to_reg(chip->base, ha->addr, reg);
			reg++;
		}
	}

#ifdef FRAME_FILTER_DEBUG
	/* Enable Receive all mode (to debug filtering_fail errors) */
	value |= SUNXI_GMAC_FRAME_FILTER_RA;
#endif
	sunxi_gmac_set_filter(chip->base, value);
	spin_unlock_bh(&chip->universal_lock);
}

static void sunxi_gmac_tx_timeout(struct net_device *ndev, unsigned int txqueue)
{
	struct sunxi_gmac *chip = netdev_priv(ndev);

	sunxi_gmac_tx_err(chip);
}

static int sunxi_gmac_ioctl(struct net_device *ndev, struct ifreq *rq, int cmd)
{
	if (!netif_running(ndev))
		return -EINVAL;

	if (!ndev->phydev)
		return -EINVAL;

	return phy_mii_ioctl(ndev->phydev, rq, cmd);
}

/* Configuration changes (passed on by ifconfig) */
static int sunxi_gmac_config(struct net_device *ndev, struct ifmap *map)
{
	if (ndev->flags & IFF_UP)	/* can't act on a running interface */
		return -EBUSY;

	/* Don't allow changing the I/O address */
	if (map->base_addr != ndev->base_addr) {
		netdev_err(ndev, "Error: Can't change I/O address\n");
		return -EOPNOTSUPP;
	}

	/* Don't allow changing the IRQ */
	if (map->irq != ndev->irq) {
		netdev_err(ndev, "Error: Can't change IRQ number %d\n", ndev->irq);
		return -EOPNOTSUPP;
	}

	return 0;
}

static int sunxi_gmac_set_mac_address(struct net_device *ndev, void *p)
{
	struct sunxi_gmac *chip = netdev_priv(ndev);
	struct sockaddr *addr = p;

	if (!is_valid_ether_addr(addr->sa_data)) {
		netdev_err(ndev, "Error: Set error mac address\n");
		return -EADDRNOTAVAIL;
	}

	memcpy(ndev->dev_addr, addr->sa_data, ndev->addr_len);
	sunxi_gmac_set_mac_addr_to_reg(chip->base, ndev->dev_addr, 0);

	return 0;
}

static int sunxi_gmac_set_features(struct net_device *ndev, netdev_features_t features)
{
	struct sunxi_gmac *chip = netdev_priv(ndev);

	if (features & NETIF_F_LOOPBACK && netif_running(ndev))
		sunxi_gmac_loopback(chip->base, 1);
	else
		sunxi_gmac_loopback(chip->base, 0);

	return 0;
}

#if IS_ENABLED(CONFIG_NET_POLL_CONTROLLER)
/* Polling receive - used by NETCONSOLE and other diagnostic tools
 * to allow network I/O with interrupts disabled.
 */
static void sunxi_gmac_poll_controller(struct net_device *dev)
{
	disable_irq(dev->irq);
	sunxi_gmac_interrupt(dev->irq, dev);
	enable_irq(dev->irq);
}
#endif

static const struct net_device_ops sunxi_gmac_netdev_ops = {
	.ndo_init = NULL,
	.ndo_open = sunxi_gmac_open,
	.ndo_start_xmit = sunxi_gmac_xmit,
	.ndo_stop = sunxi_gmac_stop,
	.ndo_change_mtu = sunxi_gmac_change_mtu,
	.ndo_fix_features = sunxi_gmac_fix_features,
	.ndo_set_rx_mode = sunxi_gmac_set_rx_mode,
	.ndo_tx_timeout = sunxi_gmac_tx_timeout,
	.ndo_do_ioctl = sunxi_gmac_ioctl,
	.ndo_set_config = sunxi_gmac_config,
#if IS_ENABLED(CONFIG_NET_POLL_CONTROLLER)
	.ndo_poll_controller = sunxi_gmac_poll_controller,
#endif
	.ndo_set_mac_address = sunxi_gmac_set_mac_address,
	.ndo_set_features = sunxi_gmac_set_features,
};

static int sunxi_gmac_check_if_running(struct net_device *ndev)
{
	if (!netif_running(ndev))
		return -EBUSY;
	return 0;
}

static int sunxi_gmac_ethtool_get_sset_count(struct net_device *netdev, int sset)
{
	int len;

	switch (sset) {
	case ETH_SS_STATS:
		len = 0;
		return len;
	default:
		return -EOPNOTSUPP;
	}
}


/**
 * sunxi_gmac_ethtool_getdrvinfo - Get various SUNXI GMAC driver information.
 * @ndev:	Pointer to net_device structure
 * @ed:		Pointer to ethtool_drvinfo structure
 *
 * This implements ethtool command for getting the driver information.
 * Issue "ethtool -i ethX" under linux prompt to execute this function.
 */
static void sunxi_gmac_ethtool_getdrvinfo(struct net_device *ndev,
				    struct ethtool_drvinfo *info)
{
	strlcpy(info->driver, "sunxi_gmac", sizeof(info->driver));

	strcpy(info->version, SUNXI_GMAC_MODULE_VERSION);
	info->fw_version[0] = '\0';
}

/**
 * sunxi_gmac_ethool_get_pauseparam - Get the pause parameter setting for Tx/Rx.
 *
 * @ndev:	Pointer to net_device structure
 * @epause:	Pointer to ethtool_pauseparam structure
 *
 * This implements ethtool command for getting sunxi_gmac ethernet pause frame
 * setting. Issue "ethtool -a ethx" to execute this function.
 */
static void sunxi_gmac_ethtool_get_pauseparam(struct net_device *ndev,
					struct ethtool_pauseparam *epause)
{
	struct sunxi_gmac *chip = netdev_priv(ndev);

	/* TODO: need to support autoneg */
	epause->tx_pause = sunxi_gmac_read_tx_flowctl(chip->base);
	epause->rx_pause = sunxi_gmac_read_rx_flowctl(chip->base);
}

/**
 * sunxi_gmac_ethtool_set_pauseparam - Set device pause paramter(flow contrl)
 *				settings.
 * @ndev:	Pointer to net_device structure
 * @epause:	Pointer to ethtool_pauseparam structure
 *
 * This implements ethtool command for enabling flow control on Rx and Tx.
 * Issue "ethtool -A ethx tx on|off" under linux prompt to execute this
 * function.
 *
 */
static int sunxi_gmac_ethtool_set_pauseparam(struct net_device *ndev,
					struct ethtool_pauseparam *epause)
{
	struct sunxi_gmac *chip = netdev_priv(ndev);

	sunxi_gmac_write_tx_flowctl(chip->base, !!epause->tx_pause);
	netdev_info(ndev, "Tx flowctrl %s\n", epause->tx_pause ? "ON" : "OFF");

	sunxi_gmac_write_rx_flowctl(chip->base, !!epause->rx_pause);
	netdev_info(ndev, "Rx flowctrl %s\n", epause->rx_pause ? "ON" : "OFF");

	return 0;
}

/**
 * sunxi_gmac_ethtool_get_wol - Get device wake-on-lan settings.
 *
 * @ndev:	Pointer to net_device structure
 * @wol:	Pointer to ethtool_wolinfo structure
 *
 * This implements ethtool command for get wake-on-lan settings.
 * Issue "ethtool -s ethx wol p|u|m|b|a|g|s|d" under linux prompt to execute
 * this function.
 */
static void sunxi_gmac_ethtool_get_wol(struct net_device *ndev,
				struct ethtool_wolinfo *wol)
{
	struct sunxi_gmac *chip = netdev_priv(ndev);

	spin_lock_irq(&chip->universal_lock);
	/* TODO: need to support wol */
	spin_unlock_irq(&chip->universal_lock);

	netdev_err(ndev, "Error: wakeup-on-lan func is not supported yet\n");
}

/**
 * sunxi_gmac_ethtool_set_wol - set device wake-on-lan settings.
 *
 * @ndev:	Pointer to net_device structure
 * @wol:	Pointer to ethtool_wolinfo structure
 *
 * This implements ethtool command for set wake-on-lan settings.
 * Issue "ethtool -s ethx wol p|u|n|b|a|g|s|d" under linux prompt to execute
 * this function.
 */
static int sunxi_gmac_ethtool_set_wol(struct net_device *ndev,
				struct ethtool_wolinfo *wol)
{
	/*
	 * TODO: Wake-on-lane function need to be supported.
	 */

	return 0;
}

static const struct ethtool_ops sunxi_gmac_ethtool_ops = {
	.begin = sunxi_gmac_check_if_running,
	.get_link = ethtool_op_get_link,
	.get_pauseparam = sunxi_gmac_ethtool_get_pauseparam,
	.set_pauseparam = sunxi_gmac_ethtool_set_pauseparam,
	.get_wol = sunxi_gmac_ethtool_get_wol,
	.set_wol = sunxi_gmac_ethtool_set_wol,
	.get_sset_count = sunxi_gmac_ethtool_get_sset_count,
	.get_drvinfo = sunxi_gmac_ethtool_getdrvinfo,
	.get_link_ksettings = phy_ethtool_get_link_ksettings,
	.set_link_ksettings = phy_ethtool_set_link_ksettings,
};

static int sunxi_gmac_hardware_init(struct platform_device *pdev)
{
	struct net_device *ndev = platform_get_drvdata(pdev);
	struct sunxi_gmac *chip = netdev_priv(ndev);
	int ret;

	ret = sunxi_gmac_power_on(chip);
	if (ret) {
		netdev_err(ndev, "Error: Gmac power on failed\n");
		ret = -EINVAL;
		goto power_on_err;
	}

	ret = sunxi_gmac_clk_enable(chip);
	if (ret) {
		netdev_err(ndev, "Error: Clk enable is failed\n");
		ret = -EINVAL;
		goto clk_enable_err;
	}

#ifdef CONFIG_AW_EPHY_AC300
       ret = pwm_config(chip->ac300_pwm, PWM_DUTY_NS, PWM_PERIOD_NS);
       if (ret) {
		netdev_err(ndev, "Error: Config ac300 pwm failed\n");
		ret = -EINVAL;
		goto pwm_config_err;
       }

       ret = pwm_enable(chip->ac300_pwm);
       if (ret) {
		netdev_err(ndev, "Error: Enable ac300 pwm failed\n");
		ret = -EINVAL;
		goto pwm_enable_err;
       }
#endif /* CONFIG_AW_EPHY_AC300 */

	return 0;

#ifdef CONFIG_AW_EPHY_AC300
pwm_enable_err:
pwm_config_err:
	sunxi_gmac_clk_disable(chip);
#endif	/* CONFIG_AW_EPHY_AC300 */
clk_enable_err:
	sunxi_gmac_power_off(chip);
power_on_err:
	return ret;
}

static void sunxi_gmac_hardware_deinit(struct platform_device *pdev)
{
	struct net_device *ndev = platform_get_drvdata(pdev);
	struct sunxi_gmac *chip = netdev_priv(ndev);

	sunxi_gmac_power_off(chip);

	sunxi_gmac_clk_disable(chip);

#ifdef CONFIG_AW_EPHY_AC300
	pwm_disable(chip->ac300_pwm);
#endif /*CONFIG_AW_EPHY_AC300 */
}

static int sunxi_gmac_resource_get(struct platform_device *pdev)
{
	struct net_device *ndev = platform_get_drvdata(pdev);
	struct sunxi_gmac *chip = netdev_priv(ndev);
	struct device_node *np = pdev->dev.of_node;
	struct resource *res;
	phy_interface_t phy_mode;
	u32 value;
	char power_vol[SUNXI_GMAC_POWER_CHAR_LENGTH];
	char power[SUNXI_GMAC_POWER_CHAR_LENGTH];
	int ret, i;

	/* External phy is selected by default */
	chip->phy_type = SUNXI_EXTERNAL_PHY;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		netdev_err(ndev, "Error: Get gmac memory failed\n");
		return -ENODEV;
	}

	chip->base = devm_ioremap_resource(&pdev->dev, res);
	if (!chip->base) {
		netdev_err(ndev, "Error: Gmac memory mapping failed\n");
		return -ENOMEM;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!res) {
		netdev_err(ndev, "Error: Get phy memory failed\n");
		return -ENODEV;
	}

	chip->syscfg_base = devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if (!chip->syscfg_base) {
		netdev_err(ndev, "Error: Phy memory mapping failed\n");
		return -ENOMEM;
	}

	ndev->irq = platform_get_irq_byname(pdev, "gmacirq");
	if (ndev->irq < 0) {
		netdev_err(ndev, "Error: Gmac irq not found\n");
		return -ENXIO;
	}

	ret = devm_request_irq(&pdev->dev, ndev->irq, sunxi_gmac_interrupt, IRQF_SHARED, dev_name(&pdev->dev), ndev);
	if (ret) {
		netdev_err(ndev, "Error: Could not request irq %d\n", ndev->irq);
		return -EINVAL;
	}

	chip->reset = devm_reset_control_get(&pdev->dev, NULL);
	if (IS_ERR(chip->reset)) {
		netdev_err(ndev, "Error: Get gmac rst failed\n");
		return -EINVAL;
	}

	chip->pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(chip->pinctrl)) {
		netdev_err(ndev, "Error: Get Pin failed\n");
		return -EIO;
	}

	chip->gmac_clk = devm_clk_get(&pdev->dev, "gmac");
	if (!chip->gmac_clk) {
		netdev_err(ndev, "Error: Get gmac clock failed\n");
		return -EINVAL;
	}

	ret = of_get_phy_mode(np, &phy_mode);
	if (!ret) {
		chip->phy_interface = phy_mode;
		if (chip->phy_interface != PHY_INTERFACE_MODE_RGMII &&
		chip->phy_interface != PHY_INTERFACE_MODE_RMII) {
			netdev_err(ndev, "Error: Get gmac phy interface failed\n");
			return -EINVAL;
		}
	}

	ret = of_property_read_u32(np, "tx-delay", &chip->tx_delay);
	if (ret) {
		netdev_warn(ndev, "Warning: Get gmac tx-delay failed, use default 0\n");
		chip->tx_delay = 0;
	}

	ret = of_property_read_u32(np, "rx-delay", &chip->rx_delay);
	if (ret) {
		netdev_warn(ndev, "Warning: Get gmac rx-delay failed, use default 0\n");
		chip->rx_delay = 0;
	}

	chip->phy_node = of_parse_phandle(np, "phy-handle", 0);
	if (!chip->phy_node) {
		netdev_err(ndev, "Error: Get gmac phy-handle failed\n");
		return -EINVAL;
	}

#ifdef CONFIG_AW_EPHY_AC300
	/*
	 * If use Internal phy such as ac300,
	 * change the phy_type to internal phy
	 */
	chip->phy_type = SUNXI_INTERNAL_PHY;
	chip->ac300_np = of_parse_phandle(np, "ac300-phy-handle", 0);
	if (!chip->ac300_np) {
		netdev_err(ndev, "Error: Get gmac ac300-phy-handle failed\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "sunxi,pwm-channel", &chip->pwm_channel);
	if (ret) {
		netdev_err(ndev, "Error: Get ac300 pwm failed\n");
		return -EINVAL;
	}

	chip->ac300_pwm = pwm_request(chip->pwm_channel, NULL);
	if (IS_ERR_OR_NULL(chip->ac300_pwm)) {
		netdev_err(ndev, "Error: Get ac300 pwm failed\n");
		return -EINVAL;
	}
#endif /* CONFIG_AW_EPHY_AC300 */

	ret = of_property_read_u32(np, "sunxi,phy-clk-type", &chip->phy_clk_type);
	if (ret) {
		netdev_err(ndev, "Error: Get gmac phy-clk-type failed\n");
		return -EINVAL;
	}

	if (chip->phy_clk_type == SUNXI_PHY_USE_CLK25M) {
		chip->phy25m_clk = devm_clk_get(&pdev->dev, "phy25m");
		if (IS_ERR_OR_NULL(chip->phy25m_clk)) {
			netdev_err(ndev, "Error: Get phy25m clk failed\n");
			return -EINVAL;
		}
	}

	for (i = 0; i < SUNXI_GMAC_POWER_CHAN_NUM; i++) {
		sprintf(power, "gmac-power%d", i);
		/* get gmac_supplyX voltage */
		sprintf(power_vol, "gmac-power%d-vol", i);
		if (!of_property_read_u32(np, power_vol, &value)) {
			chip->gmac_supply_vol[i] = value;
			netdev_dbg(ndev, "Info: Gmac_power_vol[%d] = %d\n", i, value);
		} else {
			chip->gmac_supply_vol[i] = 0;
		}
		chip->gmac_supply[i] = regulator_get(&pdev->dev, power);

		if (IS_ERR(chip->gmac_supply[i]))
			netdev_err(ndev, "Error: gmac-power%d get error\n", i);
		else
			netdev_dbg(ndev, "gmac-power%d get success\n", i);
	}

	return 0;
}

static void sunxi_gmac_resource_put(struct platform_device *pdev)
{
#ifdef CONFIG_AW_EPHY_AC300
	struct net_device *ndev = platform_get_drvdata(pdev);
	struct sunxi_gmac *chip = netdev_priv(ndev);

	pwm_free(chip->ac300_pwm);
#endif /* CONFIG_AW_EPHY_AC300 */
}

static void sunxi_gmac_sysfs_create(struct device *dev)
{
	device_create_file(dev, &dev_attr_gphy_test);
	device_create_file(dev, &dev_attr_mii_read);
	device_create_file(dev, &dev_attr_mii_write);
	device_create_file(dev, &dev_attr_loopback);
	device_create_file(dev, &dev_attr_extra_tx_stats);
	device_create_file(dev, &dev_attr_extra_rx_stats);
}

static void sunxi_gmac_sysfs_destroy(struct device *dev)
{
	device_remove_file(dev, &dev_attr_gphy_test);
	device_remove_file(dev, &dev_attr_mii_read);
	device_remove_file(dev, &dev_attr_mii_write);
	device_remove_file(dev, &dev_attr_loopback);
	device_remove_file(dev, &dev_attr_extra_tx_stats);
	device_remove_file(dev, &dev_attr_extra_rx_stats);
}

/**
 * sunxi_gmac_probe - GMAC device probe
 * @pdev: The SUNXI GMAC device that we are removing
 *
 * Called when probing for GMAC device. We get details of instances and
 * resource information from platform init and register a network device
 * and allocate resources necessary for driver to perform
 *
 */
static int sunxi_gmac_probe(struct platform_device *pdev)
{
	int ret;
	struct net_device *ndev;
	struct sunxi_gmac *chip;

	dev_dbg(&pdev->dev, "%s() BEGIN\n", __func__);

	pdev->dev.coherent_dma_mask = DMA_BIT_MASK(32);
	pdev->dev.dma_mask = &pdev->dev.coherent_dma_mask;

	ndev = alloc_etherdev(sizeof(*ndev));
	if (!ndev) {
		dev_err(&pdev->dev, "Error: Allocate network device failed\n");
		ret = -ENOMEM;
		goto alloc_etherdev_err;
	}
	SET_NETDEV_DEV(ndev, &pdev->dev);

	chip = netdev_priv(ndev);
	platform_set_drvdata(pdev, ndev);

	ret = sunxi_gmac_resource_get(pdev);
	if (ret) {
		dev_err(&pdev->dev, "Error: Get gmac hardware resource failed\n");
		goto resource_get_err;
	}

	ret = sunxi_gmac_hardware_init(pdev);
	if (ret) {
		dev_err(&pdev->dev, "Error: Init gmac hardware resource failed\n");
		goto hardware_init_err;
	}

	/*
	 * setup the netdevice
	 * fillup netdevice base memory and ops
	 * fillup netdevice ethtool ops
	 */
	ether_setup(ndev);
	ndev->netdev_ops = &sunxi_gmac_netdev_ops;
	netdev_set_default_ethtool_ops(ndev, &sunxi_gmac_ethtool_ops);
	ndev->base_addr = (unsigned long)chip->base;
	chip->ndev = ndev;
	chip->dev = &pdev->dev;

	/* fillup netdevice features and flags */
	ndev->hw_features = NETIF_F_SG | NETIF_F_HIGHDMA | NETIF_F_IP_CSUM |
				NETIF_F_IPV6_CSUM | NETIF_F_RXCSUM | NETIF_F_GRO;
	ndev->features |= ndev->hw_features;
	ndev->hw_features |= NETIF_F_LOOPBACK;
	ndev->priv_flags |= IFF_UNICAST_FLT;
	ndev->watchdog_timeo = msecs_to_jiffies(watchdog);

	/* add napi poll method */
	netif_napi_add(ndev, &chip->napi, sunxi_gmac_poll, SUNXI_GMAC_BUDGET);

	spin_lock_init(&chip->universal_lock);
	spin_lock_init(&chip->tx_lock);

	ret = register_netdev(ndev);
	if (ret) {
		dev_err(&pdev->dev, "Error: Register %s failed\n", ndev->name);
		goto register_err;
	}

	/* Before open the device, the mac address should be set */
	sunxi_gmac_check_addr(ndev, mac_str);

	ret = sunxi_gmac_dma_desc_init(ndev);
	if (ret) {
		dev_err(&pdev->dev, "Error: Init dma descriptor failed\n");
		goto init_dma_desc_err;
	}

	sunxi_gmac_sysfs_create(&pdev->dev);

	dev_dbg(&pdev->dev, "%s() SUCCESS\n", __func__);

	return 0;

init_dma_desc_err:
	unregister_netdev(ndev);
register_err:
	netif_napi_del(&chip->napi);
	sunxi_gmac_hardware_deinit(pdev);
hardware_init_err:
	sunxi_gmac_resource_put(pdev);
resource_get_err:
	platform_set_drvdata(pdev, NULL);
	free_netdev(ndev);
alloc_etherdev_err:
	return ret;
}

static int sunxi_gmac_remove(struct platform_device *pdev)
{
	struct net_device *ndev = platform_get_drvdata(pdev);
	struct sunxi_gmac *chip = netdev_priv(ndev);

	sunxi_gmac_sysfs_destroy(&pdev->dev);
	sunxi_gmac_dma_desc_deinit(chip);
	unregister_netdev(ndev);
	netif_napi_del(&chip->napi);
	sunxi_gmac_hardware_deinit(pdev);
	sunxi_gmac_resource_put(pdev);
	platform_set_drvdata(pdev, NULL);
	free_netdev(ndev);
	return 0;
}

static const struct of_device_id sunxi_gmac_of_match[] = {
	{.compatible = "allwinner,sunxi-gmac",},
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_gmac_of_match);

static struct platform_driver sunxi_gmac_driver = {
	.probe	= sunxi_gmac_probe,
	.remove = sunxi_gmac_remove,
	.driver = {
		   .name = "sunxi-gmac",
		   .owner = THIS_MODULE,
		   .pm = &sunxi_gmac_pm_ops,
		   .of_match_table = sunxi_gmac_of_match,
	},
};
module_platform_driver(sunxi_gmac_driver);

#ifndef MODULE
static int __init sunxi_gmac_set_mac_addr(char *str)
{
	char *p = str;

	/**
	 * mac address: xx:xx:xx:xx:xx:xx
	 * The reason why memcpy 18 bytes is
	 * the `/0`.
	 */
	if (str && strlen(str))
		memcpy(mac_str, p, MAC_ADDR_LEN);

	return 0;
}
/* TODO: When used more than one mac,
 * parsing the mac address becomes a problem.
 * Maybe use this way: mac0_addr=, mac1_addr=
 */
__setup("mac_addr=", sunxi_gmac_set_mac_addr);
#endif /* MODULE */

MODULE_DESCRIPTION("Allwinner GMAC driver");
MODULE_AUTHOR("xuminghui <xuminghui@allwinnertech.com>");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_VERSION(SUNXI_GMAC_MODULE_VERSION);
