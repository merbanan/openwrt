/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2025
 * Author: Benjamin Larsson <benjamin.larsson@genexis.eu>
 */

#ifndef ECONET_751221_ETH_H
#define ECONET_751221_ETH_H

#include <linux/debugfs.h>
#include <linux/etherdevice.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/reset.h>
#include <net/dsa.h>

#define ECONET_MAX_NUM_GDM_PORTS	2
#define ECONET_MAX_NUM_QDMA		2
#define ECONET_MAX_NUM_IRQ_BANKS	1
#define ECONET_MAX_DSA_PORTS		7
#define ECONET_MAX_NUM_RSTS		3
#define ECONET_MAX_NUM_XSI_RSTS		5
#define ECONET_MAX_MTU			2048
#define ECONET_MAX_PACKET_SIZE		2048
/* QDMA1 (LAN) can handle 8 channels and QDMA2 (WAN) can handle 32 */
#define ECONET_NUM_QOS_CHANNELS		4
#define ECONET_NUM_QOS_QUEUES		8
#define ECONET_NUM_TX_RING		2
#define ECONET_NUM_RX_RING		2
#define ECONET_NUM_NETDEV_TX_RINGS	(ECONET_NUM_TX_RING + \
					 ECONET_NUM_QOS_CHANNELS)

#define IRQ_QUEUE_LEN			1024

// FIXME, all these values are guess work
// 1024 is used in the ref sdk when using Pon in one path
// these buffers also consume memory
#define TX_DSCP_NUM			256
#define RX_DSCP_NUM(_n)			\
	((_n) ==  1 ? 1024 : 1024)

enum {
	QDMA_INT_REG_IDX0,
	QDMA_INT_REG_MAX
};

enum {
	CRSN_08 = 0x8,
	CRSN_21 = 0x15, /* KA */
	CRSN_22 = 0x16, /* hit bind and force route to CPU */
	CRSN_24 = 0x18,
	CRSN_25 = 0x19,
};

enum {
	FE_PSE_PORT_CDM1,
	FE_PSE_PORT_GDM1,
	FE_PSE_PORT_GDM2,
	FE_PSE_PORT_CDM1_HWF,
	FE_PSE_PORT_PPE,
	FE_PSE_PORT_CDM2,
	FE_PSE_PORT_CDM2_HWF,
	FE_PSE_PORT_DROP = 0x7,
};

enum {
	DEV_STATE_INITIALIZED,
};

struct econet_queue_entry {
	union {
		void *buf;
		struct sk_buff *skb;
	};
	dma_addr_t dma_addr;
	u16 dma_len;
};

struct econet_queue {
	struct econet_qdma *qdma;

	/* protect concurrent queue accesses */
	spinlock_t lock;
	struct econet_queue_entry *entry;
	struct econet_qdma_desc *desc;
	u16 head;
	u16 tail;

	int queued;
	int ndesc;
	int free_thr;
	int buf_size;

	struct napi_struct napi;
	struct page_pool *page_pool;
	struct sk_buff *skb;
};

struct econet_tx_irq_queue {
	struct econet_qdma *qdma;

	struct napi_struct napi;

	int size;
	u32 *q;
};

struct econet_hw_stats {
	/* protect concurrent hw_stats accesses */
	spinlock_t lock;
	struct u64_stats_sync syncp;

	/* get_stats64 */
	u64 rx_ok_pkts;
	u64 tx_ok_pkts;
	u64 rx_ok_bytes;
	u64 tx_ok_bytes;
	u64 rx_multicast;
	u64 rx_errors;
	u64 rx_drops;
	u64 tx_drops;
	u64 rx_crc_error;
	u64 rx_over_errors;
	/* ethtool stats */
	u64 tx_broadcast;
	u64 tx_multicast;
	u64 tx_len[7];
	u64 rx_broadcast;
	u64 rx_fragment;
	u64 rx_jabber;
	u64 rx_len[7];
};

struct econet_irq_bank {
	struct econet_qdma *qdma;

	/* protect concurrent irqmask accesses */
	spinlock_t irq_lock;
	u32 irqmask;
	int irq;
};

struct econet_qdma {
	struct econet_eth *eth;
	void __iomem *regs;

	atomic_t users;

	struct econet_irq_bank irq_bank;

	struct econet_tx_irq_queue q_tx_irq;

	struct econet_queue q_tx[ECONET_NUM_TX_RING];
	struct econet_queue q_rx[ECONET_NUM_RX_RING];
};

struct econet_gdm_port {
	struct econet_qdma *qdma;
	struct net_device *dev;
	int id;

	//struct econet_hw_stats stats;

	//DECLARE_BITMAP(qos_sq_bmap, ECONET_NUM_QOS_CHANNELS);

	/* qos stats counters */
	//u64 cpu_tx_packets;
	//u64 fwd_tx_packets;

	struct metadata_dst *dsa_meta[ECONET_MAX_DSA_PORTS];
};

struct econet_eth {
	struct device *dev;

	unsigned long state;
	void __iomem *fe_regs;


//	struct econet_ppe *ppe;
//	struct rhashtable flow_table;

	struct reset_control_bulk_data rsts[ECONET_MAX_NUM_RSTS];
//	struct reset_control_bulk_data xsi_rsts[ECONET_MAX_NUM_XSI_RSTS];

	struct net_device *napi_dev;

	struct econet_qdma qdma[ECONET_MAX_NUM_QDMA];
	struct econet_gdm_port *ports[ECONET_MAX_NUM_GDM_PORTS];
};

u32 econet_rr(void __iomem *base, u32 offset);
void econet_wr(void __iomem *base, u32 offset, u32 val);
u32 econet_rmw(void __iomem *base, u32 offset, u32 mask, u32 val);

#define econet_fe_rr(eth, offset)				\
	econet_rr((eth)->fe_regs, (offset))
#define econet_fe_wr(eth, offset, val)				\
	econet_wr((eth)->fe_regs, (offset), (val))
#define econet_fe_rmw(eth, offset, mask, val)			\
	econet_rmw((eth)->fe_regs, (offset), (mask), (val))
#define econet_fe_set(eth, offset, val)				\
	econet_rmw((eth)->fe_regs, (offset), 0, (val))
#define econet_fe_clear(eth, offset, val)			\
	econet_rmw((eth)->fe_regs, (offset), (val), 0)

#define econet_qdma_rr(qdma, offset)				\
	econet_rr((qdma)->regs, (offset))
#define econet_qdma_wr(qdma, offset, val)			\
	econet_wr((qdma)->regs, (offset), (val))
#define econet_qdma_rmw(qdma, offset, mask, val)		\
	econet_rmw((qdma)->regs, (offset), (mask), (val))
#define econet_qdma_set(qdma, offset, val)			\
	econet_rmw((qdma)->regs, (offset), 0, (val))
#define econet_qdma_clear(qdma, offset, val)			\
	econet_rmw((qdma)->regs, (offset), (val), 0)

static inline bool econet_is_lan_gdm_port(struct econet_gdm_port *port)
{
	/* GDM1 port on EN751221 SoC is connected to the lan dsa switch.
	 * GDM2 can be used as wan port
	 */
	return port->id == 1;
}

#endif /* ECONET_751221_ETH_H */
