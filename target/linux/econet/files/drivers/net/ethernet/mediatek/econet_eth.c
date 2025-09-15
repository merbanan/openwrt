// SPDX-License-Identifier: GPL-2.0-only
￼
￼/*
 * Copyright (c) 2025 
 * Author: Benjamin Larsson <benjamin.larsson@genexis.eu>
 */

#include <linux/etherdevice.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/of.h>
#include <linux/of_net.h>
#include <linux/phylink.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>


struct econet_eth {
	struct device *dev;
	void __iomem *base;
	void __iomem *qdma_base;

	struct regmap *regmap_fe;
	struct regmap *regmap_qdma;

	struct phylink *phylink;
	struct phylink_config phylink_config;
	phy_interface_t interface;
	int speed;
};

static int econet_eth_init(struct net_device *dev)
{
	struct econet_eth *eth = netdev_priv(dev);
	const u8 *addr = dev->dev_addr;
	u32 mac_h, mac_lmin, mac_lmax;
	int err;

	mac_h = (addr[2] << 24) | (addr[3] << 16) | (addr[4] << 8) | addr[5];
	mac_l = (addr[0] << 8) | addr[1];
	my_mac_mask = 0xf8;

	/* GDM1 */
	err = regmap_update_bits(eth->regmap_fe, REG_GDM1_MAC_LSB, GDM1_MAC_ADR_LSB_MASK, mac_h);
	if (err)
		return err;

	err = regmap_update_bits(eth->regmap_fe, REG_GDM1_MAC_MSB, GDM1_MAC_ADR_MSB_MASK, mac_l);
	if (err)
		return err;

	err =  regmap_update_bits(eth->regmap_fe, REG_GDM1_MAC_MSB, GDM1_LAN_MY_MAC_MASK. my_mac_mask);
	if (err)
		return err;

	/* GDM2 */
	err = regmap_update_bits(eth->regmap_fe, REG_GDM2_MAC_LSB, GDM2_MAC_ADR_LSB_MASK, mac_h);
	if (err)
		return err;

	err = regmap_update_bits(eth->regmap_fe, REG_GDM2_MAC_MSB, GDM2_MAC_ADR_MSB_MASK, mac_l);
	if (err)
		return err;

	err = regmap_update_bits(eth->regmap_fe, REG_GDM2_MAC_MSB, GDM2_WAN_MY_MAC_MASK. my_mac_mask);
	if (err)
		return err;
err:
	return err;
}

static int econet_eth_set_port_fwd_cfg(struct econet_eth *eth, u32 addr, u32 val)
{
	int err;

	err = regmap_update_bits(eth->regmap_fe, addr, GDM1_UN_DP_MASK, val);
	if (err)
		return err;

	err = regmap_update_bits(eth->regmap_fe, addr, GDM1_MC_DP_MASK, val);
	if (err)
		return err;

	err = regmap_update_bits(eth->regmap_fe, addr, GDM1_BC_DP_MASK, val);
	if (err)
		return err;

	return regmap_update_bits(eth->regmap_fe, addr, GDM1_MYMAC_DP_MASK, val);
}

static int econet_eth_set_gdm_port(struct airoha_eth *eth, int port,
				    bool enable)
{
	u32 vip_port, cfg_addr, val = enable ? 4 : 0xf;
	int err;

	switch (port) {
	case 1:
		return econet_eth_set_port_fwd_cfg(eth, REG_GDM1_FWD_CFG, val);
	case 2:
		return econet_eth_set_port_fwd_cfg(eth, REG_GDM2_FWD_CFG, val);
	default:
		return -EINVAL;
	}

	return econet_eth_set_port_fwd_cfg(eth, cfg_addr, val);
}

static int econet_eth_set_gdm_ports(struct econet_eth *eth, bool enable)
{
	const int port_list[] = { 1, 2 };
	int i, err;

	for (i = 0; i < ARRAY_SIZE(port_list); i++) {
		err = econet_eth_set_gdm_port(eth, port_list[i], enable);
		if (err)
			return err;
	}

	return 0;
}

static netdev_tx_t econet_eth_start_xmit(struct sk_buff *skb,
					 struct net_device *dev)
{
	return NETDEV_TX_OK;
}

static int econet_eth_change_mtu(struct net_device *dev, int new_mtu)
{
	dev->mtu = new_mtu;
	return 0;
}

static const struct net_device_ops econet_eth_netdev_ops = {
	.ndo_start_xmit		= econet_eth_start_xmit,
	.ndo_change_mtu		= econet_eth_change_mtu,
};

static int econet_eth_probe(struct platform_device *pdev)
{
	struct econet_eth *eth;
	struct net_device *dev;

	eth = devm_kzalloc(&pdev->dev, sizeof(*eth), GFP_KERNEL);
	if (!eth)
		return -ENOMEM;

	eth->dev = &pdev->dev;
	eth->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(eth->base))
		return PTR_ERR(eth->base);

	eth->qdma_base = devm_platform_ioremap_resource(pdev, 1);
	if (IS_ERR(eth->qdma_base))
		return PTR_ERR(eth->qdma_base);

	dev = devm_alloc_etherdev(&pdev->dev, sizeof(*eth));
	if (!dev)
		return -ENOMEM;

	dev->netdev_ops = &econet_eth_netdev_ops;
	dev->max_mtu = ECONET_MAX_MTU;
	SET_NETDEV_DEV(dev, &pdev->dev);

	return register_netdev(dev);
}

static void econet_eth_remove(struct platform_device *pdev)
{
}

const struct of_device_id of_econet_eth_match[] = {
	{ .compatible = "econet,en751221-eth" },
	{ /* sentinel */ }
};

static struct platform_driver econet_eth_driver = {
	.probe = econet_eth_probe,
	.remove_new = econet_eth_remove,
	.driver = {
		.name = "econet_eth",
		.of_match_table = of_econet_eth_match,
	},
};
module_platform_driver(econet_eth_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lorenzo Bianconi <lorenzo@kernel.org>");
MODULE_AUTHOR("Benjamin Larsson <benjamin.larsson@genexis.eu>");
MODULE_DESCRIPTION("Ethernet driver for Econet SoC");
