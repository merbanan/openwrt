/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2025
 * Author: Benjamin Larsson <benjamin.larsson@genexis.eu>
 */

#ifndef ECONET_751221_REGS_H
#define ECONET_751221_REGS_H

#include <linux/types.h>


#define ECONET_RX_ETH_HLEN	(ETH_HLEN + ETH_FCS_LEN)

#define ECONET_MAX_DEVS		2


/* FE */
#define PSE_BASE			0x0100
#define CDM1_BASE			0x0400
#define GDM1_BASE			0x0500
#define PPE1_BASE			0x0c00

#define CDM2_BASE			0x1400
#define GDM2_BASE			0x1500

#define GDM_BASE(_n)			\
	 ((_n) == 2 ? GDM2_BASE : GDM1_BASE)

#define REG_FE_DMA_GLO_CFG		0x0000
#define FE_DMA_GLO_L2_SPACE_MASK	GENMASK(7, 4)

#define REG_FE_RST_GLO_CFG		0x0004
#define FE_RST_PSE_RESET		BIT(0)

#define REG_FE_FOE_TS			0x0010

#define REG_PSE_IQ_REV1			(PSE_BASE + 0x08)
#define PSE_IQ_RES1_P2_MASK		GENMASK(23, 16)

#define REG_PSE_IQ_REV2			(PSE_BASE + 0x0c)
#define PSE_IQ_RES2_P5_MASK		GENMASK(15, 8)
#define PSE_IQ_RES2_P4_MASK		GENMASK(7, 0)


#define REG_FE_VIP_EN(_n)		(0x0300 + ((_n) << 3))
#define PATN_FCPU_EN_MASK		BIT(7)
#define PATN_SWP_EN_MASK		BIT(6)
#define PATN_DP_EN_MASK			BIT(5)
#define PATN_SP_EN_MASK			BIT(4)
#define PATN_TYPE_MASK			GENMASK(3, 1)
#define PATN_EN_MASK			BIT(0)

#define REG_FE_VIP_PATN(_n)		(0x0304 + ((_n) << 3))
#define PATN_DP_MASK			GENMASK(31, 16)
#define PATN_SP_MASK			GENMASK(15, 0)

#define REG_CDM1_VLAN_CTRL		CDM1_BASE
#define CDM1_VLAN_MASK			GENMASK(31, 16)
#define UNTAG_EN			BIT(1)
#define STAG_EN				BIT(0)

#define REG_CDM1_PPP_GEN		(CDM1_BASE + 0x04)
#define PPP_INS				BIT(16)
#define SESS_ID				GENMASK(15, 0)

#define REG_CDM1_FWD_CFG		(CDM1_BASE + 0x08)

#define REG_CDM2_FWD_CFG		(CDM2_BASE + 0x08)

#define REG_CDM1_CRSN_QSEL(_n)		(CDM1_BASE + 0x10 + ((_n) << 2))
#define CDM1_QSEL_Q0 			0
#define CDM1_QSEL_Q1L 			2
#define CDM1_QSEL_Q1H 			3

#define REG_CDM2_FWD_CFG		(CDM2_BASE + 0x08)
#define CDM2_OAM_QSEL_MASK		GENMASK(31, 27)
#define CDM2_VIP_QSEL_MASK		GENMASK(24, 20)

#define REG_CDM2_CRSN_QSEL(_n)		(CDM2_BASE + 0x10 + ((_n) << 2))
#define CDM2_CRSN_QSEL_REASON_MASK(_n)	\
	GENMASK(4 + (((_n) % 4) << 3),	(((_n) % 4) << 3))

#define REG_GDM_FWD_CFG(_n)		GDM_BASE(_n)
#define GDM_DROP_CRC_ERR		BIT(23)
#define GDM_IP4_CKSUM			BIT(22)
#define GDM_TCP_CKSUM			BIT(21)
#define GDM_UDP_CKSUM			BIT(20)
#define GDM_STRIP_CRC			BIT(16)
#define GDM_MYMACFQ_MASK		GENMASK(15, 12)
#define GDM_BCFQ_MASK			GENMASK(11, 8)
#define GDM_MCFQ_MASK			GENMASK(7, 4)
#define GDM_OCFQ_MASK			GENMASK(3, 0)

