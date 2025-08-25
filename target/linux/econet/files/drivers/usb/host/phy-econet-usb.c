// SPDX-License-Identifier: GPL-2.0
/*
 * Author: Benjamin Larsson <benjamin.larsson@genexis.eu>
 */

#include <dt-bindings/phy/phy.h>
#include <linux/bitfield.h>
#include <linux/math.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>

/* SSUSB SIFSLV FMREG (0x100) */
#define FMREG					0x100
#define   FMCR0					0x00
#define     RG_MONCLK_SEL			GENMASK(27, 26)
#define       CN_MONCLK_SEL0			FIELD_PREP_CONST(RG_MONCLK_SEL, 0x0)
#define       CN_MONCLK_SEL1			FIELD_PREP_CONST(RG_MONCLK_SEL, 0x1)
#define       CN_MONCLK_SEL2			FIELD_PREP_CONST(RG_MONCLK_SEL, 0x2)
#define       CN_MONCLK_SEL3			FIELD_PREP_CONST(RG_MONCLK_SEL, 0x3)
#define     RG_FREQDET_EN			BIT(24)
#define     RG_CYCLECNT				GENMASK(23, 0)
#define   FMMONR0				0x0c
#define     RG_FM_OUT				GENMASK(31, 0)
#define   FMMONR1				0x10
#define     RG_FM_VALID				BIT(0)
#define     RG_FRCK_EN				BIT(8)

/* SSUSB SIFSLV U2PHY COM (0x800, 0x1000) */
#define U2PHY_COM				0x00
#define   USBPHYACR0				0x10
#define     RG_HSTX_SRCAL_EN			BIT(23)
#define     RG_HSTX_SRCTRL			GENMASK(18, 16)
#define   USBPHYACR3				0x1C
#define     RG_REV				GENMASK(7, 0)
#define   U2PHYDCR1				0x64
#define     RG_USB20_SW_PLLMODE			GENMASK(19, 18)

/* SSUSB SIFSLV U3PHYD (0x900) */
#define U3PHYD					0x00
#define   LFPS1					0x0c
#define     RG_SSUSB_FWAKE_TH			GENMASK(21, 16)

/* SSUSB SIFSLV U3PHYD BANK2 (0xA00) offset based on SSUSB SIFSLV U3PHYD */
#define U3PHYD_BANK2				0x100
#define   B2_PHYD_RXDET1			0x028
#define     RG_SSUSB_RXDET_STB2_SET		GENMASK(17, 9)
#define   B2_PHYD_RXDET2			0x02C
#define     RG_SSUSB_RXDET_STB2_SET_P3		GENMASK(8, 0)

/* SSUSB SIFSLV U3PHYA (0xB00) offset based on SSUSB SIFSLV U3PHYD */
#define U3PHYA					0x200
#define   U3PHYA_REG2				0x008
#define     RG_SSUSB_SYSPLL_LF			BIT(31)
#define     RG_SSUSB_SYSPLL_FBDIV		GENMASK(30, 24)
#define     RG_SSUSB_SYSPLL_POSDIV		GENMASK(23, 22)
#define     RG_SSUSB_SYSPLL_VCO_DIV_SEL		BIT(21)
#define     RG_SSUSB_SYSPLL_BLP			BIT(20)
#define     RG_SSUSB_SYSPLL_BP			BIT(19)
#define     RG_SSUSB_SYSPLL_BR			BIT(18)
#define     RG_SSUSB_SYSPLL_BC			BIT(17)
#define     RG_SSUSB_SYSPLL_DIVEN		GENMASK(16, 14)
#define     RG_SSUSB_SYSPLL_FPEN		BIT(13)
#define     RG_SSUSB_SYSPLL_MONCK_EN		BIT(12)
#define     RG_SSUSB_SYSPLL_MONVC_EN		BIT(11)
#define     RG_SSUSB_SYSPLL_MONREF_EN		BIT(10)
#define     RG_SSUSB_SYSPLL_VOD_EN		BIT(9)
#define     RG_SSUSB_SYSPLL_CK_SEL		BIT(8)
#define   U3PHYA_REG4				0x010
#define     RG_SSUSB_SYSPLL_PCW_NCPO		GENMASK(31, 1)
#define   U3PHYA_REG9				0x024
#define     RG_SSUSB_PLL_DDS_DMY		GENMASK(31, 16)
#define     RG_SSUSB_PLL_SSC_PRD		GENMASK(15, 0)

