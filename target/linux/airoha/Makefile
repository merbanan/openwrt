include $(TOPDIR)/rules.mk

ARCH:=arm
BOARD:=airoha
BOARDNAME:=Airoha ARM
SUBTARGETS:=en7523
FEATURES:=dt squashfs nand ramdisk gpio source-only

KERNEL_PATCHVER:=5.15
KERNEL_TESTING_PATCHVER:=6.1

include $(INCLUDE_DIR)/target.mk

$(eval $(call BuildTarget))
