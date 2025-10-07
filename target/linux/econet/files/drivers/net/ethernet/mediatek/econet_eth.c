/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2025 
 * Author: Benjamin Larsson <benjamin.larsson@genexis.eu>
 */

/* Very heavily based on code written by Lorenzo Bianconi <lorenzo@kernel.org> */

#include <linux/of.h>
#include <linux/of_net.h>
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
//	int bank = qdma->irq_bank;
	unsigned long flags;

	spin_lock_irqsave(&irq_bank->irq_lock, flags);

	irq_bank->irqmask &= ~clear;
	irq_bank->irqmask |= set;
	econet_qdma_wr(qdma, REG_INT_ENABLE,
		       irq_bank->irqmask);
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

/*
static void econet_qdma_irq_disable(struct econet_irq_bank *irq_bank,
				    int index, u32 mask)
{
	econet_qdma_set_irqmask(irq_bank, index, mask, 0);
}
*/

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

static void econet_eth_set_port_fwd_cfg(struct econet_eth *eth, u32 addr, u32 val)
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
	econet_fe_wr(eth, REG_FE_VIP_EN(0), PATN_FCPU_EN_MASK | FIELD_PREP(PATN_TYPE_MASK, 2) | PATN_EN_MASK);

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
	econet_fe_rmw(eth, REG_CDM1_CRSN_QSEL(1), 0x3,
				 CDM1_QSEL_Q1L);
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
//	econet_fe_pse_ports_init(eth);

// 	econet_fe_set(eth, REG_GDM_MISC_CFG,
// 		      GDM2_RDM_ACK_WAIT_PREF_MASK |
// 		      GDM2_CHN_VLD_MODE_MASK);
// 	econet_fe_rmw(eth, REG_CDM2_FWD_CFG, CDM2_OAM_QSEL_MASK,
// 		      FIELD_PREP(CDM2_OAM_QSEL_MASK, 15));

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
//XPON_PHY_EN
//EPON_MAC_EN
//GPON_MAC_EN
	econet_qdma_irq_enable(&qdma->irq_bank, REG_INT_ENABLE,
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

//	econet_qdma_init_qos(qdma);

	econet_qdma_set(qdma, REG_TXQ_CNGST_CFG,
			TXQ_CNGST_DROP_EN | TXQ_CNGST_DEI_DROP_EN);
//	econet_qdma_init_qos_stats(qdma);

	return 0;
}


static irqreturn_t econet_irq_handler(int irq, void *dev_instance)
{
	struct econet_irq_bank *irq_bank = dev_instance;
	struct econet_qdma *qdma = irq_bank->qdma;
	u32 rx_intr_mask = 0, rx_intr;
	u32 intr;
	int i;

	intr = econet_qdma_rr(qdma, REG_INT_STATUS);
	intr &= irq_bank->irqmask;
	econet_qdma_wr(qdma, REG_INT_STATUS, intr);

	if (!test_bit(DEV_STATE_INITIALIZED, &qdma->eth->state))
		return IRQ_NONE;

	rx_intr = intr & (RX1_DONE_INT | RX0_DONE_INT);
	if (rx_intr) {
//		econet_qdma_irq_disable(irq_bank, QDMA_INT_REG_IDX0, rx_intr);
		rx_intr_mask |= rx_intr;
	}

	for (i = 0; rx_intr_mask && i < ARRAY_SIZE(qdma->q_rx); i++) {
		if (!qdma->q_rx[i].ndesc)
			continue;

		if (rx_intr_mask & BIT(i))
			napi_schedule(&qdma->q_rx[i].napi);
	}

	if (intr & (TX1_DONE_INT | TX0_DONE_INT)) {
//		if (!(intr & TX_DONE_INT_MASK(i)))
//			continue;

//		econet_qdma_irq_disable(irq_bank, QDMA_INT_REG_IDX0,
//					TX_DONE_INT_MASK);
		napi_schedule(&qdma->q_tx_irq.napi);
	}

	return IRQ_HANDLED;
}

