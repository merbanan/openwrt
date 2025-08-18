// SPDX-License-Identifier: GPL-2.0
/*
 * Author: Christian Marangi <ansuelsmth@gmail.com>
 * Author: Benjamin Larsson <benjamin.larsson@genexis.eu>
 */

#include <dt-bindings/phy/phy.h>
#include <linux/bitfield.h>
#include <linux/math.h>
#include <linux/module.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>

/* U2PHY */
#define AIROHA_USB_PHY_FMCR0			0x100
#define   AIROHA_USB_PHY_MONCLK_SEL		GENMASK(27, 26)
#define   AIROHA_USB_PHY_MONCLK_SEL0		FIELD_PREP_CONST(AIROHA_USB_PHY_MONCLK_SEL, 0x0)
#define   AIROHA_USB_PHY_MONCLK_SEL1		FIELD_PREP_CONST(AIROHA_USB_PHY_MONCLK_SEL, 0x1)
#define   AIROHA_USB_PHY_MONCLK_SEL2		FIELD_PREP_CONST(AIROHA_USB_PHY_MONCLK_SEL, 0x2)
#define   AIROHA_USB_PHY_MONCLK_SEL3		FIELD_PREP_CONST(AIROHA_USB_PHY_MONCLK_SEL, 0x3)
#define   AIROHA_USB_PHY_FREQDET_EN		BIT(24)
#define   AIROHA_USB_PHY_CYCLECNT		GENMASK(23, 0)
#define AIROHA_USB_PHY_FMMONR0			0x10c
#define   AIROHA_USB_PHY_USB_FM_OUT		GENMASK(31, 0)
#define AIROHA_USB_PHY_FMMONR1			0x110
#define   AIROHA_USB_PHY_FRCK_EN		BIT(8)
/*
#define AIROHA_USB_PHY_USBPHYACR4		0x310
#define   AIROHA_USB_PHY_USB20_FS_CR		GENMASK(10, 8)
#define   AIROHA_USB_PHY_USB20_FS_CR_MAX	FIELD_PREP_CONST(AIROHA_USB_PHY_USB20_FS_CR, 0x0)
#define   AIROHA_USB_PHY_USB20_FS_CR_NORMAL	FIELD_PREP_CONST(AIROHA_USB_PHY_USB20_FS_CR, 0x2)
#define   AIROHA_USB_PHY_USB20_FS_CR_SMALLER	FIELD_PREP_CONST(AIROHA_USB_PHY_USB20_FS_CR, 0x4)
#define   AIROHA_USB_PHY_USB20_FS_CR_MIN	FIELD_PREP_CONST(AIROHA_USB_PHY_USB20_FS_CR, 0x6)
#define   AIROHA_USB_PHY_USB20_FS_SR		GENMASK(2, 0)
#define   AIROHA_USB_PHY_USB20_FS_SR_MAX	FIELD_PREP_CONST(AIROHA_USB_PHY_USB20_FS_SR, 0x0)
#define   AIROHA_USB_PHY_USB20_FS_SR_NORMAL	FIELD_PREP_CONST(AIROHA_USB_PHY_USB20_FS_SR, 0x2)
#define   AIROHA_USB_PHY_USB20_FS_SR_SMALLER	FIELD_PREP_CONST(AIROHA_USB_PHY_USB20_FS_SR, 0x4)
#define   AIROHA_USB_PHY_USB20_FS_SR_MIN	FIELD_PREP_CONST(AIROHA_USB_PHY_USB20_FS_SR, 0x6)
#define AIROHA_USB_PHY_USBPHYACR5		0x314
#define   AIROHA_USB_PHY_USB20_HSTX_SRCAL_EN	BIT(15)
#define   AIROHA_USB_PHY_USB20_HSTX_SRCTRL	GENMASK(14, 12)
#define AIROHA_USB_PHY_USBPHYACR6		0x318
#define   AIROHA_USB_PHY_USB20_BC11_SW_EN	BIT(23)
#define   AIROHA_USB_PHY_USB20_DISCTH		GENMASK(7, 4)
#define   AIROHA_USB_PHY_USB20_DISCTH_400	FIELD_PREP_CONST(AIROHA_USB_PHY_USB20_DISCTH, 0x0)
#define   AIROHA_USB_PHY_USB20_DISCTH_420	FIELD_PREP_CONST(AIROHA_USB_PHY_USB20_DISCTH, 0x1)
#define   AIROHA_USB_PHY_USB20_DISCTH_440	FIELD_PREP_CONST(AIROHA_USB_PHY_USB20_DISCTH, 0x2)
#define   AIROHA_USB_PHY_USB20_DISCTH_460	FIELD_PREP_CONST(AIROHA_USB_PHY_USB20_DISCTH, 0x3)
#define   AIROHA_USB_PHY_USB20_DISCTH_480	FIELD_PREP_CONST(AIROHA_USB_PHY_USB20_DISCTH, 0x4)
#define   AIROHA_USB_PHY_USB20_DISCTH_500	FIELD_PREP_CONST(AIROHA_USB_PHY_USB20_DISCTH, 0x5)
#define   AIROHA_USB_PHY_USB20_DISCTH_520	FIELD_PREP_CONST(AIROHA_USB_PHY_USB20_DISCTH, 0x6)
#define   AIROHA_USB_PHY_USB20_DISCTH_540	FIELD_PREP_CONST(AIROHA_USB_PHY_USB20_DISCTH, 0x7)
#define   AIROHA_USB_PHY_USB20_DISCTH_560	FIELD_PREP_CONST(AIROHA_USB_PHY_USB20_DISCTH, 0x8)
#define   AIROHA_USB_PHY_USB20_DISCTH_580	FIELD_PREP_CONST(AIROHA_USB_PHY_USB20_DISCTH, 0x9)
#define   AIROHA_USB_PHY_USB20_DISCTH_600	FIELD_PREP_CONST(AIROHA_USB_PHY_USB20_DISCTH, 0xa)
#define   AIROHA_USB_PHY_USB20_DISCTH_620	FIELD_PREP_CONST(AIROHA_USB_PHY_USB20_DISCTH, 0xb)
#define   AIROHA_USB_PHY_USB20_DISCTH_640	FIELD_PREP_CONST(AIROHA_USB_PHY_USB20_DISCTH, 0xc)
#define   AIROHA_USB_PHY_USB20_DISCTH_660	FIELD_PREP_CONST(AIROHA_USB_PHY_USB20_DISCTH, 0xd)
#define   AIROHA_USB_PHY_USB20_DISCTH_680	FIELD_PREP_CONST(AIROHA_USB_PHY_USB20_DISCTH, 0xe)
#define   AIROHA_USB_PHY_USB20_DISCTH_700	FIELD_PREP_CONST(AIROHA_USB_PHY_USB20_DISCTH, 0xf)
#define   AIROHA_USB_PHY_USB20_SQTH		GENMASK(3, 0)
#define   AIROHA_USB_PHY_USB20_SQTH_85		FIELD_PREP_CONST(AIROHA_USB_PHY_USB20_SQTH, 0x0)
#define   AIROHA_USB_PHY_USB20_SQTH_90		FIELD_PREP_CONST(AIROHA_USB_PHY_USB20_SQTH, 0x1)
#define   AIROHA_USB_PHY_USB20_SQTH_95		FIELD_PREP_CONST(AIROHA_USB_PHY_USB20_SQTH, 0x2)
#define   AIROHA_USB_PHY_USB20_SQTH_100		FIELD_PREP_CONST(AIROHA_USB_PHY_USB20_SQTH, 0x3)
#define   AIROHA_USB_PHY_USB20_SQTH_105		FIELD_PREP_CONST(AIROHA_USB_PHY_USB20_SQTH, 0x4)
#define   AIROHA_USB_PHY_USB20_SQTH_110		FIELD_PREP_CONST(AIROHA_USB_PHY_USB20_SQTH, 0x5)
#define   AIROHA_USB_PHY_USB20_SQTH_115		FIELD_PREP_CONST(AIROHA_USB_PHY_USB20_SQTH, 0x6)
#define   AIROHA_USB_PHY_USB20_SQTH_120		FIELD_PREP_CONST(AIROHA_USB_PHY_USB20_SQTH, 0x7)
#define   AIROHA_USB_PHY_USB20_SQTH_125		FIELD_PREP_CONST(AIROHA_USB_PHY_USB20_SQTH, 0x8)
#define   AIROHA_USB_PHY_USB20_SQTH_130		FIELD_PREP_CONST(AIROHA_USB_PHY_USB20_SQTH, 0x9)
#define   AIROHA_USB_PHY_USB20_SQTH_135		FIELD_PREP_CONST(AIROHA_USB_PHY_USB20_SQTH, 0xa)
#define   AIROHA_USB_PHY_USB20_SQTH_140		FIELD_PREP_CONST(AIROHA_USB_PHY_USB20_SQTH, 0xb)
#define   AIROHA_USB_PHY_USB20_SQTH_145		FIELD_PREP_CONST(AIROHA_USB_PHY_USB20_SQTH, 0xc)
#define   AIROHA_USB_PHY_USB20_SQTH_150		FIELD_PREP_CONST(AIROHA_USB_PHY_USB20_SQTH, 0xd)
#define   AIROHA_USB_PHY_USB20_SQTH_155		FIELD_PREP_CONST(AIROHA_USB_PHY_USB20_SQTH, 0xe)
#define   AIROHA_USB_PHY_USB20_SQTH_160		FIELD_PREP_CONST(AIROHA_USB_PHY_USB20_SQTH, 0xf)

#define AIROHA_USB_PHY_U2PHYDTM1		0x36c
#define   AIROHA_USB_PHY_FORCE_IDDIG		BIT(9)
#define   AIROHA_USB_PHY_IDDIG			BIT(1)

#define AIROHA_USB_PHY_GPIO_CTLD		0x80c
#define   AIROHA_USB_PHY_C60802_GPIO_CTLD	GENMASK(31, 0)
#define     AIROHA_USB_PHY_SSUSB_IP_SW_RST	BIT(31)
#define     AIROHA_USB_PHY_MCU_BUS_CK_GATE_EN	BIT(30)
#define     AIROHA_USB_PHY_FORCE_SSUSB_IP_SW_RST BIT(29)
#define     AIROHA_USB_PHY_SSUSB_SW_RST		BIT(28)

#define AIROHA_USB_PHY_U3_PHYA_REG0		0xb00
#define   AIROHA_USB_PHY_SSUSB_BG_DIV		GENMASK(29, 28)
#define   AIROHA_USB_PHY_SSUSB_BG_DIV_2		FIELD_PREP_CONST(AIROHA_USB_PHY_SSUSB_BG_DIV, 0x0)
#define   AIROHA_USB_PHY_SSUSB_BG_DIV_4		FIELD_PREP_CONST(AIROHA_USB_PHY_SSUSB_BG_DIV, 0x1)
#define   AIROHA_USB_PHY_SSUSB_BG_DIV_8		FIELD_PREP_CONST(AIROHA_USB_PHY_SSUSB_BG_DIV, 0x2)
#define   AIROHA_USB_PHY_SSUSB_BG_DIV_16	FIELD_PREP_CONST(AIROHA_USB_PHY_SSUSB_BG_DIV, 0x3)
#define AIROHA_USB_PHY_U3_PHYA_REG1		0xb04
#define   AIROHA_USB_PHY_SSUSB_XTAL_TOP_RESERVE	GENMASK(25, 10)
#define AIROHA_USB_PHY_U3_PHYA_REG6		0xb18
#define   AIROHA_USB_PHY_SSUSB_CDR_RESERVE	GENMASK(31, 24)
#define AIROHA_USB_PHY_U3_PHYA_REG8		0xb20
#define   AIROHA_USB_PHY_SSUSB_CDR_RST_DLY	GENMASK(7, 6)
#define   AIROHA_USB_PHY_SSUSB_CDR_RST_DLY_32	FIELD_PREP_CONST(AIROHA_USB_PHY_SSUSB_CDR_RST_DLY, 0x0)
#define   AIROHA_USB_PHY_SSUSB_CDR_RST_DLY_64	FIELD_PREP_CONST(AIROHA_USB_PHY_SSUSB_CDR_RST_DLY, 0x1)
#define   AIROHA_USB_PHY_SSUSB_CDR_RST_DLY_128	FIELD_PREP_CONST(AIROHA_USB_PHY_SSUSB_CDR_RST_DLY, 0x2)
#define   AIROHA_USB_PHY_SSUSB_CDR_RST_DLY_216	FIELD_PREP_CONST(AIROHA_USB_PHY_SSUSB_CDR_RST_DLY, 0x3)
*/
#define AIROHA_USB_PHY_U3_PHYA_DA_REG19		0xc38
#define   AIROHA_USB_PHY_SSUSB_PLL_SSC_DELTA1_U3 GENMASK(15, 0)