/* SSUSB SIFSLV U3PHYA DA (0xC00) offset based on SSUSB SIFSLV U3PHYD */
#define U3PHYA_DA				0x300
#define   U3PHYA_DA_REG9			0x01C
#define     RG_SSUSB_PLL_FBKDIV_PE2H		GENMASK(30, 24)
#define     RG_SSUSB_PLL_FBKDIV_PE1D		GENMASK(22, 16)
#define     RG_SSUSB_PLL_FBKDIV_PE1H		GENMASK(14, 8)
#define     RG_SSUSB_PLL_FBKDIV_U3		GENMASK(6, 0)
#define   U3PHYA_DA_REG12			0x024
#define     RG_SSUSB_PLL_PCW_NCPO_U3		GENMASK(30, 0)
#define   U3PHYA_DA_REG13			0x028
#define     RG_SSUSB_PLL_PCW_NCPO_PE1H		GENMASK(30, 0)
#define   U3PHYA_DA_REG14			0x02C
#define     RG_SSUSB_PLL_PCW_NCPO_PE1D		GENMASK(30, 0)
#define   U3PHYA_DA_REG15			0x030
#define     RG_SSUSB_PLL_PCW_NCPO_PE2H		GENMASK(30, 0)
#define   U3PHYA_DA_REG19			0x038
#define     RG_SSUSB_PLL_SSC_DELTA1_PE1H	GENMASK(31, 16)
#define     RG_SSUSB_PLL_SSC_DELTA1_U3		GENMASK(15, 0)
#define   U3PHYA_DA_REG20			0x03C
#define     RG_SSUSB_PLL_SSC_DELTA1_PE2H	GENMASK(31, 16)
#define     RG_SSUSB_PLL_SSC_DELTA1_PE1D	GENMASK(15, 0)
#define   U3PHYA_DA_REG21			0x040
#define     RG_SSUSB_PLL_SSC_DELTA_U3		GENMASK(31, 16)
#define     RG_SSUSB_PLL_SSC_DELTA1_PE2D	GENMASK(15, 0)
#define   U3PHYA_DA_REG23			0x044
#define     RG_SSUSB_PLL_SSC_DELTA_PE1D		GENMASK(31, 16)
#define     RG_SSUSB_PLL_SSC_DELTA_PE1H		GENMASK(15, 0)
#define   U3PHYA_DA_REG25			0x048
#define     RG_SSUSB_PLL_SSC_DELTA_PE2D		GENMASK(31, 16)
#define     RG_SSUSB_PLL_SSC_DELTA_PE2H		GENMASK(15, 0)

#define ECONET_USB_PHY_U2_FM_DET_CYCLE_CNT	1024
#define ECONET_USB_PHY_REF_CK			20
#define ECONET_USB_PHY_U2_SR_COEF		28
#define ECONET_USB_PHY_U2_SR_COEF_DIVISOR	1000

#define ECONET_USB_PHY_FREQDET_SLEEP		1000 /* 1ms */
#define ECONET_USB_PHY_FREQDET_TIMEOUT		(ECONET_USB_PHY_FREQDET_SLEEP * 10)

#define ECONET_USB_PHY_MAX_INSTANCE		3


struct econet_phy_instance {
	struct phy *phy;
	struct regmap *regmap;
	u32 index;
	u32 type;
	u32 port_id;
	bool setup_25mhz_xtal;
};

struct econet_usb_phy_priv {
	struct device *dev;

	unsigned int id;
	void __iomem *base;
	struct regmap *regmap;
	struct econet_phy_instance **phys;
	int nphys;
};

