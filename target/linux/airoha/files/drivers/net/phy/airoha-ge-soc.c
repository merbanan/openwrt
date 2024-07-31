// SPDX-License-Identifier: GPL-2.0+
#include <linux/bitfield.h>
#include <linux/module.h>
#include <linux/phy.h>

/* This code is based on mediatek-ge-soc.c
 * The AN7581 SoC uses a switch and set of
 * phys that are very similar to MT7530.
 *
 * The same defines is used for the registers.
 *
 */

#define MTK_EXT_PAGE_ACCESS		0x1f
#define MTK_PHY_PAGE_STANDARD		0x0000
#define MTK_PHY_PAGE_EXTENDED		0x0001
#define MTK_PHY_PAGE_EXTENDED_2		0x0002
#define MTK_PHY_PAGE_EXTENDED_3		0x0003
#define MTK_PHY_RG_LPI_PCS_DSP_CTRL_REG11	0x11

#define MTK_PHY_PAGE_EXTENDED_2A30	0x2a30
#define MTK_PHY_PAGE_EXTENDED_52B5	0x52b5

#define MTK_PHY_LPI_REG_14			0x14
#define MTK_PHY_LPI_WAKE_TIMER_1000_MASK	GENMASK(8, 0)

#define MTK_PHY_LPI_REG_1c			0x1c
#define MTK_PHY_SMI_DET_ON_THRESH_MASK		GENMASK(13, 8)

#define MTK_PHY_PAGE_EXTENDED_2A30		0x2a30
#define MTK_PHY_PAGE_EXTENDED_52B5		0x52b5

/* Registers on Token Ring debug nodes */
/* ch_addr = 0x0, node_addr = 0x7, data_addr = 0x15 */
#define NORMAL_MSE_LO_THRESH_MASK		GENMASK(15, 8) /* NormMseLoThresh */

/* ch_addr = 0x0, node_addr = 0xf, data_addr = 0x3c */
#define REMOTE_ACK_COUNT_LIMIT_CTRL_MASK	GENMASK(2, 1) /* RemAckCntLimitCtrl */

/* ch_addr = 0x1, node_addr = 0xd, data_addr = 0x20 */
#define VCO_SLICER_THRESH_HIGH_MASK		GENMASK(23, 0) /* VcoSlicerThreshBitsHigh */

/* ch_addr = 0x1, node_addr = 0xf, data_addr = 0x0 */
#define DFE_TAIL_EANBLE_VGA_TRHESH_1000		GENMASK(5, 1) /* DfeTailEnableVgaThresh1000 */

/* ch_addr = 0x1, node_addr = 0xf, data_addr = 0x1 */
#define MRVL_TR_FIX_100KP_MASK			GENMASK(22, 20) /* MrvlTrFix100Kp */
#define MRVL_TR_FIX_100KF_MASK			GENMASK(19, 17) /* MrvlTrFix100Kf */
#define MRVL_TR_FIX_1000KP_MASK			GENMASK(16, 14) /* MrvlTrFix1000Kp */
#define MRVL_TR_FIX_1000KF_MASK			GENMASK(13, 11) /* MrvlTrFix1000Kf */

/* ch_addr = 0x1, node_addr = 0xf, data_addr = 0x12 */
#define VGA_DECIMATION_RATE_MASK		GENMASK(8, 5) /* VgaDecRate */

/* ch_addr = 0x1, node_addr = 0xf, data_addr = 0x17 */
#define SLAVE_DSP_READY_TIME_MASK		GENMASK(22, 15) /* SlvDSPreadyTime */
#define MASTER_DSP_READY_TIME_MASK		GENMASK(14, 7) /* MasDSPreadyTime */

/* ch_addr = 0x1, node_addr = 0xf, data_addr = 0x18 */
#define ENABLE_RANDOM_UPDOWN_COUNTER_TRIGGER	BIT(8) /* EnabRandUpdTrig */

/* ch_addr = 0x1, node_addr = 0xf, data_addr = 0x20 */
#define RESET_SYNC_OFFSET_MASK			GENMASK(11, 8) /* ResetSyncOffset */

/* ch_addr = 0x1, node_addr = 0xf, data_addr = 0x38 (PMA) */
#define VGA_STATE_D				GENMASK(23, 19)
#define VGA_STATE_C				GENMASK(18, 14)
#define VGA_STATE_B				GENMASK(13, 9)
#define VGA_STATE_A				GENMASK(8, 4)

