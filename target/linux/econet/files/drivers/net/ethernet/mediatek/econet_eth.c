#include <linux/etherdevice.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/of.h>
#include <linux/platform_device.h>

#define ECONET_RX_ETH_HLEN	(ETH_HLEN + ETH_FCS_LEN)
#define ECONET_MAX_MTU		(2000 - ECONET_RX_ETH_HLEN)

struct econet_eth {
	struct device *dev;
	void __iomem *base;
	void __iomem *qdma_base;
};

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
