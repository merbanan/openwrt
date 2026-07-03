/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2025
 * Author: Benjamin Larsson <benjamin.larsson@genexis.eu>
 *
 * PPE (hardware flow offload) support for the EcoNet EN751221.
 *
 * Naming and code structure follow the mainline Airoha PPE driver
 * (drivers/net/ethernet/airoha/airoha_ppe.c by Lorenzo Bianconi). The
 * hardware model, however, is the MediaTek NETSYS-v1 PPE (80-byte FoE
 * entries, 16384-entry DRAM table, no NPU) as HW-verified on the EN751221
 * by the xr500v port; the FoE entry field layout matches the EcoNet vendor
 * SDK (foe_fdb_7512.h).
 */

#ifndef ECONET_751221_PPE_H
#define ECONET_751221_PPE_H

#include <linux/bitfield.h>
#include <linux/if_ether.h>
#include <linux/in6.h>
#include <linux/rhashtable.h>

struct econet_eth;

/* 80-byte FoE entry, 16384 entries in DRAM (mtk NETSYS-v1 / EN7512 80B mode) */
#define PPE_ENTRY_SIZE			80
#define PPE_NUM_ENTRIES			(16 * 1024)
#define PPE_HASH_MASK			(PPE_NUM_ENTRIES - 1)
#define PPE_HASH_SEED			0x12345678

enum {
	ECONET_FOE_STATE_INVALID,
	ECONET_FOE_STATE_UNBIND,
	ECONET_FOE_STATE_BIND,
	ECONET_FOE_STATE_FIN,
};

enum {
	PPE_PKT_TYPE_IPV4_HNAPT		= 0,
	PPE_PKT_TYPE_IPV4_ROUTE		= 1,
	PPE_PKT_TYPE_BRIDGE		= 2,
	PPE_PKT_TYPE_IPV4_DSLITE	= 3,
	PPE_PKT_TYPE_IPV6_ROUTE_3T	= 4,
	PPE_PKT_TYPE_IPV6_ROUTE_5T	= 5,
	PPE_PKT_TYPE_IPV6_6RD		= 7,
};

/* CPU reasons (crsn) reported in the RX descriptor, mtk_ppe values */
enum {
	PPE_CPU_REASON_HIT_UNBIND		= 0x0e,
	PPE_CPU_REASON_HIT_UNBIND_RATE_REACHED	= 0x0f,
	PPE_CPU_REASON_HIT_BIND_TTL_1		= 0x11,
	PPE_CPU_REASON_PPE_BYPASS		= 0x1e,
	PPE_CPU_REASON_INVALID			= 0x1f,
};

/* Information block 1 */
#define ECONET_FOE_IB1_UNBIND_PREBIND		BIT(24)
#define ECONET_FOE_IB1_UNBIND_PACKETS		GENMASK(23, 8)
#define ECONET_FOE_IB1_UNBIND_TIMESTAMP		GENMASK(7, 0)

#define ECONET_FOE_IB1_BIND_STATIC		BIT(31)
#define ECONET_FOE_IB1_BIND_UDP			BIT(30)
#define ECONET_FOE_IB1_BIND_STATE		GENMASK(29, 28)
#define ECONET_FOE_IB1_BIND_PACKET_TYPE		GENMASK(27, 25)
#define ECONET_FOE_IB1_BIND_TTL			BIT(24)
#define ECONET_FOE_IB1_BIND_TUNNEL_DECAP	BIT(23)
#define ECONET_FOE_IB1_BIND_PPPOE		BIT(22)
#define ECONET_FOE_IB1_BIND_VPM			GENMASK(21, 20)
#define ECONET_FOE_IB1_BIND_VLAN_LAYER		GENMASK(19, 16)
#define ECONET_FOE_IB1_BIND_KEEPALIVE		BIT(15)
#define ECONET_FOE_IB1_BIND_TIMESTAMP		GENMASK(14, 0)

/*
 * Information block 2.
 *
 * The force-egress-port field position is the #1 thing to confirm on
 * silicon: airoha names it PSE_PORT at GENMASK(8,5); mainline mtk / the
 * xr500v port use DEST_PORT at GENMASK(7,5); the EcoNet vendor SDK
 * (foe_fdb_7512.h) places fpidx at GENMASK(26,24). We follow the airoha
 * layout here per the naming convention - VERIFY against a bound-flow FoE
 * readback before trusting HW egress.
 */
#define ECONET_FOE_IB2_DSCP			GENMASK(31, 24)
#define ECONET_FOE_IB2_PORT_AG			GENMASK(23, 13)
#define ECONET_FOE_IB2_PCP			BIT(12)
#define ECONET_FOE_IB2_MULTICAST		BIT(11)
#define ECONET_FOE_IB2_FAST_PATH		BIT(10)
#define ECONET_FOE_IB2_PSE_QOS			BIT(9)
#define ECONET_FOE_IB2_PSE_PORT			GENMASK(8, 5)
#define ECONET_FOE_IB2_NBQ			GENMASK(4, 0)

#define ECONET_FOE_MAC_SMAC_ID			GENMASK(20, 16)
#define ECONET_FOE_MAC_PPPOE_ID			GENMASK(15, 0)

