// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2024 AIROHA Inc
 * Author: Lorenzo Bianconi <lorenzo@kernel.org>
 */

#ifndef _PHY_AIROHA_PCIE_H
#define _PHY_AIROHA_PCIE_H

/* CSR_2L */
#define REG_CSR_2L_CMN				0x0000
#define CSR_2L_PXP_CMN_LANE_EN			BIT(0)
#define CSR_2L_PXP_CMN_TRIM_MASK		GENMASK(28, 24)

#define REG_CSR_2L_JCPLL_IB_EXT			0x0004
#define REG_CSR_2L_JCPLL_LPF_SHCK_EN		BIT(8)
#define CSR_2L_PXP_JCPLL_CHP_IBIAS		GENMASK(21, 16)
#define CSR_2L_PXP_JCPLL_CHP_IOFST		GENMASK(29, 24)

#define REG_CSR_2L_JCPLL_LPF_BR			0x0008
#define CSR_2L_PXP_JCPLL_LPF_BR			GENMASK(4, 0)
#define CSR_2L_PXP_JCPLL_LPF_BC			GENMASK(12, 8)
#define CSR_2L_PXP_JCPLL_LPF_BP			GENMASK(20, 16)
#define CSR_2L_PXP_JCPLL_LPF_BWR		GENMASK(28, 24)

#define REG_CSR_2L_JCPLL_LPF_BWC		0x000c
#define CSR_2L_PXP_JCPLL_LPF_BWC		GENMASK(4, 0)
#define CSR_2L_PXP_JCPLL_KBAND_CODE		GENMASK(23, 16)
#define CSR_2L_PXP_JCPLL_KBAND_DIV		GENMASK(26, 24)

#define REG_CSR_2L_JCPLL_KBAND_KFC		0x0010
#define CSR_2L_PXP_JCPLL_KBAND_KFC		GENMASK(1, 0)
#define CSR_2L_PXP_JCPLL_KBAND_KF		GENMASK(9, 8)
#define CSR_2L_PXP_JCPLL_KBAND_KS		GENMASK(17, 16)
#define CSR_2L_PXP_JCPLL_POSTDIV_EN		BIT(24)

#define REG_CSR_2L_JCPLL_MMD_PREDIV_MODE	0x0014
#define CSR_2L_PXP_JCPLL_MMD_PREDIV_MODE	GENMASK(1, 0)
#define CSR_2L_PXP_JCPLL_POSTDIV_D2		BIT(16)
#define CSR_2L_PXP_JCPLL_POSTDIV_D5		BIT(24)

#define CSR_2L_PXP_JCPLL_MONCK			0x0018
#define CSR_2L_PXP_JCPLL_REFIN_DIV		GENMASK(25, 24)

#define REG_CSR_2L_JCPLL_RST_DLY		0x001c
#define CSR_2L_PXP_JCPLL_RST_DLY		GENMASK(2, 0)
#define CSR_2L_PXP_JCPLL_RST			BIT(8)
#define CSR_2L_PXP_JCPLL_SDM_DI_EN		BIT(16)
#define CSR_2L_PXP_JCPLL_SDM_DI_LS		GENMASK(25, 24)

#define REG_CSR_2L_JCPLL_SDM_IFM		0x0020
#define CSR_2L_PXP_JCPLL_SDM_IFM		BIT(0)

#define REG_CSR_2L_JCPLL_SDM_HREN		0x0024
#define CSR_2L_PXP_JCPLL_SDM_HREN		BIT(0)
#define CSR_2L_PXP_JCPLL_TCL_AMP_EN		BIT(8)
#define CSR_2L_PXP_JCPLL_TCL_AMP_GAIN		GENMASK(18, 16)
#define CSR_2L_PXP_JCPLL_TCL_AMP_VREF		GENMASK(28, 24)

#define REG_CSR_2L_JCPLL_TCL_CMP		0x0028
#define CSR_2L_PXP_JCPLL_TCL_LPF_EN		BIT(16)
#define CSR_2L_PXP_JCPLL_TCL_LPF_BW		GENMASK(26, 24)

