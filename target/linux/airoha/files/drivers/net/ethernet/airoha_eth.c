// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2024 AIROHA Inc
 * Author: Lorenzo Bianconi <lorenzo@kernel.org>
 */
#include <linux/etherdevice.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/of.h>
#include <linux/of_net.h>
#include <linux/phylink.h>
#include <linux/platform_device.h>
#include "airoha_eth.h"

static u32 airoha_rr(void __iomem *base, u32 offset)
{
	return readl(base + offset);
}

static void airoha_wr(void __iomem *base, u32 offset, u32 val)
{
	writel(val, base + offset);
}

static u32 airoha_rmw(void __iomem *base, u32 offset, u32 mask, u32 val)
{
	val |= airoha_rr(base, offset) & ~mask;
	airoha_wr(base, offset, val);

	return val;
}

#define airoha_fe_rr(eth, offset)		airoha_rr(eth->fe_regs, offset)
#define airoha_fe_wr(eth, offset, val)		airoha_wr(eth->fe_regs, offset, val)
#define airoha_fe_rmw(eth, offset, mask, val)	airoha_rmw(eth->fe_regs, offset, mask, val)
#define airoha_fe_set(eth, offset, val)		airoha_rmw(eth->fe_regs, offset, 0, val)
#define airoha_fe_clear(eth, offset, val)	airoha_rmw(eth->fe_regs, offset, val, 0)

#define airoha_qdma_rr(eth, offset)		airoha_rr(eth->qdma_regs, offset)
#define airoha_qdma_wr(eth, offset, val)	airoha_wr(eth->qdma_regs, offset, val)
#define airoha_qdma_rmw(eth, offset, mask, val)	airoha_rmw(eth->qdma_regs, offset, mask, val)
#define airoha_qdma_set(eth, offset, val)	airoha_rmw(eth->qdma_regs, offset, 0, val)
#define airoha_qdma_clear(eth, offset, val)	airoha_rmw(eth->qdma_regs, offset, val, 0)

static int airoha_dev_init(struct net_device *dev)
{
	struct airoha_eth *eth = netdev_priv(dev);
	const u8 *addr = dev->dev_addr;
	u32 mac_h, mac_lmin;

	mac_h = (addr[0] << 16) | (addr[1] << 8) | addr[2];
	mac_lmin = (addr[3] << 16) | (addr[4] << 8) | addr[5];

	airoha_fe_wr(eth, REG_FE_LAN_MAC_H, mac_h);
	airoha_fe_wr(eth, REG_FE_LAN_MAC_LMIN, mac_lmin);
	airoha_fe_wr(eth, REG_FE_LAN_MAC_LMAX, mac_lmin);

	return 0;
}

static void airoha_set_port_fwd_cfg(struct airoha_eth *eth, u32 addr, u32 val)
{
	airoha_fe_rmw(eth, addr, GDMA1_OCFQ_MASK,
		      FIELD_PREP(GDMA1_OCFQ_MASK, val));
	airoha_fe_rmw(eth, addr, GDMA1_MCFQ_MASK,
		      FIELD_PREP(GDMA1_MCFQ_MASK, val));
	airoha_fe_rmw(eth, addr, GDMA1_BCFQ_MASK,
		      FIELD_PREP(GDMA1_BCFQ_MASK, val));
	airoha_fe_rmw(eth, addr, GDMA1_UCFQ_MASK,
		      FIELD_PREP(GDMA1_UCFQ_MASK, val));
}

static int airoha_set_gdma_port(struct airoha_eth *eth, int port, bool enable)
{
	u32 vip_port, cfg_addr, val = enable ? 4 : 0xf;

	switch (port) {
	case 0:
		vip_port = BIT(22);
		cfg_addr = REG_GDMA3_FWD_CFG;
		break;
	case 1:
		vip_port = BIT(23);
		cfg_addr = REG_GDMA3_FWD_CFG;
		break;
	case 2:
		vip_port = BIT(25);
		cfg_addr = REG_GDMA4_FWD_CFG;
		break;
	case 4:
		vip_port = BIT(24);
		cfg_addr = REG_GDMA4_FWD_CFG;
		break;
	default:
		return -EINVAL;
	}

	if (enable) {
		airoha_fe_set(eth, REG_FE_VIP_PORT_EN, vip_port);
		airoha_fe_set(eth, REG_FE_IFC_PORT_EN, vip_port);
	} else {
		airoha_fe_clear(eth, REG_FE_VIP_PORT_EN, vip_port);
		airoha_fe_clear(eth, REG_FE_IFC_PORT_EN, vip_port);
	}

	airoha_set_port_fwd_cfg(eth, cfg_addr, val);

	return 0;
}

