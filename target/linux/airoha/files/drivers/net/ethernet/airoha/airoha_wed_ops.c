// SPDX-License-Identifier: GPL-2.0-only
/*
 * Export the mtk_soc_wed_ops pointer used by mt76 WiFi drivers to
 * attach to WED.  This object is compiled as a standalone module so it
 * can be inserted before any WiFi driver loads, even if airoha-eth is
 * built as a module too.
 *
 * Mirrors drivers/net/ethernet/mediatek/mtk_wed_ops.c
 */

#include <linux/soc/mediatek/mtk_wed.h>

const struct mtk_wed_ops __rcu *mtk_soc_wed_ops;
EXPORT_SYMBOL_GPL(mtk_soc_wed_ops);
