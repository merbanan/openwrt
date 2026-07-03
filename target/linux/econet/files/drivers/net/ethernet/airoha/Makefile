# SPDX-License-Identifier: GPL-2.0-only
#
# NOTE (econet target overlay): replaces the mainline airoha Makefile to add
# the EcoNet econet-eth driver alongside the airoha-eth driver. Keep in sync
# with the kernel's airoha Makefile.
#
obj-$(CONFIG_NET_AIROHA) += airoha-eth.o
airoha-eth-y := airoha_eth.o airoha_ppe.o
airoha-eth-$(CONFIG_DEBUG_FS) += airoha_ppe_debugfs.o
obj-$(CONFIG_NET_AIROHA_NPU) += airoha_npu.o

obj-$(CONFIG_NET_ECONET) += econet-eth.o
econet-eth-y := econet_eth.o econet_ppe.o