#define REG_CSR_2L_JCPLL_VCODIV			0x002c
#define CSR_2L_PXP_JCPLL_VCO_CFIX		GENMASK(9, 8)
#define CSR_2L_PXP_JCPLL_VCO_HALFLSB_EN		BIT(16)
#define CSR_2L_PXP_JCPLL_VCO_SCAPWR		GENMASK(26, 24)

#define REG_CSR_2L_JCPLL_VCO_TCLVAR		0x0030
#define CSR_2L_PXP_JCPLL_VCO_TCLVAR		GENMASK(2, 0)

#define REG_CSR_2L_JCPLL_SSC				0x0038
#define CSR_2L_PXP_JCPLL_SSC_EN			BIT(0)
#define CSR_2L_PXP_JCPLL_SSC_PHASE_INI		BIT(8)
#define CSR_2L_PXP_JCPLL_SSC_TRI_EN		BIT(16)

#define REG_CSR_2L_JCPLL_SSC_DELTA1		0x003c
#define CSR_2L_PXP_JCPLL_SSC_DELTA1		GENMASK(15, 0)
#define CSR_2L_PXP_JCPLL_SSC_DELTA		GENMASK(31, 16)

#define REG_CSR_2L_JCPLL_SSC_PERIOD		0x0040
#define CSR_2L_PXP_JCPLL_SSC_PERIOD		GENMASK(15, 0)

#define REG_CSR_2L_JCPLL_TCL_VTP_EN		0x004c
#define CSR_2L_PXP_JCPLL_SPARE_LOW		GENMASK(31, 24)

#define REG_CSR_2L_JCPLL_TCL_KBAND_VREF		0x0050
#define CSR_2L_PXP_JCPLL_TCL_KBAND_VREF		GENMASK(4, 0)
#define CSR_2L_PXP_JCPLL_VCO_KBAND_MEAS_EN	BIT(24)

#define REG_CSR_2L_750M_SYS_CK			0x0054
#define CSR_2L_PXP_TXPLL_LPF_SHCK_EN		BIT(16)
#define CSR_2L_PXP_TXPLL_CHP_IBIAS		GENMASK(29, 24)

#define REG_CSR_2L_TXPLL_CHP_IOFST		0x0058
#define CSR_2L_PXP_TXPLL_CHP_IOFST		GENMASK(5, 0)
#define CSR_2L_PXP_TXPLL_LPF_BR			GENMASK(12, 8)
#define CSR_2L_PXP_TXPLL_LPF_BC			GENMASK(20, 16)
#define CSR_2L_PXP_TXPLL_LPF_BP			GENMASK(28, 24)

#define REG_CSR_2L_TXPLL_LPF_BWR		0x005c
#define CSR_2L_PXP_TXPLL_LPF_BWR		GENMASK(4, 0)
#define CSR_2L_PXP_TXPLL_LPF_BWC		GENMASK(12, 8)
#define CSR_2L_PXP_TXPLL_KBAND_CODE		GENMASK(31, 24)

#define REG_CSR_2L_TXPLL_KBAND_DIV		0x0060
#define CSR_2L_PXP_TXPLL_KBAND_DIV		GENMASK(2, 0)
#define CSR_2L_PXP_TXPLL_KBAND_KFC		GENMASK(9, 8)
#define CSR_2L_PXP_TXPLL_KBAND_KF		GENMASK(17, 16)
#define CSR_2L_PXP_txpll_KBAND_KS		GENMASK(25, 24)

#define REG_CSR_2L_TXPLL_POSTDIV		0x0064
#define CSR_2L_PXP_TXPLL_POSTDIV_EN		BIT(0)
#define CSR_2L_PXP_TXPLL_MMD_PREDIV_MODE	GENMASK(9, 8)
#define CSR_2L_PXP_TXPLL_PHY_CK1_EN		BIT(24)

#define REG_CSR_2L_TXPLL_PHY_CK2		0x0068
#define CSR_2L_PXP_TXPLL_REFIN_INTERNAL		BIT(24)

