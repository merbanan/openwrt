/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2025 
 * Author: Benjamin Larsson <benjamin.larsson@genexis.eu>
 */

/* Very heavily based on code written by Lorenzo Bianconi <lorenzo@kernel.org> */

#include <linux/etherdevice.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/of.h>
#include <linux/of_net.h>
#include <linux/phylink.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>

#include "econet_regs.h"
#include "econet_eth.h"

u32 econet_rr(void __iomem *base, u32 offset)
{
	return readl(base + offset);
}

void econet_wr(void __iomem *base, u32 offset, u32 val)
{
	writel(val, base + offset);
}

u32 econet_rmw(void __iomem *base, u32 offset, u32 mask, u32 val)
{
	val |= (econet_rr(base, offset) & ~mask);
	econet_wr(base, offset, val);

	return val;
}

static void econet_set_macaddr(struct econet_gdm_port *port, const u8 *addr)
{
	struct econet_eth *eth = port->qdma->eth;
	u32 val, reg;

	reg = econet_is_lan_gdm_port(port) ? REG_GDM_MAC_LSB(1)
					   : REG_GDM_MAC_LSB(2);

	val = (addr[2] << 24) | (addr[3] << 16) | (addr[4] << 8) | addr[5];
	econet_fe_wr(eth, reg, val);

	reg = econet_is_lan_gdm_port(port) ? REG_GDM_MAC_MSB(1)
					   : REG_GDM_MAC_MSB(2);

	val = (0xf8 << 16) | (addr[0] << 8) | addr[1];
	econet_fe_wr(eth, reg, val);
}

static void econet_fe_maccr_init(struct econet_eth *eth)
{
	int p;

	for (p = 1; p <= ARRAY_SIZE(eth->ports); p++)
		econet_fe_set(eth, REG_GDM_FWD_CFG(p),
			      GDM_TCP_CKSUM | GDM_UDP_CKSUM | GDM_IP4_CKSUM |
			      GDM_DROP_CRC_ERR);

	econet_fe_rmw(eth, REG_CDM1_VLAN_CTRL, CDM1_VLAN_MASK,
		      FIELD_PREP(CDM1_VLAN_MASK, 0x8100));

	econet_fe_set(eth, REG_FE_CPORT_CFG, FE_CPORT_PAD);
}

static void econet_eth_set_port_fwd_cfg(struct econet_eth *eth, u32 addr, u32 val)
{

	econet_fe_rmw(eth, addr, GDM_OCFQ_MASK,
		      FIELD_PREP(GDM_OCFQ_MASK, val));
	econet_fe_rmw(eth, addr, GDM_MCFQ_MASK,
		      FIELD_PREP(GDM_MCFQ_MASK, val));
	econet_fe_rmw(eth, addr, GDM_BCFQ_MASK,
		      FIELD_PREP(GDM_BCFQ_MASK, val));
	econet_fe_rmw(eth, addr, GDM_MYMACFQ_MASK,
		      FIELD_PREP(GDM_MYMACFQ_MASK, val));
}