#define AIROHA_USB_PHY_U2_FM_DET_CYCLE_CNT	1024
#define AIROHA_USB_PHY_REF_CK			20
#define AIROHA_USB_PHY_U2_SR_COEF		28
#define AIROHA_USB_PHY_U2_SR_COEF_DIVISOR	1000

#define AIROHA_USB_PHY_FREQDET_SLEEP		1000 /* 1ms */
#define AIROHA_USB_PHY_FREQDET_TIMEOUT		(AIROHA_USB_PHY_FREQDET_SLEEP * 10)

#define AIROHA_USB_PHY_MAX_INSTANCE		2

#define ECONET_U2PHY_COM_P0			0x800
#define ECONET_U2PHY_COM_P1			0x1000
#define ECONET_U2PHY_COM_P0_USBPHYACR0		0x810
#define ECONET_U2PHY_COM_P1_USBPHYACR0		0x1010
#define   ECONET_USB_PHY_USB20_HSTX_SRCAL_EN	BIT(23)
#define   ECONET_USB_PHY_USB20_HSTX_SRCTRL	GENMASK(18, 16)


#define ECONET_SIFSLV_U3PHYD_PHYD_LFPS1		0x90C
#define   ECONET_RG_SSUSB_FWAKE_TH		GENMASK(21, 16)
#define ECONET_SIFSLV_U3PHYD_B2_PHYD_RXDET1	0xA28
#define   ECONET_SSUSB_RXDET_STB2_SET		GENMASK(17, 9)
#define ECONET_SIFSLV_U3PHYD_B2_PHYD_RXDET2	0xA2C
#define   ECONET_SSUSB_RXDET_STB2_SET_P3	GENMASK(8, 0)

