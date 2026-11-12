include $(TOPDIR)/rules.mk

PKG_NAME:=vedge1000-sha-bench
PKG_VERSION:=1.0
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/vedge1000-sha-bench
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=vEdge 1000 SHA hardware benchmark
  DEPENDS:=@TARGET_octeon +libopenssl
endef

define Package/vedge1000-sha-bench/description
  Verify the Octeon SHA-1 and SHA-256 drivers through AF_ALG and compare
  their performance with the OpenSSL software implementation.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/sha-bench.c $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(TARGET_CC) $(TARGET_CFLAGS) $(TARGET_CPPFLAGS) -Wall -Wextra \
		-o $(PKG_BUILD_DIR)/vedge1000-sha-bench $(PKG_BUILD_DIR)/sha-bench.c \
		$(TARGET_LDFLAGS) -lcrypto
endef

define Package/vedge1000-sha-bench/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/vedge1000-sha-bench $(1)/usr/bin/
endef

$(eval $(call BuildPackage,vedge1000-sha-bench))