static int econet_qdma_init_irq_bank(struct platform_device *pdev,
				      struct econet_qdma *qdma)
{
	struct econet_eth *eth = qdma->eth;
	int id = qdma - &eth->qdma[0];

	struct econet_irq_bank *irq_bank = &(qdma->irq_bank);
	int err, irq_index = 4 * id;
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

		e->buf = page_address(page) + offset;
		e->dma_addr = page_pool_get_dma_addr(page) + offset;
		e->dma_len = SKB_WITH_OVERHEAD(q->buf_size);

		val = FIELD_PREP(QDMA_DESC_LEN_MASK, e->dma_len);
		WRITE_ONCE(desc->ctrl, cpu_to_le32(val));
		WRITE_ONCE(desc->addr, cpu_to_le32(e->dma_addr));
		val = FIELD_PREP(QDMA_DESC_NEXT_ID_MASK, q->head);
		WRITE_ONCE(desc->data, cpu_to_le32(val));
		WRITE_ONCE(desc->msg0, 0);
		WRITE_ONCE(desc->msg1, 0);
		WRITE_ONCE(desc->msg2, 0);
		WRITE_ONCE(desc->msg3, 0);

		econet_qdma_rmw(qdma, REG_RX_CPU_IDX(qid),
				RX_RING_CPU_IDX_MASK,
				FIELD_PREP(RX_RING_CPU_IDX_MASK, q->head));
	}

	return nframes;
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

//	netif_napi_add(eth->napi_dev, &q->napi, econet_qdma_rx_napi_poll);

	econet_qdma_wr(qdma, REG_RX_RING_BASE(qid), dma_addr);
	thr = clamp(ndesc >> 3, 1, 32);

	if (qid==1) {
		econet_qdma_rmw(qdma, REG_RX_RING_SIZE,
				RX_RING_SIZE_MASK_1,
				FIELD_PREP(RX_RING_SIZE_MASK_1, ndesc));

		econet_qdma_rmw(qdma, REG_RX_RING_THR, RX_RING_THR_MASK(qid),
				FIELD_PREP(RX_RING_THR_MASK_1, thr));
	} else {
		econet_qdma_rmw(qdma, REG_RX_RING_SIZE,
				RX_RING_SIZE_MASK_1,
				FIELD_PREP(RX_RING_SIZE_MASK_0, ndesc));

		econet_qdma_rmw(qdma, REG_RX_RING_THR, RX_RING_THR_MASK(qid),
				FIELD_PREP(RX_RING_THR_MASK_0, thr));
	}
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

		if (!((RX1_DONE_INT | RX0_DONE_INT) & BIT(i))) {
			/* rx-queue not binded to irq */
			continue;
		}

		err = econet_qdma_init_rx_queue(&qdma->q_rx[i], qdma,
						RX_DSCP_NUM(i));
		if (err)
			return err;
	}

	return 0;
}

static int econet_qdma_init_tx_queue(struct econet_queue *q,
				     struct econet_qdma *qdma, int size)
{
	struct econet_eth *eth = qdma->eth;
	int i, qid = q - &qdma->q_tx[0];
	dma_addr_t dma_addr;

	spin_lock_init(&q->lock);
	q->ndesc = size;
	q->qdma = qdma;
	q->free_thr = 1 + MAX_SKB_FRAGS;

	q->entry = devm_kzalloc(eth->dev, q->ndesc * sizeof(*q->entry),
				GFP_KERNEL);
	if (!q->entry)
		return -ENOMEM;

	q->desc = dmam_alloc_coherent(eth->dev, q->ndesc * sizeof(*q->desc),
				      &dma_addr, GFP_KERNEL);
	if (!q->desc)
		return -ENOMEM;

