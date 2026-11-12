include $(TOPDIR)/rules.mk

PKG_NAME:=vedge1000-crypto-bench
PKG_VERSION:=1.0
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/vedge1000-crypto-bench
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=vEdge 1000 AES hardware benchmark
  DEPENDS:=@TARGET_octeon +libopenssl
endef

define Package/vedge1000-crypto-bench/description
  Compare AES-CBC encryption and decryption on the Octeon COP2
  coprocessor with the OpenSSL software implementation.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/crypto-bench.c $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(TARGET_CC) $(TARGET_CFLAGS) $(TARGET_CPPFLAGS) -Wall -Wextra \
		-o $(PKG_BUILD_DIR)/crypto_bench $(PKG_BUILD_DIR)/crypto-bench.c \
		$(TARGET_LDFLAGS) -lcrypto
endef

define Package/vedge1000-crypto-bench/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/crypto_bench $(1)/usr/bin/
endef

$(eval $(call BuildPackage,vedge1000-crypto-bench))