static int u2_slew_rate_calibration(struct econet_usb_phy_priv *priv,
	struct econet_phy_instance *instance)
{
	struct regmap *regmap = priv->regmap;
	struct regmap *com = instance->regmap;
	int fm_out;
	u32 srctrl;

	dev_info(priv->dev, "%s\n", __func__);

	/* Enable HS TX SR calibration */
	regmap_set_bits(com, U2PHY_COM + USBPHYACR0, RG_HSTX_SRCAL_EN);

	usleep_range(1000, 1500);

	/* Enable Free run clock */
	regmap_set_bits(regmap, FMREG + FMMONR1,	RG_FRCK_EN);

	/* Select Monitor Clock */
	if (instance->port_id == 0)
		regmap_update_bits(regmap, U2PHY_COM + FMCR0, RG_MONCLK_SEL, CN_MONCLK_SEL0);
	else if (instance->port_id == 1)
		regmap_update_bits(regmap, U2PHY_COM + FMCR0, RG_MONCLK_SEL, CN_MONCLK_SEL1);
	else {
		dev_err(priv->dev, "invalid port id (%d)\n", instance->port_id);
		return -EINVAL;
	}

	/* Set cyclecnt */
	regmap_update_bits(regmap, FMREG + FMCR0, RG_CYCLECNT,
			   FIELD_PREP_CONST(RG_CYCLECNT, ECONET_USB_PHY_U2_FM_DET_CYCLE_CNT));

	/* Enable Frequency meter */
	regmap_set_bits(regmap, FMREG + FMCR0, RG_FREQDET_EN);

	/* Timeout can happen and we will apply workaround at the end */
	regmap_read_poll_timeout(regmap, FMREG + FMMONR0, fm_out,
				 fm_out, ECONET_USB_PHY_FREQDET_SLEEP,
				 ECONET_USB_PHY_FREQDET_TIMEOUT);
//FIXME
	/* Disable Frequency meter */
	regmap_clear_bits(regmap, FMREG + FMCR0, RG_FREQDET_EN);

	/* Disable Free run clock */
	regmap_clear_bits(regmap, FMREG + FMMONR1, RG_FRCK_EN);

	/* Disable HS TX SR calibration */
	regmap_clear_bits(com, U2PHY_COM + USBPHYACR0, RG_HSTX_SRCAL_EN);

	usleep_range(1000, 1500);

	/* Frequency was not detected, use default SR calibration value */
	if (!fm_out) {
		srctrl = 0x5;
		dev_err(priv->dev, "Frequency not detected, using default SR calibration.\n");
	/* (1024 / FM_OUT) * REF_CK * U2_SR_COEF (round to the nearest digits) */
	} else {
		srctrl = ECONET_USB_PHY_REF_CK * ECONET_USB_PHY_U2_SR_COEF;
		srctrl = (srctrl * ECONET_USB_PHY_U2_FM_DET_CYCLE_CNT) / fm_out;
		srctrl = DIV_ROUND_CLOSEST(srctrl, ECONET_USB_PHY_U2_SR_COEF_DIVISOR);
		dev_dbg(priv->dev, "SR calibration applied: %x\n", srctrl);
	}

	regmap_update_bits(com, U2PHY_COM + FMCR0, USBPHYACR0,
			   FIELD_PREP(RG_HSTX_SRCTRL, srctrl));

	return 0;
}

static int econet_usb_phy_u2_init(struct econet_usb_phy_priv *priv,
	struct econet_phy_instance *instance)
{
	struct regmap *com = instance->regmap;

	dev_info(priv->dev, "%s, port id = %d\n", __func__, instance->port_id);

	if (instance->port_id == 0)
		regmap_update_bits(com, U2PHY_COM + USBPHYACR3, RG_REV, 0x8);
	else if (instance->port_id == 1)
		regmap_update_bits(com, U2PHY_COM + USBPHYACR3, RG_REV, 0x0);

	u2_slew_rate_calibration(priv, instance);