/* ch_addr = 0x2, node_addr = 0xd, data_addr = 0x0 */
#define FFE_UPDATE_GAIN_FORCE_VAL_MASK		GENMASK(9, 7) /* FfeUpdGainForceVal */
#define FFE_UPDATE_GAIN_FORCE			BIT(6) /* FfeUpdGainForce */

/* ch_addr = 0x2, node_addr = 0xd, data_addr = 0x3 */
#define TR_FREEZE_MASK				GENMASK(11, 0) /* TrFreeze */

/* ch_addr = 0x2, node_addr = 0xd, data_addr = 0x6 */
/* SS: Steady-state, KP: Proportional Gain */
#define SS_TR_KP100_MASK			GENMASK(21, 19) /* SSTrKp100 */
#define SS_TR_KF100_MASK			GENMASK(18, 16) /* SSTrKf100 */
#define SS_TR_KP1000_MASTER_MASK		GENMASK(15, 13) /* SSTrKp1000Mas */
#define SS_TR_KF1000_MASTER_MASK		GENMASK(12, 10) /* SSTrKf1000Mas */
#define SS_TR_KP1000_SLAVE_MASK			GENMASK(9, 7)   /* SSTrKp1000Slv */
#define SS_TR_KF1000_SLAVE_MASK			GENMASK(6, 4)   /* SSTrKf1000Slv */

/* ch_addr = 0x2, node_addr = 0xd, data_addr = 0x8 */
/* clear this bit if wanna select from AFE */
#define EEE1000_SELECT_SIGNEL_DETECTION_FROM_DFE	BIT(4) /* Regsigdet_sel_1000 */

/* ch_addr = 0x2, node_addr = 0xd, data_addr = 0xd */
#define EEE1000_STAGE2_TR_KF_MASK		GENMASK(13, 11) /* RegEEE_st2TrKf1000 */

/* ch_addr = 0x2, node_addr = 0xd, data_addr = 0xf */
#define SLAVE_WAKETR_TIMER_MASK			GENMASK(20, 11) /* RegEEE_slv_waketr_timer_tar */
#define SLAVE_REMTX_TIMER_MASK			GENMASK(10, 1) /* RegEEE_slv_remtx_timer_tar */

/* ch_addr = 0x2, node_addr = 0xd, data_addr = 0x10 */
#define SLAVE_WAKEINT_TIMER_MASK		GENMASK(10, 1) /* RegEEE_slv_wake_int_timer_tar */

/* ch_addr = 0x2, node_addr = 0xd, data_addr = 0x14 */
#define TR_FREEZE_TIMER2_MASK			GENMASK(9, 0) /* RegEEE_trfreeze_timer2 */

/* ch_addr = 0x2, node_addr = 0xd, data_addr = 0x1c */
#define EEE100_LPSYNC_STAGE1_UPDATE_TIMER_MASK	GENMASK(8, 0) /* RegEEE100Stg1_tar */

/* ch_addr = 0x2, node_addr = 0xd, data_addr = 0x25 */
#define WAKE_SLAVE_TR_WAIT_DFE_DETECTION_EN	BIT(11) /* REGEEE_wake_slv_tr_wait_dfesigdet_en */

#define ANALOG_INTERNAL_OPERATION_MAX_US	20
#define TXRESERVE_MIN				0
#define TXRESERVE_MAX				7

#define MTK_PHY_ANARG_RG			0x10
#define   MTK_PHY_TCLKOFFSET_MASK		GENMASK(12, 8)

/* Registers on DEV07 */
#define MTK_PHY_RG_DEV07_REG3C			0x3c	/* EEE Advertisement Register */
#define   MTK_PHY_ADV_EEE_10G_KR		BIT(6)
#define   MTK_PHY_ADV_EEE_10G_KX4		BIT(5)
#define   MTK_PHY_ADV_EEE_1G_KX			BIT(4)
#define   MTK_PHY_ADV_EEE_10G_T			BIT(3)
#define   MTK_PHY_ADV_EEE_1G_T_PRE		BIT(2)
#define   MTK_PHY_ADV_EEE_100M_TX_PRE		BIT(1)