#define REG_CSR_2L_TXPLL_REFIN_DIV		0x006c
#define CSR_2L_PXP_TXPLL_REFIN_DIV		GENMASK(1, 0)
#define CSR_2L_PXP_TXPLL_RST_DLY		GENMASK(10, 8)
#define CSR_2L_PXP_TXPLL_PLL_RSTB		BIT(16)

#define REG_CSR_2L_TXPLL_SDM_DI_LS		0x0070
#define CSR_2L_PXP_TXPLL_SDM_DI_LS		GENMASK(1, 0)
#define CSR_2L_PXP_TXPLL_SDM_IFM		BIT(8)
#define CSR_2L_PXP_TXPLL_SDM_ORD		GENMASK(25, 24)

#define REG_CSR_2L_TXPLL_SDM_OUT		0x0074
#define CSR_2L_PXP_TXPLL_TCL_AMP_EN		BIT(16)
#define CSR_2L_PXP_TXPLL_TCL_AMP_GAIN		GENMASK(26, 24)

#define REG_CSR_2L_TXPLL_TCL_AMP_VREF		0x0078
#define CSR_2L_PXP_TXPLL_TCL_AMP_VREF		GENMASK(4, 0)
#define CSR_2L_PXP_TXPLL_TCL_LPF_EN		BIT(24)

#define REG_CSR_2L_TXPLL_TCL_LPF_BW		0x007c
#define CSR_2L_PXP_TXPLL_TCL_LPF_BW		GENMASK(2, 0)
#define CSR_2L_PXP_TXPLL_VCO_CFIX		GENMASK(17, 16)
#define CSR_2L_PXP_TXPLL_VCO_HALFLSB_EN		BIT(24)

#define REG_CSR_2L_TXPLL_VCO_SCAPWR		0x0080
#define CSR_2L_PXP_TXPLL_VCO_SCAPWR		GENMASK(2, 0)

#define REG_CSR_2L_TXPLL_SSC			0x0084
#define CSR_2L_PXP_TXPLL_SSC_EN			BIT(0)
#define CSR_2L_PXP_TXPLL_SSC_PHASE_INI		BIT(8)

#define REG_CSR_2L_TXPLL_SSC_DELTA1		0x0088
#define CSR_2L_PXP_TXPLL_SSC_DELTA1		GENMASK(15, 0)
#define CSR_2L_PXP_TXPLL_SSC_DELTA		GENMASK(31, 16)

#define REG_CSR_2L_TXPLL_SSC_PERIOD		0x008c
#define CSR_2L_PXP_txpll_SSC_PERIOD		GENMASK(15, 0)

#define REG_CSR_2L_TXPLL_VTP			0x0090
#define CSR_2L_PXP_TXPLL_VTP_EN			BIT(0)

#define REG_CSR_2L_TXPLL_TCL_VTP		0x0098
#define CSR_2L_PXP_TXPLL_SPARE_L		GENMASK(31, 24)

#define REG_CSR_2L_TXPLL_TCL_KBAND_VREF		0x009c
#define CSR_2L_PXP_TXPLL_TCL_KBAND_VREF		GENMASK(4, 0)
#define CSR_2L_PXP_TXPLL_VCO_KBAND_MEAS_EN	BIT(24)

#define REG_CSR_2L_TXPLL_POSTDIV_D256		0x00a0
#define CSR_2L_PXP_CLKTX0_AMP			GENMASK(10, 8)
#define CSR_2L_PXP_CLKTX0_OFFSET		GENMASK(17, 16)
#define CSR_2L_PXP_CLKTX0_SR			GENMASK(25, 24)

#define REG_CSR_2L_CLKTX0_FORCE_OUT1		0x00a4
#define CSR_2L_PXP_CLKTX0_HZ			BIT(8)
#define CSR_2L_PXP_CLKTX0_IMP_SEL		GENMASK(20, 16)
#define CSR_2L_PXP_CLKTX1_AMP			GENMASK(26, 24)

#define REG_CSR_2L_CLKTX1_OFFSET		0x00a8
#define CSR_2L_PXP_CLKTX1_OFFSET		GENMASK(1, 0)
#define CSR_2L_PXP_CLKTX1_SR			GENMASK(9, 8)
#define CSR_2L_PXP_CLKTX1_HZ			BIT(24)

