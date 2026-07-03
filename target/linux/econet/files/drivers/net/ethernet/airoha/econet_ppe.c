// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2025
 * Author: Benjamin Larsson <benjamin.larsson@genexis.eu>
 *
 * PPE (hardware flow offload) engine for the EcoNet EN751221.
 *
 * Function decomposition and naming follow the mainline Airoha PPE driver
 * (airoha_ppe.c); the hardware programming follows the MediaTek NETSYS-v1
 * PPE (80-byte DRAM FoE table, no NPU) as HW-verified on the EN751221 by the
 * xr500v port, using the register values from mtk_ppe_start().
 *
 * This is the phase-1 core: it allocates the FoE table and programs the
 * engine, but leaves GLO_CFG.EN at its reset value and the GDM forwarding
 * pointed at the CPU. With an all-miss table every packet is forwarded to
 * the CPU, so the datapath is unaffected until flow offload and GDM->PPE
 * steering are wired up.
 */

#include <linux/dma-mapping.h>
#include <linux/if_ether.h>
#include <linux/platform_device.h>
#include <linux/unaligned.h>
#include <net/dsa.h>
#include <net/flow_offload.h>
#include <net/pkt_cls.h>

#include "econet_regs.h"
#include "econet_eth.h"
#include "econet_ppe.h"

static DEFINE_SPINLOCK(ppe_lock);

static const struct rhashtable_params econet_flow_table_params = {
	.head_offset = offsetof(struct econet_flow_table_entry, node),
	.key_offset = offsetof(struct econet_flow_table_entry, cookie),
	.key_len = sizeof(unsigned long),
	.automatic_shrinking = true,
};

static u32 econet_ppe_get_timestamp(struct econet_ppe *ppe)
{
	return econet_fe_rr(ppe->eth, REG_FE_FOE_TS) &
	       ECONET_FOE_IB1_BIND_TIMESTAMP;
}

static void econet_ppe_cache_clear(struct econet_eth *eth)
{
	/* Toggle the cache "clear" bit to invalidate the lookup cache after a
	 * DRAM entry write (mtk_ppe_cache_clear; bit9 HW-verified on EN751221).
	 */
	econet_fe_set(eth, REG_PPE_CACHE_CTL, PPE_CACHE_CTL_CLEAR);
	econet_fe_clear(eth, REG_PPE_CACHE_CTL, PPE_CACHE_CTL_CLEAR);
	econet_fe_set(eth, REG_PPE_CACHE_CTL, PPE_CACHE_CTL_EN);
}

static struct econet_foe_entry *
econet_ppe_foe_get_entry_locked(struct econet_ppe *ppe, u32 hash)
{
	if (hash >= PPE_NUM_ENTRIES)
		return NULL;

	return ppe->foe + hash * sizeof(struct econet_foe_entry);
}

struct econet_foe_entry *econet_ppe_foe_get_entry(struct econet_ppe *ppe,
						  u32 hash)
{
	struct econet_foe_entry *hwe;

	spin_lock_bh(&ppe_lock);
	hwe = econet_ppe_foe_get_entry_locked(ppe, hash);
	spin_unlock_bh(&ppe_lock);

	return hwe;
}

/*
 * Commit a FoE entry to the DRAM table. The barrier order is load-bearing
 * (mtk __mtk_foe_entry_commit): write the payload first, then the info block
 * ib1 last, so the engine never observes a BIND state over a half-written
 * entry. Finish with a cache flush so the next lookup sees DRAM.
 */
static void econet_foe_entry_commit(struct econet_ppe *ppe,
				    struct econet_foe_entry *e, u32 hash)
{
	u32 ts = econet_ppe_get_timestamp(ppe);
	struct econet_foe_entry *hwe;

	hwe = econet_ppe_foe_get_entry_locked(ppe, hash);
	if (!hwe)
		return;