	/* set SW PLL Stable mode to 1 for U2 LPM device remote wakeup */
	regmap_update_bits(com, U2PHY_COM + U2PHYDCR1, RG_USB20_SW_PLLMODE,
			   FIELD_PREP(RG_USB20_SW_PLLMODE, 0x1));

	usleep_range(1000, 1500);

	return 0;
}

static int u3_setup_25mhz_xtal(struct econet_usb_phy_priv *priv, struct econet_phy_instance *instance)
{
	struct regmap *u3phyd = instance->regmap;

	dev_info(priv->dev, "%s\n", __func__);

	/* Setup 25MHz XTAL */
	regmap_update_bits(u3phyd, U3PHYA_DA + U3PHYA_DA_REG9, RG_SSUSB_PLL_FBKDIV_U3,
			   FIELD_PREP(RG_SSUSB_PLL_FBKDIV_U3, 0x18));
	regmap_update_bits(u3phyd, U3PHYA_DA + U3PHYA_DA_REG9, RG_SSUSB_PLL_FBKDIV_PE1H,
			   FIELD_PREP(RG_SSUSB_PLL_FBKDIV_PE1H, 0x18));
	regmap_update_bits(u3phyd, U3PHYA_DA + U3PHYA_DA_REG9, RG_SSUSB_PLL_FBKDIV_PE2H,
			   FIELD_PREP(RG_SSUSB_PLL_FBKDIV_PE2H, 0x18));
	regmap_update_bits(u3phyd, U3PHYA_DA + U3PHYA_DA_REG12, RG_SSUSB_PLL_PCW_NCPO_U3,
			   FIELD_PREP(RG_SSUSB_PLL_PCW_NCPO_U3, 0x18000000));
	regmap_update_bits(u3phyd, U3PHYA_DA + U3PHYA_DA_REG13, RG_SSUSB_PLL_PCW_NCPO_PE1H,
			   FIELD_PREP(RG_SSUSB_PLL_PCW_NCPO_PE1H, 0x18000000));
	regmap_update_bits(u3phyd, U3PHYA_DA + U3PHYA_DA_REG15, RG_SSUSB_PLL_PCW_NCPO_PE2H,
			   FIELD_PREP(RG_SSUSB_PLL_PCW_NCPO_PE2H, 0x18000000));
	regmap_update_bits(u3phyd, U3PHYA_DA + U3PHYA_DA_REG19, RG_SSUSB_PLL_SSC_DELTA1_PE1H,
			   FIELD_PREP(RG_SSUSB_PLL_SSC_DELTA1_PE1H, 0x4a));
	regmap_update_bits(u3phyd, U3PHYA_DA + U3PHYA_DA_REG19, RG_SSUSB_PLL_SSC_DELTA1_U3,
			   FIELD_PREP(RG_SSUSB_PLL_SSC_DELTA1_U3, 0x4a));
	regmap_update_bits(u3phyd, U3PHYA_DA + U3PHYA_DA_REG19, RG_SSUSB_PLL_SSC_DELTA1_PE2H,
			   FIELD_PREP(RG_SSUSB_PLL_SSC_DELTA1_PE2H, 0x4a));
	regmap_update_bits(u3phyd, U3PHYA_DA + U3PHYA_DA_REG21, RG_SSUSB_PLL_SSC_DELTA_U3,
			   FIELD_PREP(RG_SSUSB_PLL_SSC_DELTA_U3, 0x48));
	regmap_update_bits(u3phyd, U3PHYA_DA + U3PHYA_DA_REG23, RG_SSUSB_PLL_SSC_DELTA_PE1H,
			   FIELD_PREP(RG_SSUSB_PLL_SSC_DELTA_PE1H, 0x48));
	regmap_update_bits(u3phyd, U3PHYA_DA + U3PHYA_DA_REG23, RG_SSUSB_PLL_SSC_DELTA_PE2H,
			   FIELD_PREP(RG_SSUSB_PLL_SSC_DELTA_PE2H, 0x48));

	regmap_update_bits(u3phyd, U3PHYA + U3PHYA_REG9, RG_SSUSB_PLL_SSC_PRD,
			   FIELD_PREP(RG_SSUSB_PLL_SSC_PRD, 0x190));
	regmap_update_bits(u3phyd, U3PHYA + U3PHYA_REG4, RG_SSUSB_SYSPLL_PCW_NCPO,
			   FIELD_PREP(RG_SSUSB_SYSPLL_PCW_NCPO, 0xe000000));
	regmap_update_bits(u3phyd, U3PHYA + U3PHYA_REG2, RG_SSUSB_SYSPLL_FBDIV,
			   FIELD_PREP(RG_SSUSB_SYSPLL_FBDIV, 0xe));
	return 0;
}