#define REG_CSR_2L_CLKTX1_IMP_SEL		0x00ac
#define CSR_2L_PXP_CLKTX1_IMP_SEL		GENMASK(4, 0)

#define REG_CSR_2L_PLL_CMN_RESERVE0		0x00b0
#define CSR_2L_PXP_PLL_RESERVE_MASK		GENMASK(15, 0)

#define REG_CSR_2L_TX0_CKLDO			0x00cc
#define CSR_2L_PXP_TX0_CKLDO_EN			BIT(0)
#define CSR_2L_PXP_TX0_DMEDGEGEN_EN		BIT(24)

#define REG_CSR_2L_TX1_CKLDO			0x00e8
#define CSR_2L_PXP_TX1_CKLDO_EN			BIT(0)
#define CSR_2L_PXP_TX1_DMEDGEGEN_EN		BIT(24)

#define REG_CSR_2L_TX1_MULTLANE			0x00ec
#define CSR_2L_PXP_TX1_MULTLANE_EN		BIT(0)

#define REG_CSR_2L_RX0_REV0			0x00fc
#define CSR_2L_PXP_VOS_PNINV			GENMASK(3, 2)
#define CSR_2L_PXP_FE_GAIN_NORMAL_MODE		GENMASK(6, 4)
#define CSR_2L_PXP_FE_GAIN_TRAIN_MODE		GENMASK(10, 8)

#define REG_CSR_2L_RX0_PHYCK_DIV		0x0100
#define CSR_2L_PXP_RX0_PHYCK_SEL		GENMASK(9, 8)
#define CSR_2L_PXP_RX0_PHYCK_RSTB		BIT(16)
#define CSR_2L_PXP_RX0_TDC_CK_SEL		BIT(24)

#define REG_CSR_2L_CDR0_PD_PICAL_CKD8_INV	0x0104
#define CSR_2L_PXP_CDR0_PD_EDGE_DISABLE		BIT(8)

#define REG_CSR_2L_CDR0_LPF_RATIO		0x0110
#define CSR_2L_PXP_CDR0_LPF_TOP_LIM		GENMASK(26, 8)

#define REG_CSR_2L_CDR0_PR_INJ_MODE		0x011c
#define CSR_2L_PXP_CDR0_INJ_FORCE_OFF		BIT(24)

#define REG_CSR_2L_CDR0_PR_BETA_DAC		0x0120
#define CSR_2L_PXP_CDR0_PR_BETA_SEL		GENMASK(19, 16)
#define CSR_2L_PXP_CDR0_PR_KBAND_DIV		GENMASK(26, 24)

#define REG_CSR_2L_CDR0_PR_VREG_IBAND		0x0124
#define CSR_2L_PXP_CDR0_PR_VREG_IBAND		GENMASK(2, 0)
#define CSR_2L_PXP_CDR0_PR_VREG_CKBUF		GENMASK(10, 8)

#define REG_CSR_2L_CDR0_PR_CKREF_DIV		0x0128
#define CSR_2L_PXP_CDR0_PR_CKREF_DIV		GENMASK(1, 0)

#define REG_CSR_2L_CDR0_PR_MONCK		0x012c
#define CSR_2L_PXP_CDR0_PR_MONCK_ENABLE		BIT(0)
#define CSR_2L_PXP_CDR0_PR_RESERVE0		GENMASK(19, 16)

#define REG_CSR_2L_CDR0_PR_COR_HBW		0x0130
#define CSR_2L_PXP_CDR0_PR_LDO_FORCE_ON		BIT(8)
#define CSR_2L_PXP_CDR0_PR_CKREF_DIV1		GENMASK(17, 16)

#define REG_CSR_2L_CDR0_PR_MONPI		0x0134
#define CSR_2L_PXP_CDR0_PR_XFICK_EN		BIT(8)

#define REG_CSR_2L_RX0_SIGDET_DCTEST		0x0140
#define CSR_2L_PXP_RX0_SIGDET_LPF_CTRL		GENMASK(9, 8)
#define CSR_2L_PXP_RX0_SIGDET_PEAK		GENMASK(25, 24)