#define REG_GDM_MAC_LSB(_n)		(GDM_BASE(_n) + 0x08)
#define GDM_MAC_ADR_LSB_MASK		GENMASK(31, 0)
#define REG_GDM_MAC_MSB(_n)		(GDM_BASE(_n) + 0x0c)
#define GDM_MY_MAC_MASK			GENMASK(23, 16)
#define GDM_MAC_ADR_MSB_MASK		GENMASK(15, 0)

#define REG_GDM_VLAN_CHK(_n)		(GDM_BASE(_n) + 0x10)
#define GDM_STAG_EN_MASK		BIT(0)

#define REG_GDM_LEN_CFG(_n)		(GDM_BASE(_n) + 0x14)
#define GDM_LONG_LEN_MASK		GENMASK(29, 16)
#define GDM_SHORT_LEN_MASK		GENMASK(13, 0)

#define REG_GDM_TXCHN_EN(_n)		(GDM_BASE(_n) + 0x24)
#define REG_GDM_RXCHN_EN(_n)		(GDM_BASE(_n) + 0x28)

#define REG_FE_CPORT_CFG		(GDM1_BASE + 0x40)
#define FE_CPORT_DIS_GSW2FE_CRC_MASK	BIT(30)
#define FE_CPORT_PAD			BIT(26)
#define FE_CPORT_PORT_XFC_MASK		BIT(25)
#define FE_CPORT_QUEUE_XFC_MASK		BIT(24)

#define REG_FE_GDM_MIB_CLEAR(_n)	(GDM_BASE(_n) + 0xf0)
#define FE_GDM_MIB_RX_CLEAR_MASK	BIT(1)
#define FE_GDM_MIB_TX_CLEAR_MASK	BIT(0)

/* GDM MIB counters (shared Frame Engine layout, matches airoha) */
#define REG_FE_GDM_TX_OK_PKT_CNT_L(_n)		(GDM_BASE(_n) + 0x104)
#define REG_FE_GDM_TX_OK_BYTE_CNT_L(_n)		(GDM_BASE(_n) + 0x10c)
#define REG_FE_GDM_TX_ETH_PKT_CNT_L(_n)		(GDM_BASE(_n) + 0x110)
#define REG_FE_GDM_TX_ETH_BYTE_CNT_L(_n)	(GDM_BASE(_n) + 0x114)
#define REG_FE_GDM_TX_ETH_DROP_CNT(_n)		(GDM_BASE(_n) + 0x118)
#define REG_FE_GDM_TX_ETH_BC_CNT(_n)		(GDM_BASE(_n) + 0x11c)
#define REG_FE_GDM_TX_ETH_MC_CNT(_n)		(GDM_BASE(_n) + 0x120)
#define REG_FE_GDM_TX_ETH_RUNT_CNT(_n)		(GDM_BASE(_n) + 0x124)
#define REG_FE_GDM_TX_ETH_LONG_CNT(_n)		(GDM_BASE(_n) + 0x128)
#define REG_FE_GDM_TX_ETH_E64_CNT_L(_n)		(GDM_BASE(_n) + 0x12c)
#define REG_FE_GDM_TX_ETH_L64_CNT_L(_n)		(GDM_BASE(_n) + 0x130)
#define REG_FE_GDM_TX_ETH_L127_CNT_L(_n)	(GDM_BASE(_n) + 0x134)
#define REG_FE_GDM_TX_ETH_L255_CNT_L(_n)	(GDM_BASE(_n) + 0x138)
#define REG_FE_GDM_TX_ETH_L511_CNT_L(_n)	(GDM_BASE(_n) + 0x13c)
#define REG_FE_GDM_TX_ETH_L1023_CNT_L(_n)	(GDM_BASE(_n) + 0x140)