#define MTK_PHY_RG_DEV07_REG3D			0x3d	/* EEE LP Advertisement Register */
#define   MTK_PHY_LP_EEE_10G_KR			BIT(6)
#define   MTK_PHY_LP_EEE_10G_KX4		BIT(5)
#define   MTK_PHY_LP_EEE_1G_KX			BIT(4)
#define   MTK_PHY_LP_EEE_10G_T			BIT(3)
#define   MTK_PHY_LP_EEE_1G_T_PRE		BIT(2)
#define   MTK_PHY_LP_EEE_100M_TX_PRE		BIT(1)

/* Registers on MDIO_MMD_VEND1 */
#define MTK_PHY_TXVLD_DA_RG			0x12
#define   MTK_PHY_DA_TX_I2MPB_A_GBE_MASK	GENMASK(15, 10)
#define   MTK_PHY_DA_TX_I2MPB_A_TBT_MASK	GENMASK(5, 0)

#define MTK_PHY_GBE_MODE_TX_DELAY_SEL		0x13
#define MTK_PHY_TEST_MODE_TX_DELAY_SEL		0x14
#define   MTK_TX_DELAY_PAIR_B_MASK		GENMASK(10, 8)
#define   MTK_TX_DELAY_PAIR_AFE			BIT(6)
#define   MTK_TX_DELAY_PAIR_D_MASK		GENMASK(2, 0)

#define MTK_PHY_TX_I2MPB_TEST_MODE_A2		0x16
#define   MTK_PHY_DA_TX_I2MPB_A_HBT_MASK	GENMASK(15, 10)
#define   MTK_PHY_DA_TX_I2MPB_A_TST_MASK	GENMASK(5, 0)

#define MTK_PHY_TX_I2MPB_TEST_MODE_B1		0x17
#define   MTK_PHY_DA_TX_I2MPB_B_GBE_MASK	GENMASK(13, 8)
#define   MTK_PHY_DA_TX_I2MPB_B_TBT_MASK	GENMASK(5, 0)

#define MTK_PHY_TX_I2MPB_TEST_MODE_B2		0x18
#define   MTK_PHY_DA_TX_I2MPB_B_HBT_MASK	GENMASK(13, 8)
#define   MTK_PHY_DA_TX_I2MPB_B_TST_MASK	GENMASK(5, 0)

#define MTK_PHY_TX_I2MPB_TEST_MODE_C1		0x19
#define   MTK_PHY_DA_TX_I2MPB_C_GBE_MASK	GENMASK(13, 8)
#define   MTK_PHY_DA_TX_I2MPB_C_TBT_MASK	GENMASK(5, 0)

#define MTK_PHY_TX_I2MPB_TEST_MODE_C2		0x20
#define   MTK_PHY_DA_TX_I2MPB_C_HBT_MASK	GENMASK(13, 8)
#define   MTK_PHY_DA_TX_I2MPB_C_TST_MASK	GENMASK(5, 0)

#define MTK_PHY_TX_I2MPB_TEST_MODE_D1		0x21
#define   MTK_PHY_DA_TX_I2MPB_D_GBE_MASK	GENMASK(13, 8)
#define   MTK_PHY_DA_TX_I2MPB_D_TBT_MASK	GENMASK(5, 0)

#define MTK_PHY_TX_I2MPB_TEST_MODE_D2		0x22
#define   MTK_PHY_DA_TX_I2MPB_D_HBT_MASK	GENMASK(13, 8)
#define   MTK_PHY_DA_TX_I2MPB_D_TST_MASK	GENMASK(5, 0)

#define MTK_PHY_TX_RX_CAL_CRITERIA_VAL		0x37

#define MTK_PHY_RG_DEV1E_REG39			0x39
#define   MTK_PHY_BYPASS_ALL_CAL		BIT(14)
#define   MTK_PHY_BYPASS_ADC_OFFSET_ANA_CAL	BIT(11)

#define MTK_PHY_RG_DEV1E_REG3E			0x3e

#define MTK_PHY_RG_DEV1E_REG96			0x96
#define   MTK_PHY_BYPASS_TX_OFFSET_CAL		BIT(15)

#define MTK_PHY_RG_DEV1E_REG9B			0x9b
#define   MTK_PHY_LCH_MSE_MDCA			GENMASK(15, 8)
#define   MTK_PHY_LCH_MSE_MDCB			GENMASK(7, 0)
#define MTK_PHY_RG_DEV1E_REG9C			0x9c
#define   MTK_PHY_LCH_MSE_MDCC			GENMASK(15, 8)
#define   MTK_PHY_LCH_MSE_MDCD			GENMASK(7, 0)