#define REG_CSR_2L_RX0_SIGDET_VTH_SEL		0x0144
#define CSR_2L_PXP_RX0_SIGDET_VTH_SEL		GENMASK(4, 0)
#define CSR_2L_PXP_RX0_FE_VB_EQ1_EN		BIT(24)

#define REG_CSR_2L_PXP_RX0_FE_VB_EQ2		0x0148
#define CSR_2L_PXP_RX0_FE_VB_EQ2_EN		BIT(0)
#define CSR_2L_PXP_RX0_FE_VB_EQ3_EN		BIT(8)
#define CSR_2L_PXP_RX0_FE_VCM_GEN_PWDB		BIT(16)

#define REG_CSR_2L_PXP_RX0_OSCAL_CTLE1IOS	0x0158
#define CSR_2L_PXP_RX0_PR_OSCAL_VGA1IOS		GENMASK(29, 24)

#define REG_CSR_2L_PXP_RX0_OSCA_VGA1VOS		0x015c
#define CSR_2L_PXP_RX0_PR_OSCAL_VGA1VOS		GENMASK(5, 0)
#define CSR_2L_PXP_RX0_PR_OSCAL_VGA2IOS		GENMASK(13, 8)

#define REG_CSR_2L_RX1_REV0			0x01b4

#define REG_CSR_2L_RX1_PHYCK_DIV		0x01b8
#define CSR_2L_PXP_RX1_PHYCK_SEL		GENMASK(9, 8)
#define CSR_2L_PXP_RX1_PHYCK_RSTB		BIT(16)
#define CSR_2L_PXP_RX1_TDC_CK_SEL		BIT(24)

#define REG_CSR_2L_CDR1_PD_PICAL_CKD8_INV	0x01bc
#define CSR_2L_PXP_CDR1_PD_EDGE_DISABLE		BIT(8)

#define REG_CSR_2L_CDR1_PR_BETA_DAC		0x01d8
#define CSR_2L_PXP_CDR1_PR_BETA_SEL		GENMASK(19, 16)
#define CSR_2L_PXP_CDR1_PR_KBAND_DIV		GENMASK(26, 24)

#define REG_CSR_2L_CDR1_PR_MONCK		0x01e4
#define CSR_2L_PXP_CDR1_PR_MONCK_ENABLE		BIT(0)
#define CSR_2L_PXP_CDR1_PR_RESERVE0		GENMASK(19, 16)

#define REG_CSR_2L_CDR1_LPF_RATIO		0x01c8
#define CSR_2L_PXP_CDR1_LPF_TOP_LIM		GENMASK(26, 8)

#define REG_CSR_2L_CDR1_PR_INJ_MODE		0x01d4
#define CSR_2L_PXP_CDR1_INJ_FORCE_OFF		BIT(24)

#define REG_CSR_2L_CDR1_PR_VREG_IBAND_VAL	0x01dc
#define CSR_2L_PXP_CDR1_PR_VREG_IBAND		GENMASK(2, 0)
#define CSR_2L_PXP_CDR1_PR_VREG_CKBUF		GENMASK(10, 8)

#define REG_CSR_2L_CDR1_PR_CKREF_DIV		0x01e0
#define CSR_2L_PXP_CDR1_PR_CKREF_DIV		GENMASK(1, 0)

#define REG_CSR_2L_CDR1_PR_COR_HBW		0x01e8
#define CSR_2L_PXP_CDR1_PR_LDO_FORCE_ON		BIT(8)
#define CSR_2L_PXP_CDR1_PR_CKREF_DIV1		GENMASK(17, 16)

#define REG_CSR_2L_CDR1_PR_MONPI		0x01ec
#define CSR_2L_PXP_CDR1_PR_XFICK_EN		BIT(8)

#define REG_CSR_2L_RX1_DAC_RANGE_EYE		0x01f4
#define CSR_2L_PXP_RX1_SIGDET_LPF_CTRL		GENMASK(25, 24)

