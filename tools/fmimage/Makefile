include $(TOPDIR)/rules.mk

PKG_NAME:=fmimage

include $(INCLUDE_DIR)/host-build.mk

define Host/Compile
	$(HOSTCC) $(HOST_CFLAGS) -o $(HOST_BUILD_DIR)/$(PKG_NAME) src/$(PKG_NAME).c -lz
endef

define Host/Install
	strip -s $(HOST_BUILD_DIR)/fmimage
	$(CP) $(HOST_BUILD_DIR)/$(PKG_NAME) $(STAGING_DIR_HOST)/bin/
endef

define Host/Clean
	rm -f $(STAGING_DIR_HOST)/bin/$(PKG_NAME)
endef

$(eval $(call HostBuild))
