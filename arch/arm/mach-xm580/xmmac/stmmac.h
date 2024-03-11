/*******************************************************************************
  Copyright (C) 2007-2009  STMicroelectronics Ltd

  This program is free software; you can redistribute it and/or modify it
  under the terms and conditions of the GNU General Public License,
  version 2, as published by the Free Software Foundation.

  This program is distributed in the hope it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.

  The full GNU General Public License is included in this distribution in
  the file called "COPYING".

  Author: Giuseppe Cavallaro <peppe.cavallaro@st.com>
*******************************************************************************/

#define DRV_MODULE_VERSION	"Nov_2010"


#include <linux/platform_device.h>

#define STMMAC_RX_COE_NONE	0
#define STMMAC_RX_COE_TYPE1	1
#define STMMAC_RX_COE_TYPE2	2

/* Define the macros for CSR clock range parameters to be passed by
 * platform code.
 * This could also be configured at run time using CPU freq framework. */

/* MDC Clock Selection define*/
#define	STMMAC_CSR_60_100M	0x0	/* MDC = clk_scr_i/42 */
#define	STMMAC_CSR_100_150M	0x1	/* MDC = clk_scr_i/62 */
#define	STMMAC_CSR_20_35M	0x2	/* MDC = clk_scr_i/16 */
#define	STMMAC_CSR_35_60M	0x3	/* MDC = clk_scr_i/26 */
#define	STMMAC_CSR_150_250M	0x4	/* MDC = clk_scr_i/102 */
#define	STMMAC_CSR_250_300M	0x5	/* MDC = clk_scr_i/122 */

/* The MDC clock could be set higher than the IEEE 802.3
 * specified frequency limit 0f 2.5 MHz, by programming a clock divider
 * of value different than the above defined values. The resultant MDIO
 * clock frequency of 12.5 MHz is applicable for the interfacing chips
 * supporting higher MDC clocks.
 * The MDC clock selection macros need to be defined for MDC clock rate
 * of 12.5 MHz, corresponding to the following selection.
 */
#define STMMAC_CSR_I_4		0x8	/* clk_csr_i/4 */
#define STMMAC_CSR_I_6		0x9	/* clk_csr_i/6 */
#define STMMAC_CSR_I_8		0xA	/* clk_csr_i/8 */
#define STMMAC_CSR_I_10		0xB	/* clk_csr_i/10 */
#define STMMAC_CSR_I_12		0xC	/* clk_csr_i/12 */
#define STMMAC_CSR_I_14		0xD	/* clk_csr_i/14 */
#define STMMAC_CSR_I_16		0xE	/* clk_csr_i/16 */
#define STMMAC_CSR_I_18		0xF	/* clk_csr_i/18 */

/* AXI DMA Burst length supported */
#define DMA_AXI_BLEN_4		(1 << 1)
#define DMA_AXI_BLEN_8		(1 << 2)
#define DMA_AXI_BLEN_16		(1 << 3)
#define DMA_AXI_BLEN_32		(1 << 4)
#define DMA_AXI_BLEN_64		(1 << 5)
#define DMA_AXI_BLEN_128	(1 << 6)
#define DMA_AXI_BLEN_256	(1 << 7)
#define DMA_AXI_BLEN_ALL (DMA_AXI_BLEN_4 | DMA_AXI_BLEN_8 | DMA_AXI_BLEN_16 \
        | DMA_AXI_BLEN_32 | DMA_AXI_BLEN_64 \
        | DMA_AXI_BLEN_128 | DMA_AXI_BLEN_256)

/* Platfrom data for platform device structure's platform_data field */
struct plat_stmmacenet_data {      
    int bus_id;
    int pbl;
    int clk_csr;
    int has_gmac;
    int enh_desc;
    int tx_coe;
    int bugged_jumbo;
    int pmt; 
    void (*fix_mac_speed)(void *priv, unsigned int speed);
    void (*bus_setup)(void __iomem *ioaddr);
    int (*init)(struct platform_device *pdev);
    void (*exit)(struct platform_device *pdev);
    void *custom_cfg;
    void *bsp_priv;
};

struct plat_stmmacphy_data {
    int bus_id;
    int phy_addr;
    unsigned int phy_mask;
    int interface;
    int (*phy_reset)(void *priv);
    void *priv;
};

#include "common.h"
#ifdef CONFIG_STMMAC_TIMER
#include "stmmac_timer.h"
#endif

struct stmmac_priv {
	/* Frequently used values are kept adjacent for cache effect */
	struct dma_desc *dma_tx ____cacheline_aligned;
	dma_addr_t dma_tx_phy;
	struct sk_buff **tx_skbuff;
	unsigned int cur_tx;
	unsigned int dirty_tx;
	unsigned int dma_tx_size;
	int tx_coalesce;

	struct dma_desc *dma_rx ;
	/* 
	 * 在开始接收时,cur_rx=dirty_rx，主机每处理一帧cur_rx便向后面移动，
	 * 那么cur_rx和dirty_rx之间的距离便是接收了的帧数
	 */
	unsigned int cur_rx;             
	unsigned int dirty_rx;
	struct sk_buff **rx_skbuff;
	dma_addr_t *rx_skbuff_dma;
	struct sk_buff_head rx_recycle;

	struct net_device *dev;
	dma_addr_t dma_rx_phy;
	unsigned int dma_rx_size;
	unsigned int dma_buf_sz;
	struct device *device;
	struct mac_device_info *hw;
	void __iomem *ioaddr;

	struct stmmac_extra_stats xstats;
	struct napi_struct napi;

	phy_interface_t phy_interface;
	int phy_addr;
	int phy_mask;
	int (*phy_reset) (void *priv);
	int rx_coe;
	int no_csum_insertion;

	int phy_irq;
	struct phy_device *phydev;
	int oldlink;
	int speed;
	int oldduplex;
	unsigned int flow_ctrl;
	unsigned int pause;
	struct mii_bus *mii;

	u32 msg_enable;
	spinlock_t lock;
    spinlock_t tx_lock;
	int wolopts;
	int wolenabled;
#ifdef CONFIG_STMMAC_TIMER
	struct stmmac_timer *tm;
#endif
#ifdef STMMAC_VLAN_TAG_USED
	struct vlan_group *vlgrp;
#endif
	struct plat_stmmacenet_data *plat;
};

extern int stmmac_mdio_unregister(struct net_device *ndev);
extern int stmmac_mdio_register(struct net_device *ndev);
extern void stmmac_set_ethtool_ops(struct net_device *netdev);
extern const struct stmmac_desc_ops enh_desc_ops;
extern const struct stmmac_desc_ops ndesc_ops;