	for (i = 0; i < q->ndesc; i++) {
		u32 val;

		val = FIELD_PREP(QDMA_DESC_DONE_MASK, 1);
		WRITE_ONCE(q->desc[i].ctrl, cpu_to_le32(val));
	}

	econet_qdma_wr(qdma, REG_TX_RING_BASE(qid), dma_addr);
	econet_qdma_rmw(qdma, REG_TX_CPU_IDX(qid), TX_RING_CPU_IDX_MASK,
			FIELD_PREP(TX_RING_CPU_IDX_MASK, q->head));
	econet_qdma_rmw(qdma, REG_TX_DMA_IDX(qid), TX_RING_DMA_IDX_MASK,
			FIELD_PREP(TX_RING_DMA_IDX_MASK, q->head));

	return 0;
}

static int econet_qdma_tx_irq_init(struct econet_tx_irq_queue *irq_q,
				   struct econet_qdma *qdma, int size)
{
	struct econet_eth *eth = qdma->eth;
	dma_addr_t dma_addr;

//	netif_napi_add_tx(eth->napi_dev, &irq_q->napi,
//			  econet_qdma_tx_napi_poll);
	irq_q->q = dmam_alloc_coherent(eth->dev, size * sizeof(u32),
				       &dma_addr, GFP_KERNEL);
	if (!irq_q->q)
		return -ENOMEM;

	memset(irq_q->q, 0xff, size * sizeof(u32));
	irq_q->size = size;
	irq_q->qdma = qdma;

	econet_qdma_wr(qdma, REG_TX_IRQ_BASE, dma_addr);
	econet_qdma_rmw(qdma, REG_TX_IRQ_CFG, TX_IRQ_DEPTH_MASK,
			FIELD_PREP(TX_IRQ_DEPTH_MASK, size));
	econet_qdma_rmw(qdma, REG_TX_IRQ_CFG, TX_IRQ_THR_MASK,
			FIELD_PREP(TX_IRQ_THR_MASK, 1));

	return 0;
}

static int econet_qdma_init_tx(struct econet_qdma *qdma)
{
	int i, err;

	err = econet_qdma_tx_irq_init(&qdma->q_tx_irq, qdma,
					IRQ_QUEUE_LEN);
	if (err)
		return err;

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
	int err = 0, id = qdma - &eth->qdma[0];
	const char *res;

	qdma->eth = eth;
	res = devm_kasprintf(eth->dev, GFP_KERNEL, "qdma%d", id);
	if (!res)
		return -ENOMEM;

	qdma->regs = devm_platform_ioremap_resource_byname(pdev, res);
	if (IS_ERR(qdma->regs))
		return dev_err_probe(eth->dev, PTR_ERR(qdma->regs),
				     "failed to iomap qdma%d regs\n", id);

	err = econet_qdma_init_irq_bank(pdev, qdma);
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

	err = econet_qdma_hw_init(qdma);
	if (err)
		return err;

	return err;
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

// 	err = econet_ppe_init(eth);
// 	if (err)
// 		return err;

	set_bit(DEV_STATE_INITIALIZED, &eth->state);

	return 0;
}


static int econet_dev_init(struct net_device *dev)
{
	struct econet_gdm_port *port = netdev_priv(dev);
	struct econet_eth *eth = port->qdma->eth;
	u32 pse_port;

	econet_set_macaddr(port, dev->dev_addr);

	switch (port->id) {
	case 2:
		pse_port = FE_PSE_PORT_PPE;
		break;
	default:
		pse_port = FE_PSE_PORT_PPE;
		break;
	}

	econet_eth_set_port_fwd_cfg(eth, REG_GDM_FWD_CFG(port->id), pse_port);

	return 0;
}


static netdev_tx_t econet_dev_start_xmit(struct sk_buff *skb,
					 struct net_device *dev)
{
	return NETDEV_TX_OK;
}

static int econet_dev_change_mtu(struct net_device *dev, int new_mtu)
{
	dev->mtu = new_mtu;
	return 0;
}