static int econet_usb_phy_u3_init(struct econet_usb_phy_priv *priv, struct econet_phy_instance *instance)
{
	struct regmap *u3phyd = instance->regmap;

	dev_info(priv->dev, "%s\n", __func__);

	/* Patch TxDetRx Timing for E1 */
	regmap_update_bits(u3phyd, U3PHYD_BANK2 + B2_PHYD_RXDET1, RG_SSUSB_RXDET_STB2_SET,
			   FIELD_PREP(RG_SSUSB_RXDET_STB2_SET, 0x10));

	regmap_update_bits(u3phyd, U3PHYD_BANK2 + B2_PHYD_RXDET2, RG_SSUSB_RXDET_STB2_SET_P3,
			   FIELD_PREP(RG_SSUSB_RXDET_STB2_SET_P3, 0x10));
	
	/* Patch LFPS Filter Threshold for E1 */
	regmap_update_bits(u3phyd, U3PHYD + LFPS1, RG_SSUSB_FWAKE_TH,
			   FIELD_PREP(RG_SSUSB_FWAKE_TH, 0x34));

	if (instance->setup_25mhz_xtal)
		u3_setup_25mhz_xtal(priv, instance);

	return 0;
}

static int econet_usb_phy_init(struct phy *phy)
{
	struct econet_phy_instance *instance = phy_get_drvdata(phy);
	struct econet_usb_phy_priv *priv = dev_get_drvdata(phy->dev.parent);

	dev_info(priv->dev, "%s\n", __func__);

	if (instance->type == PHY_TYPE_USB2)
		return econet_usb_phy_u2_init(priv, instance);
	else if (instance->type == PHY_TYPE_USB3)
		return econet_usb_phy_u3_init(priv, instance);

	return -EINVAL;
}

static int econet_usb_phy_exit(struct phy *phy)
{
	return 0;
}

static struct phy *econet_usb_phy_xlate(struct device *dev,
					const struct of_phandle_args *args)
{
	struct econet_usb_phy_priv *priv = dev_get_drvdata(dev);
	struct econet_phy_instance *instance = NULL;
	struct device_node *phy_np = args->np;
	int index;

	dev_info(priv->dev, "%s\n", __func__);

	if (args->args_count != 1) {
		dev_err(dev, "invalid number of cells in 'phy' property\n");
		return ERR_PTR(-EINVAL);
	}

	for (index = 0; index < ECONET_USB_PHY_MAX_INSTANCE; index++)
		if (phy_np == priv->phys[index]->phy->dev.of_node) {
			instance = priv->phys[index];
			break;
		}

	if (!instance) {
		dev_err(dev, "failed to find appropriate phy\n");
		return ERR_PTR(-EINVAL);
	}

	instance->type = args->args[0];
	if (!(instance->type == PHY_TYPE_USB2 || instance->type == PHY_TYPE_USB3)) {
		dev_err(dev, "unsupported device type: %d\n", instance->type);
		return ERR_PTR(-EINVAL);
	}

	return instance->phy;
}

static int econet_usb_phy_power_on(struct phy *phy)
{
	struct econet_phy_instance *instance = phy_get_drvdata(phy);
	struct econet_usb_phy_priv *priv = dev_get_drvdata(phy->dev.parent);

	dev_info(priv->dev, "%s\n", __func__);

	if (instance->type == PHY_TYPE_USB2) {
		dev_dbg(priv->dev, "%s PHY_TYPE_USB2\n", __func__);

	} else if (instance->type == PHY_TYPE_USB3) {
		dev_dbg(priv->dev, "%s PHY_TYPE_USB3\n", __func__);
	}

	return 0;
}