static void econet_fe_vip_setup(struct econet_eth *eth)
{
	econet_fe_wr(eth, REG_FE_VIP_PATN(0), 0x01);
	econet_fe_wr(eth, REG_FE_VIP_EN(0), PATN_FCPU_EN_MASK | FIELD_PREP(PATN_TYPE_MASK, 2) | PATN_EN_MASK);

	econet_fe_wr(eth, REG_FE_VIP_PATN(1), 0x0806);
	econet_fe_wr(eth, REG_FE_VIP_EN(1),
		     PATN_FCPU_EN_MASK | FIELD_PREP(PATN_TYPE_MASK, 0) |
		     PATN_EN_MASK);

	econet_fe_wr(eth, REG_FE_VIP_PATN(2), 0x02);
	econet_fe_wr(eth, REG_FE_VIP_EN(2),
		     PATN_FCPU_EN_MASK | FIELD_PREP(PATN_TYPE_MASK, 2) |
		     PATN_EN_MASK);

	econet_fe_wr(eth, REG_FE_VIP_PATN(3), 0x8863);
	econet_fe_wr(eth, REG_FE_VIP_EN(3),
		     PATN_FCPU_EN_MASK | FIELD_PREP(PATN_TYPE_MASK, 0) |
		     PATN_EN_MASK);

	econet_fe_wr(eth, REG_FE_VIP_PATN(4), 0xc021);
	econet_fe_wr(eth, REG_FE_VIP_EN(4),
		     PATN_FCPU_EN_MASK | PATN_SP_EN_MASK |
		     FIELD_PREP(PATN_TYPE_MASK, 1) | PATN_EN_MASK);

	econet_fe_wr(eth, REG_FE_VIP_PATN(5), 0x3a);
	econet_fe_wr(eth, REG_FE_VIP_EN(5),
		     PATN_FCPU_EN_MASK | PATN_SP_EN_MASK |
		     FIELD_PREP(PATN_TYPE_MASK, 2) | PATN_EN_MASK);
}

static void econet_fe_crsn_qsel_init(struct econet_eth *eth)
{
	/* CDM1_CRSN_QSEL */
	econet_fe_rmw(eth, REG_CDM1_CRSN_QSEL(1), 0x3,
				 CDM1_QSEL_Q1L);
}

static int econet_fe_init(struct econet_eth *eth)
{
	econet_fe_maccr_init(eth);

	/* PSE IQ reserve */
	econet_fe_rmw(eth, REG_PSE_IQ_REV1, PSE_IQ_RES1_P2_MASK,
		      FIELD_PREP(PSE_IQ_RES1_P2_MASK, 0x20));
	econet_fe_rmw(eth, REG_PSE_IQ_REV2,
		      PSE_IQ_RES2_P5_MASK | PSE_IQ_RES2_P4_MASK,
		      FIELD_PREP(PSE_IQ_RES2_P5_MASK, 0x40));


	econet_fe_vip_setup(eth);
//	econet_fe_pse_ports_init(eth);

// 	econet_fe_set(eth, REG_GDM_MISC_CFG,
// 		      GDM2_RDM_ACK_WAIT_PREF_MASK |
// 		      GDM2_CHN_VLD_MODE_MASK);
// 	econet_fe_rmw(eth, REG_CDM2_FWD_CFG, CDM2_OAM_QSEL_MASK,
// 		      FIELD_PREP(CDM2_OAM_QSEL_MASK, 15));

	econet_fe_crsn_qsel_init(eth);

	econet_fe_clear(eth, REG_FE_CPORT_CFG, FE_CPORT_DIS_GSW2FE_CRC_MASK);
	econet_fe_clear(eth, REG_FE_CPORT_CFG, FE_CPORT_QUEUE_XFC_MASK);
	econet_fe_set(eth, REG_FE_CPORT_CFG, FE_CPORT_PORT_XFC_MASK);

	econet_fe_set(eth, REG_CDM1_VLAN_CTRL, UNTAG_EN);
	econet_fe_set(eth, REG_CDM1_VLAN_CTRL, STAG_EN);

	return 0;
}