#define MTK_PHY_RG_DEV1E_REG9D			0x9d
#define   MTK_PHY_MSE_SLICER_ERR_THRES		GENMASK(15, 11)

#define MTK_PHY_RG_DEV1E_REG9E			0x9e
#define   MTK_PHY_MSE_SLICER_ERR_OVER_SUM_A	GENMASK(15, 0)
#define MTK_PHY_RG_DEV1E_REG9F			0x9f
#define   MTK_PHY_MSE_SLICER_ERR_OVER_SUM_B	GENMASK(15, 0)
#define MTK_PHY_RG_DEV1E_REGA0			0xa0
#define   MTK_PHY_MSE_SLICER_ERR_OVER_SUM_C	GENMASK(15, 0)
#define MTK_PHY_RG_DEV1E_REGA1			0xa1
#define   MTK_PHY_MSE_SLICER_ERR_OVER_SUM_D	GENMASK(15, 0)

#define MTK_PHY_RG_DEV1E_REGA2			0xa2
#define   MTK_PHY_LCH_SIGNALDETECT		BIT(15)
#define   MTK_PHY_LCH_LINKPULSE			BIT(14)
#define   MTK_PHY_LCH_DESCRAMBLERLOCK1000	BIT(13)
#define   MTK_PHY_LCH_DESCRAMBLERLOCK100	BIT(12)
#define   MTK_PHY_LCH_LCH_LINKSTATUS1000_OK	BIT(11)
#define   MTK_PHY_LCH_LINKSTATUS100_OK		BIT(10)
#define   MTK_PHY_LCH_LINKSTATUS10_OK		BIT(9)
#define   MTK_PHY_LCH_MRPAGERX			BIT(8)
#define   MTK_PHY_LCH_MRAUTONEGCOMPLETE		BIT(7)
#define   MTK_PHY_DA_MDIX			BIT(6)
#define   MTK_PHY_FULLDUPLEXENABLE		BIT(5)
#define   MTK_PHY_MSCONFIG1000			BIT(4)
#define   MTK_PHY_FINAL_SPEED_1000		BIT(3)
#define   MTK_PHY_FINAL_SPEED_100		BIT(2)
#define   MTK_PHY_FINAL_SPEED_10		BIT(1)

#define MTK_PHY_MCC_CTRL_AND_TX_POWER_CTRL	0xa6
#define   MTK_MCC_NEARECHO_OFFSET_MASK		GENMASK(15, 8)

#define MTK_PHY_RXADC_CTRL_RG7			0xc6
#define   MTK_PHY_DA_AD_BUF_BIAS_LP_MASK	GENMASK(9, 8)

#define MTK_DC_CTRL_RG9				0xc8
#define   MTK_PHY_DA_RX_PSBN_TBT_MASK		GENMASK(14, 12)
#define   MTK_PHY_DA_RX_PSBN_HBT_MASK		GENMASK(10, 8)
#define   MTK_PHY_DA_RX_PSBN_GBE_MASK		GENMASK(6, 4)
#define   MTK_PHY_DA_RX_PSBN_LP_MASK		GENMASK(2, 0)

#define MTK_PHY_LDO_OUTPUT_V			0xd7

#define MTK_PHY_RG_ANA_CAL_RG0			0xdb
#define   MTK_PHY_RG_CAL_CKINV			BIT(12)
#define   MTK_PHY_RG_ANA_CALEN			BIT(8)
#define   MTK_PHY_RG_REXT_CALEN			BIT(4)
#define   MTK_PHY_RG_ZCALEN_A			BIT(0)

#define MTK_PHY_RG_ANA_CAL_RG1			0xdc
#define   MTK_PHY_RG_ZCALEN_B			BIT(12)
#define   MTK_PHY_RG_ZCALEN_C			BIT(8)
#define   MTK_PHY_RG_ZCALEN_D			BIT(4)
#define   MTK_PHY_RG_TXVOS_CALEN		BIT(0)

#define MTK_PHY_RG_DEV1E_REGDD			0xdd
#define   MTK_PHY_RG_TX_A_AMP_CAL_EN		BIT(12)
#define   MTK_PHY_RG_TX_B_AMP_CAL_EN		BIT(8)
#define   MTK_PHY_RG_TX_C_AMP_CAL_EN		BIT(4)
#define   MTK_PHY_RG_TX_D_AMP_CAL_EN		BIT(0)

