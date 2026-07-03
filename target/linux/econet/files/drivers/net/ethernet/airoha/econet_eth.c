// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2025
 * Author: Benjamin Larsson <benjamin.larsson@genexis.eu>
 *
 * Very heavily based on the mainline Airoha ethernet driver by
 * Lorenzo Bianconi <lorenzo@kernel.org>. The QDMA datapath (RX/TX rings,
 * TX done queue, NAPI) mirrors that driver; register offsets and the
 * single-IRQ-bank layout are specific to the EcoNet EN751221 SoC.
 */

#include <linux/of.h>
#include <linux/of_net.h>
#include <linux/unaligned.h>
#include <linux/of_reserved_mem.h>
#include <linux/platform_device.h>
#include <linux/tcp.h>
#include <linux/u64_stats_sync.h>
#include <net/dst_metadata.h>
#include <net/page_pool/helpers.h>
#include <net/pkt_cls.h>
#include <uapi/linux/ppp_defs.h>

#include "econet_regs.h"
#include "econet_eth.h"
#include "econet_ppe.h"

u32 econet_rr(void __iomem *base, u32 offset)
{
	return readl(base + offset);
}

void econet_wr(void __iomem *base, u32 offset, u32 val)
{
	writel(val, base + offset);
}

u32 econet_rmw(void __iomem *base, u32 offset, u32 mask, u32 val)
{
	val |= (econet_rr(base, offset) & ~mask);
	econet_wr(base, offset, val);

	return val;
}

static void econet_qdma_set_irqmask(struct econet_irq_bank *irq_bank,
				    int index, u32 clear, u32 set)
{
	struct econet_qdma *qdma = irq_bank->qdma;
	unsigned long flags;

	if (WARN_ON_ONCE(index >= ARRAY_SIZE(irq_bank->irqmask)))
		return;

	spin_lock_irqsave(&irq_bank->irq_lock, flags);

	irq_bank->irqmask[index] &= ~clear;
	irq_bank->irqmask[index] |= set;
	econet_qdma_wr(qdma, REG_INT_ENABLE, irq_bank->irqmask[index]);
	/* Read irq_enable register in order to guarantee the update above
	 * completes in the spinlock critical section.
	 */
	econet_qdma_rr(qdma, REG_INT_ENABLE);

	spin_unlock_irqrestore(&irq_bank->irq_lock, flags);
}

static void econet_qdma_irq_enable(struct econet_irq_bank *irq_bank,
				   int index, u32 mask)
{
	econet_qdma_set_irqmask(irq_bank, index, 0, mask);
}

static void econet_qdma_irq_disable(struct econet_irq_bank *irq_bank,
				    int index, u32 mask)
{
	econet_qdma_set_irqmask(irq_bank, index, mask, 0);
}

static struct econet_gdm_port *econet_qdma_get_port(struct econet_qdma *qdma)
{
	struct econet_eth *eth = qdma->eth;
	int i;

	for (i = 0; i < ARRAY_SIZE(eth->ports); i++)
		if (eth->ports[i] && eth->ports[i]->qdma == qdma)
			return eth->ports[i];

	return NULL;
}

static void econet_set_macaddr(struct econet_gdm_port *port, const u8 *addr)
{
	struct econet_eth *eth = port->qdma->eth;
	u32 val, reg;

	reg = econet_is_lan_gdm_port(port) ? REG_GDM_MAC_LSB(1)
					   : REG_GDM_MAC_LSB(2);

	val = (addr[2] << 24) | (addr[3] << 16) | (addr[4] << 8) | addr[5];
	econet_fe_wr(eth, reg, val);

	reg = econet_is_lan_gdm_port(port) ? REG_GDM_MAC_MSB(1)
					   : REG_GDM_MAC_MSB(2);

	val = (0xf8 << 16) | (addr[0] << 8) | addr[1];
	econet_fe_wr(eth, reg, val);
}

static void econet_fe_maccr_init(struct econet_eth *eth)
{
	int p;

	for (p = 1; p <= ARRAY_SIZE(eth->ports); p++)
		econet_fe_set(eth, REG_GDM_FWD_CFG(p),
			      GDM_TCP_CKSUM | GDM_UDP_CKSUM | GDM_IP4_CKSUM |
			      GDM_DROP_CRC_ERR);

	econet_fe_rmw(eth, REG_CDM1_VLAN_CTRL, CDM1_VLAN_MASK,
		      FIELD_PREP(CDM1_VLAN_MASK, 0x8100));

	econet_fe_set(eth, REG_FE_CPORT_CFG, FE_CPORT_PAD);
}

static void econet_eth_set_port_fwd_cfg(struct econet_eth *eth, u32 addr,
					u32 val)
{
	econet_fe_rmw(eth, addr, GDM_OCFQ_MASK,
		      FIELD_PREP(GDM_OCFQ_MASK, val));
	econet_fe_rmw(eth, addr, GDM_MCFQ_MASK,
		      FIELD_PREP(GDM_MCFQ_MASK, val));
	econet_fe_rmw(eth, addr, GDM_BCFQ_MASK,
		      FIELD_PREP(GDM_BCFQ_MASK, val));
	econet_fe_rmw(eth, addr, GDM_MYMACFQ_MASK,
		      FIELD_PREP(GDM_MYMACFQ_MASK, val));
}

static void econet_fe_vip_setup(struct econet_eth *eth)
{
	econet_fe_wr(eth, REG_FE_VIP_PATN(0), 0x01);
	econet_fe_wr(eth, REG_FE_VIP_EN(0),
		     PATN_FCPU_EN_MASK | FIELD_PREP(PATN_TYPE_MASK, 2) |
		     PATN_EN_MASK);

	econet_fe_wr(eth, REG_FE_VIP_PATN(1), 0x0806);
	econet_fe_wr(eth, REG_FE_VIP_EN(1),
		     PATN_FCPU_EN_MASK | FIELD_PREP(PATN_TYPE_MASK, 0) |
		     PATN_EN_MASK);

	econet_fe_wr(eth, REG_FE_VIP_PATN(2), 0x02);
	econet_fe_wr(eth, REG_FE_VIP_EN(2),
		     PATN_FCPU_EN_MASK | FIELD_PREP(PATN_TYPE_MASK, 2) |
		     PATN_EN_MASK);

	econet_fe_wr(eth, REG_FE_VIP_PATN(3), 0x8863);
	econet_fe_wr(eth, REG_FE_VIP_EN(3),
		     PATN_FCPU_EN_MASK | FIELD_PREP(PATN_TYPE_MASK, 0) |
		     PATN_EN_MASK);

	econet_fe_wr(eth, REG_FE_VIP_PATN(4), 0xc021);
	econet_fe_wr(eth, REG_FE_VIP_EN(4),
		     PATN_FCPU_EN_MASK | PATN_SP_EN_MASK |
		     FIELD_PREP(PATN_TYPE_MASK, 1) | PATN_EN_MASK);

	econet_fe_wr(eth, REG_FE_VIP_PATN(5), 0x3a);
	econet_fe_wr(eth, REG_FE_VIP_EN(5),
		     PATN_FCPU_EN_MASK | PATN_SP_EN_MASK |
		     FIELD_PREP(PATN_TYPE_MASK, 2) | PATN_EN_MASK);
}

static void econet_fe_crsn_qsel_init(struct econet_eth *eth)
{
	/* CDM1_CRSN_QSEL */
	econet_fe_rmw(eth, REG_CDM1_CRSN_QSEL(1), 0x3, CDM1_QSEL_Q1L);
}

static int econet_fe_init(struct econet_eth *eth)
{
	econet_fe_maccr_init(eth);

	/* PSE IQ reserve */
	econet_fe_rmw(eth, REG_PSE_IQ_REV1, PSE_IQ_RES1_P2_MASK,
		      FIELD_PREP(PSE_IQ_RES1_P2_MASK, 0x20));
	econet_fe_rmw(eth, REG_PSE_IQ_REV2,
		      PSE_IQ_RES2_P5_MASK | PSE_IQ_RES2_P4_MASK,
		      FIELD_PREP(PSE_IQ_RES2_P5_MASK, 0x40));

	econet_fe_vip_setup(eth);

	econet_fe_crsn_qsel_init(eth);

	econet_fe_clear(eth, REG_FE_CPORT_CFG, FE_CPORT_DIS_GSW2FE_CRC_MASK);
	econet_fe_clear(eth, REG_FE_CPORT_CFG, FE_CPORT_QUEUE_XFC_MASK);
	econet_fe_set(eth, REG_FE_CPORT_CFG, FE_CPORT_PORT_XFC_MASK);

	econet_fe_set(eth, REG_CDM1_VLAN_CTRL, UNTAG_EN);
	econet_fe_set(eth, REG_CDM1_VLAN_CTRL, STAG_EN);

	return 0;
}