static int airoha_set_gdma_ports(struct airoha_eth *eth, bool enable)
{
	const int port_list[] = { 0, 1, 2, 4 };
	int i, err;

	for (i = 0; i < ARRAY_SIZE(port_list); i++) {
		err = airoha_set_gdma_port(eth, port_list[i], enable);
		if (err)
			return err;
	}

	return 0;
}

static int airoha_dev_open(struct net_device *dev)
{
	struct airoha_eth *eth = netdev_priv(dev);

	netif_start_queue(dev);

	return airoha_set_gdma_ports(eth, true);
}

static int airoha_dev_stop(struct net_device *dev)
{
	struct airoha_eth *eth = netdev_priv(dev);

	netif_stop_queue(dev);

	return airoha_set_gdma_ports(eth, false);
}

static netdev_tx_t airoha_dev_start_xmit(struct sk_buff *skb,
					 struct net_device *dev)
{
	return NETDEV_TX_OK;
}

static int airoha_dev_change_mtu(struct net_device *dev, int new_mtu)
{
	dev->mtu = new_mtu;
	return 0;
}

static const struct net_device_ops airoha_netdev_ops = {
	.ndo_init		= airoha_dev_init,
	.ndo_open		= airoha_dev_open,
	.ndo_stop		= airoha_dev_stop,
	.ndo_start_xmit		= airoha_dev_start_xmit,
	.ndo_change_mtu		= airoha_dev_change_mtu,
};

static const struct phylink_mac_ops airoha_phylink_ops = {
	.validate = phylink_generic_validate,
};

static void airoha_maccr_init(struct airoha_eth *eth)
{
	airoha_fe_set(eth, REG_GDMA1_FWD_CFG, GDMA1_TCP_CKSUM);
	airoha_set_port_fwd_cfg(eth, REG_GDMA1_FWD_CFG, 0);

	airoha_fe_set(eth, REG_FE_CPORT_CFG, FE_CPORT_PAD);
	airoha_fe_rmw(eth, REG_CDMA1_VLAN_CTRL, CDMA1_VLAN_MASK,
		      FIELD_PREP(CDMA1_VLAN_MASK, 0x8100));
	airoha_fe_rmw(eth, REG_GDMA1_LEN_CFG, GDMA1_SHORT_LEN_MASK,
		      FIELD_PREP(GDMA1_SHORT_LEN_MASK, 60));
	airoha_fe_rmw(eth, REG_GDMA1_LEN_CFG, GDMA1_LONG_LEN_MASK,
		      FIELD_PREP(GDMA1_LONG_LEN_MASK, 4004));
}

static int airoha_qdma_init_rx_queue(struct airoha_eth *eth,
				     struct airoha_queue *q, int size)
{
	struct airoha_qdma_desc *desc;
	int qid = q - &eth->q_rx[0];
	dma_addr_t desc_paddr;
	void *desc_addr;

	spin_lock_init(&q->lock);
	q->ndesc = size;

	desc_addr = dmam_alloc_coherent(eth->dev, q->ndesc * sizeof(*desc),
					&desc_paddr, GFP_KERNEL);
	if (!desc_addr)
		return -ENOMEM;

	airoha_qdma_wr(eth, REG_RX_RING_BASE(qid), desc_paddr);
	airoha_qdma_rmw(eth, REG_RX_RING_SIZE(qid), RX_RING_SIZE_MASK,
			FIELD_PREP(RX_RING_SIZE_MASK, size));
	airoha_qdma_rmw(eth, REG_RX_RING_SIZE(qid), RX_RING_THR_MASK,
			FIELD_PREP(RX_RING_THR_MASK, size / 4));
	airoha_qdma_rmw(eth, REG_RX_CPU_IDX(qid), RX_RING_CPU_IDX_MASK,
			q->head);
	airoha_qdma_rmw(eth, REG_RX_DMA_IDX(qid), RX_RING_DMA_IDX_MASK,
			q->head);

	return 0;
}

static int airoha_qdma_init_rx(struct airoha_eth *eth)
{
	int err;

	err = airoha_qdma_init_rx_queue(eth, &eth->q_rx[0], RX0_DSCP_NUM);
	if (err)
		return err;

	return airoha_qdma_init_rx_queue(eth, &eth->q_rx[1], RX1_DSCP_NUM);
}

static int airoha_qdma_init_tx_queue(struct airoha_eth *eth,
				     struct airoha_queue *q, int size)
{
	struct airoha_qdma_desc *desc;
	int qid = q - &eth->q_xmit[0];
	dma_addr_t desc_paddr;
	void *desc_addr;