struct econet_usb_phy_instance {
	struct phy *phy;
	u32 type;
};

struct econet_usb_phy_priv {
	struct device *dev;

	struct regmap *regmap;

	unsigned int id;

	struct econet_usb_phy_instance *phys[AIROHA_USB_PHY_MAX_INSTANCE];
};

static int econet_usb_phy_u2_slew_rate_calibration(struct econet_usb_phy_priv *priv)
{
	u32 fm_out;
	u32 srctrl;

	/* Enable HS TX SR calibration */
	regmap_set_bits(priv->regmap, priv->id == 0 ? ECONET_U2PHY_COM_P0_USBPHYACR0 :ECONET_U2PHY_COM_P1_USBPHYACR0,		ECONET_USB_PHY_USB20_HSTX_SRCAL_EN);

	usleep_range(1000, 1500);

	/* Enable Free run clock */
	regmap_set_bits(priv->regmap, AIROHA_USB_PHY_FMMONR1,
			AIROHA_USB_PHY_FRCK_EN);

	/* Select Monitor Clock */
	regmap_update_bits(priv->regmap, AIROHA_USB_PHY_FMCR0,
			   AIROHA_USB_PHY_MONCLK_SEL,
			   priv->id == 0 ? AIROHA_USB_PHY_MONCLK_SEL0 :
					   AIROHA_USB_PHY_MONCLK_SEL1);

	/* Set cyclecnt */
	regmap_update_bits(priv->regmap, AIROHA_USB_PHY_FMCR0,
			   AIROHA_USB_PHY_CYCLECNT,
			   FIELD_PREP(AIROHA_USB_PHY_CYCLECNT,
				      AIROHA_USB_PHY_U2_FM_DET_CYCLE_CNT));

	/* Enable Frequency meter */
	regmap_set_bits(priv->regmap, AIROHA_USB_PHY_FMCR0,
			AIROHA_USB_PHY_FREQDET_EN);

	/* Timeout can happen and we will apply workaround at the end */
	regmap_read_poll_timeout(priv->regmap, AIROHA_USB_PHY_FMMONR0, fm_out,
				 fm_out, AIROHA_USB_PHY_FREQDET_SLEEP,
				 AIROHA_USB_PHY_FREQDET_TIMEOUT);

	/* Disable Frequency meter */
	regmap_clear_bits(priv->regmap, AIROHA_USB_PHY_FMCR0,
			  AIROHA_USB_PHY_FREQDET_EN);

	/* Disable Free run clock */
	regmap_clear_bits(priv->regmap, AIROHA_USB_PHY_FMMONR1,
			  AIROHA_USB_PHY_FRCK_EN);

	/* Disable HS TX SR calibration */
	regmap_clear_bits(priv->regmap, priv->id == 0 ? ECONET_U2PHY_COM_P0_USBPHYACR0 :ECONET_U2PHY_COM_P1_USBPHYACR0,		ECONET_USB_PHY_USB20_HSTX_SRCAL_EN);

	usleep_range(1000, 1500);

	/* Frequency was not detected, use default SR calibration value */
	if (!fm_out) {
//FIXME
		srctrl = 0x5;
		dev_err(priv->dev, "Frequency not detected, using default SR calibration.\n");
	/* (1024 / FM_OUT) * REF_CK * U2_SR_COEF (round to the nearest digits) */
	} else {
		srctrl = AIROHA_USB_PHY_REF_CK * AIROHA_USB_PHY_U2_SR_COEF;
		srctrl = (srctrl * AIROHA_USB_PHY_U2_FM_DET_CYCLE_CNT) / fm_out;
		srctrl = DIV_ROUND_CLOSEST(srctrl, AIROHA_USB_PHY_U2_SR_COEF_DIVISOR);
		dev_dbg(priv->dev, "SR calibration applied: %x\n", srctrl);
	}

 	regmap_update_bits(priv->regmap, priv->id == 0 ? ECONET_U2PHY_COM_P0_USBPHYACR0 :ECONET_U2PHY_COM_P1_USBPHYACR0,
 			   ECONET_USB_PHY_USB20_HSTX_SRCTRL,
 			   FIELD_PREP(ECONET_USB_PHY_USB20_HSTX_SRCTRL, srctrl));

	return 0;
}