	memcpy((u8 *)hwe + sizeof(hwe->ib1), (u8 *)e + sizeof(e->ib1),
	       PPE_ENTRY_SIZE - sizeof(hwe->ib1));
	wmb();

	e->ib1 &= ~ECONET_FOE_IB1_BIND_TIMESTAMP;
	e->ib1 |= FIELD_PREP(ECONET_FOE_IB1_BIND_TIMESTAMP, ts);
	WRITE_ONCE(hwe->ib1, e->ib1);
	dma_wmb();

	econet_ppe_cache_clear(ppe->eth);
}

/*
 * RX bind trigger. Called from the RX path when the descriptor reports
 * crsn == HIT_UNBIND_RATE_REACHED with the raw HW FoE slot (ppe_entry).
 * Rate-limited to one attempt per slot per jiffy. The pending flows are
 * populated by the (still to be added) flow-offload front-end.
 */
void econet_ppe_check_skb(struct econet_ppe *ppe, struct sk_buff *skb,
			  u16 hash)
{
	struct econet_flow_table_entry *e;
	struct econet_foe_entry *hwe;
	struct hlist_node *n;
	u32 state;

	if (hash > PPE_HASH_MASK)
		return;

	if (ppe->foe_check_time[hash] == (u16)jiffies)
		return;

	spin_lock_bh(&ppe_lock);

	hwe = econet_ppe_foe_get_entry_locked(ppe, hash);
	if (!hwe)
		goto unlock;

	state = FIELD_GET(ECONET_FOE_IB1_BIND_STATE, hwe->ib1);
	if (state == ECONET_FOE_STATE_BIND)
		goto unlock;

	ppe->foe_check_time[hash] = (u16)jiffies;

	hlist_for_each_entry_safe(e, n, &ppe->foe_flow[hash], list) {
		if (e->hash != 0xffff)
			continue;

		e->data.ib1 &= ~ECONET_FOE_IB1_BIND_STATE;
		e->data.ib1 |= FIELD_PREP(ECONET_FOE_IB1_BIND_STATE,
					  ECONET_FOE_STATE_BIND);
		econet_foe_entry_commit(ppe, &e->data, hash);
		e->hash = hash;
		break;
	}

unlock:
	spin_unlock_bh(&ppe_lock);
}

/* ---- TC / flowtable offload front-end (IPv4 L4 path) -------------------- */

/* mtk-style 5-tuple hash into the DRAM FoE table (must match the HW seed). */
static u32 econet_ppe_foe_hash(struct econet_foe_entry *e)
{
	u32 hv1, hv2, hv3, hash;

	hv1 = e->ipv4.orig_tuple.ports;
	hv2 = e->ipv4.orig_tuple.dest_ip;
	hv3 = e->ipv4.orig_tuple.src_ip;

	hash = (hv1 & hv2) | ((~hv1) & hv3);
	hash = (hash >> 24) | ((hash & 0xffffff) << 8);
	hash ^= hv1 ^ hv2 ^ hv3;
	hash ^= hash >> 16;
	hash &= PPE_NUM_ENTRIES - 1;

	return hash;
}

static void econet_ppe_flow_mangle_eth(const struct flow_action_entry *act,
				       void *eth)
{
	void *dest = eth + act->mangle.offset;
	const void *src = &act->mangle.val;

	if (act->mangle.offset > 8)
		return;

	if (act->mangle.mask == 0xffff) {
		src += 2;
		dest += 2;
	}

	memcpy(dest, src, act->mangle.mask ? 2 : 4);
}

static int econet_ppe_flow_mangle_ports(const struct flow_action_entry *act,
					struct econet_flow_data *data)
{
	u32 val = be32_to_cpu((__force __be32)act->mangle.val);