static int econet_qdma_hw_init(struct econet_qdma *qdma)
{
	/* clear pending irqs */
	econet_qdma_wr(qdma, REG_INT_STATUS, 0xffffffff);

	/* setup irqs */
	econet_qdma_irq_enable(&qdma->irq_banks[0], QDMA_INT_REG_IDX0,
			       RX1_COHERENT_EN |
			       TX1_COHERENT_EN |
			       RX0_COHERENT_EN |
			       TX0_COHERENT_EN |
			       RX_PKT_OVERFLOW_EN |
			       IRQ_FULL_EN |
			       NO_RX1_CPU_DSCP_EN |
			       RX1_DONE_EN |
			       TX1_DONE_EN |
			       NO_RX0_CPU_DSCP_EN |
			       RX0_DONE_EN |
			       TX0_DONE_EN);

	econet_qdma_wr(qdma, REG_QDMA_GLOBAL_CFG,
		       GLOBAL_CFG_RX_2B_OFFSET_MASK |
		       GLOBAL_CFG_MSG_WORD_SWAP_MASK |
		       GLOBAL_CFG_DSCP_BYTE_SWAP_MASK |
		       GLOBAL_CFG_PAYLOAD_BYTE_SWAP_MASK |
		       GLOBAL_CFG_TX_IMMEDIATE_DONE_MASK |
		       GLOBAL_CFG_IRQ_EN_MASK |
		       GLOBAL_CFG_TX_WB_DONE_MASK |
		       FIELD_PREP(GLOBAL_CFG_BURST_SIZE_MASK, 3) |
		       GLOBAL_CFG_RX_DMA_EN_MASK |
		       GLOBAL_CFG_TX_DMA_EN_MASK);

	econet_qdma_set(qdma, REG_TXQ_CNGST_CFG,
			TXQ_CNGST_DROP_EN | TXQ_CNGST_DEI_DROP_EN);

	return 0;
}

static irqreturn_t econet_irq_handler(int irq, void *dev_instance)
{
	static const u32 rx_done[ECONET_NUM_RX_RING] = {
		RX0_DONE_INT, RX1_DONE_INT,
	};
	struct econet_irq_bank *irq_bank = dev_instance;
	struct econet_qdma *qdma = irq_bank->qdma;
	u32 intr;
	int i;

	/* EN751221 has a single interrupt status/enable register per QDMA */
	intr = econet_qdma_rr(qdma, REG_INT_STATUS);
	intr &= irq_bank->irqmask[QDMA_INT_REG_IDX0];
	econet_qdma_wr(qdma, REG_INT_STATUS, intr);

	if (!test_bit(DEV_STATE_INITIALIZED, &qdma->eth->state))
		return IRQ_NONE;

	for (i = 0; i < ARRAY_SIZE(qdma->q_rx); i++) {
		if (!qdma->q_rx[i].ndesc)
			continue;

		if (intr & rx_done[i]) {
			econet_qdma_irq_disable(irq_bank, QDMA_INT_REG_IDX0,
						rx_done[i]);
			napi_schedule(&qdma->q_rx[i].napi);
		}
	}

	if (intr & (TX0_DONE_INT | TX1_DONE_INT)) {
		econet_qdma_irq_disable(irq_bank, QDMA_INT_REG_IDX0,
					TX0_DONE_INT | TX1_DONE_INT);
		napi_schedule(&qdma->q_tx_irq[0].napi);
	}

	return IRQ_HANDLED;
}

static int econet_qdma_init_irq_banks(struct platform_device *pdev,
				      struct econet_qdma *qdma)
{
	struct econet_eth *eth = qdma->eth;
	int i, id = qdma - &eth->qdma[0];

	for (i = 0; i < ARRAY_SIZE(qdma->irq_banks); i++) {
		struct econet_irq_bank *irq_bank = &qdma->irq_banks[i];
		int err, irq_index = ARRAY_SIZE(qdma->irq_banks) * id + i;
		const char *name;

		spin_lock_init(&irq_bank->irq_lock);
		irq_bank->qdma = qdma;

		irq_bank->irq = platform_get_irq(pdev, irq_index);
		if (irq_bank->irq < 0)
			return irq_bank->irq;

		name = devm_kasprintf(eth->dev, GFP_KERNEL,
				      KBUILD_MODNAME ".%d", irq_index);
		if (!name)
			return -ENOMEM;

		err = devm_request_irq(eth->dev, irq_bank->irq,
				       econet_irq_handler, IRQF_SHARED, name,
				       irq_bank);
		if (err)
			return err;
	}

	return 0;
}

static int econet_qdma_fill_rx_queue(struct econet_queue *q)
{
	struct econet_qdma *qdma = q->qdma;
	int qid = q - &qdma->q_rx[0];
	int nframes = 0;

	while (q->queued < q->ndesc - 1) {
		struct econet_queue_entry *e = &q->entry[q->head];
		struct econet_qdma_desc *desc = &q->desc[q->head];
		struct page *page;
		int offset;
		u32 val;

		page = page_pool_dev_alloc_frag(q->page_pool, &offset,
						q->buf_size);
		if (!page)
			break;

		q->head = (q->head + 1) % q->ndesc;
		q->queued++;
		nframes++;

		offset += ECONET_RX_HEADROOM;
		e->buf = page_address(page) + offset;
		e->dma_addr = page_pool_get_dma_addr(page) + offset;
		e->dma_len = SKB_WITH_OVERHEAD(ECONET_RX_LEN(q->buf_size));

		val = FIELD_PREP(QDMA_DESC_LEN_MASK, e->dma_len);
		WRITE_ONCE(desc->ctrl, cpu_to_le32(val));
		WRITE_ONCE(desc->addr, cpu_to_le32(e->dma_addr));
		val = FIELD_PREP(QDMA_DESC_NEXT_ID_MASK, q->head);
		WRITE_ONCE(desc->data, cpu_to_le32(val));
		WRITE_ONCE(desc->msg0, 0);
		WRITE_ONCE(desc->msg1, 0);
		WRITE_ONCE(desc->msg2, 0);
		WRITE_ONCE(desc->msg3, 0);
	}

	if (nframes)
		econet_qdma_rmw(qdma, REG_RX_CPU_IDX(qid), RX_RING_CPU_IDX_MASK,
				FIELD_PREP(RX_RING_CPU_IDX_MASK, q->head));

	return nframes;
}