static int econet_usb_phy_u2_init(struct econet_usb_phy_priv *priv)
{
	/* set SW PLL Stable mode to 1 for U2 LPM device remote wakeup */
	
#define ECONET_SIFSLV_U2PHY_U2PHYDCR1			0x864
#define   ECONET_RG_USB20_SW_PLLMODE			GENMASK(19, 18)

	regmap_update_bits(priv->regmap, ECONET_SIFSLV_U2PHY_U2PHYDCR1,
			   ECONET_RG_USB20_SW_PLLMODE,
			   FIELD_PREP(ECONET_RG_USB20_SW_PLLMODE, 0x1));

	usleep_range(1000, 1500);

	return 0;
}

static int econet_usb_phy_u3_init(struct econet_usb_phy_priv *priv)
{
	/* Patch TxDetRx Timing for E1 */
	regmap_update_bits(priv->regmap, ECONET_SIFSLV_U3PHYD_B2_PHYD_RXDET1,
			   ECONET_SSUSB_RXDET_STB2_SET, FIELD_PREP(ECONET_SSUSB_RXDET_STB2_SET, 0x10));

	regmap_update_bits(priv->regmap, ECONET_SIFSLV_U3PHYD_B2_PHYD_RXDET2,
			   ECONET_SSUSB_RXDET_STB2_SET_P3, FIELD_PREP(ECONET_SSUSB_RXDET_STB2_SET_P3, 0x10));
	
	/* Patch LFPS Filter Threshold for E1 */
	regmap_update_bits(priv->regmap, ECONET_SIFSLV_U3PHYD_PHYD_LFPS1,
			   ECONET_RG_SSUSB_FWAKE_TH, FIELD_PREP(ECONET_RG_SSUSB_FWAKE_TH, 0x34));

	//if (!(VPint(CR_AHB_HWCONF) & 0x01)) //EN7512
	//if ((VPint(CR_AHB_HWCONF) & 0x40000))
		//return 0;


	/* Setup 25MHz XTAL */
#define ECONET_SIFSLV_U3PHYA_DA_SSUSB_PLL_FBKDIV	0xC1C
#define   ECONET_RG_SSUSB_PLL_FBKDIV_PE2H		GENMASK(30, 24)
#define   ECONET_RG_SSUSB_PLL_FBKDIV_PE1D		GENMASK(22, 16)
#define   ECONET_RG_SSUSB_PLL_FBKDIV_PE1H		GENMASK(14, 8)
#define   ECONET_RG_SSUSB_PLL_FBKDIV_U3			GENMASK(6, 0)

	regmap_update_bits(priv->regmap, ECONET_SIFSLV_U3PHYA_DA_SSUSB_PLL_FBKDIV,
			   ECONET_RG_SSUSB_PLL_FBKDIV_U3, FIELD_PREP(ECONET_RG_SSUSB_PLL_FBKDIV_U3, 0x18));

	regmap_update_bits(priv->regmap, ECONET_SIFSLV_U3PHYA_DA_SSUSB_PLL_FBKDIV,
			   ECONET_RG_SSUSB_PLL_FBKDIV_PE1H, FIELD_PREP(ECONET_RG_SSUSB_PLL_FBKDIV_PE1H, 0x18));

	regmap_update_bits(priv->regmap, ECONET_SIFSLV_U3PHYA_DA_SSUSB_PLL_FBKDIV,
			   ECONET_RG_SSUSB_PLL_FBKDIV_PE2H, FIELD_PREP(ECONET_RG_SSUSB_PLL_FBKDIV_PE2H, 0x18));
			
#define ECONET_SIFSLV_U3PHYA_DA_SSUSB_PLL_U3_PCW_NCPO	0xC24
#define ECONET_SIFSLV_U3PHYA_DA_SSUSB_PLL_PE1H_PCW_NCPO	0xC28
#define ECONET_SIFSLV_U3PHYA_DA_SSUSB_PLL_PE1D_PCW_NCPO	0xC2C
#define ECONET_SIFSLV_U3PHYA_DA_SSUSB_PLL_PE2H_PCW_NCPO	0xC30

	regmap_write(priv->regmap, ECONET_SIFSLV_U3PHYA_DA_SSUSB_PLL_U3_PCW_NCPO,
			   0x18000000);
	regmap_write(priv->regmap, ECONET_SIFSLV_U3PHYA_DA_SSUSB_PLL_PE1H_PCW_NCPO,
			   0x18000000);
	regmap_write(priv->regmap, ECONET_SIFSLV_U3PHYA_DA_SSUSB_PLL_PE2H_PCW_NCPO,
			   0x18000000);

#define ECONET_SIFSLV_U3PHYA_DA_SSUSB_PLL_SSC_DELTA1	0xC38
#define   ECONET_RG_SSUSB_PLL_SSC_DELTA1_PE1H		GENMASK(31, 16)
#define   ECONET_RG_SSUSB_PLL_SSC_DELTA1_U3		GENMASK(15, 0)

	regmap_update_bits(priv->regmap, ECONET_SIFSLV_U3PHYA_DA_SSUSB_PLL_SSC_DELTA1,
			   ECONET_RG_SSUSB_PLL_SSC_DELTA1_PE1H, FIELD_PREP(ECONET_RG_SSUSB_PLL_SSC_DELTA1_PE1H, 0x4a));

	regmap_update_bits(priv->regmap, ECONET_SIFSLV_U3PHYA_DA_SSUSB_PLL_SSC_DELTA1,
			   ECONET_RG_SSUSB_PLL_SSC_DELTA1_U3, FIELD_PREP(ECONET_RG_SSUSB_PLL_SSC_DELTA1_U3, 0x4a));
			
#define ECONET_SIFSLV_U3PHYA_DA_SSUSB_PLL_SSC_DELTA1_2	0xC3C
#define   ECONET_RG_SSUSB_PLL_SSC_DELTA1_PE2H		GENMASK(31, 16)
#define   ECONET_RG_SSUSB_PLL_SSC_DELTA1_PE1D		GENMASK(15, 0)

	regmap_update_bits(priv->regmap, ECONET_SIFSLV_U3PHYA_DA_SSUSB_PLL_SSC_DELTA1_2,
			   ECONET_RG_SSUSB_PLL_SSC_DELTA1_PE2H, FIELD_PREP(ECONET_RG_SSUSB_PLL_SSC_DELTA1_PE2H, 0x4a));
			
#define ECONET_SIFSLV_U3PHYA_DA_SSUSB_PLL_SSC_DELTA1_3	0xC40
#define   ECONET_RG_SSUSB_PLL_SSC_DELTA_U3		GENMASK(31, 16)
#define   ECONET_RG_SSUSB_PLL_SSC_DELTA1_PE2D		GENMASK(15, 0)

	regmap_update_bits(priv->regmap, ECONET_SIFSLV_U3PHYA_DA_SSUSB_PLL_SSC_DELTA1_3,
			   ECONET_RG_SSUSB_PLL_SSC_DELTA_U3, FIELD_PREP(ECONET_RG_SSUSB_PLL_SSC_DELTA_U3, 0x48));

			
#define ECONET_SIFSLV_U3PHYA_DA_SSUSB_PLL_SSC_DELTA1_4	0xC44
#define   ECONET_RG_SSUSB_PLL_SSC_DELTA_PE1D		GENMASK(31, 16)
#define   ECONET_RG_SSUSB_PLL_SSC_DELTA_PE1H		GENMASK(15, 0)

	regmap_update_bits(priv->regmap, ECONET_SIFSLV_U3PHYA_DA_SSUSB_PLL_SSC_DELTA1_4,
			   ECONET_RG_SSUSB_PLL_SSC_DELTA_PE1H, FIELD_PREP(ECONET_RG_SSUSB_PLL_SSC_DELTA_PE1H, 0x48));

#define ECONET_SIFSLV_U3PHYA_DA_SSUSB_PLL_SSC_DELTA1_5	0xC48
#define   ECONET_RG_SSUSB_PLL_SSC_DELTA_PE2D		GENMASK(31, 16)
#define   ECONET_RG_SSUSB_PLL_SSC_DELTA_PE2H		GENMASK(15, 0)

	regmap_update_bits(priv->regmap, ECONET_SIFSLV_U3PHYA_DA_SSUSB_PLL_SSC_DELTA1_5,
			   ECONET_RG_SSUSB_PLL_SSC_DELTA_PE2H, FIELD_PREP(ECONET_RG_SSUSB_PLL_SSC_DELTA_PE2H, 0x48));

#define ECONET_SIFSLV_U3PHYA_SSUSB_PLL_DDS		0xB24
#define   ECONET_RG_SSUSB_PLL_DDS_DMY			GENMASK(31, 16)
#define   ECONET_RG_SSUSB_PLL_SSC_PRD			GENMASK(15, 0)

	regmap_update_bits(priv->regmap, ECONET_SIFSLV_U3PHYA_SSUSB_PLL_DDS,
			   ECONET_RG_SSUSB_PLL_SSC_PRD, FIELD_PREP(ECONET_RG_SSUSB_PLL_SSC_PRD, 0x190));
			
#define ECONET_SIFSLV_U3PHYA_SSUSB_SYSPLL_DDS_NCPO_PCW	0xB10

	regmap_write(priv->regmap, ECONET_SIFSLV_U3PHYA_SSUSB_SYSPLL_DDS_NCPO_PCW,
			   0x1c000000);

#define ECONET_SIFSLV_U3PHYA_SSUSB_SYSPLL		0xB08
#define   ECONET_RG_SSUSB_SYSPLL_LF			BIT(31)
#define   ECONET_RG_SSUSB_SYSPLL_FBDIV			GENMASK(30, 24)
#define   ECONET_RG_SSUSB_SYSPLL_POSDIV			GENMASK(23, 22)
#define   ECONET_RG_SSUSB_SYSPLL_VCO_DIV_SEL		BIT(21)
#define   ECONET_RG_SSUSB_SYSPLL_BLP			BIT(20)
#define   ECONET_RG_SSUSB_SYSPLL_BP			BIT(19)
#define   ECONET_RG_SSUSB_SYSPLL_BR			BIT(18)
#define   ECONET_RG_SSUSB_SYSPLL_BC			BIT(17)
#define   ECONET_RG_SSUSB_SYSPLL_DIVEN			GENMASK(16, 14)
#define   ECONET_RG_SSUSB_SYSPLL_FPEN			BIT(13)
#define   ECONET_RG_SSUSB_SYSPLL_MONCK_EN		BIT(12)
#define   ECONET_RG_SSUSB_SYSPLL_MONVC_EN		BIT(11)
#define   ECONET_RG_SSUSB_SYSPLL_MONREF_EN		BIT(10)
#define   ECONET_RG_SSUSB_SYSPLL_VOD_EN			BIT(9)
#define   ECONET_RG_SSUSB_SYSPLL_CK_SEL			BIT(8)

	regmap_update_bits(priv->regmap, ECONET_SIFSLV_U3PHYA_SSUSB_SYSPLL,
			   ECONET_RG_SSUSB_SYSPLL_FBDIV, FIELD_PREP(ECONET_RG_SSUSB_SYSPLL_FBDIV, 0xe));
	return 0;
}