#define REG_FE_GDM_RX_OK_PKT_CNT_L(_n)		(GDM_BASE(_n) + 0x148)
#define REG_FE_GDM_RX_FC_DROP_CNT(_n)		(GDM_BASE(_n) + 0x14c)
#define REG_FE_GDM_RX_RC_DROP_CNT(_n)		(GDM_BASE(_n) + 0x150)
#define REG_FE_GDM_RX_OVERFLOW_DROP_CNT(_n)	(GDM_BASE(_n) + 0x154)
#define REG_FE_GDM_RX_ERROR_DROP_CNT(_n)	(GDM_BASE(_n) + 0x158)
#define REG_FE_GDM_RX_OK_BYTE_CNT_L(_n)		(GDM_BASE(_n) + 0x15c)
#define REG_FE_GDM_RX_ETH_PKT_CNT_L(_n)		(GDM_BASE(_n) + 0x160)
#define REG_FE_GDM_RX_ETH_BYTE_CNT_L(_n)	(GDM_BASE(_n) + 0x164)
#define REG_FE_GDM_RX_ETH_DROP_CNT(_n)		(GDM_BASE(_n) + 0x168)
#define REG_FE_GDM_RX_ETH_BC_CNT(_n)		(GDM_BASE(_n) + 0x16c)
#define REG_FE_GDM_RX_ETH_MC_CNT(_n)		(GDM_BASE(_n) + 0x170)
#define REG_FE_GDM_RX_ETH_CRC_ERR_CNT(_n)	(GDM_BASE(_n) + 0x174)
#define REG_FE_GDM_RX_ETH_FRAG_CNT(_n)		(GDM_BASE(_n) + 0x178)
#define REG_FE_GDM_RX_ETH_JABBER_CNT(_n)	(GDM_BASE(_n) + 0x17c)
#define REG_FE_GDM_RX_ETH_RUNT_CNT(_n)		(GDM_BASE(_n) + 0x180)
#define REG_FE_GDM_RX_ETH_LONG_CNT(_n)		(GDM_BASE(_n) + 0x184)
#define REG_FE_GDM_RX_ETH_E64_CNT_L(_n)		(GDM_BASE(_n) + 0x188)
#define REG_FE_GDM_RX_ETH_L64_CNT_L(_n)		(GDM_BASE(_n) + 0x18c)
#define REG_FE_GDM_RX_ETH_L127_CNT_L(_n)	(GDM_BASE(_n) + 0x190)
#define REG_FE_GDM_RX_ETH_L255_CNT_L(_n)	(GDM_BASE(_n) + 0x194)
#define REG_FE_GDM_RX_ETH_L511_CNT_L(_n)	(GDM_BASE(_n) + 0x198)
#define REG_FE_GDM_RX_ETH_L1023_CNT_L(_n)	(GDM_BASE(_n) + 0x19c)

/* High 32 bits of the 64-bit MIB counters */
#define REG_FE_GDM_TX_OK_PKT_CNT_H(_n)		(GDM_BASE(_n) + 0x280)
#define REG_FE_GDM_TX_OK_BYTE_CNT_H(_n)		(GDM_BASE(_n) + 0x284)
#define REG_FE_GDM_TX_ETH_PKT_CNT_H(_n)		(GDM_BASE(_n) + 0x288)
#define REG_FE_GDM_TX_ETH_BYTE_CNT_H(_n)	(GDM_BASE(_n) + 0x28c)
#define REG_FE_GDM_RX_OK_PKT_CNT_H(_n)		(GDM_BASE(_n) + 0x290)
#define REG_FE_GDM_RX_OK_BYTE_CNT_H(_n)		(GDM_BASE(_n) + 0x294)
#define REG_FE_GDM_RX_ETH_PKT_CNT_H(_n)		(GDM_BASE(_n) + 0x298)
#define REG_FE_GDM_RX_ETH_BYTE_CNT_H(_n)	(GDM_BASE(_n) + 0x29c)
#define REG_FE_GDM_TX_ETH_E64_CNT_H(_n)		(GDM_BASE(_n) + 0x2b8)
#define REG_FE_GDM_TX_ETH_L64_CNT_H(_n)		(GDM_BASE(_n) + 0x2bc)
#define REG_FE_GDM_TX_ETH_L127_CNT_H(_n)	(GDM_BASE(_n) + 0x2c0)
#define REG_FE_GDM_TX_ETH_L255_CNT_H(_n)	(GDM_BASE(_n) + 0x2c4)
#define REG_FE_GDM_TX_ETH_L511_CNT_H(_n)	(GDM_BASE(_n) + 0x2c8)
#define REG_FE_GDM_TX_ETH_L1023_CNT_H(_n)	(GDM_BASE(_n) + 0x2cc)
#define REG_FE_GDM_RX_ETH_E64_CNT_H(_n)		(GDM_BASE(_n) + 0x2e8)
#define REG_FE_GDM_RX_ETH_L64_CNT_H(_n)		(GDM_BASE(_n) + 0x2ec)
#define REG_FE_GDM_RX_ETH_L127_CNT_H(_n)	(GDM_BASE(_n) + 0x2f0)
#define REG_FE_GDM_RX_ETH_L255_CNT_H(_n)	(GDM_BASE(_n) + 0x2f4)
#define REG_FE_GDM_RX_ETH_L511_CNT_H(_n)	(GDM_BASE(_n) + 0x2f8)
#define REG_FE_GDM_RX_ETH_L1023_CNT_H(_n)	(GDM_BASE(_n) + 0x2fc)