static int econet_qdma_rx_process(struct econet_queue *q, int budget)
{
	enum dma_data_direction dir = page_pool_get_dma_dir(q->page_pool);
	struct econet_qdma *qdma = q->qdma;
	struct econet_eth *eth = qdma->eth;
	struct econet_gdm_port *port;
	int qid = q - &qdma->q_rx[0];
	int done = 0;

	port = econet_qdma_get_port(qdma);

	while (done < budget) {
		struct econet_queue_entry *e = &q->entry[q->tail];
		struct econet_qdma_desc *desc = &q->desc[q->tail];
		struct net_device *netdev;
		int data_len, len;
		struct page *page;
		u32 desc_ctrl;

		desc_ctrl = le32_to_cpu(READ_ONCE(desc->ctrl));
		if (!(desc_ctrl & QDMA_DESC_DONE_MASK))
			break;

		dma_rmb();

		q->tail = (q->tail + 1) % q->ndesc;
		q->queued--;

		dma_sync_single_for_cpu(eth->dev, e->dma_addr, e->dma_len, dir);

		page = virt_to_head_page(e->buf);
		len = FIELD_GET(QDMA_DESC_LEN_MASK, desc_ctrl);
		data_len = q->skb ? ECONET_RX_LEN(q->buf_size) : e->dma_len;
		if (!len || data_len < len)
			goto free_frag;

		if (!port)
			goto free_frag;
		netdev = port->dev;

		if (!q->skb) { /* first buffer */
			q->skb = napi_build_skb(e->buf - ECONET_RX_HEADROOM,
						q->buf_size);
			if (!q->skb)
				goto free_frag;

			skb_reserve(q->skb, ECONET_RX_HEADROOM);
			__skb_put(q->skb, len);
			skb_mark_for_recycle(q->skb);
			q->skb->dev = netdev;
			q->skb->protocol = eth_type_trans(q->skb, netdev);
			q->skb->ip_summed = CHECKSUM_UNNECESSARY;
			skb_record_rx_queue(q->skb, qid);
		} else { /* scattered frame */
			struct skb_shared_info *shinfo = skb_shinfo(q->skb);
			int nr_frags = shinfo->nr_frags;

			if (nr_frags >= ARRAY_SIZE(shinfo->frags))
				goto free_frag;

			skb_add_rx_frag(q->skb, nr_frags, page,
					e->buf - page_address(page), len,
					q->buf_size);
		}

		if (desc_ctrl & QDMA_DESC_MORE_MASK)
			continue;

		/* PPE bind trigger: the FE reports the CPU reason and the raw
		 * HW FoE slot in RX msg1. On HIT_UNBIND_RATE_REACHED bind the
		 * hot flow. Done here (after skb->dev/protocol are set) so the
		 * check_skb path never sees a half-built skb.
		 */
		if (eth->ppe) {
			u32 msg1 = le32_to_cpu(READ_ONCE(desc->msg1));
			u32 reason, ppe_entry;

			reason = FIELD_GET(QDMA_ETH_RXMSG_CRSN_MASK, msg1);
			ppe_entry = FIELD_GET(QDMA_ETH_RXMSG_PPE_ENTRY_MASK,
					      msg1);
			if (reason == PPE_CPU_REASON_HIT_UNBIND_RATE_REACHED)
				econet_ppe_check_skb(eth->ppe, q->skb,
						     ppe_entry);
		}

		/*
		 * WHNAT RX demux (F3): a frame the PPE NAT'd then force-fed to
		 * the CPU (F2 half-offload) carries a magic tag
		 * [magic_etype][vif_idx] where the VLAN header would be. Strip
		 * it and reinject to the owning AP vif so mac80211 transmits it
		 * to the client; there is no DMA path from the PPE to the WiFi
		 * chip, hence this CPU bounce. HW-empirical: the inserted-tag
		 * format must be confirmed on silicon together with the F2 FoE
		 * fields (see econet_ppe_foe_entry_prepare).
		 */
		if (eth->ppe &&
		    q->skb->protocol == htons(ECONET_WHNAT_MAGIC_ETYPE)) {
			struct net_device *vif;
			unsigned char *mac;
			u16 idx, real_etype;

			/* eth_type_trans() already pulled the 14B header incl.
			 * the magic etype; the vif idx and the original etype
			 * are the next four bytes. Rewind to the MAC header. */
			skb_push(q->skb, ETH_HLEN);
			mac = q->skb->data;
			idx = get_unaligned_be16(mac + ETH_HLEN);
			real_etype = get_unaligned_be16(mac + ETH_HLEN + 2);

			vif = econet_whnat_vif_by_idx(idx);
			if (!vif) {
				dev_kfree_skb(q->skb);
				q->skb = NULL;
				continue;
			}

			/* drop the 4-byte magic tag: slide the MACs over it */
			memmove(mac + 4, mac, 2 * ETH_ALEN);
			skb_pull(q->skb, 4);
			skb_reset_mac_header(q->skb);
			q->skb->protocol = htons(real_etype);
			q->skb->dev = vif;

			done++;
			dev_queue_xmit(q->skb);
			dev_put(vif);
			q->skb = NULL;
			continue;
		}

		done++;
		napi_gro_receive(&q->napi, q->skb);
		q->skb = NULL;
		continue;
free_frag:
		if (q->skb) {
			dev_kfree_skb(q->skb);
			q->skb = NULL;
		}
		page_pool_put_full_page(q->page_pool, page, true);
	}
	econet_qdma_fill_rx_queue(q);

	return done;
}

static int econet_qdma_rx_napi_poll(struct napi_struct *napi, int budget)
{
	struct econet_queue *q = container_of(napi, struct econet_queue, napi);
	int cur, done = 0;

	do {
		cur = econet_qdma_rx_process(q, budget - done);
		done += cur;
	} while (cur && done < budget);

	if (done < budget && napi_complete(napi)) {
		struct econet_qdma *qdma = q->qdma;
		int qid = q - &qdma->q_rx[0];
		u32 mask = qid ? RX1_DONE_INT : RX0_DONE_INT;

		econet_qdma_irq_enable(&qdma->irq_banks[0], QDMA_INT_REG_IDX0,
				       mask);
	}

	return done;
}

static int econet_qdma_init_rx_queue(struct econet_queue *q,
				     struct econet_qdma *qdma, int ndesc)
{
	const struct page_pool_params pp_params = {
		.order = 0,
		.pool_size = 256,
		.flags = PP_FLAG_DMA_MAP | PP_FLAG_DMA_SYNC_DEV,
		.dma_dir = DMA_FROM_DEVICE,
		.max_len = PAGE_SIZE,
		.nid = NUMA_NO_NODE,
		.dev = qdma->eth->dev,
		.napi = &q->napi,
	};
	struct econet_eth *eth = qdma->eth;
	int qid = q - &qdma->q_rx[0], thr;
	dma_addr_t dma_addr;

	q->buf_size = PAGE_SIZE / 2;
	q->ndesc = ndesc;
	q->qdma = qdma;

	q->entry = devm_kzalloc(eth->dev, q->ndesc * sizeof(*q->entry),
				GFP_KERNEL);
	if (!q->entry)
		return -ENOMEM;

	q->page_pool = page_pool_create(&pp_params);
	if (IS_ERR(q->page_pool)) {
		int err = PTR_ERR(q->page_pool);

		q->page_pool = NULL;
		return err;
	}

	q->desc = dmam_alloc_coherent(eth->dev, q->ndesc * sizeof(*q->desc),
				      &dma_addr, GFP_KERNEL);
	if (!q->desc)
		return -ENOMEM;

	netif_napi_add(eth->napi_dev, &q->napi, econet_qdma_rx_napi_poll);

	econet_qdma_wr(qdma, REG_RX_RING_BASE(qid), dma_addr);
	thr = clamp(ndesc >> 3, 1, 32);

	econet_qdma_rmw(qdma, REG_RX_RING_SIZE, RX_RING_SIZE_MASK(qid),
			qid ? FIELD_PREP(RX_RING_SIZE_MASK_1, ndesc)
			    : FIELD_PREP(RX_RING_SIZE_MASK_0, ndesc));
	econet_qdma_rmw(qdma, REG_RX_RING_THR, RX_RING_THR_MASK(qid),
			qid ? FIELD_PREP(RX_RING_THR_MASK_1, thr)
			    : FIELD_PREP(RX_RING_THR_MASK_0, thr));

	econet_qdma_rmw(qdma, REG_RX_DMA_IDX(qid), RX_RING_DMA_IDX_MASK,
			FIELD_PREP(RX_RING_DMA_IDX_MASK, q->head));

	econet_qdma_fill_rx_queue(q);

	return 0;
}

static int econet_qdma_init_rx(struct econet_qdma *qdma)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(qdma->q_rx); i++) {
		int err;

		err = econet_qdma_init_rx_queue(&qdma->q_rx[i], qdma,
						RX_DSCP_NUM(i));
		if (err)
			return err;
	}

	return 0;
}

static void econet_unmap_xmit_buf(struct econet_eth *eth,
				  struct econet_queue_entry *e)
{
	switch (e->dma_type) {
	case ECONET_DMA_MAP_PAGE:
		dma_unmap_page(eth->dev, e->dma_addr, e->dma_len,
			       DMA_TO_DEVICE);
		break;
	case ECONET_DMA_MAP_SINGLE:
		dma_unmap_single(eth->dev, e->dma_addr, e->dma_len,
				 DMA_TO_DEVICE);
		break;
	case ECONET_DMA_UNMAPPED:
	default:
		break;
	}
	e->dma_type = ECONET_DMA_UNMAPPED;
}

static void econet_qdma_wake_netdev_txqs(struct econet_queue *q)
{
	struct econet_qdma *qdma = q->qdma;
	struct econet_eth *eth = qdma->eth;
	int i, qid = q - &qdma->q_tx[0];

	for (i = 0; i < ARRAY_SIZE(eth->ports); i++) {
		struct econet_gdm_port *port = eth->ports[i];
		struct net_device *netdev;
		int j;

		if (!port || port->qdma != qdma)
			continue;

		/* Multiple net_device TX queues can share the same hw QDMA TX
		 * queue, so wake every net_device TX queue feeding this ring.
		 */
		netdev = port->dev;
		for (j = 0; j < netdev->num_tx_queues; j++) {
			if (econet_qdma_get_txq(qdma, j) != qid)
				continue;

			netif_wake_subqueue(netdev, j);
		}
	}
	q->txq_stopped = false;
}