#define REG_CSR_2L_RX1_SIGDET_NOVTH		0x01f8
#define CSR_2L_PXP_RX1_SIGDET_PEAK		GENMASK(9, 8)
#define CSR_2L_PXP_RX1_SIGDET_VTH_SEL		GENMASK(20, 16)

#define REG_CSR_2L_RX1_FE_VB_EQ1		0x0200
#define CSR_2L_PXP_RX1_FE_VB_EQ1_EN		BIT(0)
#define CSR_2L_PXP_RX1_FE_VB_EQ2_EN		BIT(8)
#define CSR_2L_PXP_RX1_FE_VB_EQ3_EN		BIT(16)
#define CSR_2L_PXP_RX1_FE_VCM_GEN_PWDB		BIT(24)

#define REG_CSR_2L_RX1_OSCAL_VGA1IOS		0x0214
#define CSR_2L_PXP_RX1_PR_OSCAL_VGA1IOS		GENMASK(5, 0)
#define CSR_2L_PXP_RX1_PR_OSCAL_VGA1VOS		GENMASK(13, 8)
#define CSR_2L_PXP_RX1_PR_OSCAL_VGA2IOS		GENMASK(21, 16)

/* PMA */
#define REG_PCIE_PMA_SS_LCPLL_PWCTL_SETTING_1	0x0004
#define PCIE_LCPLL_MAN_PWDB			BIT(0)

#define REG_PCIE_PMA_SEQUENCE_DISB_CTRL1	0x010c
#define PCIE_DISB_RX_SDCAL_EN			BIT(0)

#define REG_PCIE_PMA_CTRL_SEQUENCE_FORCE_CTRL1	0x0114
#define PCIE_FORCE_RX_SDCAL_EN			BIT(0)

#define REG_PCIE_PMA_SS_RX_FREQ_DET1		0x014c
#define PCIE_PLL_FT_LOCK_CYCLECNT		GENMASK(15, 0)
#define PCIE_PLL_FT_UNLOCK_CYCLECNT		GENMASK(31, 16)

#define REG_PCIE_PMA_SS_RX_FREQ_DET2		0x0150
#define PCIE_LOCK_TARGET_BEG			GENMASK(15, 0)
#define PCIE_LOCK_TARGET_END			GENMASK(31, 16)

#define REG_PCIE_PMA_SS_RX_FREQ_DET3		0x0154
#define PCIE_UNLOCK_TARGET_BEG			GENMASK(15, 0)
#define PCIE_UNLOCK_TARGET_END			GENMASK(31, 16)

#define REG_PCIE_PMA_SS_RX_FREQ_DET4		0x0158
#define PCIE_FREQLOCK_DET_EN			GENMASK(2, 0)
#define PCIE_LOCK_LOCKTH			GENMASK(11, 8)
#define PCIE_UNLOCK_LOCKTH			GENMASK(15, 12)

#define REG_PCIE_PMA_SS_RX_CAL1			0x0160
#define REG_PCIE_PMA_SS_RX_CAL2			0x0164
#define PCIE_CAL_OUT_OS				GENMASK(11, 8)

#define REG_PCIE_PMA_SS_RX_SIGDET0		0x0168
#define PCIE_SIGDET_WIN_NONVLD_TIMES		GENMASK(28, 24)

#define REG_PCIE_PMA_TX_RESET			0x0260
#define PCIE_TX_TOP_RST				BIT(0)
#define PCIE_TX_CAL_RST				BIT(8)

#define REG_PCIE_PMA_RX_FORCE_MODE0		0x0294
#define PCIE_FORCE_DA_XPON_RX_FE_GAIN_CTRL	GENMASK(1, 0)

#define REG_PCIE_PMA_SS_DA_XPON_PWDB0		0x034c
#define PCIE_DA_XPON_CDR_PR_PWDB		BIT(8)