static irqreturn_t econet_irq_handler(int irq, void *dev_instance)
{
	struct econet_irq_bank *irq_bank = dev_instance;
	struct econet_qdma *qdma = irq_bank->qdma;
	u32 rx_intr_mask = 0, rx_intr;
	u32 intr;
	int i;

	intr = econet_qdma_rr(qdma, REG_INT_STATUS);
	intr &= irq_bank->irqmask;
	econet_qdma_wr(qdma, REG_INT_STATUS, intr);

	if (!test_bit(DEV_STATE_INITIALIZED, &qdma->eth->state))
		return IRQ_NONE;

	rx_intr = intr & (RX1_DONE_INT | RX0_DONE_INT);
	if (rx_intr) {
//		econet_qdma_irq_disable(irq_bank, QDMA_INT_REG_IDX0, rx_intr);
		rx_intr_mask |= rx_intr;
	}

	for (i = 0; rx_intr_mask && i < ARRAY_SIZE(qdma->q_rx); i++) {
		if (!qdma->q_rx[i].ndesc)
			continue;

		if (rx_intr_mask & BIT(i))
			napi_schedule(&qdma->q_rx[i].napi);
	}

	if (intr & (TX1_DONE_INT | TX0_DONE_INT)) {
//		if (!(intr & TX_DONE_INT_MASK(i)))
//			continue;

//		econet_qdma_irq_disable(irq_bank, QDMA_INT_REG_IDX0,
//					TX_DONE_INT_MASK);
		napi_schedule(&qdma->q_tx_irq.napi);
	}

	return IRQ_HANDLED;
}

static int econet_qdma_init_irq_bank(struct platform_device *pdev,
				      struct econet_qdma *qdma)
{
	struct econet_eth *eth = qdma->eth;
	int id = qdma - &eth->qdma[0];

	struct econet_irq_bank *irq_bank = &(qdma->irq_bank);
	int err, irq_index = 4 * id;
	const char *name;

	spin_lock_init(&irq_bank->irq_lock);
	irq_bank->qdma = qdma;

	irq_bank->irq = platform_get_irq(pdev, irq_index);
	if (irq_bank->irq < 0)
		return irq_bank->irq;

	name = devm_kasprintf(eth->dev, GFP_KERNEL,
				KBUILD_MODNAME ".%d", irq_index);
	if (!name)
		return -ENOMEM;

	err = devm_request_irq(eth->dev, irq_bank->irq,
				econet_irq_handler, IRQF_SHARED, name,
				irq_bank);
	if (err)
		return err;

	return 0;
}

static int econet_qdma_init(struct platform_device *pdev,
			    struct econet_eth *eth,
			    struct econet_qdma *qdma)
{
	int err = 0, id = qdma - &eth->qdma[0];
	const char *res;

	qdma->eth = eth;
	res = devm_kasprintf(eth->dev, GFP_KERNEL, "qdma%d", id);
	if (!res)
		return -ENOMEM;

	qdma->regs = devm_platform_ioremap_resource_byname(pdev, res);
	if (IS_ERR(qdma->regs))
		return dev_err_probe(eth->dev, PTR_ERR(qdma->regs),
				     "failed to iomap qdma%d regs\n", id);

	err = econet_qdma_init_irq_bank(pdev, qdma);
	if (err)
		return err;
//
// 	err = econet_qdma_init_rx(qdma);
// 	if (err)
// 		return err;
//
// 	err = econet_qdma_init_tx(qdma);
// 	if (err)
// 		return err;
//
// 	err = econet_qdma_init_hfwd_queues(qdma);
// 	if (err)
// 		return err;

// 	err = econet_qdma_hw_init(qdma);
// 	if (err)
// 		return err;

	return err;
}

static int econet_hw_init(struct platform_device *pdev,
			  struct econet_eth *eth)
{
	int err, i;

	err = reset_control_bulk_assert(ARRAY_SIZE(eth->rsts), eth->rsts);
	if (err)
		return err;

	msleep(20);
	err = reset_control_bulk_deassert(ARRAY_SIZE(eth->rsts), eth->rsts);
	if (err)
		return err;

	msleep(20);
	err = econet_fe_init(eth);
	if (err)
		return err;

	for (i = 0; i < ARRAY_SIZE(eth->qdma); i++) {
		err = econet_qdma_init(pdev, eth, &eth->qdma[i]);
		if (err)
			return err;
	}

// 	err = econet_ppe_init(eth);
// 	if (err)
// 		return err;

	set_bit(DEV_STATE_INITIALIZED, &eth->state);

	return 0;
}