#define MTK_PHY_RG_ANA_CAL_RG5			0xe0
#define   MTK_PHY_RG_REXT_TRIM_MASK		GENMASK(13, 8)
#define   MTK_PHY_RG_ZCAL_CTRL_MASK		GENMASK(5, 0)

#define MTK_PHY_RG_DEV1E_REGE1			0xe1
#define   MTK_PHY_RG_CAL_REFSEL_1V			BIT(4)

#define MTK_PHY_RG_TX_FILTER			0xfe

#define MTK_PHY_RG_BG_VOLT_OUT			0x100

#define MTK_PHY_RG_DEV1E_REG107			0x107
#define   MTK_PHY_RG_RTUNE_CAL_EN		BIT(12)

#define MTK_PHY_RG_LPI_PCS_DSP_CTRL_REG120	0x120
#define   MTK_PHY_LPI_SIG_EN_LO_THRESH1000_MASK	GENMASK(12, 8)
#define   MTK_PHY_LPI_SIG_EN_HI_THRESH1000_MASK	GENMASK(4, 0)

#define MTK_PHY_RG_LPI_PCS_DSP_CTRL_REG122	0x122
#define   MTK_PHY_LPI_NORM_MSE_HI_THRESH1000_MASK	GENMASK(7, 0)

#define MTK_PHY_RG_LPI_PCS_DSP_CTRL_REG123	0x123
#define   MTK_PHY_LPI_NORM_MSE_LO_THRESH100_MASK	GENMASK(15, 8)
#define   MTK_PHY_LPI_NORM_MSE_HI_THRESH100_MASK	GENMASK(7, 0)

#define MTK_PHY_RG_TESTMUX_ADC_CTRL		0x144
#define   MTK_PHY_RG_TXEN_DIG_MASK		GENMASK(5, 5)

#define MTK_PHY_RG_MDI_CTRL			0x145

#define MTK_PHY_RG_DEV1E_REG171			0x171
#define   MTK_PHY_BYPASS_TX_RX_OFFSET_CANCEL	GENMASK(8, 7)

#define MTK_PHY_RG_CR_TX_AMP_OFFSET_A_B		0x172
#define   MTK_PHY_CR_TX_AMP_OFFSET_A_MASK	GENMASK(13, 8)
#define   MTK_PHY_CR_TX_AMP_OFFSET_B_MASK	GENMASK(6, 0)

#define MTK_PHY_RG_CR_TX_AMP_OFFSET_C_D		0x173
#define   MTK_PHY_CR_TX_AMP_OFFSET_C_MASK	GENMASK(13, 8)
#define   MTK_PHY_CR_TX_AMP_OFFSET_D_MASK	GENMASK(6, 0)

#define MTK_PHY_RG_DEV1E_REG174			0x174
#define   MTK_PHY_TX_R50_AMP_OFFSET_PAIR_A_MASK	GENMASK(15, 8)
#define   MTK_PHY_TX_R50_AMP_OFFSET_PAIR_B_MASK	GENMASK(7, 0)

#define MTK_PHY_RG_DEV1E_REG175			0x175
#define   MTK_PHY_TX_R50_AMP_OFFSET_PAIR_C_MASK	GENMASK(15, 8)
#define   MTK_PHY_TX_R50_AMP_OFFSET_PAIR_D_MASK	GENMASK(7, 0)

#define MTK_PHY_RG_AD_CAL_COMP			0x17a
#define   MTK_PHY_AD_CAL_COMP_OUT		BIT(8)
#define   MTK_PHY_AD_CAL_COMP_OUT_SHIFT		(8)

#define MTK_PHY_RG_AD_CAL_CLK			0x17b
#define   MTK_PHY_DA_CAL_CLK			BIT(0)

#define MTK_PHY_RG_AD_CALIN			0x17c
#define   MTK_PHY_DA_CALIN_FLAG			BIT(0)

#define MTK_PHY_RG_DASN_DAC_IN0_A		0x17d
#define   MTK_PHY_DASN_DAC_IN0_A_MASK		GENMASK(9, 0)

#define MTK_PHY_RG_DASN_DAC_IN0_B		0x17e
#define   MTK_PHY_DASN_DAC_IN0_B_MASK		GENMASK(9, 0)

#define MTK_PHY_RG_DASN_DAC_IN0_C		0x17f
#define   MTK_PHY_DASN_DAC_IN0_C_MASK		GENMASK(9, 0)

#define MTK_PHY_RG_DASN_DAC_IN0_D		0x180
#define   MTK_PHY_DASN_DAC_IN0_D_MASK		GENMASK(9, 0)