#define REG_PPE_GLO_CFG				(PPE1_BASE + 0x200)
#define PPE_GLO_CFG_BUSY_MASK			BIT(31)
#define PPE_GLO_CFG_FLOW_DROP_UPDATE_MASK	BIT(9)
#define PPE_GLO_CFG_PSE_HASH_OFS_MASK		BIT(6)
#define PPE_GLO_CFG_PPE_BSWAP_MASK		BIT(5)
#define PPE_GLO_CFG_TTL_DROP_MASK		BIT(4)
#define PPE_GLO_CFG_IP4_CS_DROP_MASK		BIT(3)
#define PPE_GLO_CFG_IP4_L4_CS_DROP_MASK		BIT(2)
#define PPE_GLO_CFG_EN_MASK			BIT(0)

#define REG_PPE_PPE_FLOW_CFG			(PPE1_BASE + 0x204)
#define PPE_FLOW_CFG_IP6_HASH_GRE_KEY_MASK	BIT(20)
#define PPE_FLOW_CFG_IP4_HASH_GRE_KEY_MASK	BIT(19)
#define PPE_FLOW_CFG_IP4_HASH_FLOW_LABEL_MASK	BIT(18)
#define PPE_FLOW_CFG_IP4_NAT_FRAG_MASK		BIT(17)
#define PPE_FLOW_CFG_IP_PROTO_BLACKLIST_MASK	BIT(16)
#define PPE_FLOW_CFG_IP4_DSLITE_MASK		BIT(14)
#define PPE_FLOW_CFG_IP4_NAPT_MASK		BIT(13)
#define PPE_FLOW_CFG_IP4_NAT_MASK		BIT(12)
#define PPE_FLOW_CFG_IP6_6RD_MASK		BIT(10)
#define PPE_FLOW_CFG_IP6_5T_ROUTE_MASK		BIT(9)
#define PPE_FLOW_CFG_IP6_3T_ROUTE_MASK		BIT(8)
#define PPE_FLOW_CFG_IP4_UDP_FRAG_MASK		BIT(7)
#define PPE_FLOW_CFG_IP4_TCP_FRAG_MASK		BIT(6)

#define REG_PPE_IP_PROTO_CHK			(PPE1_BASE + 0x208)
#define PPE_IP_PROTO_CHK_IPV4_MASK		GENMASK(31, 16)
#define PPE_IP_PROTO_CHK_IPV6_MASK		GENMASK(15, 0)

