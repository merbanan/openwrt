/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * EN7523 WED (WiFi Ethernet Datapath) driver
 * Private header — included only by airoha_wed.c and airoha_wed_debugfs.c
 *
 * Based on drivers/net/ethernet/mediatek/mtk_wed.h
 * Copyright (C) 2021 Felix Fietkau <nbd@nbd.name>
 */

#ifndef __AIROHA_WED_PRIV_H
#define __AIROHA_WED_PRIV_H

#include <linux/soc/mediatek/mtk_wed.h>
#include <linux/debugfs.h>
#include <linux/regmap.h>
#include <linux/netdevice.h>

/* Pull in all WED/WDMA register definitions from the MediaTek driver.
 * The Makefile adds -I$(srctree)/drivers/net/ethernet/mediatek so that
 * this bare include resolves to the private mtk_wed_regs.h header.
 */
#include "mtk_wed_regs.h"

/* EN7523 PCIe controller bases (PCIe0 = domain 0, PCIe1 = domain 1) */
#define EN7523_PCIE_BASE(n)	(0x1fa91000 + (n) * 0x1000)

struct mtk_wed_wo;

struct mtk_wed_soc_data {
	struct {
		u32 tx_bm_tkid;
		u32 wpdma_rx_ring[MTK_WED_RX_QUEUES];
		u32 reset_idx_tx_mask;
		u32 reset_idx_rx_mask;
	} regmap;
	u32 tx_ring_desc_size;
	u32 wdma_desc_size;
};

struct mtk_wed_amsdu {
	void *txd;
	dma_addr_t txd_phy;
};

struct mtk_wed_hw {
	const struct mtk_wed_soc_data *soc;
	struct device_node *node;
	struct device *dev;
	struct regmap *regs;
	void __iomem *wdma;
	phys_addr_t wdma_phy;
	struct dentry *debugfs_dir;
	struct mtk_wed_device *wed_dev;
	u32 pcie_base;
	u32 debugfs_reg;
	u32 num_flows;
	u8 version;
	char dirname[5];
	int irq;
	int index;
};

static inline bool airoha_wed_is_v1(struct mtk_wed_hw *hw)
{
	return hw->version == 1;
}

static inline void
wed_w32(struct mtk_wed_device *dev, u32 reg, u32 val)
{
	regmap_write(dev->hw->regs, reg, val);
}

static inline u32
wed_r32(struct mtk_wed_device *dev, u32 reg)
{
	unsigned int val;

	regmap_read(dev->hw->regs, reg, &val);

	return val;
}

static inline void
wdma_w32(struct mtk_wed_device *dev, u32 reg, u32 val)
{
	writel(val, dev->hw->wdma + reg);
}

static inline u32
wdma_r32(struct mtk_wed_device *dev, u32 reg)
{
	return readl(dev->hw->wdma + reg);
}

static inline u32
wpdma_tx_r32(struct mtk_wed_device *dev, int ring, u32 reg)
{
	if (!dev->tx_ring[ring].wpdma)
		return 0;

	return readl(dev->tx_ring[ring].wpdma + reg);
}

static inline void
wpdma_tx_w32(struct mtk_wed_device *dev, int ring, u32 reg, u32 val)
{
	if (!dev->tx_ring[ring].wpdma)
		return;

	writel(val, dev->tx_ring[ring].wpdma + reg);
}

static inline u32
wpdma_txfree_r32(struct mtk_wed_device *dev, u32 reg)
{
	if (!dev->txfree_ring.wpdma)
		return 0;

	return readl(dev->txfree_ring.wpdma + reg);
}

static inline void
wpdma_txfree_w32(struct mtk_wed_device *dev, u32 reg, u32 val)
{
	if (!dev->txfree_ring.wpdma)
		return;

	writel(val, dev->txfree_ring.wpdma + reg);
}

#ifdef CONFIG_NET_AIROHA_SOC_WED
void airoha_wed_add_hw(struct device_node *np, int index);
void airoha_wed_exit(void);
int airoha_wed_flow_add(int index);
void airoha_wed_flow_remove(int index);
void airoha_wed_fe_reset(void);
void airoha_wed_fe_reset_complete(void);
#else
static inline void airoha_wed_add_hw(struct device_node *np, int index) {}
static inline void airoha_wed_exit(void) {}
static inline int airoha_wed_flow_add(int index) { return -EINVAL; }
static inline void airoha_wed_flow_remove(int index) {}
static inline void airoha_wed_fe_reset(void) {}
static inline void airoha_wed_fe_reset_complete(void) {}
#endif

#ifdef CONFIG_DEBUG_FS
void airoha_wed_hw_add_debugfs(struct mtk_wed_hw *hw);
#else
static inline void airoha_wed_hw_add_debugfs(struct mtk_wed_hw *hw) {}
#endif

#endif /* __AIROHA_WED_PRIV_H */