static int econet_qdma_tx_napi_poll(struct napi_struct *napi, int budget)
{
	struct econet_tx_irq_queue *irq_q;
	int id, done = 0, irq_queued;
	struct econet_qdma *qdma;
	struct econet_eth *eth;
	u32 status, head;

	irq_q = container_of(napi, struct econet_tx_irq_queue, napi);
	qdma = irq_q->qdma;
	id = irq_q - &qdma->q_tx_irq[0];
	eth = qdma->eth;

	status = econet_qdma_rr(qdma, REG_IRQ_STATUS(id));
	head = FIELD_GET(IRQ_HEAD_IDX_MASK, status);
	head = head % irq_q->size;
	irq_queued = FIELD_GET(IRQ_ENTRY_LEN_MASK, status);

	while (irq_queued > 0 && done < budget) {
		u32 qid, val = irq_q->q[head];
		struct econet_qdma_desc *desc;
		struct econet_queue_entry *e;
		struct econet_queue *q;
		u32 index, desc_ctrl;
		struct sk_buff *skb;

		if (val == 0xff)
			break;

		irq_q->q[head] = 0xff; /* mark as done */
		head = (head + 1) % irq_q->size;
		irq_queued--;
		done++;

		qid = FIELD_GET(IRQ_RING_IDX_MASK, val);
		if (qid >= ARRAY_SIZE(qdma->q_tx))
			continue;

		q = &qdma->q_tx[qid];
		if (!q->ndesc)
			continue;

		index = FIELD_GET(IRQ_DESC_IDX_MASK, val);
		if (index >= q->ndesc)
			continue;

		spin_lock_bh(&q->lock);

		if (!q->queued)
			goto unlock;

		desc = &q->desc[index];
		desc_ctrl = le32_to_cpu(desc->ctrl);

		if (!(desc_ctrl & QDMA_DESC_DONE_MASK) &&
		    !(desc_ctrl & QDMA_DESC_DROP_MASK))
			goto unlock;

		e = &q->entry[index];
		skb = e->skb;
		e->skb = NULL;

		econet_unmap_xmit_buf(eth, e);
		list_add_tail(&e->list, &q->tx_list);

		WRITE_ONCE(desc->msg0, 0);
		WRITE_ONCE(desc->msg1, 0);
		q->queued--;

		if (skb) {
			struct netdev_queue *txq;

			txq = skb_get_tx_queue(skb->dev, skb);
			netdev_tx_completed_queue(txq, 1, skb->len);
			dev_kfree_skb_any(skb);
		}

		if (q->txq_stopped && q->ndesc - q->queued >= q->free_thr)
			econet_qdma_wake_netdev_txqs(q);

unlock:
		spin_unlock_bh(&q->lock);
	}

	if (done) {
		int i, len = done >> 7;

		for (i = 0; i < len; i++)
			econet_qdma_rmw(qdma, REG_IRQ_CLEAR_LEN(id),
					IRQ_CLEAR_LEN_MASK, 0x80);
		econet_qdma_rmw(qdma, REG_IRQ_CLEAR_LEN(id),
				IRQ_CLEAR_LEN_MASK, (done & 0x7f));
	}

	if (done < budget && napi_complete(napi))
		econet_qdma_irq_enable(&qdma->irq_banks[0], QDMA_INT_REG_IDX0,
				       TX_DONE_INT_MASK(id));

	return done;
}

static int econet_qdma_init_tx_queue(struct econet_queue *q,
				     struct econet_qdma *qdma, int size)
{
	struct econet_eth *eth = qdma->eth;
	int i, qid = q - &qdma->q_tx[0];
	dma_addr_t dma_addr;

	spin_lock_init(&q->lock);
	q->qdma = qdma;
	q->free_thr = 1 + MAX_SKB_FRAGS;
	INIT_LIST_HEAD(&q->tx_list);

	q->entry = devm_kzalloc(eth->dev, size * sizeof(*q->entry),
				GFP_KERNEL);
	if (!q->entry)
		return -ENOMEM;

	q->desc = dmam_alloc_coherent(eth->dev, size * sizeof(*q->desc),
				      &dma_addr, GFP_KERNEL);
	if (!q->desc)
		return -ENOMEM;

	for (i = 0; i < size; i++) {
		u32 val = FIELD_PREP(QDMA_DESC_DONE_MASK, 1);

		list_add_tail(&q->entry[i].list, &q->tx_list);
		WRITE_ONCE(q->desc[i].ctrl, cpu_to_le32(val));
	}
	q->ndesc = size;

	econet_qdma_wr(qdma, REG_TX_RING_BASE(qid), dma_addr);
	econet_qdma_rmw(qdma, REG_TX_CPU_IDX(qid), TX_RING_CPU_IDX_MASK,
			FIELD_PREP(TX_RING_CPU_IDX_MASK, 0));
	econet_qdma_rmw(qdma, REG_TX_DMA_IDX(qid), TX_RING_DMA_IDX_MASK,
			FIELD_PREP(TX_RING_DMA_IDX_MASK, 0));

	return 0;
}

static int econet_qdma_tx_irq_init(struct econet_tx_irq_queue *irq_q,
				   struct econet_qdma *qdma, int size)
{
	int id = irq_q - &qdma->q_tx_irq[0];
	struct econet_eth *eth = qdma->eth;
	dma_addr_t dma_addr;

	irq_q->q = dmam_alloc_coherent(eth->dev, size * sizeof(u32),
				       &dma_addr, GFP_KERNEL);
	if (!irq_q->q)
		return -ENOMEM;

	memset(irq_q->q, 0xff, size * sizeof(u32));
	irq_q->size = size;
	irq_q->qdma = qdma;

	netif_napi_add_tx(eth->napi_dev, &irq_q->napi,
			  econet_qdma_tx_napi_poll);

	econet_qdma_wr(qdma, REG_TX_IRQ_BASE(id), dma_addr);
	econet_qdma_rmw(qdma, REG_TX_IRQ_CFG(id), TX_IRQ_DEPTH_MASK,
			FIELD_PREP(TX_IRQ_DEPTH_MASK, size));
	econet_qdma_rmw(qdma, REG_TX_IRQ_CFG(id), TX_IRQ_THR_MASK,
			FIELD_PREP(TX_IRQ_THR_MASK, 1));

	return 0;
}

static int econet_qdma_init_tx(struct econet_qdma *qdma)
{
	int i, err;

	for (i = 0; i < ARRAY_SIZE(qdma->q_tx_irq); i++) {
		err = econet_qdma_tx_irq_init(&qdma->q_tx_irq[i], qdma,
					      IRQ_QUEUE_LEN(i));
		if (err)
			return err;
	}

	for (i = 0; i < ARRAY_SIZE(qdma->q_tx); i++) {
		err = econet_qdma_init_tx_queue(&qdma->q_tx[i], qdma,
						TX_DSCP_NUM);
		if (err)
			return err;
	}

	return 0;
}

static int econet_qdma_init_hfwd_queues(struct econet_qdma *qdma)
{
	int size, index, num_desc = HW_DSCP_NUM;
	struct econet_eth *eth = qdma->eth;
	int id = qdma - &eth->qdma[0];
	u32 status, buf_size;
	dma_addr_t dma_addr;
	const char *name;

	name = devm_kasprintf(eth->dev, GFP_KERNEL, "qdma%d-buf", id);
	if (!name)
		return -ENOMEM;

	buf_size = id ? ECONET_MAX_PACKET_SIZE / 2 : ECONET_MAX_PACKET_SIZE;
	index = of_property_match_string(eth->dev->of_node,
					 "memory-region-names", name);
	if (index >= 0) {
		struct reserved_mem *rmem;
		struct device_node *np;

		/* Consume reserved memory for hw forwarding buffers queue if
		 * available in the DTS
		 */
		np = of_parse_phandle(eth->dev->of_node, "memory-region",
				      index);
		if (!np)
			return -ENODEV;

		rmem = of_reserved_mem_lookup(np);
		of_node_put(np);
		dma_addr = rmem->base;
		/* Compute the number of hw descriptors according to the
		 * reserved memory size and the payload buffer size
		 */
		num_desc = div_u64(rmem->size, buf_size);
	} else {
		size = buf_size * num_desc;
		if (!dmam_alloc_coherent(eth->dev, size, &dma_addr,
					 GFP_KERNEL))
			return -ENOMEM;
	}

	econet_qdma_wr(qdma, REG_FWD_BUF_BASE, dma_addr);

	size = num_desc * sizeof(struct econet_qdma_fwd_desc);
	if (!dmam_alloc_coherent(eth->dev, size, &dma_addr, GFP_KERNEL))
		return -ENOMEM;

	econet_qdma_wr(qdma, REG_FWD_DSCP_BASE, dma_addr);
	/* QDMA0: 2KB. QDMA1: 1KB */
	econet_qdma_rmw(qdma, REG_HW_FWD_DSCP_CFG,
			HW_FWD_DSCP_PAYLOAD_SIZE_MASK,
			FIELD_PREP(HW_FWD_DSCP_PAYLOAD_SIZE_MASK, !!id));
	econet_qdma_rmw(qdma, REG_HW_FWD_DSCP_CFG, FWD_DSCP_LOW_THR_MASK,
			FIELD_PREP(FWD_DSCP_LOW_THR_MASK, 128));

	econet_qdma_rmw(qdma, REG_LMGR_INIT_CFG,
			LMGR_INIT_START | HW_FWD_DESC_NUM_MASK,
			FIELD_PREP(HW_FWD_DESC_NUM_MASK, num_desc) |
			LMGR_INIT_START);

	return read_poll_timeout(econet_qdma_rr, status,
				 !(status & LMGR_INIT_START), USEC_PER_MSEC,
				 30 * USEC_PER_MSEC, true, qdma,
				 REG_LMGR_INIT_CFG);
}