static int econet_usb_phy_init(struct phy *phy)
{
	struct econet_usb_phy_instance *instance = phy_get_drvdata(phy);
	struct econet_usb_phy_priv *priv = dev_get_drvdata(phy->dev.parent);

	if (instance->type == PHY_TYPE_USB2)
		return econet_usb_phy_u2_init(priv);

	econet_usb_phy_u3_init(priv);
	return econet_usb_phy_u2_slew_rate_calibration(priv);
}

static int econet_usb_phy_exit(struct phy *phy)
{
	return 0;
}


static struct phy *econet_usb_phy_xlate(struct device *dev,
					const struct of_phandle_args *args)
{
	struct econet_usb_phy_priv *priv = dev_get_drvdata(dev);
	struct econet_usb_phy_instance *instance = NULL;
	struct device_node *phy_np = args->np;
	int index;

	if (args->args_count != 1) {
		dev_err(dev, "invalid number of cells in 'phy' property\n");
		return ERR_PTR(-EINVAL);
	}

	for (index = 0; index < AIROHA_USB_PHY_MAX_INSTANCE; index++)
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

static const struct phy_ops econet_phy = {
	.init		= econet_usb_phy_init,
 	.exit		= econet_usb_phy_exit,
// 	.power_on	= econet_usb_phy_power_on,
// 	.power_off	= econet_usb_phy_power_off,
// 	.set_mode	= econet_usb_phy_set_mode,
	.owner		= THIS_MODULE,
};

static const struct regmap_config econet_usb_phy_regmap_config = {
	.reg_bits = 32,
	.val_bits = 32,
	.reg_stride = 4,
};

static int econet_usb_phy_probe(struct platform_device *pdev)
{
	struct phy_provider *phy_provider;
	struct econet_usb_phy_priv *priv;
	struct device *dev = &pdev->dev;
	struct device_node *child_np;
	void *base;
	int port;
	int ret;

	dev_err(dev, "Econet USB phy probe\n");
	
	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->dev = dev;

	ret = of_property_read_u32(dev->of_node, "econet,port-id", &priv->id);
	if (ret)
		return dev_err_probe(dev, ret, "port ID is mandatory for USB PHY calibration.\n");

	if (priv->id > 1)
		return dev_err_probe(dev, -EINVAL, "only 2 USB port are supported on the SoC.\n");

	base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(base))
		return PTR_ERR(base);

	priv->regmap = devm_regmap_init_mmio(dev, base, &econet_usb_phy_regmap_config);
	if (IS_ERR(priv->regmap))
		return PTR_ERR(priv->regmap);

	platform_set_drvdata(pdev, priv);

	port = 0;
	for_each_child_of_node(dev->of_node, child_np) {
		struct econet_usb_phy_instance  *instance;

		instance = devm_kzalloc(dev, sizeof(*instance), GFP_KERNEL);
		if (!instance) {
			ret = -ENOMEM;
			goto put_child;
		}

		priv->phys[port] = instance;

		instance->phy = devm_phy_create(dev, child_np, &econet_phy);
		if (IS_ERR(instance->phy)) {
			dev_err_probe(dev, PTR_ERR(instance->phy), "failed to create phy\n");
			ret = PTR_ERR(instance->phy);
			goto put_child;
		}

		phy_set_drvdata(instance->phy, instance);

		port++;
	}

	phy_provider = devm_of_phy_provider_register(&pdev->dev, econet_usb_phy_xlate);

	return PTR_ERR_OR_ZERO(phy_provider);

put_child:
	of_node_put(child_np);
	return ret;
}

static const struct of_device_id econet_phy_id_table[] = {
	{ .compatible = "econet,en751221-usb-phy" },
	{ },
};
MODULE_DEVICE_TABLE(of, econet_phy_id_table);

static struct platform_driver econet_usb_driver = {
	.probe		= econet_usb_phy_probe,
	.driver		= {
		.name	= "econet-usb-phy",
		.of_match_table = econet_phy_id_table,
	},
};

module_platform_driver(econet_usb_driver);

MODULE_AUTHOR("Christian Marangi <ansuelsmth@gmail.com>");
MODULE_AUTHOR("Benjamin Larsson <benjamin.larsson@genexis.eu>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Econet USB PHY driver");
