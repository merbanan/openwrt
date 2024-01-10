DTS_DIR := $(DTS_DIR)/airoha

define Device/airoha_en7581-evb
  DEVICE_VENDOR := Airoha
  DEVICE_MODEL := EN7581 Evaluation Board
  DEVICE_DTS := en7581-evb
  DEVICE_DTS_DIR := ../dts
endef
TARGET_DEVICES += airoha_en7581-evb