static int econet_qdma_init(struct platform_device *pdev,
			    struct econet_eth *eth,
			    struct econet_qdma *qdma)
{
	int err, id = qdma - &eth->qdma[0];
	const char *res;

	qdma->eth = eth;
	res = devm_kasprintf(eth->dev, GFP_KERNEL, "qdma%d", id);
	if (!res)
		return -ENOMEM;

	qdma->regs = devm_platform_ioremap_resource_byname(pdev, res);
	if (IS_ERR(qdma->regs))
		return dev_err_probe(eth->dev, PTR_ERR(qdma->regs),
				     "failed to iomap qdma%d regs\n", id);

	err = econet_qdma_init_irq_banks(pdev, qdma);
	if (err)
		return err;

	err = econet_qdma_init_rx(qdma);
	if (err)
		return err;

	err = econet_qdma_init_tx(qdma);
	if (err)
		return err;

	err = econet_qdma_init_hfwd_queues(qdma);
	if (err)
		return err;

	return econet_qdma_hw_init(qdma);
}

static int econet_hw_init(struct platform_device *pdev,
			  struct econet_eth *eth)
{
	int err, i;

	err = reset_control_bulk_assert(ARRAY_SIZE(eth->rsts), eth->rsts);
	if (err)
		return err;

	msleep(20);
	err = reset_control_bulk_deassert(ARRAY_SIZE(eth->rsts), eth->rsts);
	if (err)
		return err;

	msleep(20);
	err = econet_fe_init(eth);
	if (err)
		return err;

	for (i = 0; i < ARRAY_SIZE(eth->qdma); i++) {
		err = econet_qdma_init(pdev, eth, &eth->qdma[i]);
		if (err)
			return err;
	}

	/* PPE hardware flow offload is optional: initialise the engine in its
	 * safe (all-miss, CPU-forwarding) state, but do not fail probe if it
	 * cannot be brought up.
	 */
	err = econet_ppe_init(eth);
	if (err)
		dev_warn(eth->dev,
			 "PPE init failed (%d), continuing without hw offload\n",
			 err);

	set_bit(DEV_STATE_INITIALIZED, &eth->state);

	return 0;
}

static void econet_dev_get_hw_stats(struct econet_gdm_port *port)
{
	struct econet_eth *eth = port->qdma->eth;
	u32 val, i = 0;

	u64_stats_update_begin(&port->stats.syncp);

	/* TX */
	val = econet_fe_rr(eth, REG_FE_GDM_TX_OK_PKT_CNT_H(port->id));
	port->stats.tx_ok_pkts += ((u64)val << 32);
	val = econet_fe_rr(eth, REG_FE_GDM_TX_OK_PKT_CNT_L(port->id));
	port->stats.tx_ok_pkts += val;

	val = econet_fe_rr(eth, REG_FE_GDM_TX_OK_BYTE_CNT_H(port->id));
	port->stats.tx_ok_bytes += ((u64)val << 32);
	val = econet_fe_rr(eth, REG_FE_GDM_TX_OK_BYTE_CNT_L(port->id));
	port->stats.tx_ok_bytes += val;

	val = econet_fe_rr(eth, REG_FE_GDM_TX_ETH_DROP_CNT(port->id));
	port->stats.tx_drops += val;

	val = econet_fe_rr(eth, REG_FE_GDM_TX_ETH_BC_CNT(port->id));
	port->stats.tx_broadcast += val;

	val = econet_fe_rr(eth, REG_FE_GDM_TX_ETH_MC_CNT(port->id));
	port->stats.tx_multicast += val;

	val = econet_fe_rr(eth, REG_FE_GDM_TX_ETH_RUNT_CNT(port->id));
	port->stats.tx_len[i] += val;

	val = econet_fe_rr(eth, REG_FE_GDM_TX_ETH_E64_CNT_H(port->id));
	port->stats.tx_len[i] += ((u64)val << 32);
	val = econet_fe_rr(eth, REG_FE_GDM_TX_ETH_E64_CNT_L(port->id));
	port->stats.tx_len[i++] += val;

	val = econet_fe_rr(eth, REG_FE_GDM_TX_ETH_L64_CNT_H(port->id));
	port->stats.tx_len[i] += ((u64)val << 32);
	val = econet_fe_rr(eth, REG_FE_GDM_TX_ETH_L64_CNT_L(port->id));
	port->stats.tx_len[i++] += val;

	val = econet_fe_rr(eth, REG_FE_GDM_TX_ETH_L127_CNT_H(port->id));
	port->stats.tx_len[i] += ((u64)val << 32);
	val = econet_fe_rr(eth, REG_FE_GDM_TX_ETH_L127_CNT_L(port->id));
	port->stats.tx_len[i++] += val;

	val = econet_fe_rr(eth, REG_FE_GDM_TX_ETH_L255_CNT_H(port->id));
	port->stats.tx_len[i] += ((u64)val << 32);
	val = econet_fe_rr(eth, REG_FE_GDM_TX_ETH_L255_CNT_L(port->id));
	port->stats.tx_len[i++] += val;

	val = econet_fe_rr(eth, REG_FE_GDM_TX_ETH_L511_CNT_H(port->id));
	port->stats.tx_len[i] += ((u64)val << 32);
	val = econet_fe_rr(eth, REG_FE_GDM_TX_ETH_L511_CNT_L(port->id));
	port->stats.tx_len[i++] += val;

	val = econet_fe_rr(eth, REG_FE_GDM_TX_ETH_L1023_CNT_H(port->id));
	port->stats.tx_len[i] += ((u64)val << 32);
	val = econet_fe_rr(eth, REG_FE_GDM_TX_ETH_L1023_CNT_L(port->id));
	port->stats.tx_len[i++] += val;

	val = econet_fe_rr(eth, REG_FE_GDM_TX_ETH_LONG_CNT(port->id));
	port->stats.tx_len[i++] += val;

	/* RX */
	val = econet_fe_rr(eth, REG_FE_GDM_RX_OK_PKT_CNT_H(port->id));
	port->stats.rx_ok_pkts += ((u64)val << 32);
	val = econet_fe_rr(eth, REG_FE_GDM_RX_OK_PKT_CNT_L(port->id));
	port->stats.rx_ok_pkts += val;

	val = econet_fe_rr(eth, REG_FE_GDM_RX_OK_BYTE_CNT_H(port->id));
	port->stats.rx_ok_bytes += ((u64)val << 32);
	val = econet_fe_rr(eth, REG_FE_GDM_RX_OK_BYTE_CNT_L(port->id));
	port->stats.rx_ok_bytes += val;

	val = econet_fe_rr(eth, REG_FE_GDM_RX_ETH_DROP_CNT(port->id));
	port->stats.rx_drops += val;

	val = econet_fe_rr(eth, REG_FE_GDM_RX_ETH_BC_CNT(port->id));
	port->stats.rx_broadcast += val;

	val = econet_fe_rr(eth, REG_FE_GDM_RX_ETH_MC_CNT(port->id));
	port->stats.rx_multicast += val;

	val = econet_fe_rr(eth, REG_FE_GDM_RX_ERROR_DROP_CNT(port->id));
	port->stats.rx_errors += val;

	val = econet_fe_rr(eth, REG_FE_GDM_RX_ETH_CRC_ERR_CNT(port->id));
	port->stats.rx_crc_error += val;

	val = econet_fe_rr(eth, REG_FE_GDM_RX_OVERFLOW_DROP_CNT(port->id));
	port->stats.rx_over_errors += val;

	val = econet_fe_rr(eth, REG_FE_GDM_RX_ETH_FRAG_CNT(port->id));
	port->stats.rx_fragment += val;

	val = econet_fe_rr(eth, REG_FE_GDM_RX_ETH_JABBER_CNT(port->id));
	port->stats.rx_jabber += val;

	i = 0;
	val = econet_fe_rr(eth, REG_FE_GDM_RX_ETH_RUNT_CNT(port->id));
	port->stats.rx_len[i] += val;

	val = econet_fe_rr(eth, REG_FE_GDM_RX_ETH_E64_CNT_H(port->id));
	port->stats.rx_len[i] += ((u64)val << 32);
	val = econet_fe_rr(eth, REG_FE_GDM_RX_ETH_E64_CNT_L(port->id));
	port->stats.rx_len[i++] += val;

	val = econet_fe_rr(eth, REG_FE_GDM_RX_ETH_L64_CNT_H(port->id));
	port->stats.rx_len[i] += ((u64)val << 32);
	val = econet_fe_rr(eth, REG_FE_GDM_RX_ETH_L64_CNT_L(port->id));
	port->stats.rx_len[i++] += val;

	val = econet_fe_rr(eth, REG_FE_GDM_RX_ETH_L127_CNT_H(port->id));
	port->stats.rx_len[i] += ((u64)val << 32);
	val = econet_fe_rr(eth, REG_FE_GDM_RX_ETH_L127_CNT_L(port->id));
	port->stats.rx_len[i++] += val;

	val = econet_fe_rr(eth, REG_FE_GDM_RX_ETH_L255_CNT_H(port->id));
	port->stats.rx_len[i] += ((u64)val << 32);
	val = econet_fe_rr(eth, REG_FE_GDM_RX_ETH_L255_CNT_L(port->id));
	port->stats.rx_len[i++] += val;

	val = econet_fe_rr(eth, REG_FE_GDM_RX_ETH_L511_CNT_H(port->id));
	port->stats.rx_len[i] += ((u64)val << 32);
	val = econet_fe_rr(eth, REG_FE_GDM_RX_ETH_L511_CNT_L(port->id));
	port->stats.rx_len[i++] += val;

	val = econet_fe_rr(eth, REG_FE_GDM_RX_ETH_L1023_CNT_H(port->id));
	port->stats.rx_len[i] += ((u64)val << 32);
	val = econet_fe_rr(eth, REG_FE_GDM_RX_ETH_L1023_CNT_L(port->id));
	port->stats.rx_len[i++] += val;

	val = econet_fe_rr(eth, REG_FE_GDM_RX_ETH_LONG_CNT(port->id));
	port->stats.rx_len[i++] += val;

	u64_stats_update_end(&port->stats.syncp);
}