static const struct phy_ops econet_phy = {
	.init		= econet_usb_phy_init,
 	.exit		= econet_usb_phy_exit,
 	.power_on	= econet_usb_phy_power_on,
//	.power_off	= econet_usb_phy_power_off,
//	.set_mode	= econet_usb_phy_set_mode,
	.owner		= THIS_MODULE,
};

static const struct regmap_config econet_usb_phy_regmap_config = {
	.reg_bits = 32,
	.val_bits = 32,
	.reg_stride = 4,
};

static int econet_usb_phy_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct phy_provider *provider;
	struct econet_usb_phy_priv *priv;
	struct resource res;
	void *base;
	int port;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	dev_info(priv->dev, "%s\n", __func__);

	priv->dev = dev;

	priv->nphys = of_get_child_count(np);
	priv->phys = devm_kcalloc(dev, priv->nphys,
				       sizeof(*priv->phys), GFP_KERNEL);
	if (!priv->phys)
		return -ENOMEM;

	base = devm_platform_ioremap_resource(pdev, 0);

	platform_set_drvdata(pdev, priv);
	priv->regmap = devm_regmap_init_mmio(dev, base, &econet_usb_phy_regmap_config);
	if (IS_ERR(priv->regmap))
		return PTR_ERR(priv->regmap);

	port = 0;
	for_each_child_of_node_scoped(np, child_np) {
		struct econet_phy_instance *instance;
		struct device *subdev;
		struct phy *phy;
		void __iomem *phy_base;
		int retval;

		instance = devm_kzalloc(dev, sizeof(*instance), GFP_KERNEL);
		if (!instance)
			return -ENOMEM;

		priv->phys[port] = instance;

		phy = devm_phy_create(dev, child_np, &econet_phy);
		if (IS_ERR(phy)) {
			dev_err(dev, "failed to create phy\n");
			return PTR_ERR(phy);
		}

		subdev = &phy->dev;
		retval = of_address_to_resource(child_np, 0, &res);
		if (retval) {
			dev_err(subdev, "failed to get address resource(id-%d)\n",
				port);
			return retval;
		}

		phy_base = devm_ioremap_resource(subdev, &res);
		if (IS_ERR(phy_base))
			return PTR_ERR(phy_base);

		instance->regmap = devm_regmap_init_mmio(subdev, phy_base,
					     &econet_usb_phy_regmap_config);
		if (IS_ERR(instance->regmap))
			return PTR_ERR(instance->regmap);

		instance->setup_25mhz_xtal = device_property_read_bool(subdev, "setup-25mhz-xtal");
		dev_info(subdev, "%s 25mhz xtal = %d\n", __func__, instance->setup_25mhz_xtal);

		device_property_read_u32(subdev, "port-id", &instance->port_id);
		dev_info(subdev, "%s port_id = %d\n", __func__, instance->port_id);

		instance->phy = phy;
		instance->index = port;
		phy_set_drvdata(phy, instance);
		port++;
	}

	provider = devm_of_phy_provider_register(dev, econet_usb_phy_xlate);

	return PTR_ERR_OR_ZERO(provider);
}

static const struct of_device_id econet_phy_id_table[] = {
	{ .compatible = "econet,en751221-usb-phy" },
	{ },
};
MODULE_DEVICE_TABLE(of, econet_phy_id_table);

static struct platform_driver econet_usb_phy_driver = {
	.probe		= econet_usb_phy_probe,
	.driver		= {
		.name	= "econet-usb-phy",
		.of_match_table = econet_phy_id_table,
	},
};

module_platform_driver(econet_usb_phy_driver);

MODULE_AUTHOR("Benjamin Larsson <benjamin.larsson@genexis.eu>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Econet USB PHY driver");