static int econet_dev_init(struct net_device *dev)
{
	struct econet_gdm_port *port = netdev_priv(dev);
	struct econet_eth *eth = port->qdma->eth;
	u32 pse_port;

	econet_set_macaddr(port, dev->dev_addr);

	switch (port->id) {
	case 2:
		pse_port = FE_PSE_PORT_PPE;
		break;
	default:
		pse_port = FE_PSE_PORT_PPE;
		break;
	}

	econet_eth_set_port_fwd_cfg(eth, REG_GDM_FWD_CFG(port->id), pse_port);

	return 0;
}


static netdev_tx_t econet_dev_start_xmit(struct sk_buff *skb,
					 struct net_device *dev)
{
	return NETDEV_TX_OK;
}

static int econet_dev_change_mtu(struct net_device *dev, int new_mtu)
{
	dev->mtu = new_mtu;
	return 0;
}

static const struct net_device_ops econet_netdev_ops = {
	.ndo_init		= econet_dev_init,
	.ndo_start_xmit		= econet_dev_start_xmit,
	.ndo_change_mtu		= econet_dev_change_mtu,
};

static void econet_qdma_start_napi(struct econet_qdma *qdma)
{
// 	int i;
// 
// 	for (i = 0; i < ARRAY_SIZE(qdma->q_tx_irq); i++)
// 		napi_enable(&qdma->q_tx_irq[i].napi);
// 
// 	for (i = 0; i < ARRAY_SIZE(qdma->q_rx); i++) {
// 		if (!qdma->q_rx[i].ndesc)
// 			continue;
// 
// 		napi_enable(&qdma->q_rx[i].napi);
// 	}
}

static int econet_alloc_gdm_port(struct econet_eth *eth,
				 struct device_node *np, int index)
{
	const __be32 *id_ptr = of_get_property(np, "reg", NULL);
	struct econet_gdm_port *port;
	struct econet_qdma *qdma;
	struct net_device *dev;
	int err, p;
	u32 id;

	if (!id_ptr) {
		dev_err(eth->dev, "missing gdm port id\n");
		return -EINVAL;
	}

	id = be32_to_cpup(id_ptr);
	p = id - 1;

	if (!id || id > ARRAY_SIZE(eth->ports)) {
		dev_err(eth->dev, "invalid gdm port id: %d\n", id);
		return -EINVAL;
	}

	if (eth->ports[p]) {
		dev_err(eth->dev, "duplicate gdm port id: %d\n", id);
		return -EINVAL;
	}

	dev = devm_alloc_etherdev_mqs(eth->dev, sizeof(*port),
				      ECONET_NUM_NETDEV_TX_RINGS,
				      ECONET_NUM_RX_RING);
	if (!dev) {
		dev_err(eth->dev, "alloc_etherdev failed\n");
		return -ENOMEM;
	}

	qdma = &eth->qdma[index % ECONET_MAX_NUM_QDMA];
	dev->netdev_ops = &econet_netdev_ops;
//	dev->ethtool_ops = &econet_ethtool_ops;
	dev->max_mtu = ECONET_MAX_MTU;
	dev->watchdog_timeo = 5 * HZ;
	dev->hw_features = NETIF_F_IP_CSUM | NETIF_F_RXCSUM |
			   NETIF_F_TSO6 | NETIF_F_IPV6_CSUM;

	dev->features |= dev->hw_features;
	dev->vlan_features = dev->hw_features;
	dev->dev.of_node = np;
//	dev->irq = qdma->irq_banks[0].irq;
	SET_NETDEV_DEV(dev, eth->dev);

	/* reserve hw queues for HTB offloading */
	err = netif_set_real_num_tx_queues(dev, ECONET_NUM_TX_RING);
	if (err)
		return err;

	err = of_get_ethdev_address(np, dev);
	if (err) {
		if (err == -EPROBE_DEFER)
			return err;

		eth_hw_addr_random(dev);
		dev_info(eth->dev, "generated random MAC address %pM\n",
			 dev->dev_addr);
	}