#define MTK_PHY_RG_DASN_DAC_IN1_A		0x181
#define   MTK_PHY_DASN_DAC_IN1_A_MASK		GENMASK(9, 0)

#define MTK_PHY_RG_DASN_DAC_IN1_B		0x182
#define   MTK_PHY_DASN_DAC_IN1_B_MASK		GENMASK(9, 0)

#define MTK_PHY_RG_DASN_DAC_IN1_C		0x183
#define   MTK_PHY_DASN_DAC_IN1_C_MASK		GENMASK(9, 0)

#define MTK_PHY_RG_DASN_DAC_IN1_D		0x184
#define   MTK_PHY_DASN_DAC_IN1_D_MASK		GENMASK(9, 0)

#define MTK_PHY_RG_TX_SLEW_CTRL			0x185

#define MTK_PHY_RG_DEV1E_REG19b			0x19b
#define   MTK_PHY_BYPASS_DSP_LPI_READY		BIT(8)

#define MTK_PHY_RG_LP_IIR2_K1_L			0x22a
#define MTK_PHY_RG_LP_IIR2_K1_U			0x22b
#define MTK_PHY_RG_LP_IIR2_K2_L			0x22c
#define MTK_PHY_RG_LP_IIR2_K2_U			0x22d
#define MTK_PHY_RG_LP_IIR2_K3_L			0x22e
#define MTK_PHY_RG_LP_IIR2_K3_U			0x22f
#define MTK_PHY_RG_LP_IIR2_K4_L			0x230
#define MTK_PHY_RG_LP_IIR2_K4_U			0x231
#define MTK_PHY_RG_LP_IIR2_K5_L			0x232
#define MTK_PHY_RG_LP_IIR2_K5_U			0x233

#define MTK_PHY_RG_DEV1E_REG234			0x234
#define   MTK_PHY_TR_OPEN_LOOP_EN_MASK		GENMASK(0, 0)
#define   MTK_PHY_LPF_X_AVERAGE_MASK		GENMASK(7, 4)
#define   MTK_PHY_TR_LP_IIR_EEE_EN		BIT(12)

#define MTK_PHY_RG_LPF_CNT_VAL			0x235

#define MTK_PHY_RG_DEV1E_REG238			0x238
#define   MTK_PHY_LPI_SLV_SEND_TX_TIMER_MASK	GENMASK(8, 0)
#define   MTK_PHY_LPI_SLV_SEND_TX_EN		BIT(12)

#define MTK_PHY_RG_DEV1E_REG239			0x239
#define   MTK_PHY_LPI_SEND_LOC_TIMER_MASK	GENMASK(8, 0)
#define   MTK_PHY_LPI_TXPCS_LOC_RCV		BIT(12)

#define MTK_PHY_RG_DEV1E_REG27C			0x27c
#define   MTK_PHY_VGASTATE_FFE_THR_ST1_MASK	GENMASK(12, 8)
#define MTK_PHY_RG_DEV1E_REG27D			0x27d
#define   MTK_PHY_VGASTATE_FFE_THR_ST2_MASK	GENMASK(4, 0)

#define MTK_PHY_RG_DEV1E_REG2C7			0x2c7
#define   MTK_PHY_MAX_GAIN_MASK			GENMASK(4, 0)
#define   MTK_PHY_MIN_GAIN_MASK			GENMASK(12, 8)

#define MTK_PHY_RG_DEV1E_REG2D1			0x2d1
#define   MTK_PHY_VCO_SLICER_THRESH_BITS_HIGH_EEE_MASK	GENMASK(7, 0)
#define   MTK_PHY_LPI_SKIP_SD_SLV_TR		BIT(8)
#define   MTK_PHY_LPI_TR_READY			BIT(9)
#define   MTK_PHY_LPI_VCO_EEE_STG0_EN		BIT(10)

#define MTK_PHY_RG_DEV1E_REG323			0x323
#define   MTK_PHY_EEE_WAKE_MAS_INT_DC		BIT(0)
#define   MTK_PHY_EEE_WAKE_SLV_INT_DC		BIT(4)

#define MTK_PHY_RG_DEV1E_REG324			0x324
#define   MTK_PHY_SMI_DETCNT_MAX_MASK		GENMASK(5, 0)
#define   MTK_PHY_SMI_DET_MAX_EN		BIT(8)