#define REG_PPE_TB_CFG				(PPE1_BASE + 0x21c)
#define PPE_TB_CFG_KEEPALIVE_MASK		GENMASK(13, 12)
#define PPE_TB_CFG_AGE_TCP_FIN_MASK		BIT(11)
#define PPE_TB_CFG_AGE_UDP_MASK			BIT(10)
#define PPE_TB_CFG_AGE_TCP_MASK			BIT(9)
#define PPE_TB_CFG_AGE_UNBIND_MASK		BIT(8)
#define PPE_TB_CFG_AGE_NON_L4_MASK		BIT(7)
#define PPE_TB_CFG_AGE_PREBIND_MASK		BIT(6)
#define PPE_TB_CFG_SEARCH_MISS_MASK		GENMASK(5, 4)
#define PPE_TB_ENTRY_SIZE_MASK			BIT(3)
#define PPE_DRAM_TB_NUM_ENTRY_MASK		GENMASK(2, 0)

#define REG_PPE_TB_BASE				(PPE1_BASE + 0x220)

#define REG_PPE_BIND_RATE			(PPE1_BASE + 0x228)
#define PPE_BIND_RATE_L2B_BIND_MASK		GENMASK(31, 16)
#define PPE_BIND_RATE_BIND_MASK			GENMASK(15, 0)

#define REG_PPE_BIND_LIMIT0			(PPE1_BASE + 0x22c)
#define PPE_BIND_LIMIT0_HALF_MASK		GENMASK(29, 16)
#define PPE_BIND_LIMIT0_QUARTER_MASK		GENMASK(13, 0)

#define REG_PPE_BIND_LIMIT1			(PPE1_BASE + 0x230)
#define PPE_BIND_LIMIT1_NON_L4_MASK		GENMASK(23, 16)
#define PPE_BIND_LIMIT1_FULL_MASK		GENMASK(13, 0)

#define REG_PPE_BND_AGE0			(PPE1_BASE + 0x23c)
#define PPE_BIND_AGE0_DELTA_NON_L4		GENMASK(30, 16)
#define PPE_BIND_AGE0_DELTA_UDP			GENMASK(14, 0)

#define REG_PPE_UNBIND_AGE			(PPE1_BASE + 0x238)
#define PPE_UNBIND_AGE_MIN_PACKETS_MASK		GENMASK(31, 16)
#define PPE_UNBIND_AGE_DELTA_MASK		GENMASK(7, 0)

#define REG_PPE_BND_AGE1			(PPE1_BASE + 0x240)
#define PPE_BIND_AGE1_DELTA_TCP_FIN		GENMASK(30, 16)
#define PPE_BIND_AGE1_DELTA_TCP			GENMASK(14, 0)

#define REG_PPE_HASH_SEED			(PPE1_BASE + 0x244)
#define PPE_HASH_SEED				0x12345678

#define REG_PPE_DFT_CPORT0			(PPE1_BASE + 0x248)
#define DFT_CPORT_MASK(_n)			GENMASK(3 + ((_n) << 2), ((_n) << 2))

#define REG_PPE_TB_USED				(PPE1_BASE + 0x224)
#define PPE_TB_USED_NUM_MASK			GENMASK(13, 0)

/*
 * FoE cache control. The bit9 "clear" flush was HW-verified on EN751221 by
 * the xr500v port; toggle it after committing a DRAM entry so the lookup
 * cache is invalidated.
 */
#define REG_PPE_CACHE_CTL			(PPE1_BASE + 0x320)
#define PPE_CACHE_CTL_EN			BIT(0)
#define PPE_CACHE_CTL_CLEAR			BIT(9)