static void econet_update_hw_stats(struct econet_gdm_port *port)
{
	struct econet_eth *eth = port->qdma->eth;

	spin_lock(&port->stats_lock);
	econet_dev_get_hw_stats(port);
	/* Reset MIB counters */
	econet_fe_set(eth, REG_FE_GDM_MIB_CLEAR(port->id),
		      FE_GDM_MIB_RX_CLEAR_MASK | FE_GDM_MIB_TX_CLEAR_MASK);
	spin_unlock(&port->stats_lock);
}

static void econet_dev_get_stats64(struct net_device *dev,
				   struct rtnl_link_stats64 *storage)
{
	struct econet_gdm_port *port = netdev_priv(dev);
	unsigned int start;

	econet_update_hw_stats(port);
	do {
		start = u64_stats_fetch_begin(&port->stats.syncp);
		storage->rx_packets = port->stats.rx_ok_pkts;
		storage->tx_packets = port->stats.tx_ok_pkts;
		storage->rx_bytes = port->stats.rx_ok_bytes;
		storage->tx_bytes = port->stats.tx_ok_bytes;
		storage->multicast = port->stats.rx_multicast;
		storage->rx_errors = port->stats.rx_errors;
		storage->rx_dropped = port->stats.rx_drops;
		storage->tx_dropped = port->stats.tx_drops;
		storage->rx_crc_errors = port->stats.rx_crc_error;
		storage->rx_over_errors = port->stats.rx_over_errors;
	} while (u64_stats_fetch_retry(&port->stats.syncp, start));
}

static void econet_ethtool_get_drvinfo(struct net_device *dev,
				       struct ethtool_drvinfo *info)
{
	struct econet_gdm_port *port = netdev_priv(dev);
	struct econet_eth *eth = port->qdma->eth;

	strscpy(info->driver, eth->dev->driver->name, sizeof(info->driver));
	strscpy(info->bus_info, dev_name(eth->dev), sizeof(info->bus_info));
}

static void econet_ethtool_get_mac_stats(struct net_device *dev,
					 struct ethtool_eth_mac_stats *stats)
{
	struct econet_gdm_port *port = netdev_priv(dev);
	unsigned int start;

	econet_update_hw_stats(port);
	do {
		start = u64_stats_fetch_begin(&port->stats.syncp);
		stats->FramesTransmittedOK = port->stats.tx_ok_pkts;
		stats->OctetsTransmittedOK = port->stats.tx_ok_bytes;
		stats->MulticastFramesXmittedOK = port->stats.tx_multicast;
		stats->BroadcastFramesXmittedOK = port->stats.tx_broadcast;
		stats->FramesReceivedOK = port->stats.rx_ok_pkts;
		stats->OctetsReceivedOK = port->stats.rx_ok_bytes;
		stats->BroadcastFramesReceivedOK = port->stats.rx_broadcast;
	} while (u64_stats_fetch_retry(&port->stats.syncp, start));
}

static const struct ethtool_rmon_hist_range econet_ethtool_rmon_ranges[] = {
	{    0,    64 },
	{   65,   127 },
	{  128,   255 },
	{  256,   511 },
	{  512,  1023 },
	{ 1024,  1518 },
	{ 1519, 10239 },
	{},
};

static void
econet_ethtool_get_rmon_stats(struct net_device *dev,
			      struct ethtool_rmon_stats *stats,
			      const struct ethtool_rmon_hist_range **ranges)
{
	struct econet_gdm_port *port = netdev_priv(dev);
	struct econet_hw_stats *hw_stats = &port->stats;
	unsigned int start;

	BUILD_BUG_ON(ARRAY_SIZE(econet_ethtool_rmon_ranges) !=
		     ARRAY_SIZE(hw_stats->tx_len) + 1);
	BUILD_BUG_ON(ARRAY_SIZE(econet_ethtool_rmon_ranges) !=
		     ARRAY_SIZE(hw_stats->rx_len) + 1);

	*ranges = econet_ethtool_rmon_ranges;
	econet_update_hw_stats(port);
	do {
		int i;

		start = u64_stats_fetch_begin(&port->stats.syncp);
		stats->fragments = hw_stats->rx_fragment;
		stats->jabbers = hw_stats->rx_jabber;
		for (i = 0; i < ARRAY_SIZE(econet_ethtool_rmon_ranges) - 1;
		     i++) {
			stats->hist[i] = hw_stats->rx_len[i];
			stats->hist_tx[i] = hw_stats->tx_len[i];
		}
	} while (u64_stats_fetch_retry(&port->stats.syncp, start));
}

static const struct ethtool_ops econet_ethtool_ops = {
	.get_drvinfo		= econet_ethtool_get_drvinfo,
	.get_eth_mac_stats	= econet_ethtool_get_mac_stats,
	.get_rmon_stats		= econet_ethtool_get_rmon_stats,
};

static int econet_dev_init(struct net_device *dev)
{
	struct econet_gdm_port *port = netdev_priv(dev);

	econet_set_macaddr(port, dev->dev_addr);

	return 0;
}

static int econet_dev_open(struct net_device *dev)
{
	struct econet_gdm_port *port = netdev_priv(dev);
	struct econet_eth *eth = port->qdma->eth;
	u32 pse_port;

	netif_tx_start_all_queues(dev);

	if (netdev_uses_dsa(dev))
		econet_fe_set(eth, REG_GDM_VLAN_CHK(port->id), GDM_STAG_EN_MASK);
	else
		econet_fe_clear(eth, REG_GDM_VLAN_CHK(port->id),
				GDM_STAG_EN_MASK);

	/*
	 * With PPE offload active, steer ingress to the PPE (it builds an
	 * unbind entry on a miss and forwards it to the CPU, so unmatched
	 * traffic still reaches the stack). Otherwise forward received frames
	 * straight to the CPU port feeding this GDM's QDMA engine
	 * (GDM1->CDM1, GDM2->CDM2). Steering only happens once the engine is
	 * enabled (same gate, set in hw_init) to avoid black-holing forwards.
	 */
	if (econet_ppe_offload_enabled() && eth->ppe)
		pse_port = FE_PSE_PORT_PPE;
	else
		pse_port = econet_is_lan_gdm_port(port) ? FE_PSE_PORT_CDM1
							: FE_PSE_PORT_CDM2;
	econet_eth_set_port_fwd_cfg(eth, REG_GDM_FWD_CFG(port->id), pse_port);

	/* The GDM<->switch link is fixed (phy-mode "internal", fixed-link),
	 * so there is no PHY to negotiate with on this side: bring the
	 * carrier up unconditionally. The switch user-port PHYs are handled
	 * by the DSA switch driver.
	 */
	netif_carrier_on(dev);

	return 0;
}