	switch (act->mangle.offset) {
	case 0:
		if ((__force __be32)act->mangle.mask == ~cpu_to_be32(0xffff))
			data->dst_port = cpu_to_be16(val);
		else
			data->src_port = cpu_to_be16(val >> 16);
		break;
	case 2:
		data->dst_port = cpu_to_be16(val);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int econet_ppe_flow_mangle_ipv4(const struct flow_action_entry *act,
				       struct econet_flow_data *data)
{
	__be32 *dest;

	switch (act->mangle.offset) {
	case offsetof(struct iphdr, saddr):
		dest = &data->v4.src_addr;
		break;
	case offsetof(struct iphdr, daddr):
		dest = &data->v4.dst_addr;
		break;
	default:
		return -EINVAL;
	}

	memcpy(dest, &act->mangle.val, sizeof(u32));

	return 0;
}

static int econet_get_dsa_port(struct net_device **dev)
{
#if IS_ENABLED(CONFIG_NET_DSA)
	struct dsa_port *dp = dsa_port_from_netdev(*dev);

	if (IS_ERR(dp))
		return -ENODEV;

	*dev = dsa_port_to_conduit(dp);
	return dp->index;
#else
	return -ENODEV;
#endif
}

static int econet_ppe_foe_entry_prepare(struct econet_eth *eth,
					struct econet_foe_entry *hwe,
					struct net_device *netdev, int type,
					struct econet_flow_data *data,
					int l4proto)
{
	u32 qdata = FIELD_PREP(ECONET_FOE_SHAPER_ID, 0x7f), val;
	int dsa_port = econet_get_dsa_port(&netdev);
	struct econet_foe_mac_info_common *l2;
	u8 smac_id = 0xf;

	memset(hwe, 0, sizeof(*hwe));

	val = FIELD_PREP(ECONET_FOE_IB1_BIND_STATE, ECONET_FOE_STATE_BIND) |
	      FIELD_PREP(ECONET_FOE_IB1_BIND_PACKET_TYPE, type) |
	      FIELD_PREP(ECONET_FOE_IB1_BIND_UDP, l4proto == IPPROTO_UDP) |
	      FIELD_PREP(ECONET_FOE_IB1_BIND_VLAN_LAYER, data->vlan.num) |
	      FIELD_PREP(ECONET_FOE_IB1_BIND_VPM, data->vlan.num) |
	      FIELD_PREP(ECONET_FOE_IB1_BIND_PPPOE, data->pppoe.num) |
	      ECONET_FOE_IB1_BIND_TTL;
	hwe->ib1 = val;

	val = FIELD_PREP(ECONET_FOE_IB2_PORT_AG, 0x1f);
	if (netdev) {
		struct econet_gdm_port *port = netdev_priv(netdev);
		u8 pse_port, channel;

		/* Egress GDM: GDM1 for LAN, GDM2 for WAN. NB: the IB2 egress
		 * port field position differs between airoha/mtk/vendor -
		 * VERIFY on HW (see econet_ppe.h).
		 */
		pse_port = port->id;

		channel = dsa_port >= 0 ? dsa_port : port->id;
		channel = channel % ECONET_NUM_QOS_CHANNELS;
		qdata |= FIELD_PREP(ECONET_FOE_CHANNEL, channel);

		val |= FIELD_PREP(ECONET_FOE_IB2_PSE_PORT, pse_port) |
		       ECONET_FOE_IB2_PSE_QOS;
		if (econet_is_lan_gdm_port(port))
			val |= ECONET_FOE_IB2_FAST_PATH;
		if (dsa_port >= 0)
			val |= FIELD_PREP(ECONET_FOE_IB2_NBQ, dsa_port);

		smac_id = port->id;
	}

	if (is_multicast_ether_addr(data->eth.h_dest))
		val |= ECONET_FOE_IB2_MULTICAST;

	if (type == PPE_PKT_TYPE_IPV4_ROUTE)
		hwe->ipv4.orig_tuple.ports = 0xa5a5a500 | (l4proto & 0xff);

	hwe->ipv4.data = qdata;
	hwe->ipv4.ib2 = val;
	l2 = &hwe->ipv4.l2.common;
	l2->etype = ETH_P_IP;

	l2->dest_mac_hi = get_unaligned_be32(data->eth.h_dest);
	l2->dest_mac_lo = get_unaligned_be16(data->eth.h_dest + 4);
	l2->src_mac_hi = get_unaligned_be32(data->eth.h_source);
	hwe->ipv4.l2.src_mac_lo = get_unaligned_be16(data->eth.h_source + 4);
	hwe->ipv4.l2.pppoe_id = data->pppoe.sid;

	if (data->vlan.num) {
		l2->vlan1 = data->vlan.hdr[0].id;
		if (data->vlan.num == 2)
			l2->vlan2 = data->vlan.hdr[1].id;
	}

	if (dsa_port >= 0) {
		/* Encode the switch user port in the FoE l2 etype
		 * (mtk_foe_entry_set_dsa). The cascaded MCM switch may also
		 * require MTK_HDR_XMIT_PASSTHROUGH (BIT(7)) - VERIFY on HW.
		 */
		l2->etype = BIT(dsa_port);
		l2->etype |= !data->vlan.num ? BIT(15) : 0;
	} else if (data->pppoe.num) {
		l2->etype = ETH_P_PPP_SES;
	}

	return 0;
}

static int econet_ppe_foe_entry_set_ipv4_tuple(struct econet_foe_entry *hwe,
					       struct econet_flow_data *data,
					       bool egress)
{
	int type = FIELD_GET(ECONET_FOE_IB1_BIND_PACKET_TYPE, hwe->ib1);
	struct econet_foe_ipv4_tuple *t;

	switch (type) {
	case PPE_PKT_TYPE_IPV4_HNAPT:
		if (egress) {
			t = &hwe->ipv4.new_tuple;
			break;
		}
		fallthrough;
	case PPE_PKT_TYPE_IPV4_ROUTE:
		t = &hwe->ipv4.orig_tuple;
		break;
	default:
		WARN_ON_ONCE(1);
		return -EINVAL;
	}

	t->src_ip = be32_to_cpu(data->v4.src_addr);
	t->dest_ip = be32_to_cpu(data->v4.dst_addr);

	if (type != PPE_PKT_TYPE_IPV4_ROUTE) {
		t->src_port = be16_to_cpu(data->src_port);
		t->dest_port = be16_to_cpu(data->dst_port);
	}

	return 0;
}

static int econet_ppe_foe_flow_commit_entry(struct econet_ppe *ppe,
					    struct econet_flow_table_entry *e)
{
	u32 hash = econet_ppe_foe_hash(&e->data);

	e->type = FLOW_TYPE_L4;
	e->hash = 0xffff;

	spin_lock_bh(&ppe_lock);
	hlist_add_head(&e->list, &ppe->foe_flow[hash]);
	spin_unlock_bh(&ppe_lock);

	return 0;
}

static void econet_ppe_foe_flow_remove_entry(struct econet_ppe *ppe,
					     struct econet_flow_table_entry *e)
{
	spin_lock_bh(&ppe_lock);

	hlist_del_init(&e->list);
	if (e->hash != 0xffff) {
		/* invalidate the bound DRAM slot */
		e->data.ib1 &= ~ECONET_FOE_IB1_BIND_STATE;
		e->data.ib1 |= FIELD_PREP(ECONET_FOE_IB1_BIND_STATE,
					  ECONET_FOE_STATE_INVALID);
		econet_foe_entry_commit(ppe, &e->data, e->hash);
		e->hash = 0xffff;
	}

	spin_unlock_bh(&ppe_lock);
}

static int econet_ppe_flow_offload_replace(struct econet_eth *eth,
					   struct flow_cls_offload *f)
{
	struct flow_rule *rule = flow_cls_offload_flow_rule(f);
	struct econet_flow_table_entry *e;
	struct econet_flow_data data = {};
	struct net_device *odev = NULL;
	struct flow_action_entry *act;
	struct econet_foe_entry hwe;
	int err, i, offload_type;
	u16 addr_type = 0;
	u8 l4proto = 0;

	if (rhashtable_lookup(&eth->flow_table, &f->cookie,
			      econet_flow_table_params))
		return -EEXIST;

	if (!flow_rule_match_key(rule, FLOW_DISSECTOR_KEY_META))
		return -EOPNOTSUPP;

	if (flow_rule_match_key(rule, FLOW_DISSECTOR_KEY_CONTROL)) {
		struct flow_match_control match;

		flow_rule_match_control(rule, &match);
		addr_type = match.key->addr_type;
		if (flow_rule_has_control_flags(match.mask->flags,
						f->common.extack))
			return -EOPNOTSUPP;
	} else {
		return -EOPNOTSUPP;
	}

	if (flow_rule_match_key(rule, FLOW_DISSECTOR_KEY_BASIC)) {
		struct flow_match_basic match;

		flow_rule_match_basic(rule, &match);
		l4proto = match.key->ip_proto;
	} else {
		return -EOPNOTSUPP;
	}

	/* IPv4 HNAPT/route only for now */
	if (addr_type != FLOW_DISSECTOR_KEY_IPV4_ADDRS)
		return -EOPNOTSUPP;
	offload_type = PPE_PKT_TYPE_IPV4_HNAPT;

	flow_action_for_each(i, act, &rule->action) {
		switch (act->id) {
		case FLOW_ACTION_MANGLE:
			if (act->mangle.htype == FLOW_ACT_MANGLE_HDR_TYPE_ETH)
				econet_ppe_flow_mangle_eth(act, &data.eth);
			break;
		case FLOW_ACTION_REDIRECT:
			odev = act->dev;
			break;
		case FLOW_ACTION_CSUM:
			break;
		case FLOW_ACTION_VLAN_PUSH:
			if (data.vlan.num == 2 ||
			    act->vlan.proto != htons(ETH_P_8021Q))
				return -EOPNOTSUPP;

			data.vlan.hdr[data.vlan.num].id = act->vlan.vid;
			data.vlan.hdr[data.vlan.num].proto = act->vlan.proto;
			data.vlan.num++;
			break;
		case FLOW_ACTION_VLAN_POP:
			break;
		case FLOW_ACTION_PPPOE_PUSH:
			if (data.pppoe.num == 1 || data.vlan.num == 2)
				return -EOPNOTSUPP;

			data.pppoe.sid = act->pppoe.sid;
			data.pppoe.num++;
			break;
		default:
			return -EOPNOTSUPP;
		}
	}

	if (!is_valid_ether_addr(data.eth.h_source) ||
	    !is_valid_ether_addr(data.eth.h_dest))
		return -EINVAL;

	err = econet_ppe_foe_entry_prepare(eth, &hwe, odev, offload_type,
					   &data, l4proto);
	if (err)
		return err;

	if (flow_rule_match_key(rule, FLOW_DISSECTOR_KEY_PORTS)) {
		struct flow_match_ports ports;

		flow_rule_match_ports(rule, &ports);
		data.src_port = ports.key->src;
		data.dst_port = ports.key->dst;
	} else {
		return -EOPNOTSUPP;
	}

	if (flow_rule_match_key(rule, FLOW_DISSECTOR_KEY_IPV4_ADDRS)) {
		struct flow_match_ipv4_addrs addrs;

		flow_rule_match_ipv4_addrs(rule, &addrs);
		data.v4.src_addr = addrs.key->src;
		data.v4.dst_addr = addrs.key->dst;
		econet_ppe_foe_entry_set_ipv4_tuple(&hwe, &data, false);
	}

	flow_action_for_each(i, act, &rule->action) {
		if (act->id != FLOW_ACTION_MANGLE)
			continue;

		switch (act->mangle.htype) {
		case FLOW_ACT_MANGLE_HDR_TYPE_TCP:
		case FLOW_ACT_MANGLE_HDR_TYPE_UDP:
			err = econet_ppe_flow_mangle_ports(act, &data);
			break;
		case FLOW_ACT_MANGLE_HDR_TYPE_IP4:
			err = econet_ppe_flow_mangle_ipv4(act, &data);
			break;
		case FLOW_ACT_MANGLE_HDR_TYPE_ETH:
			break;
		default:
			return -EOPNOTSUPP;
		}

		if (err)
			return err;
	}

	err = econet_ppe_foe_entry_set_ipv4_tuple(&hwe, &data, true);
	if (err)
		return err;

	e = kzalloc(sizeof(*e), GFP_KERNEL);
	if (!e)
		return -ENOMEM;

	e->cookie = f->cookie;
	memcpy(&e->data, &hwe, sizeof(e->data));

	err = econet_ppe_foe_flow_commit_entry(eth->ppe, e);
	if (err)
		goto free_entry;

	err = rhashtable_insert_fast(&eth->flow_table, &e->node,
				     econet_flow_table_params);
	if (err < 0)
		goto remove_foe_entry;

	return 0;

remove_foe_entry:
	econet_ppe_foe_flow_remove_entry(eth->ppe, e);
free_entry:
	kfree(e);

	return err;
}

static int econet_ppe_flow_offload_destroy(struct econet_eth *eth,
					   struct flow_cls_offload *f)
{
	struct econet_flow_table_entry *e;

	e = rhashtable_lookup(&eth->flow_table, &f->cookie,
			      econet_flow_table_params);
	if (!e)
		return -ENOENT;

	econet_ppe_foe_flow_remove_entry(eth->ppe, e);
	rhashtable_remove_fast(&eth->flow_table, &e->node,
			       econet_flow_table_params);
	kfree(e);

	return 0;
}

static int econet_ppe_flow_offload_cmd(struct econet_eth *eth,
				       struct flow_cls_offload *f)
{
	switch (f->command) {
	case FLOW_CLS_REPLACE:
		return econet_ppe_flow_offload_replace(eth, f);
	case FLOW_CLS_DESTROY:
		return econet_ppe_flow_offload_destroy(eth, f);
	case FLOW_CLS_STATS:
		/* Per-flow stats need the NPU, which EN751221 lacks. */
		return -EOPNOTSUPP;
	default:
		return -EOPNOTSUPP;
	}
}

int econet_ppe_setup_tc_block_cb(struct econet_eth *eth, void *type_data)
{
	if (!eth->ppe)
		return -EOPNOTSUPP;

	return econet_ppe_flow_offload_cmd(eth, type_data);
}

static int econet_ppe_hw_init(struct econet_ppe *ppe)
{
	struct econet_eth *eth = ppe->eth;
	u32 val;
	int err;

	/* Wait for the engine to be idle before touching its config. */
	err = read_poll_timeout(econet_fe_rr, val,
				!(val & PPE_GLO_CFG_BUSY_MASK),
				10, 10 * USEC_PER_MSEC, false, eth,
				REG_PPE_GLO_CFG);
	if (err)
		return err;

	econet_fe_wr(eth, REG_PPE_TB_BASE, ppe->foe_dma);

	/* Aging deltas (mtk_ppe_start): non-L4/UDP 60s, TCP 60s, TCP-FIN 1s. */
	econet_fe_rmw(eth, REG_PPE_BND_AGE0,
		      PPE_BIND_AGE0_DELTA_NON_L4 | PPE_BIND_AGE0_DELTA_UDP,
		      FIELD_PREP(PPE_BIND_AGE0_DELTA_NON_L4, 60) |
		      FIELD_PREP(PPE_BIND_AGE0_DELTA_UDP, 60));
	econet_fe_rmw(eth, REG_PPE_BND_AGE1,
		      PPE_BIND_AGE1_DELTA_TCP_FIN | PPE_BIND_AGE1_DELTA_TCP,
		      FIELD_PREP(PPE_BIND_AGE1_DELTA_TCP_FIN, 1) |
		      FIELD_PREP(PPE_BIND_AGE1_DELTA_TCP, 60));

	/*
	 * Table config: SEARCH_MISS = 3 (FORWARD_BUILD: build an unbind entry
	 * and forward the miss to the CPU), 80-byte entries (mode 1, per the
	 * EN7512 vendor SDK), 16384 DRAM entries.
	 */
	econet_fe_rmw(eth, REG_PPE_TB_CFG,
		      PPE_TB_CFG_SEARCH_MISS_MASK | PPE_TB_ENTRY_SIZE_MASK |
		      PPE_TB_CFG_KEEPALIVE_MASK | PPE_DRAM_TB_NUM_ENTRY_MASK,
		      FIELD_PREP(PPE_TB_CFG_SEARCH_MISS_MASK, 3) |
		      FIELD_PREP(PPE_TB_ENTRY_SIZE_MASK, 1) |
		      FIELD_PREP(PPE_DRAM_TB_NUM_ENTRY_MASK,
				 __ffs(PPE_NUM_ENTRIES >> 10)));

	econet_fe_rmw(eth, REG_PPE_BIND_RATE,
		      PPE_BIND_RATE_L2B_BIND_MASK | PPE_BIND_RATE_BIND_MASK,
		      FIELD_PREP(PPE_BIND_RATE_L2B_BIND_MASK, 0x1e) |
		      FIELD_PREP(PPE_BIND_RATE_BIND_MASK, 0x1e));

	econet_fe_wr(eth, REG_PPE_HASH_SEED, PPE_HASH_SEED);
	econet_fe_clear(eth, REG_PPE_PPE_FLOW_CFG, PPE_FLOW_CFG_IP6_6RD_MASK);
	econet_fe_set(eth, REG_PPE_PPE_FLOW_CFG,
		      PPE_FLOW_CFG_IP4_NAT_MASK | PPE_FLOW_CFG_IP4_NAPT_MASK);

	econet_ppe_cache_clear(eth);

	return 0;
}

int econet_ppe_init(struct econet_eth *eth)
{
	int size = PPE_NUM_ENTRIES * sizeof(struct econet_foe_entry);
	struct econet_ppe *ppe;
	int err;

	ppe = devm_kzalloc(eth->dev, sizeof(*ppe), GFP_KERNEL);
	if (!ppe)
		return -ENOMEM;

	ppe->eth = eth;

	ppe->foe = dmam_alloc_coherent(eth->dev, size, &ppe->foe_dma,
				       GFP_KERNEL);
	if (!ppe->foe)
		return -ENOMEM;

	ppe->foe_flow = devm_kcalloc(eth->dev, PPE_NUM_ENTRIES,
				     sizeof(*ppe->foe_flow), GFP_KERNEL);
	if (!ppe->foe_flow)
		return -ENOMEM;

	ppe->foe_check_time = devm_kcalloc(eth->dev, PPE_NUM_ENTRIES,
					   sizeof(*ppe->foe_check_time),
					   GFP_KERNEL);
	if (!ppe->foe_check_time)
		return -ENOMEM;

	err = econet_ppe_hw_init(ppe);
	if (err)
		return err;

	err = rhashtable_init(&eth->flow_table, &econet_flow_table_params);
	if (err)
		return err;

	eth->ppe = ppe;

	return 0;
}

static void econet_flow_table_free(void *ptr, void *arg)
{
	kfree(ptr);
}

void econet_ppe_deinit(struct econet_eth *eth)
{
	struct econet_ppe *ppe = eth->ppe;

	if (!ppe)
		return;

	/* Stop the engine; the devm-managed tables are freed automatically. */
	econet_fe_clear(eth, REG_PPE_GLO_CFG, PPE_GLO_CFG_EN_MASK);
	eth->ppe = NULL;

	rhashtable_free_and_destroy(&eth->flow_table, econet_flow_table_free,
				    NULL);
}
