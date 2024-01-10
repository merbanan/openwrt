ARCH:=aarch64
SUBTARGET:=en7581
BOARDNAME:=EN7581
CPU_TYPE:=cortex-a53
DEFAULT_PACKAGES += wpad-basic-mbedtls uboot-envtools
KERNELNAME:=Image dtbs

define Target/Description
	Build firmware images for Airoha mt7581 ARM based boards.
endef