#define MTK_PHY_RG_DEV1E_REG326			0x326
#define   MTK_PHY_LPI_MODE_SD_ON		BIT(0)
#define   MTK_PHY_RESET_RANDUPD_CNT		BIT(1)
#define   MTK_PHY_TREC_UPDATE_ENAB_CLR		BIT(2)
#define   MTK_PHY_LPI_QUIT_WAIT_DFE_SIG_DET_OFF	BIT(4)
#define   MTK_PHY_TR_READY_SKIP_AFE_WAKEUP	BIT(5)

#define MTK_PHY_LDO_PUMP_EN_PAIRAB		0x502
#define MTK_PHY_LDO_PUMP_EN_PAIRCD		0x503

#define MTK_PHY_DA_TX_R50_PAIR_A		0x53d
#define MTK_PHY_DA_TX_R50_PAIR_B		0x53e
#define MTK_PHY_DA_TX_R50_PAIR_C		0x53f
#define MTK_PHY_DA_TX_R50_PAIR_D		0x540

/* Registers on MDIO_MMD_VEND2 */
#define MTK_PHY_FLAG_CTRL0			0x15

#define MTK_PHY_FLAG_CTRL1			0x16

#define MTK_PHY_CLK_OUTPUT			0x19
#define   MTK_PHY_CLK_OUTPUT_EN			BIT(12)
#define   MTK_PHY_CLK_OUTPUT_DIS		BIT(4)

#define MTK_PHY_FLAG_MONITOR			0x1A
#define   MTK_PHY_FLAG_MONITOR1			GENMASK(15, 8)
#define   MTK_PHY_FLAG_MONITOR0			GENMASK(7, 0)

#define MTK_PHY_LED_BASIC_CTRL			0x21
#define   MTK_PHY_LED_ENCHANCE_MODE		BIT(15)
#define   MTK_PHY_LED_EVENT_ALL			BIT(4)
#define   MTK_PHY_LED_CLOCK_EN			BIT(3)
#define   MTK_PHY_LED_TIMING_TEST		BIT(2)
#define   MTK_PHY_LED_MODE			GENMASK(1, 0)

#define MTK_PHY_LED_ON_DURATION			0x22
#define   MTK_PHY_LED_ON_DUR			GENMASK(15, 0)

#define MTK_PHY_LED_BLINK_DURATION		0x23
#define   MTK_PHY_LED_BLK_DUR			GENMASK(15, 0)

#define MTK_PHY_LED0_ON_CTRL			0x24
#define MTK_PHY_LED1_ON_CTRL			0x26
#define   MTK_PHY_LED_ON_MASK			GENMASK(6, 0)
#define   MTK_PHY_LED_ON_LINK1000		BIT(0)
#define   MTK_PHY_LED_ON_LINK100		BIT(1)
#define   MTK_PHY_LED_ON_LINK10			BIT(2)
#define   MTK_PHY_LED_ON_LINK			(MTK_PHY_LED_ON_LINK10 |\
						 MTK_PHY_LED_ON_LINK100 |\
						 MTK_PHY_LED_ON_LINK1000)
#define   MTK_PHY_LED_ON_LINKDOWN		BIT(3)
#define   MTK_PHY_LED_ON_FDX			BIT(4) /* Full duplex */
#define   MTK_PHY_LED_ON_HDX			BIT(5) /* Half duplex */
#define   MTK_PHY_LED_ON_FORCE_ON		BIT(6)
#define   MTK_PHY_LED_ON_POLARITY		BIT(14)
#define   MTK_PHY_LED_ON_ENABLE			BIT(15)

#define MTK_PHY_LED0_BLINK_CTRL			0x25
#define MTK_PHY_LED1_BLINK_CTRL			0x27
#define   MTK_PHY_LED_BLINK_1000TX		BIT(0)
#define   MTK_PHY_LED_BLINK_1000RX		BIT(1)
#define   MTK_PHY_LED_BLINK_100TX		BIT(2)
#define   MTK_PHY_LED_BLINK_100RX		BIT(3)
#define   MTK_PHY_LED_BLINK_10TX		BIT(4)
#define   MTK_PHY_LED_BLINK_10RX		BIT(5)
#define   MTK_PHY_LED_BLINK_RX			(MTK_PHY_LED_BLINK_10RX |\
						 MTK_PHY_LED_BLINK_100RX |\
						 MTK_PHY_LED_BLINK_1000RX)
