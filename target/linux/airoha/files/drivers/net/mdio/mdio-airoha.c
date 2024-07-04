// SPDX-License-Identifier: GPL-2.0
/* FILE NAME:  mdio-arht.c
 * PURPOSE:
 *      Airoha MDIO bus Controller Driver
 * NOTES:
 *
 */

#include <linux/delay.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_mdio.h>
#include <linux/phy.h>
#include <linux/platform_device.h>
#include <linux/sched.h>

struct airoha_mdio_priv {
	struct mii_bus *mii_bus;
};

static int airoha_mdio_read(struct mii_bus *bus, int phy_id, int reg)
{
//	int read_data;
//
//#if defined(RDKB_BUILD)
//	read_data = ETHER_MDIO_READ(phy_id, reg);
//#endif
//    return read_data;
	return 0;
}

static int airoha_mdio_write(struct mii_bus *bus, int phy_id,
			    int reg, u16 val)
{	
#if defined(RDKB_BUILD)
	ETHER_MDIO_WRITE(phy_id, reg, val);
#endif
	return 0;
}

static int airoha_mdio_probe(struct platform_device *pdev)
{
	struct airoha_mdio_priv *priv;
	struct mii_bus *bus;
	int rc;

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv) 
		return -ENOMEM;

	priv->mii_bus = mdiobus_alloc();
	if (!priv->mii_bus) {
		dev_err(&pdev->dev, "MDIO bus alloc failed\n");
		return -ENOMEM;
	}

	bus = priv->mii_bus;
	bus->priv = priv;
	bus->name = "airoha mdio bus";
	snprintf(bus->id, MII_BUS_ID_SIZE, "%s-%d", pdev->name, pdev->id);
	bus->parent = &pdev->dev;
	bus->read = airoha_mdio_read;
	bus->write = airoha_mdio_write;


	rc = of_mdiobus_register(bus, pdev->dev.of_node);
	if (rc) {
		dev_err(&pdev->dev, "MDIO bus registration failed\n");
		goto err_airoha_mdio;
	}

	platform_set_drvdata(pdev, priv);

	return 0;

err_airoha_mdio:
	mdiobus_free(bus);
	return rc;
}

static int airoha_mdio_remove(struct platform_device *pdev)
{
	struct airoha_mdio_priv *priv = platform_get_drvdata(pdev);

	mdiobus_unregister(priv->mii_bus);
	mdiobus_free(priv->mii_bus);

	return 0;
}

static const struct of_device_id airoha_mdio_of_match[] = {
	{ .compatible = "airoha,en7581-mdio", },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, airoha_mdio_of_match);

static struct platform_driver airoha_mdio_driver = {
	.driver = {
		.name = "arht-mdio",
		.of_match_table = airoha_mdio_of_match,
	},
	.probe = airoha_mdio_probe,
	.remove = airoha_mdio_remove,
};
module_platform_driver(airoha_mdio_driver);

MODULE_LICENSE("GPL v2");