static int econet_dev_stop(struct net_device *dev)
{
	struct econet_gdm_port *port = netdev_priv(dev);
	struct econet_eth *eth = port->qdma->eth;

	netif_carrier_off(dev);
	netif_tx_disable(dev);
	econet_eth_set_port_fwd_cfg(eth, REG_GDM_FWD_CFG(port->id),
				    FE_PSE_PORT_DROP);

	return 0;
}

static netdev_tx_t econet_dev_start_xmit(struct sk_buff *skb,
					 struct net_device *dev)
{
	struct econet_gdm_port *port = netdev_priv(dev);
	struct econet_qdma *qdma = port->qdma;
	struct econet_eth *eth = qdma->eth;
	u32 nr_frags, msg0, msg1, len;
	struct econet_queue_entry *e;
	struct netdev_queue *txq;
	struct econet_queue *q;
	LIST_HEAD(tx_list);
	dma_addr_t addr;
	int i = 0, qid;
	u16 index;
	u8 fport;

	qid = econet_qdma_get_txq(qdma, skb_get_queue_mapping(skb));

	/* EN751221 uses software special-tagging (mainline tag_mtk): the MTK
	 * special tag stays inside the skb and the switch reads it directly,
	 * so no descriptor sp_tag is programmed here.
	 */
	msg0 = FIELD_PREP(QDMA_ETH_TXMSG_QUEUE_MASK,
			  qid % ECONET_NUM_QOS_QUEUES) |
	       FIELD_PREP(QDMA_ETH_TXMSG_CHAN_MASK,
			  qid / ECONET_NUM_QOS_QUEUES);

	/* GDM1 == FE_PSE_PORT_GDM1, GDM2 == FE_PSE_PORT_GDM2 */
	fport = port->id;
	msg1 = FIELD_PREP(QDMA_ETH_TXMSG_FPORT_MASK, fport);

	if (skb->ip_summed == CHECKSUM_PARTIAL)
		msg1 |= QDMA_ETH_TXMSG_TCO_MASK | QDMA_ETH_TXMSG_UCO_MASK |
			QDMA_ETH_TXMSG_ICO_MASK;

	/* TSO: fill MSS info in tcp checksum field */
	if (skb_is_gso(skb)) {
		if (skb_cow_head(skb, 0))
			goto error;

		if (skb_shinfo(skb)->gso_type & (SKB_GSO_TCPV4 |
						 SKB_GSO_TCPV6)) {
			__be16 csum = cpu_to_be16(skb_shinfo(skb)->gso_size);

			tcp_hdr(skb)->check = (__force __sum16)csum;
			msg1 |= QDMA_ETH_TXMSG_TSO_MASK;
		}
	}

	q = &qdma->q_tx[qid];
	if (WARN_ON_ONCE(!q->ndesc))
		goto error;

	spin_lock_bh(&q->lock);

	txq = netdev_get_tx_queue(dev, qid);
	nr_frags = 1 + skb_shinfo(skb)->nr_frags;

	if (q->queued + nr_frags > q->ndesc) {
		/* not enough space in the queue */
		netif_tx_stop_queue(txq);
		q->txq_stopped = true;
		spin_unlock_bh(&q->lock);
		return NETDEV_TX_BUSY;
	}

	e = list_first_entry(&q->tx_list, struct econet_queue_entry, list);
	len = skb_headlen(skb);
	addr = dma_map_single(eth->dev, skb->data, len, DMA_TO_DEVICE);
	if (unlikely(dma_mapping_error(eth->dev, addr)))
		goto error_unlock;

	e->dma_type = ECONET_DMA_MAP_SINGLE;
	index = e - q->entry;

	while (true) {
		struct econet_qdma_desc *desc = &q->desc[index];
		skb_frag_t *frag = &skb_shinfo(skb)->frags[i];
		u32 val;

		list_move_tail(&e->list, &tx_list);
		e->skb = i == nr_frags - 1 ? skb : NULL;
		e->dma_addr = addr;
		e->dma_len = len;

		e = list_first_entry(&q->tx_list, struct econet_queue_entry,
				     list);
		index = e - q->entry;

		val = FIELD_PREP(QDMA_DESC_LEN_MASK, len);
		if (i < nr_frags - 1)
			val |= FIELD_PREP(QDMA_DESC_MORE_MASK, 1);
		WRITE_ONCE(desc->ctrl, cpu_to_le32(val));
		WRITE_ONCE(desc->addr, cpu_to_le32(addr));
		val = FIELD_PREP(QDMA_DESC_NEXT_ID_MASK, index);
		WRITE_ONCE(desc->data, cpu_to_le32(val));
		WRITE_ONCE(desc->msg0, cpu_to_le32(msg0));
		WRITE_ONCE(desc->msg1, cpu_to_le32(msg1));
		/* EN751221 only uses two TX message words */
		WRITE_ONCE(desc->msg2, 0);

		if (++i == nr_frags)
			break;

		len = skb_frag_size(frag);
		addr = skb_frag_dma_map(eth->dev, frag, 0, len, DMA_TO_DEVICE);
		if (unlikely(dma_mapping_error(eth->dev, addr)))
			goto error_unmap;

		e->dma_type = ECONET_DMA_MAP_PAGE;
	}
	q->queued += i;

	skb_tx_timestamp(skb);
	netdev_tx_sent_queue(txq, skb->len);
	if (q->ndesc - q->queued < q->free_thr) {
		netif_tx_stop_queue(txq);
		q->txq_stopped = true;
	}

	if (netif_xmit_stopped(txq) || !netdev_xmit_more())
		econet_qdma_rmw(qdma, REG_TX_CPU_IDX(qid),
				TX_RING_CPU_IDX_MASK,
				FIELD_PREP(TX_RING_CPU_IDX_MASK, index));

	spin_unlock_bh(&q->lock);

	return NETDEV_TX_OK;

error_unmap:
	list_for_each_entry(e, &tx_list, list)
		econet_unmap_xmit_buf(eth, e);
	list_splice(&tx_list, &q->tx_list);
error_unlock:
	spin_unlock_bh(&q->lock);
error:
	dev_kfree_skb_any(skb);
	dev->stats.tx_dropped++;

	return NETDEV_TX_OK;
}

static int econet_dev_change_mtu(struct net_device *dev, int new_mtu)
{
	dev->mtu = new_mtu;
	return 0;
}

static int econet_dev_setup_tc_block_cb(enum tc_setup_type type,
					void *type_data, void *cb_priv)
{
	struct net_device *dev = cb_priv;
	struct econet_gdm_port *port = netdev_priv(dev);
	struct econet_eth *eth = port->qdma->eth;

	if (!tc_can_offload(dev))
		return -EOPNOTSUPP;

	if (type != TC_SETUP_CLSFLOWER)
		return -EOPNOTSUPP;

	return econet_ppe_setup_tc_block_cb(eth, type_data);
}

static int econet_dev_setup_tc_block(struct net_device *dev,
				     struct flow_block_offload *f)
{
	flow_setup_cb_t *cb = econet_dev_setup_tc_block_cb;
	static LIST_HEAD(block_cb_list);
	struct flow_block_cb *block_cb;

	if (f->binder_type != FLOW_BLOCK_BINDER_TYPE_CLSACT_INGRESS)
		return -EOPNOTSUPP;

	f->driver_block_list = &block_cb_list;
	switch (f->command) {
	case FLOW_BLOCK_BIND:
		block_cb = flow_block_cb_lookup(f->block, cb, dev);
		if (block_cb) {
			flow_block_cb_incref(block_cb);
			return 0;
		}
		block_cb = flow_block_cb_alloc(cb, dev, dev, NULL);
		if (IS_ERR(block_cb))
			return PTR_ERR(block_cb);

		flow_block_cb_incref(block_cb);
		flow_block_cb_add(block_cb, f);
		list_add_tail(&block_cb->driver_list, &block_cb_list);
		return 0;
	case FLOW_BLOCK_UNBIND:
		block_cb = flow_block_cb_lookup(f->block, cb, dev);
		if (!block_cb)
			return -ENOENT;

		if (!flow_block_cb_decref(block_cb)) {
			flow_block_cb_remove(block_cb, f);
			list_del(&block_cb->driver_list);
		}
		return 0;
	default:
		return -EOPNOTSUPP;
	}
}

static int econet_dev_setup_tc(struct net_device *dev, enum tc_setup_type type,
			       void *type_data)
{
	switch (type) {
	case TC_SETUP_BLOCK:
	case TC_SETUP_FT:
		return econet_dev_setup_tc_block(dev, type_data);
	default:
		return -EOPNOTSUPP;
	}
}