/* per-entry "data" word (QoS/accounting), airoha layout */
#define ECONET_FOE_ACTDP			GENMASK(31, 24)
#define ECONET_FOE_SHAPER_ID			GENMASK(23, 16)
#define ECONET_FOE_CHANNEL			GENMASK(15, 11)
#define ECONET_FOE_QID				GENMASK(10, 8)

struct econet_foe_mac_info_common {
	u16 vlan1;
	u16 etype;

	u32 dest_mac_hi;

	u16 vlan2;
	u16 dest_mac_lo;

	u32 src_mac_hi;
};

struct econet_foe_mac_info {
	struct econet_foe_mac_info_common common;

	u16 pppoe_id;
	u16 src_mac_lo;

	u32 meter;
};

struct econet_foe_bridge {
	u32 dest_mac_hi;

	u16 src_mac_hi;
	u16 dest_mac_lo;

	u32 src_mac_lo;

	u32 ib2;

	u32 rsv[5];

	u32 data;

	struct econet_foe_mac_info l2;
};

struct econet_foe_ipv4_tuple {
	u32 src_ip;
	u32 dest_ip;
	union {
		struct {
			u16 dest_port;
			u16 src_port;
		};
		struct {
			u8 protocol;
			u8 _pad[3]; /* fill with 0xa5a5a5 */
		};
		u32 ports;
	};
};

struct econet_foe_ipv4 {
	struct econet_foe_ipv4_tuple orig_tuple;

	u32 ib2;

	struct econet_foe_ipv4_tuple new_tuple;

	u32 rsv[2];

	u32 data;

	struct econet_foe_mac_info l2;
};

struct econet_foe_ipv4_dslite {
	struct econet_foe_ipv4_tuple ip4;

	u32 ib2;

	u8 flow_label[3];
	u8 priority;

	u32 rsv[4];

	u32 data;

	struct econet_foe_mac_info l2;
};

struct econet_foe_ipv6 {
	u32 src_ip[4];
	u32 dest_ip[4];

	union {
		struct {
			u16 dest_port;
			u16 src_port;
		};
		struct {
			u8 protocol;
			u8 pad[3];
		};
		u32 ports;
	};

	u32 data;

	u32 ib2;

	struct econet_foe_mac_info_common l2;

	u32 meter;
};

struct econet_foe_entry {
	union {
		struct {
			u32 ib1;
			union {
				struct econet_foe_bridge bridge;
				struct econet_foe_ipv4 ipv4;
				struct econet_foe_ipv4_dslite dslite;
				struct econet_foe_ipv6 ipv6;
				DECLARE_FLEX_ARRAY(u32, d);
			};
		};
		u8 data[PPE_ENTRY_SIZE];
	};
};

enum econet_flow_entry_type {
	FLOW_TYPE_L4,
	FLOW_TYPE_L2,
	FLOW_TYPE_L2_SUBFLOW,
};

struct econet_flow_data {
	struct ethhdr eth;

	union {
		struct {
			__be32 src_addr;
			__be32 dst_addr;
		} v4;
	};

	__be16 src_port;
	__be16 dst_port;

	struct {
		struct {
			u16 id;
			__be16 proto;
		} hdr[2];
		u8 num;
	} vlan;
	struct {
		u16 sid;
		u8 num;
	} pppoe;
};

struct econet_flow_table_entry {
	union {
		struct hlist_node list;		/* L4 flow entry */
		struct {
			struct rhash_head l2_node;	/* L2 flow entry */
			struct hlist_head l2_flows;	/* L2 subflows list */
		};
	};

	struct hlist_node l2_subflow_node;	/* L2 subflow entry */
	u32 hash;

	enum econet_flow_entry_type type;

	struct rhash_head node;
	unsigned long cookie;

	/* Must be last -- ends in a flexible-array member. */
	struct econet_foe_entry data;
};

struct econet_ppe {
	struct econet_eth *eth;

	void *foe;			/* FoE table (coherent DMA) */
	dma_addr_t foe_dma;

	struct rhashtable l2_flows;

	struct hlist_head *foe_flow;
	u16 *foe_check_time;
};

/* econet_ppe.c */
int econet_ppe_init(struct econet_eth *eth);
void econet_ppe_deinit(struct econet_eth *eth);
struct econet_foe_entry *econet_ppe_foe_get_entry(struct econet_ppe *ppe,
						  u32 hash);
void econet_ppe_check_skb(struct econet_ppe *ppe, struct sk_buff *skb,
			  u16 hash);
int econet_ppe_setup_tc_block_cb(struct econet_eth *eth, void *type_data);

/*
 * WHNAT (WiFi HW-NAT half-offload) vif registry (F1). The mt76 driver
 * registers each AP vif's netdev here (keyed by its mvif index) so the PPE
 * force-to-CPU downstream demux can mux NAT'd frames back to the right BSS.
 * See the whnat-wifi-hwnat notes; F2 (egress FoE build) and F3 (RX demux)
 * datapath integration depend on the PPE offload being active.
 */
#define ECONET_WHNAT_MAX_VIF		16	/* OEM HWNAT_WLAN_IF_MAXNUM */
#define ECONET_WHNAT_MAGIC_ETYPE	0x5678

void econet_whnat_register_vif(struct net_device *dev, int idx);
void econet_whnat_unregister_vif(int idx);
struct net_device *econet_whnat_vif_by_idx(int idx);
int econet_whnat_idx_by_mac(const u8 *mac);
void econet_whnat_flush_vifs(void);

#endif /* ECONET_751221_PPE_H */