	spin_lock_init(&q->lock);
	q->ndesc = size;

	desc_addr = dmam_alloc_coherent(eth->dev, q->ndesc * sizeof(*desc),
					&desc_paddr, GFP_KERNEL);
	if (!desc_addr)
		return -ENOMEM;

	airoha_qdma_wr(eth, REG_TX_RING_BASE(qid), desc_paddr);

	return 0;
}

static int airoha_qdma_init_tx(struct airoha_eth *eth)
{
	int err;

	err = airoha_qdma_init_tx_queue(eth, &eth->q_xmit[0], TX0_DSCP_NUM);
	if (err)
		return err;

	return airoha_qdma_init_tx_queue(eth, &eth->q_xmit[1], TX1_DSCP_NUM);
}

static int airoha_qdma_init(struct airoha_eth *eth)
{
	int err;

	airoha_qdma_rmw(eth, REG_LMGR_INIT_CFG, HWFWD_PKTSIZE_OVERHEAD_MASK,
			FIELD_PREP(HWFWD_PKTSIZE_OVERHEAD_MASK, 0x14));

	err = airoha_qdma_init_tx(eth);
	if (err)
		return err;

	return airoha_qdma_init_rx(eth);
}

static int airoha_hw_init(struct airoha_eth *eth)
{
	airoha_maccr_init(eth);

	return airoha_qdma_init(eth);
}

static int airoha_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct net_device *net_dev;
	phy_interface_t phy_mode;
	struct airoha_eth *eth;
	int err;

	net_dev = devm_alloc_etherdev(dev, sizeof(*eth));
	if (!net_dev) {
		dev_err(dev, "alloc_etherdev failed\n");
		return -ENOMEM;
	}

	eth = netdev_priv(net_dev);
	eth->fe_regs = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(eth->fe_regs))
		return dev_err_probe(dev, PTR_ERR(eth->fe_regs),
				     "failed to iomap fe regs\n");

	eth->qdma_regs = devm_platform_ioremap_resource(pdev, 1);
	if (IS_ERR(eth->qdma_regs))
		return dev_err_probe(dev, PTR_ERR(eth->qdma_regs),
				     "failed to iomap qdma regs\n");

	net_dev->netdev_ops = &airoha_netdev_ops;
	net_dev->max_mtu = AIROHA_MAX_MTU;
	net_dev->watchdog_timeo = 5 * HZ;
	/* FIXME: check HW features */
	net_dev->hw_features = AIROHA_HW_FEATURES;
	net_dev->features |= net_dev->hw_features;
	net_dev->dev.of_node = np;
	/* FIXME: missing IRQ line here */
	SET_NETDEV_DEV(net_dev, dev);

	err = of_get_ethdev_address(np, net_dev);
	if (err) {
		if (err == -EPROBE_DEFER)
			return err;

		eth_hw_addr_random(net_dev);
		dev_err(dev, "generated random MAC address %pM\n",
			net_dev->dev_addr);
	}

	eth->dev = dev;
	err = airoha_hw_init(eth);
	if (err)
		return err;

	/* phylink create */
	err = of_get_phy_mode(np, &phy_mode);
	if (err) {
		dev_err(dev, "incorrect phy-mode\n");
		return err;
	}

	/* mac config is not set */
	eth->interface = PHY_INTERFACE_MODE_NA;
	eth->speed = SPEED_UNKNOWN;

	eth->phylink_config.dev = &net_dev->dev;
	eth->phylink_config.type = PHYLINK_NETDEV;
	eth->phylink_config.mac_capabilities = MAC_ASYM_PAUSE | MAC_SYM_PAUSE |
					       MAC_10 | MAC_100 | MAC_1000 |
					       MAC_2500FD;

	/* FIXME: missing phylink_ops callbacks */
	eth->phylink = phylink_create(&eth->phylink_config,
				      of_fwnode_handle(np), phy_mode,
				      &airoha_phylink_ops);
	if (IS_ERR(eth->phylink))
		return PTR_ERR(eth->phylink);

	return register_netdev(net_dev);
}

static void airoha_remove(struct platform_device *pdev)
{
}

const struct of_device_id of_airoha_match[] = {
	{ .compatible = "airoha,en7581-eth" },
	{ /* sentinel */ }
};

static struct platform_driver airoha_driver = {
	.probe = airoha_probe,
	.remove_new = airoha_remove,
	.driver = {
		.name = "airoha_eth",
		.of_match_table = of_airoha_match,
	},
};
module_platform_driver(airoha_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lorenzo Bianconi <lorenzo@kernel.org>");
MODULE_DESCRIPTION("Ethernet driver for Airoha SoC");