static const struct net_device_ops econet_netdev_ops = {
	.ndo_init		= econet_dev_init,
	.ndo_open		= econet_dev_open,
	.ndo_stop		= econet_dev_stop,
	.ndo_start_xmit		= econet_dev_start_xmit,
	.ndo_get_stats64	= econet_dev_get_stats64,
	.ndo_setup_tc		= econet_dev_setup_tc,
	.ndo_change_mtu		= econet_dev_change_mtu,
};

static void econet_qdma_start_napi(struct econet_qdma *qdma)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(qdma->q_tx_irq); i++)
		napi_enable(&qdma->q_tx_irq[i].napi);

	for (i = 0; i < ARRAY_SIZE(qdma->q_rx); i++) {
		if (!qdma->q_rx[i].ndesc)
			continue;

		napi_enable(&qdma->q_rx[i].napi);
	}
}

static void econet_qdma_stop_napi(struct econet_qdma *qdma)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(qdma->q_tx_irq); i++)
		napi_disable(&qdma->q_tx_irq[i].napi);

	for (i = 0; i < ARRAY_SIZE(qdma->q_rx); i++) {
		if (!qdma->q_rx[i].ndesc)
			continue;

		napi_disable(&qdma->q_rx[i].napi);
	}
}

static int econet_alloc_gdm_port(struct econet_eth *eth,
				 struct device_node *np, int index)
{
	const __be32 *id_ptr = of_get_property(np, "reg", NULL);
	struct econet_gdm_port *port;
	struct econet_qdma *qdma;
	struct net_device *dev;
	int err, p;
	u32 id;

	if (!id_ptr) {
		dev_err(eth->dev, "missing gdm port id\n");
		return -EINVAL;
	}

	id = be32_to_cpup(id_ptr);
	p = id - 1;

	if (!id || id > ARRAY_SIZE(eth->ports)) {
		dev_err(eth->dev, "invalid gdm port id: %d\n", id);
		return -EINVAL;
	}

	if (eth->ports[p]) {
		dev_err(eth->dev, "duplicate gdm port id: %d\n", id);
		return -EINVAL;
	}

	dev = devm_alloc_etherdev_mqs(eth->dev, sizeof(*port),
				      ECONET_NUM_NETDEV_TX_RINGS,
				      ECONET_NUM_RX_RING);
	if (!dev) {
		dev_err(eth->dev, "alloc_etherdev failed\n");
		return -ENOMEM;
	}

	qdma = &eth->qdma[index % ECONET_MAX_NUM_QDMA];
	dev->netdev_ops = &econet_netdev_ops;
	dev->ethtool_ops = &econet_ethtool_ops;
	dev->max_mtu = ECONET_MAX_MTU;
	dev->watchdog_timeo = 5 * HZ;
	dev->hw_features = NETIF_F_IP_CSUM | NETIF_F_RXCSUM |
			   NETIF_F_TSO6 | NETIF_F_IPV6_CSUM;
	if (eth->ppe)
		dev->hw_features |= NETIF_F_HW_TC;

	dev->features |= dev->hw_features;
	dev->vlan_features = dev->hw_features;
	dev->dev.of_node = np;
	SET_NETDEV_DEV(dev, eth->dev);

	/* reserve hw queues for HTB offloading */
	err = netif_set_real_num_tx_queues(dev, ECONET_NUM_TX_RING);
	if (err)
		return err;

	err = of_get_ethdev_address(np, dev);
	if (err) {
		if (err == -EPROBE_DEFER)
			return err;

		eth_hw_addr_random(dev);
		dev_info(eth->dev, "generated random MAC address %pM\n",
			 dev->dev_addr);
	}

	port = netdev_priv(dev);
	u64_stats_init(&port->stats.syncp);
	spin_lock_init(&port->stats_lock);
	port->qdma = qdma;
	port->dev = dev;
	port->id = id;
	eth->ports[p] = port;

	err = register_netdev(dev);
	if (err)
		return err;

#ifdef CONFIG_BQL
	/* The QDMA signals TX-done per packet (the TX done-queue interrupt
	 * threshold is 1), so BQL's dynamic queue limit auto-tunes down to
	 * ~1 packet (~86 bytes); the resulting per-packet XOFF/XON serialises
	 * TX to a few Mbps. Floor the BQL limit so the pipe stays full.
	 */
	{
		unsigned int i;

		for (i = 0; i < dev->num_tx_queues; i++)
			netdev_get_tx_queue(dev, i)->dql.min_limit = 262144;
	}
#endif

	return 0;
}

static int econet_probe(struct platform_device *pdev)
{
	struct device_node *np;
	struct econet_eth *eth;
	int i, err;

	eth = devm_kzalloc(&pdev->dev, sizeof(*eth), GFP_KERNEL);
	if (!eth)
		return -ENOMEM;

	eth->dev = &pdev->dev;

	err = dma_set_mask_and_coherent(eth->dev, DMA_BIT_MASK(32));
	if (err) {
		dev_err(eth->dev, "failed configuring DMA mask\n");
		return err;
	}

	eth->fe_regs = devm_platform_ioremap_resource_byname(pdev, "fe");
	if (IS_ERR(eth->fe_regs))
		return dev_err_probe(eth->dev, PTR_ERR(eth->fe_regs),
				     "failed to iomap fe regs\n");

	eth->rsts[0].id = "fe";
	eth->rsts[1].id = "qdma0";
	eth->rsts[2].id = "qdma1";
	err = devm_reset_control_bulk_get_exclusive(eth->dev,
						    ARRAY_SIZE(eth->rsts),
						    eth->rsts);
	if (err) {
		dev_err(eth->dev, "failed to get bulk reset lines\n");
		return err;
	}

	eth->napi_dev = alloc_netdev_dummy(0);
	if (!eth->napi_dev)
		return -ENOMEM;

	/* Enable threaded NAPI by default */
	eth->napi_dev->threaded = true;
	strscpy(eth->napi_dev->name, "qdma_eth",
		sizeof(eth->napi_dev->name));
	platform_set_drvdata(pdev, eth);

	err = econet_hw_init(pdev, eth);
	if (err)
		goto error_napi_stop;

	for (i = 0; i < ARRAY_SIZE(eth->qdma); i++)
		econet_qdma_start_napi(&eth->qdma[i]);

	i = 0;
	for_each_child_of_node(pdev->dev.of_node, np) {
		if (!of_device_is_compatible(np, "econet,eth-mac"))
			continue;

		if (!of_device_is_available(np))
			continue;

		err = econet_alloc_gdm_port(eth, np, i++);
		if (err) {
			of_node_put(np);
			goto error_napi_stop;
		}
	}

	return 0;

error_napi_stop:
	for (i = 0; i < ARRAY_SIZE(eth->qdma); i++)
		econet_qdma_stop_napi(&eth->qdma[i]);

	for (i = 0; i < ARRAY_SIZE(eth->ports); i++) {
		struct econet_gdm_port *port = eth->ports[i];

		if (port && port->dev->reg_state == NETREG_REGISTERED)
			unregister_netdev(port->dev);
	}

	if (eth->napi_dev)
		free_netdev(eth->napi_dev);
	platform_set_drvdata(pdev, NULL);

	return err;
}

static void econet_remove(struct platform_device *pdev)
{
	struct econet_eth *eth = platform_get_drvdata(pdev);
	int i;

	if (!eth)
		return;

	/* Drop WHNAT vif holds so mt76 (if still loaded) does not leak them. */
	econet_whnat_flush_vifs();

	econet_ppe_deinit(eth);

	for (i = 0; i < ARRAY_SIZE(eth->qdma); i++)
		econet_qdma_stop_napi(&eth->qdma[i]);

	for (i = 0; i < ARRAY_SIZE(eth->ports); i++) {
		struct econet_gdm_port *port = eth->ports[i];

		if (port && port->dev->reg_state == NETREG_REGISTERED)
			unregister_netdev(port->dev);
	}

	free_netdev(eth->napi_dev);
	platform_set_drvdata(pdev, NULL);
}

const struct of_device_id of_econet_match[] = {
	{ .compatible = "econet,en751221-eth" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, of_econet_match);

static struct platform_driver econet_driver = {
	.probe = econet_probe,
	.remove = econet_remove,
	.driver = {
		.name = KBUILD_MODNAME,
		.of_match_table = of_econet_match,
	},
};
module_platform_driver(econet_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Lorenzo Bianconi <lorenzo@kernel.org>");
MODULE_AUTHOR("Benjamin Larsson <benjamin.larsson@genexis.eu>");
MODULE_DESCRIPTION("Ethernet driver for Econet SoC");