/* QDMA */
#define REG_QDMA_GLOBAL_CFG			0x0004
#define GLOBAL_CFG_RX_2B_OFFSET_MASK		BIT(31)
#define GLOBAL_CFG_DMA_PREFERENCE_MASK		GENMASK(30, 29)
#define GLOBAL_CFG_MSG_WORD_SWAP_MASK		BIT(28)
#define GLOBAL_CFG_DSCP_BYTE_SWAP_MASK		BIT(27)
#define GLOBAL_CFG_PAYLOAD_BYTE_SWAP_MASK	BIT(26)
#define GLOBAL_CFG_MULTICAST_MODIFY_FP_MASK	BIT(25)	// FIXME
#define GLOBAL_CFG_OAM_MODIFY_MASK		BIT(24) // FIXME
#define GLOBAL_CFG_RESET_DONE_MASK		BIT(22) // FIXME
#define GLOBAL_CFG_MULTICAST_EN_MASK		BIT(21) // FIXME
#define GLOBAL_CFG_TX_IMMEDIATE_DONE_MASK	BIT(20)
#define GLOBAL_CFG_IRQ_EN_MASK			BIT(19)
#define GLOBAL_CFG_RD_BYPASS_WR_MASK		BIT(17) // FIXME
#define GLOBAL_CFG_QDMA_LOOPBACK_MASK		BIT(16)
#define GLOBAL_CFG_LPBK_RXQ_SEL_MASK		GENMASK(13, 8) // FIXME
#define GLOBAL_CFG_CHECK_DONE_MASK		BIT(7)
#define GLOBAL_CFG_TX_WB_DONE_MASK		BIT(6)
#define GLOBAL_CFG_BURST_SIZE_MASK		GENMASK(5, 4)
#define GLOBAL_CFG_RX_DMA_BUSY_MASK		BIT(3)
#define GLOBAL_CFG_RX_DMA_EN_MASK		BIT(2)
#define GLOBAL_CFG_TX_DMA_BUSY_MASK		BIT(1)
#define GLOBAL_CFG_TX_DMA_EN_MASK		BIT(0)

#define REG_CPU_TX_RING0_DSCP_BASE		0x0008
#define REG_CPU_RX_RING0_DSCP_BASE		0x000C
#define REG_CPU_TX_RING0_IDX			0x0010
#define CPU_TX_RING0_IDX_MASK			GENMASK(11, 0)
#define REG_DMA_TX_RING0_IDX			0x0014
#define DMA_TX_RING0_IDX_MASK			GENMASK(11, 0)
#define REG_CPU_RX_RING0_IDX			0x0018
#define CPU_RX_RING0_IDX_MASK			GENMASK(11, 0)
#define REG_DMA_RX_RING0_IDX			0x001C
#define DMA_RX_RING0_IDX_MASK			GENMASK(11, 0)

#define REG_FWD_DSCP_BASE			0x0020
#define REG_FWD_BUF_BASE			0x0024
#define REG_HW_FWD_DSCP_CFG			0x0028

#define HW_FWD_DSCP_PAYLOAD_SIZE_MASK		GENMASK(29, 28)
#define FWD_DSCP_LOW_THR_MASK			GENMASK(12, 0)

#define REG_LMGR_INIT_CFG			0x0030
#define LMGR_INIT_START				BIT(31)
#define HW_FWD_BUF_PKTSIZE_OVERHEAD_EN		BIT(24)
#define HW_FWD_PKTSIZE_OVERHEAD_MASK		GENMASK(23, 16)
#define HW_FWD_DESC_NUM_MASK			GENMASK(12, 0)

#define REG_INT_STATUS				0x0050
#define XPON_PHY_INT				BIT(24)
#define EPON_MAC_INT				BIT(17)
#define GPON_MAC_INT				BIT(16)
#define RX1_COHERENT_INT			BIT(15)
#define TX1_COHERENT_INT			BIT(14)
#define RX0_COHERENT_INT			BIT(13)
#define TX0_COHERENT_INT			BIT(12)
#define RX_PKT_OVERFLOW_INT			BIT(11)
#define FWD_DSCP_LOW_INT			BIT(10)
#define IRQ_FULL_INT				BIT(9)
#define FWD_DSCP_EMPTY_INT			BIT(8)
#define NO_RX1_CPU_DSCP_INT			BIT(7)
#define NO_TX1_CPU_DSCP_INT			BIT(6)
#define RX1_DONE_INT				BIT(5)
#define TX1_DONE_INT				BIT(4)
#define NO_RX0_CPU_DSCP_INT			BIT(3)
#define NO_TX0_CPU_DSCP_INT			BIT(2)
#define RX0_DONE_INT				BIT(1)
#define TX0_DONE_INT				BIT(0)