	port = netdev_priv(dev);
//	u64_stats_init(&port->stats.syncp);
//	spin_lock_init(&port->stats.lock);
	port->qdma = qdma;
	port->dev = dev;
	port->id = id;
	eth->ports[p] = port;

// 	err = econet_metadata_dst_alloc(port);
// 	if (err)
// 		return err;

	err = register_netdev(dev);
	if (err)
		goto free_metadata_dst;

	return 0;

free_metadata_dst:
	//econet_metadata_dst_free(port);
	return err;
}

static int econet_probe(struct platform_device *pdev)
{
	struct device_node *np;
	struct econet_eth *eth;
	int i, err;

	eth = devm_kzalloc(&pdev->dev, sizeof(*eth), GFP_KERNEL);
	if (!eth)
		return -ENOMEM;

	eth->dev = &pdev->dev;

	err = dma_set_mask_and_coherent(eth->dev, DMA_BIT_MASK(32));
	if (err) {
		dev_err(eth->dev, "failed configuring DMA mask\n");
		return err;
	}

	if (IS_ERR(eth->fe_regs))
		return dev_err_probe(eth->dev, PTR_ERR(eth->fe_regs),
				     "failed to iomap fe regs\n");

	eth->rsts[0].id = "fe";
	eth->rsts[1].id = "pdma";
	eth->rsts[2].id = "qdma";
	err = devm_reset_control_bulk_get_exclusive(eth->dev,
						    ARRAY_SIZE(eth->rsts),
						    eth->rsts);
	if (err) {
		dev_err(eth->dev, "failed to get bulk reset lines\n");
		return err;
	}

	eth->napi_dev = alloc_netdev_dummy(0);
	if (!eth->napi_dev)
		return -ENOMEM;

	/* Enable threaded NAPI by default */
	eth->napi_dev->threaded = true;
	strscpy(eth->napi_dev->name, "qdma_eth", sizeof(eth->napi_dev->name));
	platform_set_drvdata(pdev, eth);

	err = econet_hw_init(pdev, eth);
	if (err)
		goto error_hw_cleanup;

	for (i = 0; i < ARRAY_SIZE(eth->qdma); i++)
		econet_qdma_start_napi(&eth->qdma[i]);

	i = 0;
	for_each_child_of_node(pdev->dev.of_node, np) {
		if (!of_device_is_compatible(np, "econet,eth-mac"))
			continue;

		if (!of_device_is_available(np))
			continue;

		err = econet_alloc_gdm_port(eth, np, i++);
		if (err) {
			of_node_put(np);
			goto error_napi_stop;
		}
	}

	return 0;

error_napi_stop:
//	for (i = 0; i < ARRAY_SIZE(eth->qdma); i++)
//		econet_qdma_stop_napi(&eth->qdma[i]);
//	econet_ppe_deinit(eth);
error_hw_cleanup:
//	for (i = 0; i < ARRAY_SIZE(eth->qdma); i++)
//		econet_hw_cleanup(&eth->qdma[i]);

//	for (i = 0; i < ARRAY_SIZE(eth->ports); i++) {
//		struct econet_gdm_port *port = eth->ports[i];

//		if (port && port->dev->reg_state == NETREG_REGISTERED) {
//			unregister_netdev(port->dev);
//			econet_metadata_dst_free(port);
//		}
//	}
//	free_netdev(eth->napi_dev);
//	platform_set_drvdata(pdev, NULL);

	return err;
}

static void econet_remove(struct platform_device *pdev)
{
}

const struct of_device_id of_econet_match[] = {
	{ .compatible = "econet,en751221-eth" },
	{ /* sentinel */ }
};

static struct platform_driver econet_driver = {
	.probe = econet_probe,
	.remove_new = econet_remove,
	.driver = {
		.name = KBUILD_MODNAME,
		.of_match_table = of_econet_match,
	},
};
module_platform_driver(econet_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Benjamin Larsson <benjamin.larsson@genexis.eu>");
MODULE_DESCRIPTION("Ethernet driver for Econet SoC");