#define   MTK_PHY_LED_BLINK_TX			(MTK_PHY_LED_BLINK_10TX |\
						 MTK_PHY_LED_BLINK_100TX |\
						 MTK_PHY_LED_BLINK_1000TX)
#define   MTK_PHY_LED_BLINK_COLLISION		BIT(6)
#define   MTK_PHY_LED_BLINK_RX_CRC_ERR		BIT(7)
#define   MTK_PHY_LED_BLINK_RX_IDLE_ERR		BIT(8)
#define   MTK_PHY_LED_BLINK_FORCE_BLINK		BIT(9)

#define MTK_PHY_LED1_DEFAULT_POLARITIES		BIT(1)

#define MTK_PHY_RG_BG_RASEL			0x115
#define   MTK_PHY_RG_BG_RASEL_MASK		GENMASK(2, 0)

enum {
	NO_PAIR,
	PAIR_A,
	PAIR_B,
	PAIR_C,
	PAIR_D,
};

enum calibration_mode {
	SW_K
};

enum CAL_ITEM {
	REXT,
	TX_OFFSET,
	TX_AMP,
	TX_R50,
	TX_VCM
};

enum CAL_MODE {
	SW_M
};

static int mtk_gephy_read_page(struct phy_device *phydev)
{
	return __phy_read(phydev, MTK_EXT_PAGE_ACCESS);
}

static int mtk_gephy_write_page(struct phy_device *phydev, int page)
{
	return __phy_write(phydev, MTK_EXT_PAGE_ACCESS, page);
}

static void mtk_gephy_config_init(struct phy_device *phydev)
{
	/* Enable HW auto downshift */
	phy_modify_paged(phydev, MTK_PHY_PAGE_EXTENDED, 0x14, 0, BIT(4));

	/* Increase SlvDPSready time */
	phy_select_page(phydev, MTK_PHY_PAGE_EXTENDED_52B5);
	__phy_write(phydev, 0x10, 0xafae);
	__phy_write(phydev, 0x12, 0x2f);
	__phy_write(phydev, 0x10, 0x8fae);
	phy_restore_page(phydev, MTK_PHY_PAGE_STANDARD, 0);

	/* Adjust 100_mse_threshold */
	phy_write_mmd(phydev, MDIO_MMD_VEND1, 0x123, 0xffff);

	/* Disable mcc */
	phy_write_mmd(phydev, MDIO_MMD_VEND1, 0xa6, 0x300);
}

static int mt7530_phy_config_init(struct phy_device *phydev)
{
	mtk_gephy_config_init(phydev);

	/* Increase post_update_timer */
	phy_write_paged(phydev, MTK_PHY_PAGE_EXTENDED_3, 0x11, 0x4b);

	return 0;
}

static int an7581_phy_config_init(struct phy_device *phydev)
{
	mtk_gephy_config_init(phydev);

	/* PHY link down power saving enable */
	phy_set_bits(phydev, 0x17, BIT(4));
	phy_clear_bits_mmd(phydev, MDIO_MMD_VEND1, 0xc6, 0x300);

	/* Set TX Pair delay selection */
	phy_write_mmd(phydev, MDIO_MMD_VEND1, 0x13, 0x404);
	phy_write_mmd(phydev, MDIO_MMD_VEND1, 0x14, 0x404);

	return 0;
}

static struct phy_driver airoha_gephy_driver[] = {
	{
		PHY_ID_MATCH_EXACT(0x03a294c1),
		.name		= "Airoha AN7581 MT7530 PHY",
		.config_init	= an7581_phy_config_init,
		/* Interrupts are handled by the switch, not the PHY
		 * itself.
		 */
		.config_intr	= genphy_no_config_intr,
		.handle_interrupt = genphy_handle_interrupt_no_ack,
		.suspend	= genphy_suspend,
		.resume		= genphy_resume,
		.read_page	= mtk_gephy_read_page,
		.write_page	= mtk_gephy_write_page,
	},
};

module_phy_driver(mtk_gephy_driver);

static struct mdio_device_id __maybe_unused airoha_gephy_tbl[] = {
	{ PHY_ID_MATCH_EXACT(0x03a294c1) },
	{ }
};

MODULE_DESCRIPTION("Airoha AN7581 MT7530 Ethernet PHY driver");
MODULE_AUTHOR("Benjamin Larsson <benjamin.larsson@genexis.eu>");
MODULE_LICENSE("GPL");

MODULE_DEVICE_TABLE(mdio, airoha_gephy_tbl);