#define REG_PCIE_PMA_SW_RESET			0x0460
#define PCIE_SW_RX_FIFO_RST			BIT(0)
#define PCIE_SW_RX_RST				BIT(1)
#define PCIE_SW_TX_RST				BIT(2)
#define PCIE_SW_PMA_RST				BIT(3)
#define PCIE_SW_ALLPCS_RST			BIT(4)
#define PCIE_SW_REF_RST				BIT(5)
#define PCIE_SW_TX_FIFO_RST			BIT(6)
#define PCIE_SW_XFI_TXPCS_RST			BIT(7)
#define PCIE_SW_XFI_RXPCS_RST			BIT(8)
#define PCIE_SW_XFI_RXPCS_BIST_RST		BIT(9)
#define PCIE_SW_HSG_TXPCS_RST			BIT(10)
#define PCIE_SW_HSG_RXPCS_RST			BIT(11)
#define PCIE_PMA_SW_RST				(PCIE_SW_RX_FIFO_RST | \
						 PCIE_SW_RX_RST | \
						 PCIE_SW_TX_RST | \
						 PCIE_SW_PMA_RST | \
						 PCIE_SW_ALLPCS_RST | \
						 PCIE_SW_REF_RST | \
						 PCIE_SW_TX_FIFO_RST | \
						 PCIE_SW_XFI_TXPCS_RST | \
						 PCIE_SW_XFI_RXPCS_RST | \
						 PCIE_SW_XFI_RXPCS_BIST_RST | \
						 PCIE_SW_HSG_TXPCS_RST | \
						 PCIE_SW_HSG_RXPCS_RST)

#define REG_PCIE_PMA_RO_RX_FREQDET		0x0530
#define PCIE_RO_FBCK_LOCK			BIT(0)
#define PCIE_RO_FL_OUT				GENMASK(31, 16)

#define REG_PCIE_PMA_FORCE_DA_PXP_CDR_PR_IDAC	0x0794
#define PCIE_FORCE_DA_PXP_CDR_PR_IDAC		GENMASK(10, 0)
#define PCIE_FORCE_SEL_DA_PXP_CDR_PR_IDAC	BIT(16)
#define PCIE_FORCE_SEL_DA_PXP_TXPLL_SDM_PCW	BIT(24)

#define REG_PCIE_PMA_FORCE_DA_PXP_TXPLL_SDM_PCW	0x0798
#define PCIE_FORCE_DA_PXP_TXPLL_SDM_PCW		GENMASK(30, 0)

#define REG_PCIE_PMA_FORCE_DA_PXP_RX_FE_VOS	0x079c
#define PCIE_FORCE_SEL_DA_PXP_JCPLL_SDM_PCW	BIT(16)

#define REG_PCIE_PMA_FORCE_DA_PXP_JCPLL_SDM_PCW	0x0800
#define PCIE_FORCE_DA_PXP_JCPLL_SDM_PCW		GENMASK(30, 0)

#define REG_PCIE_PMA_FORCE_DA_PXP_CDR_PD_PWDB	0x081c
#define PCIE_FORCE_DA_PXP_CDR_PD_PWDB		BIT(0)
#define PCIE_FORCE_SEL_DA_PXP_CDR_PD_PWDB	BIT(8)

#define REG_PCIE_PMA_FORCE_DA_PXP_CDR_PR_LPF_C	0x0820
#define PCIE_FORCE_DA_PXP_CDR_PR_LPF_C_EN	BIT(0)
#define PCIE_FORCE_SEL_DA_PXP_CDR_PR_LPF_C_EN	BIT(8)
#define PCIE_FORCE_DA_PXP_CDR_PR_LPF_R_EN	BIT(16)
#define PCIE_FORCE_SEL_DA_PXP_CDR_PR_LPF_R_EN	BIT(24)

#define REG_PCIE_PMA_FORCE_DA_PXP_CDR_PR_PIEYE_PWDB	0x0824
#define PCIE_FORCE_DA_PXP_CDR_PR_PWDB			BIT(16)
#define PCIE_FORCE_SEL_DA_PXP_CDR_PR_PWDB		BIT(24)

#define REG_PCIE_PMA_FORCE_PXP_JCPLL_CKOUT	0x0828
#define PCIE_FORCE_DA_PXP_JCPLL_CKOUT_EN	BIT(0)
#define PCIE_FORCE_SEL_DA_PXP_JCPLL_CKOUT_EN	BIT(8)
#define PCIE_FORCE_DA_PXP_JCPLL_EN		BIT(16)
#define PCIE_FORCE_SEL_DA_PXP_JCPLL_EN		BIT(24)