#define REG_INT_ENABLE				0x0054
#define XPON_PHY_EN				BIT(24)
#define EPON_MAC_EN				BIT(17)
#define GPON_MAC_EN				BIT(16)
#define RX1_COHERENT_EN				BIT(15)
#define TX1_COHERENT_EN				BIT(14)
#define RX0_COHERENT_EN				BIT(13)
#define TX0_COHERENT_EN				BIT(12)
#define RX_PKT_OVERFLOW_EN			BIT(11)
#define FWD_DSCP_LOW_EN				BIT(10)
#define IRQ_FULL_EN				BIT(9)
#define FWD_DSCP_EMPTY_EN			BIT(8)
#define NO_RX1_CPU_DSCP_EN			BIT(7)
#define NO_TX1_CPU_DSCP_EN			BIT(6)
#define RX1_DONE_EN				BIT(5)
#define TX1_DONE_EN				BIT(4)
#define NO_RX0_CPU_DSCP_EN			BIT(3)
#define NO_TX0_CPU_DSCP_EN			BIT(2)
#define RX0_DONE_EN				BIT(1)
#define TX0_DONE_EN				BIT(0)


#define REG_RX_RING_BASE(_n)	\
	((_n) == 1 ? 0x010C : 0x000C)
#define REG_TX_RING_BASE(_n)	\
	((_n) == 1 ? 0x0108 : 0x0008)

#define REG_RX_RING_SIZE			0x0100
#define RX_RING_SIZE_MASK(_n)	\
	((_n) == 1 ? GENMASK(27, 16) : GENMASK(11, 0))
#define RX_RING_SIZE_MASK_0			GENMASK(11, 0)
#define RX_RING_SIZE_MASK_1			GENMASK(27, 16)

#define REG_RX_RING_THR				0x0104
#define RX_RING_THR_MASK(_n)	\
	((_n) == 1 ? GENMASK(27, 16) : GENMASK(11, 0))
#define RX_RING_THR_MASK_0			GENMASK(11, 0)
#define RX_RING_THR_MASK_1			GENMASK(27, 16)

/*
 * Per-ring index registers: ring 0 lives in the base block (+0x08..0x1C),
 * ring 1 is the same block +0x100. This must match REG_{RX,TX}_RING_BASE above.
 */
#define REG_RX_DMA_IDX(_n)	\
	((_n) == 1 ? 0x11C : 0x01C)
#define RX_RING_DMA_IDX_MASK			GENMASK(11, 0)

#define REG_RX_CPU_IDX(_n)	\
	((_n) == 1 ? 0x118 : 0x018)
#define RX_RING_CPU_IDX_MASK			GENMASK(11, 0)

#define REG_TX_CPU_IDX(_n)	\
	((_n) == 1 ? 0x110 : 0x010)
#define TX_RING_CPU_IDX_MASK			GENMASK(11, 0)

#define REG_TX_DMA_IDX(_n)	\
	((_n) == 1 ? 0x014 : 0x114)

#define TX_RING_DMA_IDX_MASK			GENMASK(11, 0)

/*
 * QDMA "CPU TX ring interrupt queue" (a.k.a. TX done list). Offsets per the
 * EN751221 programming guide (QDMA register summary, base +0x00):
 *   0x60 IRQ_BASE, 0x64 IRQ_CFG, 0x68 IRQ_CLRLEN, 0x6C IRQ_STATUS.
 * EN751221 has a single such queue per QDMA engine.
 */
/*
 * EN751221 has a single TX done queue per QDMA engine (index stride 0), so
 * these registers accept an index argument only to keep the airoha-derived
 * function bodies unchanged; the offset does not depend on it.
 */
#define REG_TX_IRQ_BASE(_n)			(0x0060 + 0 * (_n))

#define REG_TX_IRQ_CFG(_n)			(0x0064 + 0 * (_n))
#define TX_IRQ_THR_MASK				GENMASK(27, 16)
#define TX_IRQ_DEPTH_MASK			GENMASK(11, 0)

#define REG_IRQ_CLEAR_LEN(_n)			(0x0068 + 0 * (_n))
#define IRQ_CLEAR_LEN_MASK			GENMASK(7, 0)

#define REG_IRQ_STATUS(_n)			(0x006C + 0 * (_n))
#define IRQ_ENTRY_LEN_MASK			GENMASK(27, 16)
#define IRQ_HEAD_IDX_MASK			GENMASK(11, 0)