static const struct net_device_ops econet_netdev_ops = {
	.ndo_init		= econet_dev_init,
	.ndo_start_xmit		= econet_dev_start_xmit,
	.ndo_change_mtu		= econet_dev_change_mtu,
};

static void econet_qdma_start_napi(struct econet_qdma *qdma)
{
// 	int i;
// 
// 	for (i = 0; i < ARRAY_SIZE(qdma->q_tx_irq); i++)
// 		napi_enable(&qdma->q_tx_irq[i].napi);
// 
// 	for (i = 0; i < ARRAY_SIZE(qdma->q_rx); i++) {
// 		if (!qdma->q_rx[i].ndesc)
// 			continue;
// 
// 		napi_enable(&qdma->q_rx[i].napi);
// 	}
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
//	dev->ethtool_ops = &econet_ethtool_ops;
	dev->max_mtu = ECONET_MAX_MTU;
	dev->watchdog_timeo = 5 * HZ;
	dev->hw_features = NETIF_F_IP_CSUM | NETIF_F_RXCSUM |
			   NETIF_F_TSO6 | NETIF_F_IPV6_CSUM;

	dev->features |= dev->hw_features;
	dev->vlan_features = dev->hw_features;
	dev->dev.of_node = np;
//	dev->irq = qdma->irq_banks[0].irq;
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
//	u64_stats_init(&port->stats.syncp);
//	spin_lock_init(&port->stats.lock);
	port->qdma = qdma;
	port->dev = dev;
	port->id = id;
	eth->ports[p] = port;

// 	err = econet_metadata_dst_alloc(port);
// 	if (err)
// 		return err;

	err = register_netdev(dev);
	if (err)
		goto free_metadata_dst;

	return 0;

free_metadata_dst:
	//econet_metadata_dst_free(port);
	return err;
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

	if (IS_ERR(eth->fe_regs))
		return dev_err_probe(eth->dev, PTR_ERR(eth->fe_regs),
				     "failed to iomap fe regs\n");

	eth->rsts[0].id = "fe";
	eth->rsts[1].id = "pdma";
	eth->rsts[2].id = "qdma";
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
	strscpy(eth->napi_dev->name, "qdma_eth", sizeof(eth->napi_dev->name));
	platform_set_drvdata(pdev, eth);

	err = econet_hw_init(pdev, eth);
	if (err)
		goto error_hw_cleanup;

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
//	for (i = 0; i < ARRAY_SIZE(eth->qdma); i++)
//		econet_qdma_stop_napi(&eth->qdma[i]);
//	econet_ppe_deinit(eth);
error_hw_cleanup:
//	for (i = 0; i < ARRAY_SIZE(eth->qdma); i++)
//		econet_hw_cleanup(&eth->qdma[i]);

//	for (i = 0; i < ARRAY_SIZE(eth->ports); i++) {
//		struct econet_gdm_port *port = eth->ports[i];

//		if (port && port->dev->reg_state == NETREG_REGISTERED) {
//			unregister_netdev(port->dev);
//			econet_metadata_dst_free(port);
//		}
//	}
//	free_netdev(eth->napi_dev);
//	platform_set_drvdata(pdev, NULL);

	return err;
}

static void econet_remove(struct platform_device *pdev)
{
}

const struct of_device_id of_econet_match[] = {
	{ .compatible = "econet,en751221-eth" },
	{ /* sentinel */ }
};

static struct platform_driver econet_driver = {
	.probe = econet_probe,
	.remove_new = econet_remove,
	.driver = {
		.name = KBUILD_MODNAME,
		.of_match_table = of_econet_match,
	},
};
module_platform_driver(econet_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lorenzo Bianconi <lorenzo@kernel.org>");
MODULE_AUTHOR("Benjamin Larsson <benjamin.larsson@genexis.eu>");
MODULE_DESCRIPTION("Ethernet driver for Econet SoC");
