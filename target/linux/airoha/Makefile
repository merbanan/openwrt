include $(TOPDIR)/rules.mk

ARCH:=arm
BOARD:=airoha
BOARDNAME:=Airoha ARM
SUBTARGETS:=en7523 en7581
FEATURES:=dt-overlay gpio squashfs nand ramdisk gpio source-only

KERNEL_PATCHVER:=5.15
KERNEL_TESTING_PATCHVER:=6.1

include $(INCLUDE_DIR)/target.mk
DEFAULT_PACKAGES += \
	kmod-leds-gpio kmod-gpio-button-hotplug

$(eval $(call BuildTarget))