/* Layout of each 32-bit entry stored in the TX done queue */
#define IRQ_RING_IDX_MASK			GENMASK(20, 16)
#define IRQ_DESC_IDX_MASK			GENMASK(15, 0)

/* Both TX rings report completions through the single TX done queue */
#define TX_DONE_INT_MASK(_n)			(TX0_DONE_INT | TX1_DONE_INT)

//FIXME not all bits are valid
/* CTRL */
#define QDMA_DESC_DONE_MASK		BIT(31)
#define QDMA_DESC_DROP_MASK		BIT(30) /* tx: drop - rx: overflow */
#define QDMA_DESC_MORE_MASK		BIT(29) /* more SG elements */
#define QDMA_DESC_LEN_MASK		GENMASK(15, 0)
/* DATA */
#define QDMA_DESC_NEXT_ID_MASK		GENMASK(15, 0)
/*
 * EN751221 TX/RX descriptor message layout, per the vendor SDK ethTxMsg_t /
 * ethRxMsg_t (this differs from airoha EN7581 - only two TX msg words are
 * used, and checksum/fport/tso live in msg1, not msg0).
 */
/* TX MSG0 */
#define QDMA_ETH_TXMSG_SP_TAG_MASK	GENMASK(27, 12)
#define QDMA_ETH_TXMSG_OAM_MASK		BIT(11)
#define QDMA_ETH_TXMSG_CHAN_MASK	GENMASK(10, 3)
#define QDMA_ETH_TXMSG_QUEUE_MASK	GENMASK(2, 0)
/* TX MSG1 */
#define QDMA_ETH_TXMSG_ICO_MASK		BIT(31)
#define QDMA_ETH_TXMSG_UCO_MASK		BIT(30)
#define QDMA_ETH_TXMSG_TCO_MASK		BIT(29)
#define QDMA_ETH_TXMSG_TSO_MASK		BIT(28)
#define QDMA_ETH_TXMSG_UDF_PMAP_MASK	GENMASK(27, 22)
#define QDMA_ETH_TXMSG_FPORT_MASK	GENMASK(21, 19)
#define QDMA_ETH_TXMSG_VLAN_EN_MASK	BIT(18)
#define QDMA_ETH_TXMSG_VLAN_TPID_MASK	GENMASK(17, 16)
#define QDMA_ETH_TXMSG_VLAN_TAG_MASK	GENMASK(15, 0)

/* RX MSG1 */
#define QDMA_ETH_RXMSG_IP6_MASK		BIT(28)
#define QDMA_ETH_RXMSG_IP4_MASK		BIT(27)
#define QDMA_ETH_RXMSG_IP4F_MASK	BIT(26)
#define QDMA_ETH_RXMSG_TACK_MASK	BIT(25)
#define QDMA_ETH_RXMSG_L2VLD_MASK	BIT(24)
#define QDMA_ETH_RXMSG_L4F_MASK		BIT(23)
#define QDMA_ETH_RXMSG_SPORT_MASK	GENMASK(22, 19)
#define QDMA_ETH_RXMSG_CRSN_MASK	GENMASK(18, 14)
#define QDMA_ETH_RXMSG_PPE_ENTRY_MASK	GENMASK(13, 0)
/* RX MSG2 */
#define QDMA_ETH_RXMSG_UNTAG_MASK	BIT(0)
/* RX MSG3 carries the switch special tag (source switch port etc.) */
#define QDMA_ETH_RXMSG_SP_TAG_MASK	GENMASK(15, 0)


#define REG_TXQ_CNGST_CFG			0x00a0
#define TXQ_CNGST_DROP_EN			BIT(31)
#define TXQ_CNGST_DEI_DROP_EN			BIT(30)



struct econet_qdma_desc {
	__be32 rsv;
	__be32 ctrl;
	__be32 addr;
	__be32 data;
	__be32 msg0;
	__be32 msg1;
	__be32 msg2;
	__be32 msg3;
};

struct econet_qdma_fwd_desc {
	__be32 addr;
	__be32 ctrl0;
	__be32 msg0;
	__be32 msg1;
};

#endif /* ECONET_751221_REGS_H */