#define REG_PCIE_PMA_FORCE_DA_PXP_RX_SCAN_RST	0x0084c
#define PCIE_FORCE_DA_PXP_RX_SIGDET_PWDB	BIT(16)
#define PCIE_FORCE_SEL_DA_PXP_RX_SIGDET_PWDB	BIT(24)

#define REG_PCIE_PMA_FORCE_DA_PXP_TXPLL_CKOUT	0x0854
#define PCIE_FORCE_DA_PXP_TXPLL_CKOUT_EN	BIT(0)
#define PCIE_FORCE_SEL_DA_PXP_TXPLL_CKOUT_EN	BIT(8)
#define PCIE_FORCE_DA_PXP_TXPLL_EN		BIT(16)
#define PCIE_FORCE_SEL_DA_PXP_TXPLL_EN		BIT(24)

#define REG_PCIE_PMA_SCAN_MODE				0x0884
#define PCIE_FORCE_DA_PXP_JCPLL_KBAND_LOAD_EN		BIT(0)
#define PCIE_FORCE_SEL_DA_PXP_JCPLL_KBAND_LOAD_EN	BIT(8)

#define REG_PCIE_PMA_DIG_RESERVE_13		0x08bc
#define PCIE_FLL_IDAC_PCIEG1			GENMASK(10, 0)
#define PCIE_FLL_IDAC_PCIEG2			GENMASK(26, 16)

#define REG_PCIE_PMA_DIG_RESERVE_14		0x08c0
#define PCIE_FLL_IDAC_PCIEG3			GENMASK(10, 0)
#define PCIE_FLL_LOAD_EN			BIT(16)

#define REG_PCIE_PMA_FORCE_DA_PXP_RX_FE_GAIN_CTRL	0x088c
#define PCIE_FORCE_DA_PXP_RX_FE_GAIN_CTRL		GENMASK(1, 0)
#define PCIE_FORCE_SEL_DA_PXP_RX_FE_GAIN_CTRL		BIT(8)

#define REG_PCIE_PMA_FORCE_DA_PXP_RX_FE_PWDB	0x0894
#define PCIE_FORCE_DA_PXP_RX_FE_PWDB		BIT(0)
#define PCIE_FORCE_SEL_DA_PXP_RX_FE_PWDB	BIT(8)

#define REG_PCIE_PMA_DIG_RESERVE_12		0x08b8
#define PCIE_FORCE_PMA_RX_SPEED			GENMASK(7, 4)
#define PCIE_FORCE_SEL_PMA_RX_SPEED		BIT(7)

#define REG_PCIE_PMA_DIG_RESERVE_17		0x08e0

#define REG_PCIE_PMA_DIG_RESERVE_18		0x08e4
#define PCIE_PXP_RX_VTH_SEL_PCIE_G1		GENMASK(4, 0)
#define PCIE_PXP_RX_VTH_SEL_PCIE_G2		GENMASK(12, 8)
#define PCIE_PXP_RX_VTH_SEL_PCIE_G3		GENMASK(20, 16)

#define REG_PCIE_PMA_DIG_RESERVE_19		0x08e8
#define PCIE_PCP_RX_REV0_PCIE_GEN1		GENMASK(31, 16)

#define REG_PCIE_PMA_DIG_RESERVE_20		0x08ec
#define PCIE_PCP_RX_REV0_PCIE_GEN2		GENMASK(15, 0)
#define PCIE_PCP_RX_REV0_PCIE_GEN3		GENMASK(31, 16)

#define REG_PCIE_PMA_DIG_RESERVE_21		0x08f0
#define REG_PCIE_PMA_DIG_RESERVE_22		0x08f4
#define REG_PCIE_PMA_DIG_RESERVE_27		0x0908
#define REG_PCIE_PMA_DIG_RESERVE_30		0x0914

#endif /* _PHY_AIROHA_PCIE_H */
